#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "OMMouseCharacter.generated.h"

class UStaticMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
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

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	/** Temporary visible marker until a production mouse skeletal mesh is integrated. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Operation Mouse|Presentation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> PlaceholderVisual;

	/** Keeps the third-person camera behind the character while following controller rotation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Operation Mouse|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Operation Mouse|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> GameplayMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Operation Mouse|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;
};
