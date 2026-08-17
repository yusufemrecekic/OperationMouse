#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OMInteractionComponent.generated.h"

class ACharacter;
class UOMInteractionPromptWidget;

/** Local focus and prompt plus small server-authoritative interaction request flow. */
UCLASS(ClassGroup = (OperationMouse), meta = (BlueprintSpawnableComponent))
class OPERATIONMOUSE_API UOMInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOMInteractionComponent();

	void BeginInteractionInput();
	void EndInteractionInput();

	AActor* GetFocusedActor() const { return FocusedActor; }
	bool IsHoldingInteraction() const { return ActiveInteractionTarget != nullptr; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void FindLocalFocus();
	void EnsurePromptWidget();
	void RefreshPrompt();
	bool IsServerInteractionValid(AActor* Target, bool bCheckInteractableState, FString* OutFailureReason = nullptr) const;
	bool IsCharacterStateValid() const;
	void CancelActiveInteraction();
	float GetSynchronizedTimeSeconds() const;

	UFUNCTION(Server, Reliable)
	void ServerBeginInteraction(AActor* Target);

	UFUNCTION(Server, Reliable)
	void ServerEndInteraction();

	UFUNCTION(Client, Reliable)
	void ClientInteractionRejected();

	UFUNCTION()
	void OnRep_ActiveInteraction();

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter;

	/** Focus exists only on the locally controlled player. */
	UPROPERTY(Transient)
	TObjectPtr<AActor> FocusedActor;

	/** Accepted Hold target, replicated only to its owning player for local progress UI. */
	UPROPERTY(ReplicatedUsing = OnRep_ActiveInteraction)
	TObjectPtr<AActor> ActiveInteractionTarget;

	UPROPERTY(ReplicatedUsing = OnRep_ActiveInteraction)
	float ActiveHoldServerStartTime = 0.0f;

	UPROPERTY(ReplicatedUsing = OnRep_ActiveInteraction)
	float ActiveHoldDuration = 0.0f;

	UPROPERTY(Transient)
	TObjectPtr<UOMInteractionPromptWidget> PromptWidget;

	UPROPERTY(EditAnywhere, Category = "Interaction|Detection", meta = (ClampMin = "1.0"))
	float DetectionDistance = 250.0f;

	UPROPERTY(EditAnywhere, Category = "Interaction|Detection", meta = (ClampMin = "1.0"))
	float DetectionRadius = 35.0f;

	/** Absolute server-side cap even if an interactable advertises a larger range. */
	UPROPERTY(EditAnywhere, Category = "Interaction|Validation", meta = (ClampMin = "1.0"))
	float MaximumServerDistance = 250.0f;

	UPROPERTY(EditAnywhere, Category = "Interaction|Debug")
	bool bDrawDebug = false;

	bool bLocalInputHeld = false;
};
