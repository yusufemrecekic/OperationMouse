#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OMInteractionPromptWidget.generated.h"

class UProgressBar;
class UTextBlock;

/** Minimal local-only prompt used until the production HUD is built. */
UCLASS()
class OPERATIONMOUSE_API UOMInteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetPromptState(const FText& Prompt, bool bShowProgress, float Progress);
	void HidePrompt();

protected:
	virtual void NativeOnInitialized() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PromptText;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> HoldProgressBar;
};
