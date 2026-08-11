#include "OMPlayerController.h"

#include "../OperationMouse.h"
#include "GameFramework/PlayerState.h"

void AOMPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[%s] PlayerController ready: %s Local=%s PlayerState=%s"),
		HasAuthority() ? TEXT("Server") : TEXT("Client"),
		*GetName(),
		IsLocalController() ? TEXT("true") : TEXT("false"),
		*GetNameSafe(PlayerState.Get()));
}

void AOMPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[Server] Controller %s possessed Pawn=%s"),
		*GetName(),
		*GetNameSafe(InPawn));
}

void AOMPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	UE_LOG(
		LogOperationMouse,
		Log,
		TEXT("[Client] Possession replicated: Controller=%s Pawn=%s"),
		*GetName(),
		*GetNameSafe(GetPawn()));
}
