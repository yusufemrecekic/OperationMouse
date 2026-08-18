#include "OMHeavyCarryableActor.h"

#include "OMCarryComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "../Characters/OMMouseCharacter.h"
#include "../OperationMouse.h"

AOMHeavyCarryableActor::AOMHeavyCarryableActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AOMHeavyCarryableActor::BeginPlay()
{
	Super::BeginPlay();
	HeavyHomeTransform = GetActorTransform();
	UpdateHeavyStatusText();
}

void AOMHeavyCarryableActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetMovementPenaltyForAllHolders(false);
	for (UOMCarryComponent* Carrier : ActiveCarriers)
	{
		if (IsValid(Carrier))
		{
			Carrier->ReleaseForRecovery(this);
		}
	}
	ActiveCarriers.Reset();
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

	RemoveInvalidCarriers();
	if (ActiveCarriers.Contains(NewCarrier) || ActiveCarriers.Num() >= 2)
	{
		return false;
	}

	ActiveCarriers.Add(NewCarrier);
	if (ActiveCarriers.Num() == 1)
	{
		FreezeAtCurrentTransform();
		SetHeavyCarryState(EOMHeavyCarryState::WaitingForSecondHolder);
	}
	else
	{
		SetHeavyCarryState(EOMHeavyCarryState::Carrying);
		SetMovementPenaltyForAllHolders(true);
		if (UStaticMeshComponent* HeavyMesh = FindComponentByClass<UStaticMeshComponent>())
		{
			HeavyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		UpdateHeavyCarryTransform();
	}

	UE_LOG(LogOperationMouse, Log, TEXT("[HeavyCarry][Joined] Carrier=%s Target=%s Holders=%d State=%s GameplayOnly=true"),
		*GetNameSafe(NewCarrier->GetOwner()), *GetName(), ActiveCarriers.Num(),
		HeavyCarryState == EOMHeavyCarryState::Carrying ? TEXT("Carrying") : TEXT("Waiting"));
	return true;
}

bool AOMHeavyCarryableActor::EndCarry(UOMCarryComponent* RequestingCarrier, const FVector& DropLocation)
{
	if (!HasAuthority() || !IsValid(RequestingCarrier) || !ActiveCarriers.Contains(RequestingCarrier))
	{
		return false;
	}

	SetMovementPenaltyForAllHolders(false);
	ActiveCarriers.RemoveSingle(RequestingCarrier);
	RemoveInvalidCarriers();

	if (ActiveCarriers.Num() == 1)
	{
		FreezeAtCurrentTransform();
		SetHeavyCarryState(EOMHeavyCarryState::WaitingForSecondHolder);
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
	return true;
}

bool AOMHeavyCarryableActor::IsHeldBy(const AOMMouseCharacter* Character) const
{
	return ActiveCarriers.ContainsByPredicate([Character](const UOMCarryComponent* Carrier)
	{
		return IsValid(Carrier) && Carrier->GetOwner() == Character;
	});
}

bool AOMHeavyCarryableActor::IsAvailableForGrab() const
{
	return IsValid(this) && !IsActorBeingDestroyed() && ActiveCarriers.Num() < 2;
}

void AOMHeavyCarryableActor::ResetToHome()
{
	if (!HasAuthority())
	{
		return;
	}

	SetMovementPenaltyForAllHolders(false);
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
	UE_LOG(LogOperationMouse, Log, TEXT("[HeavyCarry][Reset] Target=%s Holders=0 GameplayOnly=true"), *GetName());
}

void AOMHeavyCarryableActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority())
	{
		return;
	}

	RemoveInvalidCarriers();
	if (HeavyCarryState == EOMHeavyCarryState::WaitingForSecondHolder)
	{
		if (ActiveCarriers.IsEmpty())
		{
			RestoreWorldPresentation(GetActorTransform());
			SetHeavyCarryState(EOMHeavyCarryState::Idle);
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

void AOMHeavyCarryableActor::FreezeAtCurrentTransform()
{
	UStaticMeshComponent* HeavyMesh = FindComponentByClass<UStaticMeshComponent>();
	if (!HeavyMesh)
	{
		return;
	}
	if (!bHeavyPresentationSaved)
	{
		SavedHeavyCollision = HeavyMesh->GetCollisionEnabled();
		bSavedHeavySimulatePhysics = HeavyMesh->IsSimulatingPhysics();
		bHeavyPresentationSaved = true;
	}
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	HeavyMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	HeavyMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	HeavyMesh->SetSimulatePhysics(false);
	HeavyMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
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
	HeavyMesh->SetSimulatePhysics(bSavedHeavySimulatePhysics);
	bHeavyPresentationSaved = false;
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
	const FVector Midpoint = (FirstPoint->GetComponentLocation() + SecondPoint->GetComponentLocation()) * 0.5f
		+ FVector::UpVector * CarryHeightOffset;
	SetActorLocation(Midpoint, false, nullptr, ETeleportType::TeleportPhysics);
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

void AOMHeavyCarryableActor::RemoveInvalidCarriers()
{
	ActiveCarriers.RemoveAll([](const UOMCarryComponent* Carrier)
	{
		return !IsValid(Carrier) || !IsValid(Carrier->GetOwner());
	});
}
