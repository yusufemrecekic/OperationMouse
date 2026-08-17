#include "OMInteractionComponent.h"

#include "OMInteractableInterface.h"
#include "OMInteractionPromptWidget.h"
#include "../OperationMouse.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

UOMInteractionComponent::UOMInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	SetIsReplicatedByDefault(true);
}

void UOMInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<ACharacter>(GetOwner());

	if (!OwnerCharacter)
	{
		UE_LOG(LogOperationMouse, Error, TEXT("InteractionComponent requires an ACharacter owner: %s"), *GetNameSafe(GetOwner()));
	}
}

void UOMInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PromptWidget)
	{
		PromptWidget->RemoveFromParent();
	}

	Super::EndPlay(EndPlayReason);
}

void UOMInteractionComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OwnerCharacter)
	{
		return;
	}

	if (OwnerCharacter->IsLocallyControlled())
	{
		EnsurePromptWidget();
		FindLocalFocus();
		RefreshPrompt();
	}

	if (GetOwner()->HasAuthority() && ActiveInteractionTarget)
	{
		FString FailureReason;
		if (!IsServerInteractionValid(ActiveInteractionTarget, false, &FailureReason))
		{
			UE_LOG(
				LogOperationMouse,
				Log,
				TEXT("[Interaction][Canceled] Interactor=%s Target=%s Reason=%s"),
				*GetNameSafe(OwnerCharacter),
				*GetNameSafe(ActiveInteractionTarget),
				*FailureReason);
			CancelActiveInteraction();
			return;
		}

		if (GetSynchronizedTimeSeconds() >= ActiveHoldServerStartTime + ActiveHoldDuration)
		{
			AActor* CompletedTarget = ActiveInteractionTarget;
			IOMInteractableInterface::Execute_CompleteInteraction(CompletedTarget, OwnerCharacter);
			UE_LOG(
				LogOperationMouse,
				Log,
				TEXT("[Interaction][Completed] Interactor=%s Target=%s Type=Hold"),
				*GetNameSafe(OwnerCharacter),
				*GetNameSafe(CompletedTarget));
			ActiveInteractionTarget = nullptr;
			ActiveHoldServerStartTime = 0.0f;
			ActiveHoldDuration = 0.0f;
			GetOwner()->ForceNetUpdate();
		}
	}
}

void UOMInteractionComponent::BeginInteractionInput()
{
	bLocalInputHeld = true;

	if (FocusedActor)
	{
		ServerBeginInteraction(FocusedActor);
	}
}

void UOMInteractionComponent::EndInteractionInput()
{
	bLocalInputHeld = false;
	ServerEndInteraction();
}

void UOMInteractionComponent::FindLocalFocus()
{
	AActor* NewFocus = nullptr;
	const APlayerController* PlayerController = Cast<APlayerController>(OwnerCharacter->GetController());
	const UWorld* World = GetWorld();
	if (PlayerController && World && IsCharacterStateValid())
	{
		const FVector TraceStart = OwnerCharacter->GetActorLocation() + FVector::UpVector * 50.0f;
		const FVector TraceDirection = PlayerController->GetControlRotation().Vector();
		const FVector TraceEnd = TraceStart + TraceDirection * DetectionDistance;

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OMInteractionFocus), false, OwnerCharacter);
		FHitResult Hit;
		const bool bHit = World->SweepSingleByChannel(
			Hit,
			TraceStart,
			TraceEnd,
			FQuat::Identity,
			ECC_Visibility,
			FCollisionShape::MakeSphere(DetectionRadius),
			QueryParams);

		if (bDrawDebug)
		{
			DrawDebugLine(World, TraceStart, TraceEnd, bHit ? FColor::Green : FColor::Red, false, 0.0f, 0, 1.5f);
			DrawDebugSphere(World, bHit ? Hit.ImpactPoint : TraceEnd, DetectionRadius, 12, bHit ? FColor::Green : FColor::Red, false, 0.0f);
		}

		if (bHit && Hit.GetActor() && Hit.GetActor()->Implements<UOMInteractableInterface>())
		{
			if (IOMInteractableInterface::Execute_CanInteract(Hit.GetActor(), OwnerCharacter))
			{
				NewFocus = Hit.GetActor();
			}
		}
	}

	FocusedActor = NewFocus;
}

void UOMInteractionComponent::EnsurePromptWidget()
{
	if (PromptWidget)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	PromptWidget = CreateWidget<UOMInteractionPromptWidget>(PlayerController, UOMInteractionPromptWidget::StaticClass());
	if (PromptWidget)
	{
		PromptWidget->AddToViewport();
		PromptWidget->HidePrompt();
	}
}

void UOMInteractionComponent::RefreshPrompt()
{
	if (!PromptWidget)
	{
		return;
	}

	AActor* DisplayTarget = ActiveInteractionTarget && bLocalInputHeld ? ActiveInteractionTarget.Get() : FocusedActor.Get();
	if (!DisplayTarget || !DisplayTarget->Implements<UOMInteractableInterface>())
	{
		PromptWidget->HidePrompt();
		return;
	}

	const FOMInteractionInfo Info = IOMInteractableInterface::Execute_GetInteractionInfo(DisplayTarget, OwnerCharacter);
	const bool bActiveHold = ActiveInteractionTarget == DisplayTarget && ActiveHoldDuration > 0.0f;
	const float Progress = bActiveHold
		? (GetSynchronizedTimeSeconds() - ActiveHoldServerStartTime) / ActiveHoldDuration
		: 0.0f;
	const FText Prompt = Info.Type == EOMInteractionType::Hold
		? FText::Format(NSLOCTEXT("OperationMouse", "HoldInteractionPrompt", "Hold [E] {0}"), Info.Prompt)
		: FText::Format(NSLOCTEXT("OperationMouse", "InstantInteractionPrompt", "[E] {0}"), Info.Prompt);

	PromptWidget->SetPromptState(Prompt, bActiveHold, Progress);
}

bool UOMInteractionComponent::IsServerInteractionValid(AActor* Target, bool bCheckInteractableState, FString* OutFailureReason) const
{
	auto Reject = [OutFailureReason](const TCHAR* Reason)
	{
		if (OutFailureReason)
		{
			*OutFailureReason = Reason;
		}
		return false;
	};

	if (!OwnerCharacter)
	{
		return Reject(TEXT("MissingOwnerCharacter"));
	}
	if (!IsValid(Target))
	{
		return Reject(TEXT("InvalidTarget"));
	}
	if (!Target->Implements<UOMInteractableInterface>())
	{
		return Reject(TEXT("MissingInteractableInterface"));
	}
	if (!IsCharacterStateValid())
	{
		return Reject(TEXT("InvalidCharacterState"));
	}

	const FOMInteractionInfo Info = IOMInteractableInterface::Execute_GetInteractionInfo(Target, OwnerCharacter);
	const FVector InteractionPoint = IOMInteractableInterface::Execute_GetInteractionPoint(Target);
	const float AllowedDistance = FMath::Min(Info.MaximumDistance, MaximumServerDistance);
	if (FVector::DistSquared(OwnerCharacter->GetActorLocation(), InteractionPoint) > FMath::Square(AllowedDistance))
	{
		return Reject(TEXT("OutOfRange"));
	}

	if (bCheckInteractableState && !IOMInteractableInterface::Execute_CanInteract(Target, OwnerCharacter))
	{
		return Reject(TEXT("CanInteractRejected"));
	}

	FHitResult SightHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OMInteractionValidation), false, OwnerCharacter);
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
		SightHit,
		OwnerCharacter->GetActorLocation() + FVector::UpVector * 50.0f,
		InteractionPoint,
		ECC_Visibility,
		QueryParams);
	if (bBlocked && SightHit.GetActor() != Target)
	{
		return Reject(TEXT("LineOfSightBlocked"));
	}

	if (OutFailureReason)
	{
		OutFailureReason->Reset();
	}
	return true;
}

bool UOMInteractionComponent::IsCharacterStateValid() const
{
	if (!OwnerCharacter)
	{
		return false;
	}

	const UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement();
	return MovementComponent && MovementComponent->IsMovingOnGround();
}

void UOMInteractionComponent::ServerBeginInteraction_Implementation(AActor* Target)
{
	if (ActiveInteractionTarget)
	{
		UE_LOG(
			LogOperationMouse,
			Log,
			TEXT("[Interaction][Rejected] Interactor=%s Target=%s Reason=AlreadyHolding"),
			*GetNameSafe(OwnerCharacter),
			*GetNameSafe(Target));
		ClientInteractionRejected();
		return;
	}

	FString FailureReason;
	if (!IsServerInteractionValid(Target, true, &FailureReason))
	{
		UE_LOG(
			LogOperationMouse,
			Log,
			TEXT("[Interaction][Rejected] Interactor=%s Target=%s Reason=%s"),
			*GetNameSafe(OwnerCharacter),
			*GetNameSafe(Target),
			*FailureReason);
		ClientInteractionRejected();
		return;
	}

	const FOMInteractionInfo Info = IOMInteractableInterface::Execute_GetInteractionInfo(Target, OwnerCharacter);
	if (!IOMInteractableInterface::Execute_BeginInteraction(Target, OwnerCharacter))
	{
		UE_LOG(
			LogOperationMouse,
			Log,
			TEXT("[Interaction][Rejected] Interactor=%s Target=%s Reason=BeginInteractionRejected"),
			*GetNameSafe(OwnerCharacter),
			*GetNameSafe(Target));
		ClientInteractionRejected();
		return;
	}

	if (Info.Type == EOMInteractionType::Instant)
	{
		IOMInteractableInterface::Execute_CompleteInteraction(Target, OwnerCharacter);
		UE_LOG(
			LogOperationMouse,
			Log,
			TEXT("[Interaction][Completed] Interactor=%s Target=%s Type=Instant"),
			*GetNameSafe(OwnerCharacter),
			*GetNameSafe(Target));
		return;
	}

	ActiveInteractionTarget = Target;
	ActiveHoldServerStartTime = GetSynchronizedTimeSeconds();
	ActiveHoldDuration = FMath::Max(0.1f, Info.HoldDuration);
	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[Interaction][Accepted] Interactor=%s Target=%s Type=Hold Duration=%.2f"),
		*GetNameSafe(OwnerCharacter),
		*GetNameSafe(Target),
		ActiveHoldDuration);
	GetOwner()->ForceNetUpdate();
}

void UOMInteractionComponent::ServerEndInteraction_Implementation()
{
	CancelActiveInteraction();
}

void UOMInteractionComponent::CancelActiveInteraction()
{
	if (!GetOwner()->HasAuthority() || !ActiveInteractionTarget)
	{
		return;
	}

	AActor* CanceledTarget = ActiveInteractionTarget;
	ActiveInteractionTarget = nullptr;
	ActiveHoldServerStartTime = 0.0f;
	ActiveHoldDuration = 0.0f;
	if (IsValid(CanceledTarget) && CanceledTarget->Implements<UOMInteractableInterface>())
	{
		IOMInteractableInterface::Execute_CancelInteraction(CanceledTarget, OwnerCharacter);
	}
	GetOwner()->ForceNetUpdate();
}

void UOMInteractionComponent::ClientInteractionRejected_Implementation()
{
	bLocalInputHeld = false;
}

void UOMInteractionComponent::OnRep_ActiveInteraction()
{
	RefreshPrompt();
}

float UOMInteractionComponent::GetSynchronizedTimeSeconds() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	const AGameStateBase* GameState = World->GetGameState();
	return GameState ? GameState->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
}

void UOMInteractionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UOMInteractionComponent, ActiveInteractionTarget, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UOMInteractionComponent, ActiveHoldServerStartTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UOMInteractionComponent, ActiveHoldDuration, COND_OwnerOnly);
}
