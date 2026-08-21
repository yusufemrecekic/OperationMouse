#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Interaction/OMInteractableInterface.h"
#include "OMMissionInteractionActor.generated.h"

class AOMMissionManager;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EOMMissionInteractionAction : uint8
{
	Start,
	CompleteObjective,
	Fail,
	Reset,
	Retry
};

/** Generic interaction adapter for mission test fixtures and future level-authored mission triggers. */
UCLASS(Blueprintable)
class OPERATIONMOUSE_API AOMMissionInteractionActor : public AActor, public IOMInteractableInterface
{
	GENERATED_BODY()

public:
	AOMMissionInteractionActor();

	virtual FOMInteractionInfo GetInteractionInfo_Implementation(AActor* Interactor) const override;
	virtual FVector GetInteractionPoint_Implementation() const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual bool BeginInteraction_Implementation(AActor* Interactor) override;
	virtual void CompleteInteraction_Implementation(AActor* Interactor) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	bool ExecuteMissionAction(AActor* Interactor);
	bool IsActionAvailable() const;
	FText GetActionLabel() const;
	void UpdateVisualState();

	UFUNCTION()
	void HandleMissionStateChanged(EOMMissionState PreviousState, EOMMissionState NewState);

	UFUNCTION()
	void OnRep_ObjectiveConsumed();

	UPROPERTY(VisibleAnywhere, Category = "Operation Mouse|Mission")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Operation Mouse|Mission")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Operation Mouse|Mission")
	TObjectPtr<UTextRenderComponent> StatusText;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Operation Mouse|Mission", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AOMMissionManager> MissionManager;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Operation Mouse|Mission", meta = (AllowPrivateAccess = "true"))
	EOMMissionInteractionAction MissionAction = EOMMissionInteractionAction::Start;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Operation Mouse|Mission", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 ObjectiveProgressAmount = 1;

	/** A server-consumed objective fixture cannot award progress twice from duplicate requests. */
	UPROPERTY(ReplicatedUsing = OnRep_ObjectiveConsumed, VisibleAnywhere, Category = "Operation Mouse|Mission")
	bool bObjectiveConsumed = false;
};
