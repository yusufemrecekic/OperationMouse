#include "OMGameMode.h"

#include "../Characters/OMMouseCharacter.h"
#include "OMGameState.h"
#include "OMPlayerController.h"
#include "OMPlayerState.h"
#include "../OperationMouse.h"

AOMGameMode::AOMGameMode()
{
	GameStateClass = AOMGameState::StaticClass();
	PlayerStateClass = AOMPlayerState::StaticClass();
	PlayerControllerClass = AOMPlayerController::StaticClass();
	DefaultPawnClass = AOMMouseCharacter::StaticClass();
}

void AOMGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[Server] Player joined: Controller=%s PlayerState=%s"),
		*GetNameSafe(NewPlayer),
		*GetNameSafe(NewPlayer ? NewPlayer->PlayerState : nullptr));
}

void AOMGameMode::RestartPlayer(AController* NewPlayer)
{
	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[Server] Spawning default pawn for Controller=%s"),
		*GetNameSafe(NewPlayer));

	Super::RestartPlayer(NewPlayer);

	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[Server] Spawn result: Controller=%s Pawn=%s"),
		*GetNameSafe(NewPlayer),
		*GetNameSafe(NewPlayer ? NewPlayer->GetPawn() : nullptr));
}
