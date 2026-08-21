#pragma once

#include "CoreMinimal.h"
#include "OMCarryableActor.h"
#include "OMHeavyCarryableActor.generated.h"

class UOMCarryComponent;
class UPrimitiveComponent;

UENUM(BlueprintType)
enum class EOMHeavyCarryState : uint8
{
	Idle,
	WaitingForSecondHolder,
	Carrying
};

/** Yusuf-owned two-player Heavy Carry gameplay with minimal replicated collision consistency. */
UCLASS(Blueprintable)
class OPERATIONMOUSE_API AOMHeavyCarryableActor : public AOMCarryableActor
{
	GENERATED_BODY()

public:
	AOMHeavyCarryableActor();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual FOMInteractionInfo GetInteractionInfo_Implementation(AActor* Interactor) const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual bool BeginInteraction_Implementation(AActor* Interactor) override;
	virtual void CompleteInteraction_Implementation(AActor* Interactor) override;

	virtual bool BeginCarry(UOMCarryComponent* NewCarrier, USceneComponent* NewCarryPoint) override;
	virtual bool EndCarry(UOMCarryComponent* RequestingCarrier, const FVector& DropLocation) override;
	virtual bool IsHeldBy(const AOMMouseCharacter* Character) const override;
	virtual bool IsAvailableForGrab() const override;
	virtual void ResetToHome() override;
	virtual FVector ConstrainHolderMovement(const AOMMouseCharacter* Holder, const FVector& DesiredWorldMovement) const override;

	UFUNCTION(BlueprintPure, Category = "Operation Mouse|Heavy Carry")
	EOMHeavyCarryState GetHeavyCarryState() const { return HeavyCarryState; }

	UFUNCTION(BlueprintPure, Category = "Operation Mouse|Heavy Carry")
	int32 GetHolderCount() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void SetHeavyCarryState(EOMHeavyCarryState NewState);
	void SetMovementPenaltyForAllHolders(bool bActive);
	void ApplyReplicatedMovementPenalty();
	void AlignCarriersToSlots();
	void RefreshCarrierCollisionIgnores();
	void ClearCarrierCollisionIgnores();
	void AddCarrierCollisionIgnore(UPrimitiveComponent* SourceComponent, AActor* TargetActor);
	void AddCarrierMovementIgnore(AOMMouseCharacter* SourceCharacter, AActor* TargetActor);
	void FreezeAtCurrentTransform();
	void RestoreWorldPresentation(const FTransform& TargetTransform);
	void UpdateHeavyCarryTransform();
	void SetHeavyCarryObstructed(bool bNewObstructed, const FHitResult& Hit);
	void SyncReplicatedCarryState();
	void ApplyReplicatedCarryPresentation();
	void CacheHeavyPresentationIfNeeded();
	TArray<AOMMouseCharacter*> GetPresentationHolders() const;
	void UpdateHeavyStatusText();
	bool RemoveInvalidCarriers();
	USceneComponent* GetSlotForCarrierIndex(int32 CarrierIndex) const;

	UFUNCTION()
	void OnRep_HeavyCarryNetworkState();

	/** First gameplay holder aligns its CarryPoint to this side of the object. */
	UPROPERTY(VisibleAnywhere, Category = "Operation Mouse|Heavy Carry")
	TObjectPtr<USceneComponent> LeftCarrySlot;

	/** Second gameplay holder aligns its CarryPoint to the opposite side. */
	UPROPERTY(VisibleAnywhere, Category = "Operation Mouse|Heavy Carry")
	TObjectPtr<USceneComponent> RightCarrySlot;

	/** Server-only gameplay components. Network clients receive only actor/state presentation below. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Operation Mouse|Heavy Carry")
	TArray<TObjectPtr<UOMCarryComponent>> ActiveCarriers;

	UPROPERTY(ReplicatedUsing = OnRep_HeavyCarryNetworkState, VisibleAnywhere, Category = "Operation Mouse|Heavy Carry")
	EOMHeavyCarryState HeavyCarryState = EOMHeavyCarryState::Idle;

	UPROPERTY(ReplicatedUsing = OnRep_HeavyCarryNetworkState)
	TObjectPtr<AOMMouseCharacter> ReplicatedFirstHolder;

	UPROPERTY(ReplicatedUsing = OnRep_HeavyCarryNetworkState)
	TObjectPtr<AOMMouseCharacter> ReplicatedSecondHolder;

	/** Server sweep normal used by both authority and owning-client movement constraint. */
	UPROPERTY(Replicated)
	FVector_NetQuantizeNormal HeavyObstructionNormal = FVector::ZeroVector;

	FTransform HeavyHomeTransform;
	TEnumAsByte<ECollisionEnabled::Type> SavedHeavyCollision = ECollisionEnabled::QueryAndPhysics;
	TEnumAsByte<ECollisionResponse> SavedHeavyPawnCollisionResponse = ECR_Block;
	bool bSavedHeavySimulatePhysics = true;
	bool bHeavyPresentationSaved = false;
	bool bHeavyCarryObstructed = false;
	TArray<TWeakObjectPtr<AOMMouseCharacter>> ReplicatedPenaltyCharacters;

	/** Only pair-specific ignores added by Heavy Carry; cleared on release/reset. */
	TArray<TWeakObjectPtr<UPrimitiveComponent>> CollisionIgnoreSources;
	TArray<TWeakObjectPtr<AActor>> CollisionIgnoreTargets;
	TArray<TWeakObjectPtr<AOMMouseCharacter>> MovementIgnoreSources;
	TArray<TWeakObjectPtr<AActor>> MovementIgnoreTargets;
};
