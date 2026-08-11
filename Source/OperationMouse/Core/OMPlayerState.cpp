#include "OMPlayerState.h"

#include "../OperationMouse.h"

void AOMPlayerState::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[%s] PlayerState ready: %s Owner=%s"),
		HasAuthority() ? TEXT("Server") : TEXT("Client"),
		*GetName(),
		*GetNameSafe(GetOwner()));
}
