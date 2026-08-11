#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "OMGameState.generated.h"

/** Replicated home for session-wide state shared with every player. */
UCLASS(Blueprintable)
class OPERATIONMOUSE_API AOMGameState : public AGameState
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
};
