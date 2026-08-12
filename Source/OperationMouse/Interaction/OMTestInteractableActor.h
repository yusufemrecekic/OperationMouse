#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OMInteractableInterface.h"
#include "OMTestInteractableActor.generated.h"

class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Minimal replicated fixture for Phase 5 Instant, Hold, and Exclusive tests. */
UCLASS(Blueprintable)
class OPERATIONMOUSE_API AOMTestInteractableActor : public AActor, public IOMInteractableInterface
{
	GENERATED_BODY()

public:
	AOMTestInteractableActor();

	virtual FOMInteractionInfo GetInteractionInfo_Implementation(AActor* Interactor) const override;
	virtual FVector GetInteractionPoint_Implementation() const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual bool BeginInteraction_Implementation(AActor* Interactor) override;
	virtual void CancelInteraction_Implementation(AActor* Interactor) override;
	virtual void CompleteInteraction_Implementation(AActor* Interactor) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	bool IsActivated() const { return bActivated; }
	int32 GetCompletionCount() const { return CompletionCount; }

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_InteractionState();

	void UpdateVisualState();

	UPROPERTY(VisibleAnywhere, Category = "Interaction Test")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Interaction Test")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Interaction Test")
	TObjectPtr<UTextRenderComponent> StatusText;

	UPROPERTY(VisibleAnywhere, Category = "Interaction Test")
	TObjectPtr<UPointLightComponent> StatusLight;

	UPROPERTY(EditAnywhere, Category = "Interaction Test")
	FOMInteractionInfo InteractionInfo;

	/** Persistent result: late joiners receive this state through property replication. */
	UPROPERTY(ReplicatedUsing = OnRep_InteractionState, VisibleAnywhere, Category = "Interaction Test")
	bool bActivated = false;

	/** Transient exclusive claim used to serialize competing server requests. */
	UPROPERTY(ReplicatedUsing = OnRep_InteractionState)
	TObjectPtr<AActor> ActiveInteractor;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Interaction Test")
	int32 CompletionCount = 0;
};
