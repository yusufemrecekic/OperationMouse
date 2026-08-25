#include "OMCloseSpaceCameraComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "../OperationMouse.h"

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
	CurrentDesiredArmLength = DesiredArmLength;
	if (const UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent())
	{
		CurrentBasePivotHeight = Capsule->GetScaledCapsuleHalfHeight() * BasePivotHeightFactor;
		CameraBoom->TargetOffset.Z = CurrentBasePivotHeight;
	}
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

void UOMCloseSpaceCameraComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!CharacterOwner || !CameraBoom || !CharacterOwner->IsLocallyControlled())
	{
		return;
	}
	InitializeLocalResolver();

	const UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
	if (!Capsule)
	{
		return;
	}

	const FRotator CameraRotation = CameraBoom->GetTargetRotation();
	const float TargetBasePivotHeight =
		Capsule->GetScaledCapsuleHalfHeight() * BasePivotHeightFactor;
	CurrentBasePivotHeight = FMath::FInterpTo(
		CurrentBasePivotHeight,
		TargetBasePivotHeight,
		DeltaTime,
		CrouchCameraBlendSpeed);

	const float TargetDesiredArmLength = DesiredArmLength
		* (CharacterOwner->bIsCrouched ? CrouchArmMultiplier : 1.0f);
	CurrentDesiredArmLength = FMath::FInterpTo(
		CurrentDesiredArmLength,
		TargetDesiredArmLength,
		DeltaTime,
		CrouchCameraBlendSpeed);

	const FVector PivotReference = CameraBoom->GetComponentLocation();
	FVector BaseOffset = OpenSpaceTargetOffset;
	BaseOffset.Z = CurrentBasePivotHeight;
	const FVector BasePivot = ResolveSafePivot(PivotReference, PivotReference + BaseOffset);
	const float BaseLimit = FindObstructionLimit(BasePivot, CameraRotation, CurrentDesiredArmLength);
	const float CompressionRatio = CurrentDesiredArmLength > UE_SMALL_NUMBER
		? FMath::Clamp(BaseLimit / CurrentDesiredArmLength, 0.0f, 1.0f)
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
	const FVector DesiredPivot = PivotReference + BaseOffset + FVector::UpVector * ExtraPivotHeight;
	const FVector SafePivot = ResolveSafePivot(PivotReference, DesiredPivot);
	CameraBoom->TargetOffset = SafePivot - PivotReference;

	const float ObstructionLimit = FindObstructionLimit(SafePivot, CameraRotation, CurrentDesiredArmLength);
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
	CurrentResolvedDistance = FMath::Clamp(SmoothedDistance, 0.0f, CurrentDesiredArmLength);
	CameraBoom->TargetArmLength = CurrentResolvedDistance;

	// Whole-character hiding is an emergency guard only. Ordinary crouch and
	// close-space compression keep the mouse visible in close third person.
	const float MinSafeDistance =
		Capsule->GetScaledCapsuleRadius() * MinSafeDistanceRadiusMultiplier;
	const float EmergencyHideDistance = FMath::Min(
		CameraProbeRadius * 0.5f,
		MinSafeDistance * 0.25f);
	const float RestoreDistance = FMath::Max(EmergencyHideDistance * 2.0f, CameraProbeRadius);
	if (!bOwnerMeshFallbackActive && CurrentResolvedDistance < EmergencyHideDistance)
	{
		SetOwnerMeshFallback(true);
	}
	else if (bOwnerMeshFallbackActive && CurrentResolvedDistance > RestoreDistance)
	{
		SetOwnerMeshFallback(false);
	}
}

FVector UOMCloseSpaceCameraComponent::ResolveSafePivot(
	const FVector& Reference,
	const FVector& DesiredPivot) const
{
	if (!GetWorld() || !CharacterOwner)
	{
		return DesiredPivot;
	}

	const FVector PivotDelta = DesiredPivot - Reference;
	const float PivotDistance = PivotDelta.Size();
	if (PivotDistance <= UE_SMALL_NUMBER)
	{
		return Reference;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OMCloseSpaceCameraPivot), false, CharacterOwner);
	QueryParams.AddIgnoredActor(CharacterOwner);

	FHitResult Hit;
	const bool bBlocked = GetWorld()->SweepSingleByChannel(
		Hit,
		Reference,
		DesiredPivot,
		FQuat::Identity,
		ECC_Camera,
		FCollisionShape::MakeSphere(FMath::Max(CameraProbeRadius, 0.1f)),
		QueryParams);
	if (!bBlocked)
	{
		return DesiredPivot;
	}

	if (Hit.bStartPenetrating)
	{
		const FVector DepenetrationNormal = Hit.Normal.GetSafeNormal(UE_SMALL_NUMBER, FVector::DownVector);
		return Reference + DepenetrationNormal * (Hit.PenetrationDepth + CameraCollisionPadding);
	}

	const float SafeDistance = FMath::Clamp(
		Hit.Distance - CameraCollisionPadding,
		0.0f,
		PivotDistance);
	return Reference + PivotDelta.GetSafeNormal() * SafeDistance;
}

float UOMCloseSpaceCameraComponent::FindObstructionLimit(
	const FVector& Pivot,
	const FRotator& CameraRotation,
	float DesiredDistance) const
{
	if (!GetWorld() || !CharacterOwner)
	{
		return DesiredDistance;
	}

	const FVector DesiredLocation = Pivot - CameraRotation.Vector() * DesiredDistance;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OMCloseSpaceCamera), false, CharacterOwner);
	QueryParams.AddIgnoredActor(CharacterOwner);

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
		return DesiredDistance;
	}
	if (Hit.bStartPenetrating)
	{
		return 0.0f;
	}

	// Hit.Distance places the swept sphere exactly at first contact. Keep a small
	// calibrated gap so the perspective near plane remains on the visible side.
	return FMath::Clamp(Hit.Distance - CameraCollisionPadding, 0.0f, DesiredDistance);
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
