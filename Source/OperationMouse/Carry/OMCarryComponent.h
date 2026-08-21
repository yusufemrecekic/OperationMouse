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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Supplies the stable scene attachment used while carrying. */
	void SetCarryPoint(USceneComponent* NewCarryPoint);

	UFUNCTION(BlueprintPure, Category = "Operation Mouse|Carry")
	bool CanGrab(AActor* Candidate);

	UFUNCTION(BlueprintCallable, Category = "Operation Mouse|Carry")
	bool TryGrab(AActor* Candidate);

	/** Routes an owning client's Drop input to the authoritative server. */
	UFUNCTION(BlueprintCallable, Category = "Operation Mouse|Carry")
	void RequestDrop();

	/** Executes Drop only on the authoritative Character. */
	UFUNCTION(BlueprintCallable, Category = "Operation Mouse|Carry")
	bool Drop();

	UFUNCTION(BlueprintPure, Category = "Operation Mouse|Carry")
	bool IsCarrying() const;

	UFUNCTION(BlueprintPure, Category = "Operation Mouse|Carry")
	AOMCarryableActor* GetCarriedActor() const;

	UFUNCTION(BlueprintPure, Category = "Operation Mouse|Carry")
	EOMCarryState GetCarryState() const;

	USceneComponent* GetCarryPoint() const { return CarryPoint; }

	/** Shared by local prediction and authority to stop only cargo-blocked movement. */
	FVector ConstrainOwnerMovement(const FVector& DesiredWorldMovement) const;

	/** Targeted recovery hook used by a carryable before it resets to its home transform. */
	void ReleaseForRecovery(AOMCarryableActor* Carryable);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool CanGrabWithReason(AActor* Candidate, FString& OutReason);
	void SetAuthoritativeCarriedActor(AOMCarryableActor* NewCarriedActor);

	UFUNCTION(Server, Reliable)
	void ServerRequestDrop();

	UFUNCTION()
	void OnRep_CarriedActor(AOMCarryableActor* PreviousCarriedActor);

	UFUNCTION()
	void HandleCarriedActorDestroyed(AActor* DestroyedActor);

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> CarryPoint;

	/** Server-owned Carry relationship replicated to owner and simulated clients. */
	UPROPERTY(ReplicatedUsing = OnRep_CarriedActor, VisibleAnywhere, Category = "Operation Mouse|Carry")
	TObjectPtr<AOMCarryableActor> CarriedActor;

	/** Server-chosen gameplay drop placement. */
	UPROPERTY(EditAnywhere, Category = "Operation Mouse|Carry", meta = (ClampMin = "0.0"))
	float DropForwardDistance = 140.0f;

	UPROPERTY(EditAnywhere, Category = "Operation Mouse|Carry")
	float DropHeightOffset = 20.0f;
};
