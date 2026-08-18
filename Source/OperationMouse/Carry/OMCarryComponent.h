#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OMCarryComponent.generated.h"

class AOMCarryableActor;
class USceneComponent;

UENUM(BlueprintType)
enum class EOMCarryState : uint8
{
	Idle,
	Carrying
};

/** Character-owned gameplay state for the lightweight Grab -> Carry -> Drop loop. */
UCLASS(ClassGroup = (OperationMouse), meta = (BlueprintSpawnableComponent))
class OPERATIONMOUSE_API UOMCarryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOMCarryComponent();

	/** Supplies the stable scene attachment used while carrying. */
	void SetCarryPoint(USceneComponent* NewCarryPoint);

	UFUNCTION(BlueprintPure, Category = "Operation Mouse|Carry")
	bool CanGrab(AActor* Candidate);

	UFUNCTION(BlueprintCallable, Category = "Operation Mouse|Carry")
	bool TryGrab(AActor* Candidate);

	UFUNCTION(BlueprintCallable, Category = "Operation Mouse|Carry")
	bool Drop();

	UFUNCTION(BlueprintPure, Category = "Operation Mouse|Carry")
	bool IsCarrying() const;

	UFUNCTION(BlueprintPure, Category = "Operation Mouse|Carry")
	AOMCarryableActor* GetCarriedActor() const;

	/** Targeted recovery hook used by a carryable before it resets to its home transform. */
	void ReleaseForRecovery(AOMCarryableActor* Carryable);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool CanGrabWithReason(AActor* Candidate, FString& OutReason);
	void ClearCarryState();

	UFUNCTION()
	void HandleCarriedActorDestroyed(AActor* DestroyedActor);

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> CarryPoint;

	UPROPERTY(Transient)
	TObjectPtr<AOMCarryableActor> CarriedActor;

	UPROPERTY(VisibleAnywhere, Category = "Operation Mouse|Carry")
	EOMCarryState CarryState = EOMCarryState::Idle;

	/** Gameplay-only drop placement. Network authority will be reviewed separately by Hilmi. */
	UPROPERTY(EditAnywhere, Category = "Operation Mouse|Carry", meta = (ClampMin = "0.0"))
	float DropForwardDistance = 140.0f;

	UPROPERTY(EditAnywhere, Category = "Operation Mouse|Carry")
	float DropHeightOffset = 20.0f;
};
