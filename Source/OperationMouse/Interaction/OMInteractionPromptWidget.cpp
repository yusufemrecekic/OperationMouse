#include "OMInteractionPromptWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UOMInteractionPromptWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PromptRoot"));
	WidgetTree->RootWidget = RootBox;

	PromptText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PromptText"));
	PromptText->SetJustification(ETextJustify::Center);
	PromptText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	PromptText->SetShadowOffset(FVector2D(1.5f, 1.5f));
	PromptText->SetShadowColorAndOpacity(FLinearColor::Black);
	RootBox->AddChildToVerticalBox(PromptText);

	HoldProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HoldProgress"));
	HoldProgressBar->SetFillColorAndOpacity(FLinearColor(0.2f, 0.8f, 0.3f, 1.0f));
	if (UVerticalBoxSlot* ProgressSlot = RootBox->AddChildToVerticalBox(HoldProgressBar))
	{
		ProgressSlot->SetPadding(FMargin(40.0f, 6.0f, 40.0f, 0.0f));
	}

	SetAnchorsInViewport(FAnchors(0.5f, 0.75f));
	SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
	SetDesiredSizeInViewport(FVector2D(360.0f, 70.0f));
	HidePrompt();
}

void UOMInteractionPromptWidget::SetPromptState(const FText& Prompt, bool bShowProgress, float Progress)
{
	if (!PromptText || !HoldProgressBar)
	{
		return;
	}

	PromptText->SetText(Prompt);
	HoldProgressBar->SetVisibility(bShowProgress ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	HoldProgressBar->SetPercent(FMath::Clamp(Progress, 0.0f, 1.0f));
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UOMInteractionPromptWidget::HidePrompt()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
