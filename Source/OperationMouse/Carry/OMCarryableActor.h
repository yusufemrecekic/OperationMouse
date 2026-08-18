#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Interaction/OMInteractableInterface.h"
#include "OMCarryableActor.generated.h"

class UOMCarryComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Lightweight reusable carryable for the Sprint 2 gameplay foundation. */
UCLASS(Blueprintable)
class OPERATIONMOUSE_API AOMCarryableActor : public AActor, public IOMInteractableInterface
{
	GENERATED_BODY()

public:
	AOMCarryableActor();

	virtual FOMInteractionInfo GetInteractionInfo_Implementation(AActor* Interactor) const override;
	virtual FVector GetInteractionPoint_Implementation() const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual bool BeginInteraction_Implementation(AActor* Interactor) override;
	virtual void CompleteInteraction_Implementation(AActor* Interactor) override;

	bool BeginCarry(UOMCarryComponent* NewCarrier, USceneComponent* NewCarryPoint);
	void EndCarry(UOMCarryComponent* RequestingCarrier, const FVector& DropLocation);

	UFUNCTION(BlueprintPure, Category = "Operation Mouse|Carry")
	bool IsAvailableForGrab() const;

	UFUNCTION(BlueprintCallable, Category = "Operation Mouse|Carry")
	void ResetToHome();

protected:
	virtual void BeginPlay() override;

private:
	void RestoreWorldState(const FTransform& TargetTransform);
	void UpdateStatusText();

	UPROPERTY(VisibleAnywhere, Category = "Operation Mouse|Carry")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Operation Mouse|Carry")
	TObjectPtr<UTextRenderComponent> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UOMCarryComponent> CurrentCarrier;

	UPROPERTY(EditAnywhere, Category = "Operation Mouse|Carry")
	FOMInteractionInfo InteractionInfo;

	FTransform HomeTransform;
	TEnumAsByte<ECollisionEnabled::Type> SavedCollisionEnabled = ECollisionEnabled::QueryAndPhysics;
	bool bSavedSimulatePhysics = true;
};
