#include "OMCloseSpaceCameraComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "../OperationMouse.h"

namespace
{
	const FName SoftCameraOccluderTag(TEXT("OMCameraSoftOccluder"));
}

UOMCloseSpaceCameraComponent::UOMCloseSpaceCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	SetIsReplicatedByDefault(false);
}

void UOMCloseSpaceCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	CharacterOwner = Cast<ACharacter>(GetOwner());
	CameraBoom = CharacterOwner ? CharacterOwner->FindComponentByClass<USpringArmComponent>() : nullptr;
	if (!bCloseSpaceCameraEnabled || !CharacterOwner || !CameraBoom)
	{
		SetComponentTickEnabled(false);
		return;
	}

	// Possession can happen after BeginPlay, so local initialization is also retried from Tick.
	if (CharacterOwner->IsLocallyControlled())
	{
		InitializeLocalResolver();
	}
}

void UOMCloseSpaceCameraComponent::InitializeLocalResolver()
{
	if (bResolverInitialized || !CharacterOwner || !CameraBoom)
	{
		return;
	}

	bOriginalSpringArmCollision = CameraBoom->bDoCollisionTest;
	bOriginalOwnerNoSee = CharacterOwner->GetMesh() && CharacterOwner->GetMesh()->bOwnerNoSee;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->TargetArmLength = DesiredArmLength;
	CameraBoom->TargetOffset = OpenSpaceTargetOffset;
	CurrentResolvedDistance = DesiredArmLength;
	CameraBoom->AddTickPrerequisiteComponent(this);
	bResolverInitialized = true;

	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[CameraFoundation] Enabled Owner=%s Desired=%.1f Probe=%.1f Padding=%.1f Retract=%.1f Extend=%.1f"),
		*GetNameSafe(CharacterOwner),
		DesiredArmLength,
		CameraProbeRadius,
		CameraCollisionPadding,
		CameraRetractSpeed,
		CameraExtendSpeed);
}

void UOMCloseSpaceCameraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RestoreLocalSoftOccluders();
	if (bResolverInitialized && CharacterOwner && CharacterOwner->GetMesh())
	{
		CharacterOwner->GetMesh()->SetOwnerNoSee(bOriginalOwnerNoSee);
	}
	if (bResolverInitialized && CameraBoom)
	{
		CameraBoom->bDoCollisionTest = bOriginalSpringArmCollision;
	}
	Super::EndPlay(EndPlayReason);
}

void UOMCloseSpaceCameraComponent::Deactivate()
{
	RestoreLocalSoftOccluders();
	Super::Deactivate();
}

void UOMCloseSpaceCameraComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bCloseSpaceCameraEnabled || !CharacterOwner || !CameraBoom || !CharacterOwner->IsLocallyControlled())
	{
		RestoreLocalSoftOccluders();
		return;
	}
	InitializeLocalResolver();

	const UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
	if (!Capsule)
	{
		return;
	}

	const FRotator CameraRotation = CameraBoom->GetTargetRotation();
	const FVector BasePivot = CameraBoom->GetComponentLocation() + OpenSpaceTargetOffset;
	TSet<TWeakObjectPtr<UPrimitiveComponent>> SoftOccluders;
	const float BaseLimit = FindObstructionLimit(BasePivot, CameraRotation, SoftOccluders);
	const float CompressionRatio = DesiredArmLength > UE_SMALL_NUMBER
		? FMath::Clamp(BaseLimit / DesiredArmLength, 0.0f, 1.0f)
		: 1.0f;
	const float TargetCloseAlpha = FMath::Clamp(
		(CloseSpaceThreshold - CompressionRatio) / FMath::Max(CloseSpaceThreshold, UE_SMALL_NUMBER),
		0.0f,
		1.0f);
	CurrentCloseSpaceAlpha = FMath::FInterpTo(
		CurrentCloseSpaceAlpha,
		TargetCloseAlpha,
		DeltaTime,
		CloseSpaceBlendSpeed);

	const float ExtraPivotHeight =
		Capsule->GetScaledCapsuleHalfHeight()
		* CloseSpaceVerticalOffsetHalfHeightMultiplier
		* CurrentCloseSpaceAlpha;
	CameraBoom->TargetOffset = OpenSpaceTargetOffset + FVector::UpVector * ExtraPivotHeight;

	const FVector ResolvedPivot = CameraBoom->GetComponentLocation() + CameraBoom->TargetOffset;
	const float ObstructionLimit = FindObstructionLimit(ResolvedPivot, CameraRotation, SoftOccluders);
	UpdateLocalSoftOccluderVisibility(SoftOccluders);
	const float InterpSpeed = ObstructionLimit < CurrentResolvedDistance
		? CameraRetractSpeed
		: CameraExtendSpeed;
	float SmoothedDistance = FMath::FInterpTo(
		CurrentResolvedDistance,
		ObstructionLimit,
		DeltaTime,
		InterpSpeed);

	// Never leave the camera center beyond the latest sphere-sweep limit.
	SmoothedDistance = FMath::Min(SmoothedDistance, ObstructionLimit);
	CurrentResolvedDistance = FMath::Clamp(SmoothedDistance, 0.0f, DesiredArmLength);
	CameraBoom->TargetArmLength = CurrentResolvedDistance;

	const float MinSafeDistance =
		Capsule->GetScaledCapsuleRadius() * MinSafeDistanceRadiusMultiplier;
	const float RestoreDistance = MinSafeDistance * 1.25f;
	if (!bOwnerMeshFallbackActive && CurrentResolvedDistance < MinSafeDistance)
	{
		SetOwnerMeshFallback(true);
	}
	else if (bOwnerMeshFallbackActive && CurrentResolvedDistance > RestoreDistance)
	{
		SetOwnerMeshFallback(false);
	}
}

float UOMCloseSpaceCameraComponent::FindObstructionLimit(
	const FVector& Pivot,
	const FRotator& CameraRotation,
	TSet<TWeakObjectPtr<UPrimitiveComponent>>& OutSoftOccluders) const
{
	if (!GetWorld() || !CharacterOwner)
	{
		return DesiredArmLength;
	}

	const FVector DesiredLocation = Pivot - CameraRotation.Vector() * DesiredArmLength;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OMCloseSpaceCamera), false, CharacterOwner);
	QueryParams.AddIgnoredActor(CharacterOwner);

	const int32 MaxSoftOccluders = FMath::Clamp(MaxSoftOccludersPerSweep, 0, 8);
	for (int32 SweepIndex = 0; SweepIndex <= MaxSoftOccluders; ++SweepIndex)
	{
		FHitResult Hit;
		const bool bBlocked = GetWorld()->SweepSingleByChannel(
			Hit,
			Pivot,
			DesiredLocation,
			FQuat::Identity,
			ECC_Camera,
			FCollisionShape::MakeSphere(FMath::Max(CameraProbeRadius, 0.1f)),
			QueryParams);
		if (!bBlocked)
		{
			return DesiredArmLength;
		}

		UPrimitiveComponent* HitComponent = Hit.GetComponent();
		if (SweepIndex < MaxSoftOccluders && IsSoftCameraOccluder(HitComponent))
		{
			OutSoftOccluders.Add(HitComponent);
			QueryParams.AddIgnoredComponent(HitComponent);
			continue;
		}

		// Hit.Distance places the swept sphere exactly at first contact. Keep a small
		// calibrated gap so the perspective near plane remains on the visible side.
		return FMath::Clamp(Hit.Distance - CameraCollisionPadding, 0.0f, DesiredArmLength);
	}

	return DesiredArmLength;
}

bool UOMCloseSpaceCameraComponent::IsSoftCameraOccluder(const UPrimitiveComponent* Component) const
{
	return Component
		&& (Component->ComponentHasTag(SoftCameraOccluderTag)
			|| (Component->GetOwner() && Component->GetOwner()->ActorHasTag(SoftCameraOccluderTag)));
}

void UOMCloseSpaceCameraComponent::UpdateLocalSoftOccluderVisibility(
	const TSet<TWeakObjectPtr<UPrimitiveComponent>>& SoftOccluders)
{
	APlayerController* LocalController = Cast<APlayerController>(CharacterOwner ? CharacterOwner->GetController() : nullptr);
	if (!LocalController || !LocalController->IsLocalController())
	{
		RestoreLocalSoftOccluders();
		return;
	}

	if (VisibilityPlayerController.IsValid() && VisibilityPlayerController.Get() != LocalController)
	{
		RestoreLocalSoftOccluders();
	}
	VisibilityPlayerController = LocalController;

	for (auto It = ManagedHiddenSoftOccluders.CreateIterator(); It; ++It)
	{
		UPrimitiveComponent* Component = It->Get();
		if (!Component || !SoftOccluders.Contains(*It))
		{
			if (Component)
			{
				LocalController->HiddenPrimitiveComponents.Remove(Component);
			}
			It.RemoveCurrent();
		}
	}

	for (const TWeakObjectPtr<UPrimitiveComponent>& SoftOccluder : SoftOccluders)
	{
		UPrimitiveComponent* Component = SoftOccluder.Get();
		if (Component && !ManagedHiddenSoftOccluders.Contains(SoftOccluder)
			&& !LocalController->HiddenPrimitiveComponents.Contains(Component))
		{
			LocalController->HiddenPrimitiveComponents.Add(Component);
			ManagedHiddenSoftOccluders.Add(Component);
		}
	}
}

void UOMCloseSpaceCameraComponent::RestoreLocalSoftOccluders()
{
	if (APlayerController* LocalController = VisibilityPlayerController.Get())
	{
		for (const TWeakObjectPtr<UPrimitiveComponent>& SoftOccluder : ManagedHiddenSoftOccluders)
		{
			if (UPrimitiveComponent* Component = SoftOccluder.Get())
			{
				LocalController->HiddenPrimitiveComponents.Remove(Component);
			}
		}
	}
	ManagedHiddenSoftOccluders.Reset();
	VisibilityPlayerController.Reset();
}

void UOMCloseSpaceCameraComponent::SetOwnerMeshFallback(bool bShouldHide)
{
	if (!CharacterOwner || bOwnerMeshFallbackActive == bShouldHide)
	{
		return;
	}

	if (USkeletalMeshComponent* Mesh = CharacterOwner->GetMesh())
	{
		Mesh->SetOwnerNoSee(bShouldHide);
		bOwnerMeshFallbackActive = bShouldHide;
	}
}
