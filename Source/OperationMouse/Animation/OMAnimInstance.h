#pragma once

#include "Animation/AnimInstance.h"
#include "CoreMinimal.h"
#include "OMAnimInstance.generated.h"

class UAnimSequence;

/**
 * Generic locomotion state shared by temporary and final character visuals.
 * Gameplay owns movement; animation reads the resulting Character state.
 */
UCLASS(Blueprintable, BlueprintType)
class OPERATIONMOUSE_API UOMAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Operation Mouse|Animation")
	float GroundSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Operation Mouse|Animation")
	float MovementDirection = 0.0f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Operation Mouse|Animation")
	float VerticalSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Operation Mouse|Animation")
	bool bIsGrounded = true;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Operation Mouse|Animation")
	bool bIsFalling = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Operation Mouse|Animation")
	bool bIsAscending = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Operation Mouse|Animation")
	bool bIsCrouched = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Operation Mouse|Animation")
	bool bIsSprinting = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Operation Mouse|Animation")
	bool bIsMantling = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Animation|Prototype")
	TObjectPtr<UAnimSequence> IdleAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Animation|Prototype")
	TObjectPtr<UAnimSequence> WalkAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Animation|Prototype")
	TObjectPtr<UAnimSequence> RunAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Animation|Prototype")
	TObjectPtr<UAnimSequence> JumpAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Animation|Prototype")
	TObjectPtr<UAnimSequence> FallAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Animation|Prototype")
	TObjectPtr<UAnimSequence> LandAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Animation|Prototype")
	TObjectPtr<UAnimSequence> CrouchIdleAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Animation|Prototype")
	TObjectPtr<UAnimSequence> CrouchWalkAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Animation|Prototype", meta = (ClampMin = "0.0"))
	float LandingVisualSeconds = 0.35f;

	float GetLandingTimeRemaining() const { return LandingTimeRemaining; }

private:
	/** Visual sprint state is inferred from replicated speed, including on simulated proxies. */
	UPROPERTY(EditDefaultsOnly, Category = "Operation Mouse|Animation", meta = (ClampMin = "0.0"))
	float SprintSpeedThreshold = 750.0f;

	bool bWasFalling = false;
	float LandingTimeRemaining = 0.0f;
	FName LastDiagnosticState = NAME_None;
};
