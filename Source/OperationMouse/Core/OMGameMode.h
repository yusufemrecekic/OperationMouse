#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "OMGameMode.generated.h"

/** Server-only authority that selects the core multiplayer gameplay classes. */
UCLASS(Blueprintable)
class OPERATIONMOUSE_API AOMGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AOMGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void RestartPlayer(AController* NewPlayer) override;
};
