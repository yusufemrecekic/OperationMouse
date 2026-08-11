#include "OMGameState.h"

#include "../OperationMouse.h"

void AOMGameState::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[%s] GameState ready: %s"),
		HasAuthority() ? TEXT("Server") : TEXT("Client"),
		*GetName());
}
