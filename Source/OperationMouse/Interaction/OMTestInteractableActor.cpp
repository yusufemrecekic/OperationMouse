#include "OMTestInteractableActor.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "../OperationMouse.h"
#include "UObject/ConstructorHelpers.h"

AOMTestInteractableActor::AOMTestInteractableActor()
{
	bReplicates = true;
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(SceneRoot);
	Mesh->SetRelativeScale3D(FVector(0.75f, 0.75f, 0.75f));
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}

	StatusText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StatusText"));
	StatusText->SetupAttachment(SceneRoot);
	StatusText->SetRelativeLocation(FVector(0.0f, 0.0f, 105.0f));
	StatusText->SetHorizontalAlignment(EHTA_Center);
	StatusText->SetWorldSize(24.0f);

	StatusLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StatusLight"));
	StatusLight->SetupAttachment(SceneRoot);
	StatusLight->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	StatusLight->SetIntensity(1800.0f);
	StatusLight->SetAttenuationRadius(220.0f);

	InteractionInfo.Prompt = NSLOCTEXT("OperationMouse", "TestInteractPrompt", "Activate");
	InteractionInfo.Type = EOMInteractionType::Instant;
	InteractionInfo.HoldDuration = 2.0f;
	InteractionInfo.MaximumDistance = 200.0f;
	InteractionInfo.bExclusive = true;
}

void AOMTestInteractableActor::BeginPlay()
{
	Super::BeginPlay();
	UpdateVisualState();
}

FOMInteractionInfo AOMTestInteractableActor::GetInteractionInfo_Implementation(AActor* Interactor) const
{
	return BuildInteractionInfo();
}

FVector AOMTestInteractableActor::GetInteractionPoint_Implementation() const
{
	return GetActorLocation() + FVector::UpVector * 45.0f;
}

bool AOMTestInteractableActor::CanInteract_Implementation(AActor* Interactor) const
{
	if (!Interactor)
	{
		return false;
	}

	const FOMInteractionInfo Info = BuildInteractionInfo();
	if (TestRole != EOMTestInteractionRole::Reset && TestRole != EOMTestInteractionRole::Fail && bActivated)
	{
		return false;
	}

	return !Info.bExclusive || !ActiveInteractor || ActiveInteractor == Interactor;
}

bool AOMTestInteractableActor::BeginInteraction_Implementation(AActor* Interactor)
{
	if (!HasAuthority() || !CanInteract_Implementation(Interactor))
	{
		return false;
	}

	if (TestRole == EOMTestInteractionRole::Fail)
	{
		UE_LOG(LogOperationMouse, Log, TEXT("[Interaction][ExpectedFail] Interactor=%s Target=%s"), *GetNameSafe(Interactor), *GetName());
		return false;
	}

	if (BuildInteractionInfo().bExclusive)
	{
		ActiveInteractor = Interactor;
		UpdateVisualState();
		ForceNetUpdate();
	}

	return true;
}

void AOMTestInteractableActor::CancelInteraction_Implementation(AActor* Interactor)
{
	if (!HasAuthority() || ActiveInteractor != Interactor || bActivated)
	{
		return;
	}

	ActiveInteractor = nullptr;
	UpdateVisualState();
	ForceNetUpdate();
}

void AOMTestInteractableActor::CompleteInteraction_Implementation(AActor* Interactor)
{
	const FOMInteractionInfo Info = BuildInteractionInfo();
	if (!HasAuthority() || (TestRole != EOMTestInteractionRole::Reset && bActivated) || (Info.bExclusive && ActiveInteractor != Interactor))
	{
		return;
	}

	if (TestRole == EOMTestInteractionRole::Reset)
	{
		for (TActorIterator<AOMTestInteractableActor> It(GetWorld()); It; ++It)
		{
			It->ResetTestState();
		}
		UE_LOG(LogOperationMouse, Log, TEXT("[Interaction][Reset] Interactor=%s Target=%s"), *GetNameSafe(Interactor), *GetName());
		return;
	}

	bActivated = true;
	ActiveInteractor = nullptr;
	++CompletionCount;
	UpdateVisualState();
	ForceNetUpdate();
}

void AOMTestInteractableActor::ResetTestState()
{
	if (!HasAuthority())
	{
		return;
	}

	bActivated = false;
	ActiveInteractor = nullptr;
	CompletionCount = 0;
	UpdateVisualState();
	ForceNetUpdate();
}

void AOMTestInteractableActor::OnRep_InteractionState()
{
	UpdateVisualState();
}

void AOMTestInteractableActor::UpdateVisualState()
{
	Mesh->SetVisibility(true);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetRelativeLocation(FVector::ZeroVector);
	Mesh->SetRelativeRotation(FRotator::ZeroRotator);

	if (bActivated)
	{
		StatusText->SetText(FText::Format(NSLOCTEXT("OperationMouse", "InteractionActivatedState", "{0}: COMPLETE"), GetRoleLabel()));
		StatusText->SetTextRenderColor(FColor::Green);
		StatusLight->SetLightColor(FLinearColor::Green);
		if (TestRole == EOMTestInteractionRole::Pickup)
		{
			Mesh->SetVisibility(false);
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		else if (TestRole == EOMTestInteractionRole::Door)
		{
			Mesh->SetRelativeRotation(FRotator(0.0f, 80.0f, 0.0f));
		}
		else
		{
			Mesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 25.0f));
		}
	}
	else if (ActiveInteractor)
	{
		StatusText->SetText(NSLOCTEXT("OperationMouse", "InteractionInUseState", "IN USE"));
		StatusText->SetTextRenderColor(FColor::Yellow);
		StatusLight->SetLightColor(FLinearColor::Yellow);
		Mesh->SetRelativeRotation(FRotator::ZeroRotator);
	}
	else
	{
		const bool bExpectedFail = TestRole == EOMTestInteractionRole::Fail;
		StatusText->SetText(FText::Format(
			bExpectedFail
				? NSLOCTEXT("OperationMouse", "InteractionExpectedFailState", "{0}: EXPECTED FAIL")
				: NSLOCTEXT("OperationMouse", "InteractionReadyState", "{0}: READY"),
			GetRoleLabel()));
		StatusText->SetTextRenderColor(bExpectedFail ? FColor::Red : FColor::Cyan);
		StatusLight->SetLightColor(bExpectedFail ? FLinearColor::Red : FLinearColor(0.0f, 0.5f, 1.0f));
	}
}

FOMInteractionInfo AOMTestInteractableActor::BuildInteractionInfo() const
{
	FOMInteractionInfo Info = InteractionInfo;
	Info.Type = EOMInteractionType::Instant;
	Info.bExclusive = true;

	switch (TestRole)
	{
	case EOMTestInteractionRole::Button:
		Info.Prompt = NSLOCTEXT("OperationMouse", "Sprint1PressButton", "Press Button");
		break;
	case EOMTestInteractionRole::Pickup:
		Info.Prompt = NSLOCTEXT("OperationMouse", "Sprint1Pickup", "Pick Up Proxy");
		break;
	case EOMTestInteractionRole::Door:
		Info.Prompt = NSLOCTEXT("OperationMouse", "Sprint1OpenDoor", "Open Door");
		break;
	case EOMTestInteractionRole::Fail:
		Info.Prompt = NSLOCTEXT("OperationMouse", "Sprint1ExpectedFail", "Test Expected Failure");
		break;
	case EOMTestInteractionRole::Reset:
		Info.Prompt = NSLOCTEXT("OperationMouse", "Sprint1Reset", "Reset Test Proxies");
		break;
	case EOMTestInteractionRole::Generic:
	default:
		break;
	}

	return Info;
}

FText AOMTestInteractableActor::GetRoleLabel() const
{
	switch (TestRole)
	{
	case EOMTestInteractionRole::Button:
		return NSLOCTEXT("OperationMouse", "Sprint1ButtonLabel", "BUTTON");
	case EOMTestInteractionRole::Pickup:
		return NSLOCTEXT("OperationMouse", "Sprint1PickupLabel", "PICKUP");
	case EOMTestInteractionRole::Door:
		return NSLOCTEXT("OperationMouse", "Sprint1DoorLabel", "DOOR");
	case EOMTestInteractionRole::Fail:
		return NSLOCTEXT("OperationMouse", "Sprint1FailLabel", "FAIL");
	case EOMTestInteractionRole::Reset:
		return NSLOCTEXT("OperationMouse", "Sprint1ResetLabel", "RESET");
	case EOMTestInteractionRole::Generic:
	default:
		return NSLOCTEXT("OperationMouse", "Sprint1GenericLabel", "INTERACTION");
	}
}

void AOMTestInteractableActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AOMTestInteractableActor, bActivated);
	DOREPLIFETIME(AOMTestInteractableActor, ActiveInteractor);
	DOREPLIFETIME(AOMTestInteractableActor, CompletionCount);
}
