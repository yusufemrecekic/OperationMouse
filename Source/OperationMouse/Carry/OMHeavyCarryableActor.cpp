#include "OMHeavyCarryableActor.h"

#include "OMCarryComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "../Characters/OMMouseCharacter.h"
#include "../OperationMouse.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

AOMHeavyCarryableActor::AOMHeavyCarryableActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	LeftCarrySlot = CreateDefaultSubobject<USceneComponent>(TEXT("LeftCarrySlot"));
	LeftCarrySlot->SetupAttachment(GetRootComponent());
	LeftCarrySlot->SetRelativeLocation(FVector(0.0f, -240.0f, -100.0f));

	RightCarrySlot = CreateDefaultSubobject<USceneComponent>(TEXT("RightCarrySlot"));
	RightCarrySlot->SetupAttachment(GetRootComponent());
	RightCarrySlot->SetRelativeLocation(FVector(0.0f, 240.0f, -100.0f));
}

void AOMHeavyCarryableActor::BeginPlay()
{
	Super::BeginPlay();
	HeavyHomeTransform = GetActorTransform();
	CacheHeavyPresentationIfNeeded();
	UpdateHeavyStatusText();
}

void AOMHeavyCarryableActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearCarrierCollisionIgnores();
	SetMovementPenaltyForAllHolders(false);
	for (UOMCarryComponent* Carrier : ActiveCarriers)
	{
		if (IsValid(Carrier))
		{
			Carrier->ReleaseForRecovery(this);
		}
	}
	ActiveCarriers.Reset();
	ApplyReplicatedMovementPenalty();
	Super::EndPlay(EndPlayReason);
}

FOMInteractionInfo AOMHeavyCarryableActor::GetInteractionInfo_Implementation(AActor* Interactor) const
{
	FOMInteractionInfo Info;
	Info.Prompt = HeavyCarryState == EOMHeavyCarryState::WaitingForSecondHolder
		? NSLOCTEXT("OperationMouse", "JoinHeavyCarryPrompt", "Join Heavy Carry")
		: NSLOCTEXT("OperationMouse", "GrabHeavyCarryPrompt", "Grab Heavy Object");
	Info.Type = EOMInteractionType::Instant;
	Info.MaximumDistance = 240.0f;
	Info.bExclusive = false;
	return Info;
}

bool AOMHeavyCarryableActor::CanInteract_Implementation(AActor* Interactor) const
{
	AOMMouseCharacter* Character = Cast<AOMMouseCharacter>(Interactor);
	UOMCarryComponent* CarryComponent = Character ? Character->FindComponentByClass<UOMCarryComponent>() : nullptr;
	return IsAvailableForGrab() && CarryComponent && CarryComponent->CanGrab(const_cast<AOMHeavyCarryableActor*>(this));
}

bool AOMHeavyCarryableActor::BeginInteraction_Implementation(AActor* Interactor)
{
	return HasAuthority() && CanInteract_Implementation(Interactor);
}

void AOMHeavyCarryableActor::CompleteInteraction_Implementation(AActor* Interactor)
{
	AOMMouseCharacter* Character = Cast<AOMMouseCharacter>(Interactor);
	UOMCarryComponent* CarryComponent = Character ? Character->FindComponentByClass<UOMCarryComponent>() : nullptr;
	if (!HasAuthority() || !CarryComponent || !CarryComponent->TryGrab(this))
	{
		UE_LOG(LogOperationMouse, Log, TEXT("[HeavyCarry][Rejected] Carrier=%s Target=%s Reason=GameplayHandoffRejected"), *GetNameSafe(Interactor), *GetName());
	}
}

bool AOMHeavyCarryableActor::BeginCarry(UOMCarryComponent* NewCarrier, USceneComponent* NewCarryPoint)
{
	if (!HasAuthority() || !IsValid(NewCarrier) || !IsValid(NewCarryPoint))
	{
		return false;
	}

	if (RemoveInvalidCarriers())
	{
		ClearCarrierCollisionIgnores();
	}
	if (ActiveCarriers.Contains(NewCarrier) || ActiveCarriers.Num() >= 2)
	{
		return false;
	}

	ActiveCarriers.Add(NewCarrier);
	if (ActiveCarriers.Num() == 1)
	{
		FreezeAtCurrentTransform();
		SetHeavyCarryState(EOMHeavyCarryState::WaitingForSecondHolder);
		RefreshCarrierCollisionIgnores();
	}
	else
	{
		SetHeavyCarryState(EOMHeavyCarryState::Carrying);
		SetMovementPenaltyForAllHolders(true);
		if (UStaticMeshComponent* HeavyMesh = FindComponentByClass<UStaticMeshComponent>())
		{
			HeavyMesh->SetSimulatePhysics(false);
			HeavyMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			HeavyMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		}
		RefreshCarrierCollisionIgnores();
		AlignCarriersToSlots();
		UpdateHeavyCarryTransform();
	}

	UE_LOG(LogOperationMouse, Log, TEXT("[HeavyCarry][Joined] Carrier=%s Target=%s Holders=%d State=%s GameplayOnly=true"),
		*GetNameSafe(NewCarrier->GetOwner()), *GetName(), ActiveCarriers.Num(),
		HeavyCarryState == EOMHeavyCarryState::Carrying ? TEXT("Carrying") : TEXT("Waiting"));
	SyncReplicatedCarryState();
	return true;
}

bool AOMHeavyCarryableActor::EndCarry(UOMCarryComponent* RequestingCarrier, const FVector& DropLocation)
{
	if (!HasAuthority() || !IsValid(RequestingCarrier) || !ActiveCarriers.Contains(RequestingCarrier))
	{
		return false;
	}

	ClearCarrierCollisionIgnores();
	SetMovementPenaltyForAllHolders(false);
	ActiveCarriers.RemoveSingle(RequestingCarrier);
	RemoveInvalidCarriers();

	if (ActiveCarriers.Num() == 1)
	{
		FreezeAtCurrentTransform();
		SetHeavyCarryState(EOMHeavyCarryState::WaitingForSecondHolder);
		RefreshCarrierCollisionIgnores();
	}
	else
	{
		FTransform DropTransform = GetActorTransform();
		DropTransform.SetLocation(DropLocation);
		DropTransform.SetRotation(FQuat::Identity);
		RestoreWorldPresentation(DropTransform);
		SetHeavyCarryState(EOMHeavyCarryState::Idle);
	}

	UE_LOG(LogOperationMouse, Log, TEXT("[HeavyCarry][Left] Carrier=%s Target=%s Holders=%d GameplayOnly=true"),
		*GetNameSafe(RequestingCarrier->GetOwner()), *GetName(), ActiveCarriers.Num());
	SyncReplicatedCarryState();
	return true;
}

bool AOMHeavyCarryableActor::IsHeldBy(const AOMMouseCharacter* Character) const
{
	if (!HasAuthority())
	{
		return IsValid(Character) && (ReplicatedFirstHolder == Character || ReplicatedSecondHolder == Character);
	}

	return ActiveCarriers.ContainsByPredicate([Character](const UOMCarryComponent* Carrier)
	{
		return IsValid(Carrier) && Carrier->GetOwner() == Character;
	});
}

bool AOMHeavyCarryableActor::IsAvailableForGrab() const
{
	return IsValid(this) && !IsActorBeingDestroyed() && GetHolderCount() < 2;
}

int32 AOMHeavyCarryableActor::GetHolderCount() const
{
	if (HasAuthority())
	{
		return ActiveCarriers.Num();
	}

	return (IsValid(ReplicatedFirstHolder) ? 1 : 0) + (IsValid(ReplicatedSecondHolder) ? 1 : 0);
}

void AOMHeavyCarryableActor::ResetToHome()
{
	if (!HasAuthority())
	{
		return;
	}

	SetMovementPenaltyForAllHolders(false);
	ClearCarrierCollisionIgnores();
	const TArray<TObjectPtr<UOMCarryComponent>> CarriersToRelease = ActiveCarriers;
	for (UOMCarryComponent* Carrier : CarriersToRelease)
	{
		if (IsValid(Carrier))
		{
			Carrier->ReleaseForRecovery(this);
		}
	}
	ActiveCarriers.Reset();
	RestoreWorldPresentation(HeavyHomeTransform);
	SetHeavyCarryState(EOMHeavyCarryState::Idle);
	SyncReplicatedCarryState();
	UE_LOG(LogOperationMouse, Log, TEXT("[HeavyCarry][Reset] Target=%s Holders=0 GameplayOnly=true"), *GetName());
}

void AOMHeavyCarryableActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority())
	{
		return;
	}

	const bool bRemovedInvalidCarrier = RemoveInvalidCarriers();
	if (bRemovedInvalidCarrier)
	{
		ClearCarrierCollisionIgnores();
		SyncReplicatedCarryState();
	}
	if (HeavyCarryState == EOMHeavyCarryState::WaitingForSecondHolder)
	{
		if (ActiveCarriers.IsEmpty())
		{
			RestoreWorldPresentation(GetActorTransform());
			SetHeavyCarryState(EOMHeavyCarryState::Idle);
		}
		else if (bRemovedInvalidCarrier)
		{
			RefreshCarrierCollisionIgnores();
		}
		return;
	}
	if (HeavyCarryState != EOMHeavyCarryState::Carrying)
	{
		return;
	}
	if (ActiveCarriers.Num() < 2)
	{
		SetMovementPenaltyForAllHolders(false);
		if (ActiveCarriers.Num() == 1)
		{
			FreezeAtCurrentTransform();
			SetHeavyCarryState(EOMHeavyCarryState::WaitingForSecondHolder);
			RefreshCarrierCollisionIgnores();
		}
		else
		{
			RestoreWorldPresentation(GetActorTransform());
			SetHeavyCarryState(EOMHeavyCarryState::Idle);
		}
		return;
	}

	UpdateHeavyCarryTransform();
}

void AOMHeavyCarryableActor::SetHeavyCarryState(EOMHeavyCarryState NewState)
{
	HeavyCarryState = NewState;
	SetActorTickEnabled(NewState != EOMHeavyCarryState::Idle);
	UpdateHeavyStatusText();
	if (HasAuthority())
	{
		SyncReplicatedCarryState();
	}
}

void AOMHeavyCarryableActor::SetMovementPenaltyForAllHolders(bool bActive)
{
	for (UOMCarryComponent* Carrier : ActiveCarriers)
	{
		if (AOMMouseCharacter* Character = IsValid(Carrier) ? Cast<AOMMouseCharacter>(Carrier->GetOwner()) : nullptr)
		{
			Character->SetHeavyCarryMovementPenaltyActive(bActive);
		}
	}
}

void AOMHeavyCarryableActor::ApplyReplicatedMovementPenalty()
{
	for (const TWeakObjectPtr<AOMMouseCharacter>& PreviousCharacter : ReplicatedPenaltyCharacters)
	{
		if (AOMMouseCharacter* Character = PreviousCharacter.Get())
		{
			Character->SetHeavyCarryMovementPenaltyActive(false);
		}
	}
	ReplicatedPenaltyCharacters.Reset();

	if (HeavyCarryState != EOMHeavyCarryState::Carrying)
	{
		return;
	}

	for (AOMMouseCharacter* Character : GetPresentationHolders())
	{
		if (IsValid(Character))
		{
			Character->SetHeavyCarryMovementPenaltyActive(true);
			ReplicatedPenaltyCharacters.Add(Character);
		}
	}
}

void AOMHeavyCarryableActor::RefreshCarrierCollisionIgnores()
{
	ClearCarrierCollisionIgnores();

	UStaticMeshComponent* HeavyMesh = FindComponentByClass<UStaticMeshComponent>();
	const TArray<AOMMouseCharacter*> HolderCharacters = GetPresentationHolders();
	for (AOMMouseCharacter* Character : HolderCharacters)
	{
		if (!IsValid(Character))
		{
			continue;
		}

		AddCarrierCollisionIgnore(HeavyMesh, Character);
		AddCarrierMovementIgnore(Character, this);
	}

	if (HeavyCarryState == EOMHeavyCarryState::Carrying && HolderCharacters.Num() == 2)
	{
		AddCarrierMovementIgnore(HolderCharacters[0], HolderCharacters[1]);
		AddCarrierMovementIgnore(HolderCharacters[1], HolderCharacters[0]);
	}
}

TArray<AOMMouseCharacter*> AOMHeavyCarryableActor::GetPresentationHolders() const
{
	TArray<AOMMouseCharacter*> Holders;
	if (HasAuthority())
	{
		for (UOMCarryComponent* Carrier : ActiveCarriers)
		{
			if (AOMMouseCharacter* Character = IsValid(Carrier) ? Cast<AOMMouseCharacter>(Carrier->GetOwner()) : nullptr)
			{
				Holders.Add(Character);
			}
		}
		return Holders;
	}

	if (IsValid(ReplicatedFirstHolder))
	{
		Holders.Add(ReplicatedFirstHolder);
	}
	if (IsValid(ReplicatedSecondHolder))
	{
		Holders.Add(ReplicatedSecondHolder);
	}
	return Holders;
}

void AOMHeavyCarryableActor::ClearCarrierCollisionIgnores()
{
	const int32 PairCount = FMath::Min(CollisionIgnoreSources.Num(), CollisionIgnoreTargets.Num());
	for (int32 PairIndex = 0; PairIndex < PairCount; ++PairIndex)
	{
		UPrimitiveComponent* SourceComponent = CollisionIgnoreSources[PairIndex].Get();
		AActor* TargetActor = CollisionIgnoreTargets[PairIndex].Get();
		if (IsValid(SourceComponent) && IsValid(TargetActor))
		{
			SourceComponent->IgnoreActorWhenMoving(TargetActor, false);
		}
	}
	CollisionIgnoreSources.Reset();
	CollisionIgnoreTargets.Reset();

	const int32 MovementPairCount = FMath::Min(MovementIgnoreSources.Num(), MovementIgnoreTargets.Num());
	for (int32 PairIndex = 0; PairIndex < MovementPairCount; ++PairIndex)
	{
		AOMMouseCharacter* SourceCharacter = MovementIgnoreSources[PairIndex].Get();
		AActor* TargetActor = MovementIgnoreTargets[PairIndex].Get();
		if (IsValid(SourceCharacter) && IsValid(TargetActor))
		{
			SourceCharacter->MoveIgnoreActorRemove(TargetActor);
		}
	}
	MovementIgnoreSources.Reset();
	MovementIgnoreTargets.Reset();
}

void AOMHeavyCarryableActor::AddCarrierCollisionIgnore(UPrimitiveComponent* SourceComponent, AActor* TargetActor)
{
	if (!IsValid(SourceComponent) || !IsValid(TargetActor)
		|| SourceComponent->GetMoveIgnoreActors().Contains(TargetActor))
	{
		return;
	}

	SourceComponent->IgnoreActorWhenMoving(TargetActor, true);
	CollisionIgnoreSources.Add(SourceComponent);
	CollisionIgnoreTargets.Add(TargetActor);
}

void AOMHeavyCarryableActor::AddCarrierMovementIgnore(AOMMouseCharacter* SourceCharacter, AActor* TargetActor)
{
	UCapsuleComponent* Capsule = IsValid(SourceCharacter) ? SourceCharacter->GetCapsuleComponent() : nullptr;
	if (!IsValid(Capsule) || !IsValid(TargetActor)
		|| Capsule->GetMoveIgnoreActors().Contains(TargetActor))
	{
		return;
	}

	SourceCharacter->MoveIgnoreActorAdd(TargetActor);
	MovementIgnoreSources.Add(SourceCharacter);
	MovementIgnoreTargets.Add(TargetActor);
}

void AOMHeavyCarryableActor::AlignCarriersToSlots()
{
	if (ActiveCarriers.Num() != 2)
	{
		return;
	}

	for (int32 CarrierIndex = 0; CarrierIndex < ActiveCarriers.Num(); ++CarrierIndex)
	{
		UOMCarryComponent* Carrier = ActiveCarriers[CarrierIndex];
		AOMMouseCharacter* Character = IsValid(Carrier) ? Cast<AOMMouseCharacter>(Carrier->GetOwner()) : nullptr;
		USceneComponent* CharacterCarryPoint = IsValid(Carrier) ? Carrier->GetCarryPoint() : nullptr;
		USceneComponent* Slot = GetSlotForCarrierIndex(CarrierIndex);
		if (!IsValid(Character) || !IsValid(CharacterCarryPoint) || !IsValid(Slot))
		{
			continue;
		}

		const FVector AlignmentDelta = Slot->GetComponentLocation() - CharacterCarryPoint->GetComponentLocation();
		Character->SetActorLocation(
			Character->GetActorLocation() + AlignmentDelta,
			true,
			nullptr,
			ETeleportType::None);
	}
}

void AOMHeavyCarryableActor::CacheHeavyPresentationIfNeeded()
{
	UStaticMeshComponent* HeavyMesh = FindComponentByClass<UStaticMeshComponent>();
	if (bHeavyPresentationSaved || !HeavyMesh)
	{
		return;
	}

	SavedHeavyCollision = HeavyMesh->GetCollisionEnabled();
	SavedHeavyPawnCollisionResponse = HeavyMesh->GetCollisionResponseToChannel(ECC_Pawn);
	bSavedHeavySimulatePhysics = HeavyMesh->IsSimulatingPhysics();
	bHeavyPresentationSaved = true;
}

void AOMHeavyCarryableActor::SyncReplicatedCarryState()
{
	if (!HasAuthority())
	{
		return;
	}

	ReplicatedFirstHolder = nullptr;
	ReplicatedSecondHolder = nullptr;
	const TArray<AOMMouseCharacter*> Holders = GetPresentationHolders();
	if (Holders.IsValidIndex(0))
	{
		ReplicatedFirstHolder = Holders[0];
	}
	if (Holders.IsValidIndex(1))
	{
		ReplicatedSecondHolder = Holders[1];
	}
	ForceNetUpdate();
}

void AOMHeavyCarryableActor::OnRep_HeavyCarryNetworkState()
{
	ApplyReplicatedCarryPresentation();
}

void AOMHeavyCarryableActor::ApplyReplicatedCarryPresentation()
{
	UStaticMeshComponent* HeavyMesh = FindComponentByClass<UStaticMeshComponent>();
	if (!HeavyMesh)
	{
		return;
	}

	ClearCarrierCollisionIgnores();
	ApplyReplicatedMovementPenalty();

	if (HeavyCarryState == EOMHeavyCarryState::Idle)
	{
		if (bHeavyPresentationSaved)
		{
			HeavyMesh->SetSimulatePhysics(false);
			HeavyMesh->SetCollisionEnabled(SavedHeavyCollision);
			HeavyMesh->SetCollisionResponseToChannel(ECC_Pawn, SavedHeavyPawnCollisionResponse);
			HeavyMesh->SetSimulatePhysics(bSavedHeavySimulatePhysics);
			bHeavyPresentationSaved = false;
		}
		UpdateHeavyStatusText();
		return;
	}

	CacheHeavyPresentationIfNeeded();
	HeavyMesh->SetSimulatePhysics(false);
	HeavyMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HeavyMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	RefreshCarrierCollisionIgnores();
	ApplyReplicatedMovementPenalty();
	UpdateHeavyStatusText();
}

void AOMHeavyCarryableActor::FreezeAtCurrentTransform()
{
	UStaticMeshComponent* HeavyMesh = FindComponentByClass<UStaticMeshComponent>();
	if (!HeavyMesh)
	{
		return;
	}
	CacheHeavyPresentationIfNeeded();
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	HeavyMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	HeavyMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	HeavyMesh->SetSimulatePhysics(false);
	HeavyMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HeavyMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	bHeavyCarryObstructed = false;
	HeavyObstructionNormal = FVector::ZeroVector;
}

void AOMHeavyCarryableActor::RestoreWorldPresentation(const FTransform& TargetTransform)
{
	UStaticMeshComponent* HeavyMesh = FindComponentByClass<UStaticMeshComponent>();
	if (!HeavyMesh)
	{
		return;
	}
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	HeavyMesh->SetSimulatePhysics(false);
	SetActorTransform(TargetTransform, false, nullptr, ETeleportType::TeleportPhysics);
	HeavyMesh->SetCollisionEnabled(SavedHeavyCollision);
	HeavyMesh->SetCollisionResponseToChannel(ECC_Pawn, SavedHeavyPawnCollisionResponse);
	HeavyMesh->SetSimulatePhysics(bSavedHeavySimulatePhysics);
	bHeavyPresentationSaved = false;
	bHeavyCarryObstructed = false;
	HeavyObstructionNormal = FVector::ZeroVector;
}

void AOMHeavyCarryableActor::UpdateHeavyCarryTransform()
{
	if (ActiveCarriers.Num() != 2)
	{
		return;
	}
	const USceneComponent* FirstPoint = ActiveCarriers[0]->GetCarryPoint();
	const USceneComponent* SecondPoint = ActiveCarriers[1]->GetCarryPoint();
	if (!IsValid(FirstPoint) || !IsValid(SecondPoint))
	{
		return;
	}
	const FVector FirstLocation = FirstPoint->GetComponentLocation();
	const FVector SecondLocation = SecondPoint->GetComponentLocation();
	const FVector HolderSeparation = SecondLocation - FirstLocation;
	const float MinimumStableSeparation =
		(LeftCarrySlot->GetRelativeLocation() - RightCarrySlot->GetRelativeLocation()).Size2D() * 0.5f;
	if (HolderSeparation.Size2D() < MinimumStableSeparation)
	{
		SetHeavyCarryObstructed(true, FHitResult());
		return;
	}

	FTransform DesiredTransform = GetActorTransform();
	if (HolderSeparation.SizeSquared2D() > UE_KINDA_SMALL_NUMBER)
	{
		const float TargetYaw = HolderSeparation.Rotation().Yaw - 90.0f;
		DesiredTransform.SetRotation(FRotator(0.0f, TargetYaw, 0.0f).Quaternion());
	}

	const FVector SlotMidpointLocal =
		(LeftCarrySlot->GetRelativeLocation() + RightCarrySlot->GetRelativeLocation()) * 0.5f;
	const FVector DesiredMidpoint = (FirstLocation + SecondLocation) * 0.5f;
	DesiredTransform.SetLocation(
		DesiredMidpoint - DesiredTransform.TransformVector(SlotMidpointLocal));

	FHitResult Hit;
	SetActorLocationAndRotation(
		DesiredTransform.GetLocation(),
		DesiredTransform.GetRotation(),
		true,
		&Hit,
		ETeleportType::None);
	SetHeavyCarryObstructed(Hit.bBlockingHit, Hit);
}

void AOMHeavyCarryableActor::SetHeavyCarryObstructed(bool bNewObstructed, const FHitResult& Hit)
{
	const FVector NewNormal = bNewObstructed && Hit.bBlockingHit
		? Hit.ImpactNormal.GetSafeNormal()
		: FVector::ZeroVector;
	if (bHeavyCarryObstructed == bNewObstructed && FVector(HeavyObstructionNormal).Equals(NewNormal, 0.01f))
	{
		return;
	}

	bHeavyCarryObstructed = bNewObstructed;
	HeavyObstructionNormal = NewNormal;
	const TArray<AOMMouseCharacter*> Holders = GetPresentationHolders();
	const UCharacterMovementComponent* FirstMovement = Holders.IsValidIndex(0) && IsValid(Holders[0])
		? Holders[0]->GetCharacterMovement()
		: nullptr;
	const UCharacterMovementComponent* SecondMovement = Holders.IsValidIndex(1) && IsValid(Holders[1])
		? Holders[1]->GetCharacterMovement()
		: nullptr;
	UE_LOG(LogOperationMouse, Log,
		TEXT("[HeavyCarry][Obstruction] Target=%s State=%s Hit=%s Role=%s HolderA=%s ModeA=%d HolderB=%s ModeB=%d Normal=%s"),
		*GetName(), bHeavyCarryObstructed ? TEXT("Blocked") : TEXT("Clear"),
		Hit.bBlockingHit ? *GetNameSafe(Hit.GetActor()) : TEXT("HolderSeparation"),
		*UEnum::GetValueAsString(GetLocalRole()),
		Holders.IsValidIndex(0) ? *GetNameSafe(Holders[0]) : TEXT("None"),
		FirstMovement ? static_cast<int32>(FirstMovement->MovementMode) : -1,
		Holders.IsValidIndex(1) ? *GetNameSafe(Holders[1]) : TEXT("None"),
		SecondMovement ? static_cast<int32>(SecondMovement->MovementMode) : -1,
		*NewNormal.ToCompactString());
	ForceNetUpdate();
}

FVector AOMHeavyCarryableActor::ConstrainHolderMovement(
	const AOMMouseCharacter* Holder,
	const FVector& DesiredWorldMovement) const
{
	FVector ConstrainedMovement = Super::ConstrainHolderMovement(Holder, DesiredWorldMovement);
	if (!IsValid(Holder) || HeavyCarryState != EOMHeavyCarryState::Carrying)
	{
		return ConstrainedMovement;
	}

	const FVector ObstructionNormal = FVector(HeavyObstructionNormal).GetSafeNormal();
	if (!ObstructionNormal.IsNearlyZero()
		&& FVector::DotProduct(ConstrainedMovement, ObstructionNormal) < 0.0f)
	{
		ConstrainedMovement = FVector::VectorPlaneProject(ConstrainedMovement, ObstructionNormal);
	}

	AOMMouseCharacter* OtherHolder = nullptr;
	for (AOMMouseCharacter* Candidate : GetPresentationHolders())
	{
		if (IsValid(Candidate) && Candidate != Holder)
		{
			OtherHolder = Candidate;
			break;
		}
	}
	if (!IsValid(OtherHolder))
	{
		return ConstrainedMovement;
	}

	FVector TowardOther = OtherHolder->GetActorLocation() - Holder->GetActorLocation();
	TowardOther.Z = 0.0f;
	const float MinimumStableSeparation =
		(LeftCarrySlot->GetRelativeLocation() - RightCarrySlot->GetRelativeLocation()).Size2D() * 0.5f;
	if (TowardOther.Size2D() <= MinimumStableSeparation + 10.0f)
	{
		TowardOther.Normalize();
		const float MovementTowardOther = FVector::DotProduct(ConstrainedMovement, TowardOther);
		if (MovementTowardOther > 0.0f)
		{
			ConstrainedMovement -= TowardOther * MovementTowardOther;
		}
	}

	return ConstrainedMovement;
}

USceneComponent* AOMHeavyCarryableActor::GetSlotForCarrierIndex(int32 CarrierIndex) const
{
	return CarrierIndex == 0 ? LeftCarrySlot.Get() : RightCarrySlot.Get();
}

void AOMHeavyCarryableActor::UpdateHeavyStatusText()
{
	UTextRenderComponent* Text = FindComponentByClass<UTextRenderComponent>();
	if (!Text)
	{
		return;
	}
	switch (HeavyCarryState)
	{
	case EOMHeavyCarryState::WaitingForSecondHolder:
		Text->SetText(NSLOCTEXT("OperationMouse", "HeavyCarryWaitingState", "HEAVY: WAITING 1/2"));
		Text->SetTextRenderColor(FColor::Yellow);
		break;
	case EOMHeavyCarryState::Carrying:
		Text->SetText(NSLOCTEXT("OperationMouse", "HeavyCarryActiveState", "HEAVY: CARRYING 2/2"));
		Text->SetTextRenderColor(FColor::Green);
		break;
	case EOMHeavyCarryState::Idle:
	default:
		Text->SetText(NSLOCTEXT("OperationMouse", "HeavyCarryReadyState", "HEAVY: READY 0/2"));
		Text->SetTextRenderColor(FColor::Cyan);
		break;
	}
}

bool AOMHeavyCarryableActor::RemoveInvalidCarriers()
{
	return ActiveCarriers.RemoveAll([](const UOMCarryComponent* Carrier)
	{
		return !IsValid(Carrier) || !IsValid(Carrier->GetOwner());
	}) > 0;
}

void AOMHeavyCarryableActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AOMHeavyCarryableActor, HeavyCarryState);
	DOREPLIFETIME(AOMHeavyCarryableActor, ReplicatedFirstHolder);
	DOREPLIFETIME(AOMHeavyCarryableActor, ReplicatedSecondHolder);
	DOREPLIFETIME(AOMHeavyCarryableActor, HeavyObstructionNormal);
}
