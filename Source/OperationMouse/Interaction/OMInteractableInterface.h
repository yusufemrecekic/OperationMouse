#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "OMInteractableInterface.generated.h"

UENUM(BlueprintType)
enum class EOMInteractionType : uint8
{
	Instant,
	Hold
};

/** Small data contract used by local prompts and authoritative validation. */
USTRUCT(BlueprintType)
struct FOMInteractionInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText Prompt = NSLOCTEXT("OperationMouse", "DefaultInteractionPrompt", "Interact");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	EOMInteractionType Type = EOMInteractionType::Instant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0.1"))
	float HoldDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "1.0"))
	float MaximumDistance = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bExclusive = true;
};

UINTERFACE(BlueprintType)
class OPERATIONMOUSE_API UOMInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

/** Server-facing interaction contract. Implementing actors own their persistent state. */
class OPERATIONMOUSE_API IOMInteractableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Operation Mouse|Interaction")
	FOMInteractionInfo GetInteractionInfo(AActor* Interactor) const;
	virtual FOMInteractionInfo GetInteractionInfo_Implementation(AActor* Interactor) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Operation Mouse|Interaction")
	FVector GetInteractionPoint() const;
	virtual FVector GetInteractionPoint_Implementation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Operation Mouse|Interaction")
	bool CanInteract(AActor* Interactor) const;
	virtual bool CanInteract_Implementation(AActor* Interactor) const;

	/** Called only by the server. Return false when a claim loses contention. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Operation Mouse|Interaction")
	bool BeginInteraction(AActor* Interactor);
	virtual bool BeginInteraction_Implementation(AActor* Interactor);

	/** Called only by the server when an accepted Hold interaction is interrupted. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Operation Mouse|Interaction")
	void CancelInteraction(AActor* Interactor);
	virtual void CancelInteraction_Implementation(AActor* Interactor);

	/** Called only by the server after Instant validation or authoritative Hold timing. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Operation Mouse|Interaction")
	void CompleteInteraction(AActor* Interactor);
	virtual void CompleteInteraction_Implementation(AActor* Interactor);
};
