#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "OMPlayerController.generated.h"

/** Owns a player's connection and possession boundary; input arrives in Phase 2. */
UCLASS(Blueprintable)
class OPERATIONMOUSE_API AOMPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnRep_Pawn() override;
};
