#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "OMMouseCharacter.generated.h"

class UStaticMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UOMInteractionComponent;
class UOMCarryComponent;
class UOMTraversalComponent;
class USceneComponent;
class USpringArmComponent;
struct FInputActionValue;

/** Replicated physical player avatar built on Unreal's CharacterMovementComponent. */
UCLASS(Blueprintable)
class OPERATIONMOUSE_API AOMMouseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AOMMouseCharacter();

	/** Gameplay-only hook used by Heavy Carry. Network ownership will synchronize this state later. */
	void SetHeavyCarryMovementPenaltyActive(bool bActive);

	UFUNCTION(BlueprintPure, Category = "Operation Mouse|Movement")
	bool IsHeavyCarryMovementPenaltyActive() const { return bHeavyCarryMovementPenaltyActive; }

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode = 0) override;

protected:
	virtual void BeginPlay() override;
	virtual bool CanJumpWhileFalling() const override;

private:
	friend class UOMTraversalComponent;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartJump();
	void StopJump();
	void AttemptJumpOrBuffer();
	void HandleMantleRejected();
	void StartSprint();
	void StopSprint();
	void StartCrouch();
	void StopCrouch();
	void StartInteraction();
	void StopInteraction();
	void SetSprinting(bool bNewSprinting);
	void RefreshMaxWalkSpeed();
	void TryConsumeBufferedJump();

	UFUNCTION(Server, Reliable)
	void ServerSetSprinting(bool bNewSprinting);

	/** Temporary visible marker until a production mouse skeletal mesh is integrated. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Operation Mouse|Presentation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> PlaceholderVisual;

	/** Keeps the third-person camera behind the character while following controller rotation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Operation Mouse|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Operation Mouse|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	/** Stable gameplay attachment point; final grip placement remains a visual-design task. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Operation Mouse|Carry", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> CarryPoint;

	/** Handles mantle detection, server validation, and the short traversal transition. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Operation Mouse|Traversal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UOMTraversalComponent> TraversalComponent;

	/** Owns local focus/prompt handling and server-authoritative interaction requests. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Operation Mouse|Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UOMInteractionComponent> InteractionComponent;

	/** Owns the lightweight Grab / Carry / Drop gameplay state. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Operation Mouse|Carry", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UOMCarryComponent> CarryComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> GameplayMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> InteractAction;

	/** Normal movement speed in Unreal units per second. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Movement", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float NormalWalkSpeed = 600.0f;

	/** Movement speed while the sprint input is held. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Movement", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float SprintSpeed = 900.0f;

	/** Applied only while two players have activated Heavy Carry gameplay. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Movement", meta = (AllowPrivateAccess = "true", ClampMin = "0.1", ClampMax = "1.0"))
	float HeavyCarrySpeedMultiplier = 0.55f;

	/** How strongly built-in CharacterMovement acceleration responds to WASD while falling. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Movement|Air", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float AirControlStrength = 1.0f;

	/** Grace period after leaving a ledge during which a jump is still accepted. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Movement|Jump", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float CoyoteTimeSeconds = 0.15f;

	/** Time a too-early jump press remains valid before landing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Movement|Jump", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float JumpInputBufferSeconds = 0.15f;

	double CoyoteTimeExpiration = -1.0;
	double BufferedJumpExpiration = -1.0;
	bool bJumpInputHeld = false;
	bool bIsSprinting = false;
	bool bHeavyCarryMovementPenaltyActive = false;
};
