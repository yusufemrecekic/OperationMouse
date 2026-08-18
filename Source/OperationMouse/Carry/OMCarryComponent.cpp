#include "OMCarryComponent.h"

#include "OMCarryableActor.h"
#include "Components/SceneComponent.h"
#include "../OperationMouse.h"

UOMCarryComponent::UOMCarryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UOMCarryComponent::SetCarryPoint(USceneComponent* NewCarryPoint)
{
	CarryPoint = NewCarryPoint;
}

bool UOMCarryComponent::CanGrab(AActor* Candidate)
{
	FString UnusedReason;
	return CanGrabWithReason(Candidate, UnusedReason);
}

bool UOMCarryComponent::CanGrabWithReason(AActor* Candidate, FString& OutReason)
{
	auto Reject = [&OutReason](const TCHAR* Reason)
	{
		OutReason = Reason;
		return false;
	};

	if (CarryState == EOMCarryState::Carrying && !IsValid(CarriedActor))
	{
		UE_LOG(
			LogOperationMouse,
			Log,
			TEXT("[Carry][Recovered] Carrier=%s Target=None Reason=InvalidCarriedActor Authority=PENDING"),
			*GetNameSafe(GetOwner()));
		ClearCarryState();
	}

	if (!IsValid(GetOwner()))
	{
		return Reject(TEXT("InvalidOwner"));
	}
	if (!IsValid(CarryPoint))
	{
		return Reject(TEXT("MissingCarryPoint"));
	}
	if (CarryState != EOMCarryState::Idle || IsValid(CarriedActor))
	{
		return Reject(TEXT("AlreadyCarrying"));
	}

	if (!IsValid(Candidate))
	{
		return Reject(TEXT("InvalidActor"));
	}
	const AOMCarryableActor* Carryable = Cast<AOMCarryableActor>(Candidate);
	if (!Carryable)
	{
		return Reject(TEXT("NotCarryable"));
	}
	if (!Carryable->IsAvailableForGrab())
	{
		return Reject(TEXT("CarryableUnavailable"));
	}

	OutReason.Reset();
	return true;
}

bool UOMCarryComponent::TryGrab(AActor* Candidate)
{
	FString FailureReason;
	if (!CanGrabWithReason(Candidate, FailureReason))
	{
		UE_LOG(
			LogOperationMouse,
			Log,
			TEXT("[Carry][Rejected] Carrier=%s Target=%s Reason=%s Authority=PENDING"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(Candidate),
			*FailureReason);
		return false;
	}

	AOMCarryableActor* Carryable = CastChecked<AOMCarryableActor>(Candidate);
	if (!Carryable->BeginCarry(this, CarryPoint))
	{
		UE_LOG(
			LogOperationMouse,
			Log,
			TEXT("[Carry][Rejected] Carrier=%s Target=%s Reason=BeginCarryRejected Authority=PENDING"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(Candidate));
		return false;
	}

	CarriedActor = Carryable;
	CarryState = EOMCarryState::Carrying;
	Carryable->OnDestroyed.AddDynamic(this, &UOMCarryComponent::HandleCarriedActorDestroyed);
	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[Carry][Grabbed] Carrier=%s Target=%s Authority=PENDING"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(CarriedActor));
	return true;
}

bool UOMCarryComponent::Drop()
{
	if (!IsValid(CarriedActor) || CarryState != EOMCarryState::Carrying)
	{
		UE_LOG(
			LogOperationMouse,
			Log,
			TEXT("[Carry][Rejected] Carrier=%s Target=%s Reason=NotCarrying Authority=PENDING"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(CarriedActor));
		ClearCarryState();
		return false;
	}

	AOMCarryableActor* DroppedActor = CarriedActor;
	DroppedActor->OnDestroyed.RemoveDynamic(this, &UOMCarryComponent::HandleCarriedActorDestroyed);
	const FVector DropLocation = GetOwner()->GetActorLocation()
		+ GetOwner()->GetActorForwardVector() * DropForwardDistance
		+ FVector::UpVector * DropHeightOffset;
	ClearCarryState();
	DroppedActor->EndCarry(this, DropLocation);

	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[Carry][Dropped] Carrier=%s Target=%s Authority=PENDING"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(DroppedActor));
	return true;
}

bool UOMCarryComponent::IsCarrying() const
{
	return CarryState == EOMCarryState::Carrying && IsValid(CarriedActor);
}

AOMCarryableActor* UOMCarryComponent::GetCarriedActor() const
{
	return IsCarrying() ? CarriedActor.Get() : nullptr;
}

void UOMCarryComponent::ReleaseForRecovery(AOMCarryableActor* Carryable)
{
	if (CarriedActor != Carryable)
	{
		return;
	}

	if (IsValid(CarriedActor))
	{
		CarriedActor->OnDestroyed.RemoveDynamic(this, &UOMCarryComponent::HandleCarriedActorDestroyed);
	}
	ClearCarryState();
	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[Carry][Recovered] Carrier=%s Target=%s Reason=TargetReset Authority=PENDING"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(Carryable));
}

void UOMCarryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsCarrying())
	{
		Drop();
	}
	else
	{
		ClearCarryState();
	}

	Super::EndPlay(EndPlayReason);
}

void UOMCarryComponent::HandleCarriedActorDestroyed(AActor* DestroyedActor)
{
	if (DestroyedActor != CarriedActor)
	{
		return;
	}

	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[Carry][Recovered] Carrier=%s Target=%s Reason=TargetDestroyed Authority=PENDING"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(DestroyedActor));
	ClearCarryState();
}

void UOMCarryComponent::ClearCarryState()
{
	CarriedActor = nullptr;
	CarryState = EOMCarryState::Idle;
}
