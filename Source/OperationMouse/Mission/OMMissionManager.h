#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OMMissionManager.generated.h"

class UOMMissionDefinition;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EOMMissionState : uint8
{
	Inactive,
	Active,
	Completed,
	Failed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOMMissionStateChanged, EOMMissionState, PreviousState, EOMMissionState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOMMissionObjectiveProgressed, int32, NewProgress, int32, ObjectiveTarget);

/**
 * Yusuf-owned reusable mission gameplay state. Networking intentionally remains
 * outside this class until the replicated mission contract is defined by Hilmi.
 */
UCLASS(Blueprintable)
class OPERATIONMOUSE_API AOMMissionManager : public AActor
{
	GENERATED_BODY()

public:
	AOMMissionManager();

	UFUNCTION(BlueprintPure, Category = "Operation Mouse|Mission")
	EOMMissionState GetMissionState() const { return MissionState; }

	UFUNCTION(BlueprintPure, Category = "Operation Mouse|Mission")
	FName GetMissionId() const;

	UFUNCTION(BlueprintPure, Category = "Operation Mouse|Mission")
	int32 GetObjectiveProgress() const { return ObjectiveProgress; }

	UFUNCTION(BlueprintPure, Category = "Operation Mouse|Mission")
	int32 GetObjectiveTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Operation Mouse|Mission")
	bool StartMission(AActor* InstigatorActor = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Operation Mouse|Mission")
	bool CompleteObjective(AActor* InstigatorActor = nullptr, int32 ProgressAmount = 1);

	UFUNCTION(BlueprintCallable, Category = "Operation Mouse|Mission")
	bool FailMission(AActor* InstigatorActor = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Operation Mouse|Mission")
	bool ResetMission(AActor* InstigatorActor = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Operation Mouse|Mission")
	bool RetryMission(AActor* InstigatorActor = nullptr);

	bool CanExecuteAction(EOMMissionState RequiredState) const;

	UPROPERTY(BlueprintAssignable, Category = "Operation Mouse|Mission")
	FOMMissionStateChanged OnMissionStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Operation Mouse|Mission")
	FOMMissionObjectiveProgressed OnObjectiveProgressed;

protected:
	virtual void BeginPlay() override;

private:
	bool TransitionTo(EOMMissionState NewState, AActor* InstigatorActor, const TCHAR* Reason);
	bool RejectTransition(const TCHAR* Action, AActor* InstigatorActor) const;
	void UpdateStatusText();
	FText GetStateText() const;

	UPROPERTY(VisibleAnywhere, Category = "Operation Mouse|Mission")
	TObjectPtr<UTextRenderComponent> StatusText;

	/** Optional data-driven override. The editable fields below keep graybox setup lightweight. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Operation Mouse|Mission", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UOMMissionDefinition> MissionDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Operation Mouse|Mission", meta = (AllowPrivateAccess = "true"))
	FName MissionId = TEXT("Mission_Prototype");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Operation Mouse|Mission", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 ObjectiveTarget = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Operation Mouse|Mission", meta = (AllowPrivateAccess = "true"))
	EOMMissionState MissionState = EOMMissionState::Inactive;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Operation Mouse|Mission", meta = (AllowPrivateAccess = "true"))
	int32 ObjectiveProgress = 0;
};
