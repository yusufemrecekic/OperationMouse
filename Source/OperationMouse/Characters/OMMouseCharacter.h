#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "OMMouseCharacter.generated.h"

class UStaticMeshComponent;

/** Replicated physical player avatar built on Unreal's CharacterMovementComponent. */
UCLASS(Blueprintable)
class OPERATIONMOUSE_API AOMMouseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AOMMouseCharacter();

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;

private:
	/** Temporary visible marker until a production mouse skeletal mesh is integrated. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Operation Mouse|Presentation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> PlaceholderVisual;
};
