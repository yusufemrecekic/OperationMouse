#include "OMCarryComponent.h"

#include "OMCarryableActor.h"
#include "Components/SceneComponent.h"
#include "../Characters/OMMouseCharacter.h"
#include "Net/UnrealNetwork.h"
#include "../OperationMouse.h"

UOMCarryComponent::UOMCarryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
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

	if (CarriedActor && !IsValid(CarriedActor))
	{
		return Reject(TEXT("InvalidCarriedActor"));
	}

	if (!IsValid(GetOwner()))
	{
		return Reject(TEXT("InvalidOwner"));
	}
	if (!IsValid(CarryPoint))
	{
		return Reject(TEXT("MissingCarryPoint"));
	}
	if (IsValid(CarriedActor))
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
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(
			LogOperationMouse,
			Warning,
			TEXT("[Carry][Rejected] Carrier=%s Target=%s Reason=NotAuthority"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(Candidate));
		return false;
	}

	FString FailureReason;
	if (!CanGrabWithReason(Candidate, FailureReason))
	{
		UE_LOG(
			LogOperationMouse,
			Log,
			TEXT("[Carry][Rejected] Carrier=%s Target=%s Reason=%s"),
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
			TEXT("[Carry][Rejected] Carrier=%s Target=%s Reason=BeginCarryRejected"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(Candidate));
		return false;
	}

	SetAuthoritativeCarriedActor(Carryable);
	Carryable->OnDestroyed.AddDynamic(this, &UOMCarryComponent::HandleCarriedActorDestroyed);
	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[Carry][Grabbed] Carrier=%s Target=%s Authority=Server"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(CarriedActor));
	return true;
}

void UOMCarryComponent::RequestDrop()
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		Drop();
		return;
	}

	ServerRequestDrop();
}

void UOMCarryComponent::ServerRequestDrop_Implementation()
{
	Drop();
}

bool UOMCarryComponent::Drop()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogOperationMouse, Warning, TEXT("[Carry][Rejected] Carrier=%s Target=%s Reason=NotAuthority"), *GetNameSafe(GetOwner()), *GetNameSafe(CarriedActor));
		return false;
	}

	if (!IsValid(CarriedActor))
	{
		UE_LOG(
			LogOperationMouse,
			Log,
			TEXT("[Carry][Rejected] Carrier=%s Target=%s Reason=NotCarrying"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(CarriedActor));
		SetAuthoritativeCarriedActor(nullptr);
		return false;
	}

	AOMMouseCharacter* OwnerCharacter = Cast<AOMMouseCharacter>(GetOwner());
	if (!CarriedActor->IsHeldBy(OwnerCharacter))
	{
		UE_LOG(
			LogOperationMouse,
			Warning,
			TEXT("[Carry][Rejected] Carrier=%s Target=%s Reason=HolderMismatch ActualHolder=%s"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(CarriedActor),
			*GetNameSafe(CarriedActor->GetCurrentHolder()));
		SetAuthoritativeCarriedActor(nullptr);
		return false;
	}

	AOMCarryableActor* DroppedActor = CarriedActor;
	const FVector DropLocation = GetOwner()->GetActorLocation()
		+ GetOwner()->GetActorForwardVector() * DropForwardDistance
		+ FVector::UpVector * DropHeightOffset;
	if (!DroppedActor->EndCarry(this, DropLocation))
	{
		UE_LOG(LogOperationMouse, Warning, TEXT("[Carry][Rejected] Carrier=%s Target=%s Reason=EndCarryRejected"), *GetNameSafe(GetOwner()), *GetNameSafe(DroppedActor));
		return false;
	}
	DroppedActor->OnDestroyed.RemoveDynamic(this, &UOMCarryComponent::HandleCarriedActorDestroyed);
	SetAuthoritativeCarriedActor(nullptr);

	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[Carry][Dropped] Carrier=%s Target=%s Authority=Server"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(DroppedActor));
	return true;
}

bool UOMCarryComponent::IsCarrying() const
{
	return IsValid(CarriedActor);
}

EOMCarryState UOMCarryComponent::GetCarryState() const
{
	return IsCarrying() ? EOMCarryState::Carrying : EOMCarryState::Idle;
}

AOMCarryableActor* UOMCarryComponent::GetCarriedActor() const
{
	return IsCarrying() ? CarriedActor.Get() : nullptr;
}

void UOMCarryComponent::ReleaseForRecovery(AOMCarryableActor* Carryable)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (CarriedActor != Carryable)
	{
		return;
	}

	if (IsValid(CarriedActor))
	{
		CarriedActor->OnDestroyed.RemoveDynamic(this, &UOMCarryComponent::HandleCarriedActorDestroyed);
	}
	SetAuthoritativeCarriedActor(nullptr);
	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[Carry][Recovered] Carrier=%s Target=%s Reason=TargetReset Authority=Server"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(Carryable));
}

void UOMCarryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwner() && GetOwner()->HasAuthority() && IsCarrying())
	{
		Drop();
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
		TEXT("[Carry][Recovered] Carrier=%s Target=%s Reason=TargetDestroyed Authority=Server"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(DestroyedActor));
	SetAuthoritativeCarriedActor(nullptr);
}

void UOMCarryComponent::SetAuthoritativeCarriedActor(AOMCarryableActor* NewCarriedActor)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	CarriedActor = NewCarriedActor;
	GetOwner()->ForceNetUpdate();
}

void UOMCarryComponent::OnRep_CarriedActor(AOMCarryableActor* PreviousCarriedActor)
{
	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[Carry][Replicated] Carrier=%s Previous=%s Current=%s State=%s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(PreviousCarriedActor),
		*GetNameSafe(CarriedActor),
		IsCarrying() ? TEXT("Carrying") : TEXT("Idle"));
}

void UOMCarryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UOMCarryComponent, CarriedActor);
}
