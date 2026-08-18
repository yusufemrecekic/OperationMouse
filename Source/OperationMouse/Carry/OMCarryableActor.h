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

	bool BeginCarry(UOMCarryComponent* NewCarrier, USceneComponent* NewCarryPoint);
	bool EndCarry(UOMCarryComponent* RequestingCarrier, const FVector& DropLocation);

	UFUNCTION(BlueprintPure, Category = "Operation Mouse|Carry")
	bool IsAvailableForGrab() const;

	UFUNCTION(BlueprintPure, Category = "Operation Mouse|Carry")
	AOMMouseCharacter* GetCurrentHolder() const { return CurrentHolder; }

	UFUNCTION(BlueprintCallable, Category = "Operation Mouse|Carry")
	void ResetToHome();

protected:
	virtual void BeginPlay() override;

private:
	friend class UOMCarryComponent;

	UFUNCTION()
	void OnRep_CurrentHolder(AOMMouseCharacter* PreviousHolder);

	void ApplyCarryPresentation(USceneComponent* NewCarryPoint);
	void ApplyDroppedPresentation();
	void SaveWorldStateIfNeeded();
	void RestoreWorldState(const FTransform& TargetTransform);
	void UpdateStatusText();

	UPROPERTY(VisibleAnywhere, Category = "Operation Mouse|Carry")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Operation Mouse|Carry")
	TObjectPtr<UTextRenderComponent> StatusText;

	/** Server-owned holder relationship used for availability and client presentation. */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHolder, VisibleAnywhere, Category = "Operation Mouse|Carry")
	TObjectPtr<AOMMouseCharacter> CurrentHolder;

	UPROPERTY(EditAnywhere, Category = "Operation Mouse|Carry")
	FOMInteractionInfo InteractionInfo;

	FTransform HomeTransform;
	TEnumAsByte<ECollisionEnabled::Type> SavedCollisionEnabled = ECollisionEnabled::QueryAndPhysics;
	bool bSavedSimulatePhysics = true;
	bool bHasSavedWorldState = false;
};
