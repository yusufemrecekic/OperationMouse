#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Interaction/OMInteractableInterface.h"
#include "OMCarryableActor.generated.h"

class UOMCarryComponent;
class AOMMouseCharacter;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Lightweight reusable carryable for the Sprint 2 gameplay foundation. */
UCLASS(Blueprintable)
class OPERATIONMOUSE_API AOMCarryableActor : public AActor, public IOMInteractableInterface
{
	GENERATED_BODY()

public:
	AOMCarryableActor();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual FOMInteractionInfo GetInteractionInfo_Implementation(AActor* Interactor) const override;
	virtual FVector GetInteractionPoint_Implementation() const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual bool BeginInteraction_Implementation(AActor* Interactor) override;
	virtual void CompleteInteraction_Implementation(AActor* Interactor) override;

	virtual bool BeginCarry(UOMCarryComponent* NewCarrier, USceneComponent* NewCarryPoint);
	virtual bool EndCarry(UOMCarryComponent* RequestingCarrier, const FVector& DropLocation);
	virtual bool IsHeldBy(const AOMMouseCharacter* Character) const;

	UFUNCTION(BlueprintPure, Category = "Operation Mouse|Carry")
	virtual bool IsAvailableForGrab() const;

	UFUNCTION(BlueprintPure, Category = "Operation Mouse|Carry")
	AOMMouseCharacter* GetCurrentHolder() const { return CurrentHolder; }

	UFUNCTION(BlueprintCallable, Category = "Operation Mouse|Carry")
	virtual void ResetToHome();

protected:
	virtual void BeginPlay() override;

private:
	friend class UOMCarryComponent;

	UFUNCTION()
	void OnRep_CurrentHolder(AOMMouseCharacter* PreviousHolder);

	UFUNCTION()
	void OnRep_WorldStateRevision();

	void ApplyCarryPresentation(USceneComponent* NewCarryPoint);
	void ApplyReplicatedWorldPresentation();
	void ReconcileReplicatedPresentation();
	void SaveWorldStateIfNeeded();
	void PublishAuthoritativeWorldState(const FTransform& TargetTransform);
	void RestoreWorldState(const FTransform& TargetTransform);
	void UpdateStatusText();

	UPROPERTY(VisibleAnywhere, Category = "Operation Mouse|Carry")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Operation Mouse|Carry")
	TObjectPtr<UTextRenderComponent> StatusText;

	/** Server-owned holder relationship used for availability and client presentation. */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHolder, VisibleAnywhere, Category = "Operation Mouse|Carry")
	TObjectPtr<AOMMouseCharacter> CurrentHolder;

	/** Last discrete Drop/Reset transform. ReplicatedMovement continues authoritative physics after it. */
	UPROPERTY(Replicated)
	FTransform AuthoritativeWorldTransform;

	/** Changes for every Drop/Reset, including repeated resets to the same transform. */
	UPROPERTY(ReplicatedUsing = OnRep_WorldStateRevision)
	uint32 WorldStateRevision = 0;

	UPROPERTY(EditAnywhere, Category = "Operation Mouse|Carry")
	FOMInteractionInfo InteractionInfo;

	FTransform HomeTransform;
	TEnumAsByte<ECollisionEnabled::Type> SavedCollisionEnabled = ECollisionEnabled::QueryAndPhysics;
	bool bSavedSimulatePhysics = true;
	bool bHasSavedWorldState = false;
};
