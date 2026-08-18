#pragma once

#include "CoreMinimal.h"
#include "OMCarryableActor.h"
#include "OMHeavyCarryableActor.generated.h"

class UOMCarryComponent;

UENUM(BlueprintType)
enum class EOMHeavyCarryState : uint8
{
	Idle,
	WaitingForSecondHolder,
	Carrying
};

/**
 * Yusuf-owned gameplay foundation for a two-player Heavy Carryable.
 * Holder state is intentionally local/non-replicated until Hilmi adds the
 * authoritative network contract in a separate pass.
 */
UCLASS(Blueprintable)
class OPERATIONMOUSE_API AOMHeavyCarryableActor : public AOMCarryableActor
{
	GENERATED_BODY()

public:
	AOMHeavyCarryableActor();

	virtual FOMInteractionInfo GetInteractionInfo_Implementation(AActor* Interactor) const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual bool BeginInteraction_Implementation(AActor* Interactor) override;
	virtual void CompleteInteraction_Implementation(AActor* Interactor) override;

	virtual bool BeginCarry(UOMCarryComponent* NewCarrier, USceneComponent* NewCarryPoint) override;
	virtual bool EndCarry(UOMCarryComponent* RequestingCarrier, const FVector& DropLocation) override;
	virtual bool IsHeldBy(const AOMMouseCharacter* Character) const override;
	virtual bool IsAvailableForGrab() const override;
	virtual void ResetToHome() override;

	UFUNCTION(BlueprintPure, Category = "Operation Mouse|Heavy Carry")
	EOMHeavyCarryState GetHeavyCarryState() const { return HeavyCarryState; }

	UFUNCTION(BlueprintPure, Category = "Operation Mouse|Heavy Carry")
	int32 GetHolderCount() const { return ActiveCarriers.Num(); }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void SetHeavyCarryState(EOMHeavyCarryState NewState);
	void SetMovementPenaltyForAllHolders(bool bActive);
	void FreezeAtCurrentTransform();
	void RestoreWorldPresentation(const FTransform& TargetTransform);
	void UpdateHeavyCarryTransform();
	void UpdateHeavyStatusText();
	void RemoveInvalidCarriers();

	/** Gameplay state only. Hilmi owns future replication/authority integration. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Operation Mouse|Heavy Carry")
	TArray<TObjectPtr<UOMCarryComponent>> ActiveCarriers;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Operation Mouse|Heavy Carry")
	EOMHeavyCarryState HeavyCarryState = EOMHeavyCarryState::Idle;

	UPROPERTY(EditAnywhere, Category = "Operation Mouse|Heavy Carry", meta = (ClampMin = "0.0"))
	float CarryHeightOffset = 45.0f;

	FTransform HeavyHomeTransform;
	TEnumAsByte<ECollisionEnabled::Type> SavedHeavyCollision = ECollisionEnabled::QueryAndPhysics;
	bool bSavedHeavySimulatePhysics = true;
	bool bHeavyPresentationSaved = false;
};
