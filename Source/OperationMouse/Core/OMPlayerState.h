#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "OMPlayerState.generated.h"

/** Replicated per-player state that survives pawn replacement. */
UCLASS(Blueprintable)
class OPERATIONMOUSE_API AOMPlayerState : public APlayerState
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
};
