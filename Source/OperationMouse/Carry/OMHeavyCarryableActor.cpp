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
	if (HasAuthority())
	{
		ReplicatedHeavyWorldState.Transform = HeavyHomeTransform;
		ReplicatedHeavyWorldState.Revision = 1;
		ReplicatedHeavyWorldState.bSimulatePhysics = bSavedHeavySimulatePhysics;
	}
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
		: HeavyCarryState == EOMHeavyCarryState::WaitingForValidPositions
			? NSLOCTEXT("OperationMouse", "AdjustHeavyCarryPrompt", "Adjust Position")
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
		SetHeavyCarryState(EOMHeavyCarryState::WaitingForValidPositions);
		if (UStaticMeshComponent* HeavyMesh = FindComponentByClass<UStaticMeshComponent>())
		{
			HeavyMesh->SetSimulatePhysics(false);
			HeavyMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			HeavyMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		}
		RefreshCarrierCollisionIgnores();
		const bool bAligned = AlignCarriersToSlots();
		bWaitingForHolderAdjustment = !bAligned;
		if (!bAligned)
		{
			const TArray<AOMMouseCharacter*> Holders = GetPresentationHolders();
			FailedAlignmentFirstHolderLocation = Holders.IsValidIndex(0) && IsValid(Holders[0])
				? Holders[0]->GetActorLocation() : FVector::ZeroVector;
			FailedAlignmentSecondHolderLocation = Holders.IsValidIndex(1) && IsValid(Holders[1])
				? Holders[1]->GetActorLocation() : FVector::ZeroVector;
		}
		else
		{
			TryEnterCarrying();
		}
	}

	UE_LOG(LogOperationMouse, Log, TEXT("[HeavyCarry][Joined] Carrier=%s Target=%s Holders=%d State=%s GameplayOnly=true"),
		*GetNameSafe(NewCarrier->GetOwner()), *GetName(), ActiveCarriers.Num(),
		*UEnum::GetValueAsString(HeavyCarryState));
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
		bWaitingForHolderAdjustment = false;
		FreezeAtCurrentTransform();
		SetHeavyCarryState(EOMHeavyCarryState::WaitingForSecondHolder);
		RefreshCarrierCollisionIgnores();
	}
	else
	{
		bWaitingForHolderAdjustment = false;
		FTransform DropTransform = GetActorTransform();
		DropTransform.SetLocation(DropLocation);
		DropTransform.SetRotation(FQuat::Identity);
		PublishAuthoritativeWorldPresentation(DropTransform);
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
	bWaitingForHolderAdjustment = false;
	PublishAuthoritativeWorldPresentation(HeavyHomeTransform);
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
			PublishAuthoritativeWorldPresentation(GetActorTransform());
			SetHeavyCarryState(EOMHeavyCarryState::Idle);
		}
		else if (bRemovedInvalidCarrier)
		{
			RefreshCarrierCollisionIgnores();
		}
		return;
	}
	if (HeavyCarryState == EOMHeavyCarryState::WaitingForValidPositions)
	{
		if (ActiveCarriers.Num() < 2)
		{
			SetMovementPenaltyForAllHolders(false);
			bWaitingForHolderAdjustment = false;
			if (ActiveCarriers.Num() == 1)
			{
				FreezeAtCurrentTransform();
				SetHeavyCarryState(EOMHeavyCarryState::WaitingForSecondHolder);
				RefreshCarrierCollisionIgnores();
			}
			else
			{
				PublishAuthoritativeWorldPresentation(GetActorTransform());
				SetHeavyCarryState(EOMHeavyCarryState::Idle);
			}
			return;
		}

		if (!bWaitingForHolderAdjustment || HaveHoldersAdjustedSinceFailedAlignment())
		{
			bWaitingForHolderAdjustment = false;
			TryEnterCarrying();
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
			PublishAuthoritativeWorldPresentation(GetActorTransform());
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

	if ((HeavyCarryState == EOMHeavyCarryState::Carrying
		|| HeavyCarryState == EOMHeavyCarryState::WaitingForValidPositions)
		&& HolderCharacters.Num() == 2)
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

bool AOMHeavyCarryableActor::AlignCarriersToSlots()
{
	if (ActiveCarriers.Num() != 2)
	{
		return false;
	}

	bool bAllAligned = true;
	const float AlignmentTolerance = GetHolderAlignmentTolerance();
	for (int32 CarrierIndex = 0; CarrierIndex < ActiveCarriers.Num(); ++CarrierIndex)
	{
		UOMCarryComponent* Carrier = ActiveCarriers[CarrierIndex];
		AOMMouseCharacter* Character = IsValid(Carrier) ? Cast<AOMMouseCharacter>(Carrier->GetOwner()) : nullptr;
		USceneComponent* CharacterCarryPoint = IsValid(Carrier) ? Carrier->GetCarryPoint() : nullptr;
		USceneComponent* Slot = GetSlotForCarrierIndex(CarrierIndex);
		if (!IsValid(Character) || !IsValid(CharacterCarryPoint) || !IsValid(Slot))
		{
			bAllAligned = false;
			continue;
		}

		const FVector AlignmentDelta = Slot->GetComponentLocation() - CharacterCarryPoint->GetComponentLocation();
		FHitResult AlignmentHit;
		Character->SetActorLocation(
			Character->GetActorLocation() + AlignmentDelta,
			true,
			&AlignmentHit,
			ETeleportType::None);
		const float RemainingError = FVector::Dist(
			CharacterCarryPoint->GetComponentLocation(),
			Slot->GetComponentLocation());
		if (AlignmentHit.bStartPenetrating || RemainingError > AlignmentTolerance)
		{
			bAllAligned = false;
			UE_LOG(LogOperationMouse, Log,
				TEXT("[HeavyCarry][Alignment] Target=%s Holder=%s Result=AdjustPosition Error=%.2f Tolerance=%.2f Hit=%s"),
				*GetName(), *GetNameSafe(Character), RemainingError, AlignmentTolerance,
				AlignmentHit.bBlockingHit ? *GetNameSafe(AlignmentHit.GetActor()) : TEXT("None"));
		}
	}
	return bAllAligned;
}

bool AOMHeavyCarryableActor::TryEnterCarrying()
{
	if (!HasAuthority() || ActiveCarriers.Num() != 2)
	{
		return false;
	}

	FTransform DesiredTransform;
	if (!BuildDesiredHeavyTransform(DesiredTransform))
	{
		SetHeavyCarryObstructed(true, FHitResult());
		return false;
	}

	const FTransform OriginalTransform = GetActorTransform();
	FHitResult Hit;
	SetActorLocationAndRotation(
		DesiredTransform.GetLocation(),
		DesiredTransform.GetRotation(),
		true,
		&Hit,
		ETeleportType::None);
	const bool bReachedTarget = !Hit.bStartPenetrating
		&& FVector::Dist(GetActorLocation(), DesiredTransform.GetLocation()) <= GetHolderAlignmentTolerance()
		&& GetActorQuat().AngularDistance(DesiredTransform.GetRotation()) <= FMath::DegreesToRadians(2.0f);
	if (!bReachedTarget)
	{
		SetActorTransform(OriginalTransform, false, nullptr, ETeleportType::TeleportPhysics);
		SetHeavyCarryObstructed(true, Hit);
		return false;
	}

	SetHeavyCarryObstructed(false, FHitResult());
	SetHeavyCarryState(EOMHeavyCarryState::Carrying);
	SetMovementPenaltyForAllHolders(true);
	RefreshCarrierCollisionIgnores();
	UE_LOG(LogOperationMouse, Log,
		TEXT("[HeavyCarry][Startup] Target=%s Result=Carrying Holders=2"), *GetName());
	return true;
}

bool AOMHeavyCarryableActor::HaveHoldersAdjustedSinceFailedAlignment() const
{
	const TArray<AOMMouseCharacter*> Holders = GetPresentationHolders();
	if (Holders.Num() != 2 || !IsValid(Holders[0]) || !IsValid(Holders[1]))
	{
		return false;
	}

	const float RequiredAdjustment = GetHolderAlignmentTolerance();
	return FVector::Dist2D(Holders[0]->GetActorLocation(), FailedAlignmentFirstHolderLocation) >= RequiredAdjustment
		|| FVector::Dist2D(Holders[1]->GetActorLocation(), FailedAlignmentSecondHolderLocation) >= RequiredAdjustment;
}

float AOMHeavyCarryableActor::GetHolderAlignmentTolerance() const
{
	float SmallestCapsuleRadius = TNumericLimits<float>::Max();
	for (AOMMouseCharacter* Holder : GetPresentationHolders())
	{
		if (const UCapsuleComponent* Capsule = IsValid(Holder) ? Holder->GetCapsuleComponent() : nullptr)
		{
			SmallestCapsuleRadius = FMath::Min(SmallestCapsuleRadius, Capsule->GetScaledCapsuleRadius());
		}
	}
	return SmallestCapsuleRadius < TNumericLimits<float>::Max()
		? SmallestCapsuleRadius * 0.25f
		: UE_KINDA_SMALL_NUMBER;
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

void AOMHeavyCarryableActor::OnRep_HeavyCarryWorldState()
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
		ApplyReplicatedIdleWorldPresentation();
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

void AOMHeavyCarryableActor::PublishAuthoritativeWorldPresentation(const FTransform& TargetTransform)
{
	if (!HasAuthority())
	{
		return;
	}

	RestoreWorldPresentation(TargetTransform);
	ReplicatedHeavyWorldState.Transform = GetActorTransform();
	ReplicatedHeavyWorldState.bSimulatePhysics = bSavedHeavySimulatePhysics;
	++ReplicatedHeavyWorldState.Revision;
	ForceNetUpdate();
}

void AOMHeavyCarryableActor::ApplyReplicatedIdleWorldPresentation()
{
	if (HasAuthority() || HeavyCarryState != EOMHeavyCarryState::Idle
		|| ReplicatedHeavyWorldState.Revision == 0
		|| ReplicatedHeavyWorldState.Revision == AppliedHeavyWorldStateRevision)
	{
		return;
	}

	UStaticMeshComponent* HeavyMesh = FindComponentByClass<UStaticMeshComponent>();
	if (!HeavyMesh)
	{
		return;
	}

	HeavyMesh->SetSimulatePhysics(false);
	SetActorTransform(
		ReplicatedHeavyWorldState.Transform,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	HeavyMesh->SetCollisionEnabled(SavedHeavyCollision);
	HeavyMesh->SetCollisionResponseToChannel(ECC_Pawn, SavedHeavyPawnCollisionResponse);
	HeavyMesh->SetSimulatePhysics(ReplicatedHeavyWorldState.bSimulatePhysics);
	if (ReplicatedHeavyWorldState.bSimulatePhysics)
	{
		HeavyMesh->WakeAllRigidBodies();
	}

	bHeavyPresentationSaved = false;
	AppliedHeavyWorldStateRevision = ReplicatedHeavyWorldState.Revision;
}

void AOMHeavyCarryableActor::UpdateHeavyCarryTransform()
{
	FTransform DesiredTransform;
	if (!BuildDesiredHeavyTransform(DesiredTransform))
	{
		SetHeavyCarryObstructed(true, FHitResult());
		return;
	}

	FHitResult Hit;
	SetActorLocationAndRotation(
		DesiredTransform.GetLocation(),
		DesiredTransform.GetRotation(),
		true,
		&Hit,
		ETeleportType::None);
	SetHeavyCarryObstructed(Hit.bBlockingHit || Hit.bStartPenetrating, Hit);
}

bool AOMHeavyCarryableActor::BuildDesiredHeavyTransform(FTransform& OutDesiredTransform) const
{
	if (ActiveCarriers.Num() != 2)
	{
		return false;
	}
	const USceneComponent* FirstPoint = ActiveCarriers[0]->GetCarryPoint();
	const USceneComponent* SecondPoint = ActiveCarriers[1]->GetCarryPoint();
	if (!IsValid(FirstPoint) || !IsValid(SecondPoint))
	{
		return false;
	}
	const FVector FirstLocation = FirstPoint->GetComponentLocation();
	const FVector SecondLocation = SecondPoint->GetComponentLocation();
	const FVector HolderSeparation = SecondLocation - FirstLocation;
	const float MinimumStableSeparation = GetMinimumStableHolderSeparation();
	if (HolderSeparation.Size2D() < MinimumStableSeparation)
	{
		return false;
	}

	OutDesiredTransform = GetActorTransform();
	if (HolderSeparation.SizeSquared2D() > UE_KINDA_SMALL_NUMBER)
	{
		const float TargetYaw = HolderSeparation.Rotation().Yaw - 90.0f;
		OutDesiredTransform.SetRotation(FRotator(0.0f, TargetYaw, 0.0f).Quaternion());
	}

	const FVector SlotMidpointLocal =
		(LeftCarrySlot->GetRelativeLocation() + RightCarrySlot->GetRelativeLocation()) * 0.5f;
	const FVector DesiredMidpoint = (FirstLocation + SecondLocation) * 0.5f;
	OutDesiredTransform.SetLocation(
		DesiredMidpoint - OutDesiredTransform.TransformVector(SlotMidpointLocal));
	return true;
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
	const float MinimumStableSeparation = GetMinimumStableHolderSeparation();
	if (TowardOther.Size2D() <= MinimumStableSeparation + HolderSeparationSafetyMargin)
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

float AOMHeavyCarryableActor::GetMinimumStableHolderSeparation() const
{
	if (!IsValid(LeftCarrySlot) || !IsValid(RightCarrySlot))
	{
		return 0.0f;
	}

	const FVector WorldSlotSeparation =
		LeftCarrySlot->GetComponentLocation() - RightCarrySlot->GetComponentLocation();
	return WorldSlotSeparation.Size2D() * 0.5f;
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
	case EOMHeavyCarryState::WaitingForValidPositions:
		Text->SetText(NSLOCTEXT("OperationMouse", "HeavyCarryAdjustState", "HEAVY: ADJUST POSITION 2/2"));
		Text->SetTextRenderColor(FColor::Orange);
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
	DOREPLIFETIME(AOMHeavyCarryableActor, ReplicatedHeavyWorldState);
	DOREPLIFETIME(AOMHeavyCarryableActor, HeavyObstructionNormal);
}
