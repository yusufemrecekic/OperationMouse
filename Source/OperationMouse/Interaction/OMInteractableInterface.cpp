#include "OMInteractableInterface.h"

FOMInteractionInfo IOMInteractableInterface::GetInteractionInfo_Implementation(AActor* Interactor) const
{
	return FOMInteractionInfo();
}

FVector IOMInteractableInterface::GetInteractionPoint_Implementation() const
{
	return FVector::ZeroVector;
}

bool IOMInteractableInterface::CanInteract_Implementation(AActor* Interactor) const
{
	return true;
}

bool IOMInteractableInterface::BeginInteraction_Implementation(AActor* Interactor)
{
	return true;
}

void IOMInteractableInterface::CancelInteraction_Implementation(AActor* Interactor)
{
}

void IOMInteractableInterface::CompleteInteraction_Implementation(AActor* Interactor)
{
}
