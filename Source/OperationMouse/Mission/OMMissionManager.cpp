#include "OMMissionManager.h"

#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Net/UnrealNetwork.h"
#include "OMMissionDefinition.h"
#include "../OperationMouse.h"

AOMMissionManager::AOMMissionManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	StatusText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StatusText"));
	StatusText->SetupAttachment(SceneRoot);
	StatusText->SetRelativeLocation(FVector(0.0f, 0.0f, 145.0f));
	StatusText->SetHorizontalAlignment(EHTA_Center);
	StatusText->SetWorldSize(28.0f);
	StatusText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AOMMissionManager::BeginPlay()
{
	Super::BeginPlay();
	ObjectiveProgress = 0;
	MissionState = EOMMissionState::Inactive;
	UpdateStatusText();
	UE_LOG(LogOperationMouse, Log, TEXT("[Mission][Initialized] Id=%s Target=%d State=Inactive"), *GetMissionId().ToString(), GetObjectiveTarget());
}

FName AOMMissionManager::GetMissionId() const
{
	return MissionDefinition ? MissionDefinition->MissionId : MissionId;
}

int32 AOMMissionManager::GetObjectiveTarget() const
{
	return FMath::Max(1, MissionDefinition ? MissionDefinition->ObjectiveTarget : ObjectiveTarget);
}

bool AOMMissionManager::CanExecuteAction(EOMMissionState RequiredState) const
{
	return MissionState == RequiredState;
}

bool AOMMissionManager::StartMission(AActor* InstigatorActor)
{
	if (!HasAuthority())
	{
		return RejectTransition(TEXT("StartMission.NotAuthority"), InstigatorActor);
	}

	if (!CanExecuteAction(EOMMissionState::Inactive))
	{
		return RejectTransition(TEXT("StartMission"), InstigatorActor);
	}

	ObjectiveProgress = 0;
	return TransitionTo(EOMMissionState::Active, InstigatorActor, TEXT("StartMission"));
}

bool AOMMissionManager::CompleteObjective(AActor* InstigatorActor, int32 ProgressAmount)
{
	if (!HasAuthority())
	{
		return RejectTransition(TEXT("CompleteObjective.NotAuthority"), InstigatorActor);
	}

	if (!CanExecuteAction(EOMMissionState::Active) || ProgressAmount <= 0)
	{
		return RejectTransition(TEXT("CompleteObjective"), InstigatorActor);
	}

	ObjectiveProgress = FMath::Clamp(ObjectiveProgress + ProgressAmount, 0, GetObjectiveTarget());
	OnObjectiveProgressed.Broadcast(ObjectiveProgress, GetObjectiveTarget());
	UE_LOG(LogOperationMouse, Log, TEXT("[Mission][Objective] Id=%s Progress=%d/%d Instigator=%s"),
		*GetMissionId().ToString(), ObjectiveProgress, GetObjectiveTarget(), *GetNameSafe(InstigatorActor));
	UpdateStatusText();
	ForceNetUpdate();

	return ObjectiveProgress >= GetObjectiveTarget()
		? TransitionTo(EOMMissionState::Completed, InstigatorActor, TEXT("ObjectiveComplete"))
		: true;
}

bool AOMMissionManager::FailMission(AActor* InstigatorActor)
{
	if (!HasAuthority())
	{
		return RejectTransition(TEXT("FailMission.NotAuthority"), InstigatorActor);
	}

	return CanExecuteAction(EOMMissionState::Active)
		? TransitionTo(EOMMissionState::Failed, InstigatorActor, TEXT("FailMission"))
		: RejectTransition(TEXT("FailMission"), InstigatorActor);
}

bool AOMMissionManager::ResetMission(AActor* InstigatorActor)
{
	if (!HasAuthority())
	{
		return RejectTransition(TEXT("ResetMission.NotAuthority"), InstigatorActor);
	}

	if (MissionState == EOMMissionState::Inactive)
	{
		return RejectTransition(TEXT("ResetMission"), InstigatorActor);
	}

	ObjectiveProgress = 0;
	return TransitionTo(EOMMissionState::Inactive, InstigatorActor, TEXT("ResetMission"));
}

bool AOMMissionManager::RetryMission(AActor* InstigatorActor)
{
	if (!HasAuthority())
	{
		return RejectTransition(TEXT("RetryMission.NotAuthority"), InstigatorActor);
	}

	if (MissionState != EOMMissionState::Failed && MissionState != EOMMissionState::Completed)
	{
		return RejectTransition(TEXT("RetryMission"), InstigatorActor);
	}

	ObjectiveProgress = 0;
	return TransitionTo(EOMMissionState::Active, InstigatorActor, TEXT("RetryMission"));
}

bool AOMMissionManager::TransitionTo(EOMMissionState NewState, AActor* InstigatorActor, const TCHAR* Reason)
{
	const EOMMissionState PreviousState = MissionState;
	MissionState = NewState;
	UpdateStatusText();
	OnMissionStateChanged.Broadcast(PreviousState, NewState);
	ForceNetUpdate();
	UE_LOG(LogOperationMouse, Log, TEXT("[Mission][State] Id=%s Role=%s %s->%s Reason=%s Instigator=%s"),
		*GetMissionId().ToString(), *GetNetworkRoleText(), *UEnum::GetValueAsString(PreviousState),
		*UEnum::GetValueAsString(NewState), Reason, *GetNameSafe(InstigatorActor));
	return true;
}

bool AOMMissionManager::RejectTransition(const TCHAR* Action, AActor* InstigatorActor) const
{
	UE_LOG(LogOperationMouse, Warning, TEXT("[Mission][Reject] Id=%s Role=%s Action=%s State=%s Instigator=%s"),
		*GetMissionId().ToString(), *GetNetworkRoleText(), Action, *UEnum::GetValueAsString(MissionState), *GetNameSafe(InstigatorActor));
	return false;
}

FString AOMMissionManager::GetNetworkRoleText() const
{
	return UEnum::GetValueAsString(GetLocalRole());
}

void AOMMissionManager::OnRep_MissionState(EOMMissionState PreviousState)
{
	UpdateStatusText();
	OnMissionStateChanged.Broadcast(PreviousState, MissionState);
	UE_LOG(LogOperationMouse, Log, TEXT("[Mission][Replication] Id=%s Role=%s State=%s"),
		*GetMissionId().ToString(), *GetNetworkRoleText(), *UEnum::GetValueAsString(MissionState));
}

void AOMMissionManager::OnRep_ObjectiveProgress()
{
	UpdateStatusText();
	OnObjectiveProgressed.Broadcast(ObjectiveProgress, GetObjectiveTarget());
	UE_LOG(LogOperationMouse, Log, TEXT("[Mission][Replication] Id=%s Role=%s Progress=%d/%d"),
		*GetMissionId().ToString(), *GetNetworkRoleText(), ObjectiveProgress, GetObjectiveTarget());
}

void AOMMissionManager::OnRep_MissionIdentity()
{
	UpdateStatusText();
	UE_LOG(LogOperationMouse, Log, TEXT("[Mission][Replication] Id=%s Role=%s Target=%d"),
		*GetMissionId().ToString(), *GetNetworkRoleText(), GetObjectiveTarget());
}

void AOMMissionManager::UpdateStatusText()
{
	if (!StatusText)
	{
		return;
	}

	StatusText->SetText(FText::Format(
		NSLOCTEXT("OperationMouse", "MissionManagerStatus", "MISSION: {0}\nSTATE: {1}\nOBJECTIVE: {2}/{3}"),
		FText::FromName(GetMissionId()), GetStateText(), FText::AsNumber(ObjectiveProgress), FText::AsNumber(GetObjectiveTarget())));
	StatusText->SetTextRenderColor(MissionState == EOMMissionState::Completed
		? FColor::Green
		: MissionState == EOMMissionState::Failed ? FColor::Red : FColor::Cyan);
}

FText AOMMissionManager::GetStateText() const
{
	switch (MissionState)
	{
	case EOMMissionState::Active:
		return NSLOCTEXT("OperationMouse", "MissionStateActive", "ACTIVE");
	case EOMMissionState::Completed:
		return NSLOCTEXT("OperationMouse", "MissionStateCompleted", "COMPLETED");
	case EOMMissionState::Failed:
		return NSLOCTEXT("OperationMouse", "MissionStateFailed", "FAILED");
	case EOMMissionState::Inactive:
	default:
		return NSLOCTEXT("OperationMouse", "MissionStateInactive", "INACTIVE");
	}
}

void AOMMissionManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AOMMissionManager, MissionId);
	DOREPLIFETIME(AOMMissionManager, ObjectiveTarget);
	DOREPLIFETIME(AOMMissionManager, MissionState);
	DOREPLIFETIME(AOMMissionManager, ObjectiveProgress);
}
