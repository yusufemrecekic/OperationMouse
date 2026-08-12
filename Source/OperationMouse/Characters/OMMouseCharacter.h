#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "OMMouseCharacter.generated.h"

class UStaticMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UOMTraversalComponent;
class USpringArmComponent;
struct FInputActionValue;

/** Replicated physical player avatar built on Unreal's CharacterMovementComponent. */
UCLASS(Blueprintable)
class OPERATIONMOUSE_API AOMMouseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AOMMouseCharacter();

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
	void SetSprinting(bool bNewSprinting);
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

	/** Handles mantle detection, server validation, and the short traversal transition. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Operation Mouse|Traversal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UOMTraversalComponent> TraversalComponent;

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

	/** Normal movement speed in Unreal units per second. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Movement", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float NormalWalkSpeed = 600.0f;

	/** Movement speed while the sprint input is held. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Movement", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float SprintSpeed = 900.0f;

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
};
