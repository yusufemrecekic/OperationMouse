#include "OMMissionInteractionActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "OMMissionManager.h"
#include "../OperationMouse.h"
#include "UObject/ConstructorHelpers.h"

AOMMissionInteractionActor::AOMMissionInteractionActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(SceneRoot);
	Mesh->SetRelativeScale3D(FVector(0.7f));
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}

	StatusText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StatusText"));
	StatusText->SetupAttachment(SceneRoot);
	StatusText->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
	StatusText->SetHorizontalAlignment(EHTA_Center);
	StatusText->SetWorldSize(24.0f);
	StatusText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AOMMissionInteractionActor::BeginPlay()
{
	Super::BeginPlay();
	if (MissionManager)
	{
		MissionManager->OnMissionStateChanged.AddDynamic(this, &AOMMissionInteractionActor::HandleMissionStateChanged);
	}
	UpdateVisualState();
}

FOMInteractionInfo AOMMissionInteractionActor::GetInteractionInfo_Implementation(AActor* Interactor) const
{
	FOMInteractionInfo Info;
	Info.Prompt = GetActionLabel();
	Info.Type = EOMInteractionType::Instant;
	Info.MaximumDistance = 220.0f;
	Info.bExclusive = false;
	return Info;
}

FVector AOMMissionInteractionActor::GetInteractionPoint_Implementation() const
{
	return GetActorLocation() + FVector::UpVector * 40.0f;
}

bool AOMMissionInteractionActor::CanInteract_Implementation(AActor* Interactor) const
{
	return IsValid(Interactor) && IsActionAvailable();
}

bool AOMMissionInteractionActor::BeginInteraction_Implementation(AActor* Interactor)
{
	return HasAuthority() && CanInteract_Implementation(Interactor);
}

void AOMMissionInteractionActor::CompleteInteraction_Implementation(AActor* Interactor)
{
	if (!HasAuthority() || !CanInteract_Implementation(Interactor))
	{
		return;
	}

	ExecuteMissionAction(Interactor);
	UpdateVisualState();
}

bool AOMMissionInteractionActor::ExecuteMissionAction(AActor* Interactor)
{
	if (!MissionManager)
	{
		return false;
	}

	switch (MissionAction)
	{
	case EOMMissionInteractionAction::CompleteObjective:
		return MissionManager->CompleteObjective(Interactor, ObjectiveProgressAmount);
	case EOMMissionInteractionAction::Fail:
		return MissionManager->FailMission(Interactor);
	case EOMMissionInteractionAction::Reset:
		return MissionManager->ResetMission(Interactor);
	case EOMMissionInteractionAction::Retry:
		return MissionManager->RetryMission(Interactor);
	case EOMMissionInteractionAction::Start:
	default:
		return MissionManager->StartMission(Interactor);
	}
}

bool AOMMissionInteractionActor::IsActionAvailable() const
{
	if (!MissionManager)
	{
		return false;
	}

	switch (MissionAction)
	{
	case EOMMissionInteractionAction::CompleteObjective:
	case EOMMissionInteractionAction::Fail:
		return MissionManager->CanExecuteAction(EOMMissionState::Active);
	case EOMMissionInteractionAction::Reset:
		return MissionManager->GetMissionState() != EOMMissionState::Inactive;
	case EOMMissionInteractionAction::Retry:
		return MissionManager->GetMissionState() == EOMMissionState::Failed
			|| MissionManager->GetMissionState() == EOMMissionState::Completed;
	case EOMMissionInteractionAction::Start:
	default:
		return MissionManager->CanExecuteAction(EOMMissionState::Inactive);
	}
}

FText AOMMissionInteractionActor::GetActionLabel() const
{
	switch (MissionAction)
	{
	case EOMMissionInteractionAction::CompleteObjective:
		return NSLOCTEXT("OperationMouse", "MissionObjectivePrompt", "Complete Objective");
	case EOMMissionInteractionAction::Fail:
		return NSLOCTEXT("OperationMouse", "MissionFailPrompt", "Fail Mission");
	case EOMMissionInteractionAction::Reset:
		return NSLOCTEXT("OperationMouse", "MissionResetPrompt", "Reset Mission");
	case EOMMissionInteractionAction::Retry:
		return NSLOCTEXT("OperationMouse", "MissionRetryPrompt", "Retry Mission");
	case EOMMissionInteractionAction::Start:
	default:
		return NSLOCTEXT("OperationMouse", "MissionStartPrompt", "Start Mission");
	}
}

void AOMMissionInteractionActor::UpdateVisualState()
{
	if (!StatusText || !Mesh)
	{
		return;
	}

	const bool bAvailable = IsActionAvailable();
	StatusText->SetText(FText::Format(NSLOCTEXT("OperationMouse", "MissionActionStatus", "{0}: {1}"),
		GetActionLabel(), bAvailable ? NSLOCTEXT("OperationMouse", "MissionActionReady", "READY")
		: NSLOCTEXT("OperationMouse", "MissionActionLocked", "LOCKED")));
	StatusText->SetTextRenderColor(!bAvailable ? FColor::Silver
		: MissionAction == EOMMissionInteractionAction::Fail ? FColor::Red
		: MissionAction == EOMMissionInteractionAction::CompleteObjective ? FColor::Yellow : FColor::Cyan);
	Mesh->SetRelativeRotation(bAvailable ? FRotator::ZeroRotator : FRotator(0.0f, 0.0f, 20.0f));
}

void AOMMissionInteractionActor::HandleMissionStateChanged(EOMMissionState PreviousState, EOMMissionState NewState)
{
	UpdateVisualState();
}
