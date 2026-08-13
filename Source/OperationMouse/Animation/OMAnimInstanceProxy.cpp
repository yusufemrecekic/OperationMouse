#include "OMAnimInstanceProxy.h"

#include "OMAnimInstance.h"
#include "AnimationRuntime.h"

FOMAnimInstanceProxy::FOMAnimInstanceProxy(UAnimInstance* InAnimInstance)
	: FAnimInstanceProxy(InAnimInstance)
{
}

void FOMAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	Super::PreUpdate(InAnimInstance, DeltaSeconds);
	if (const UOMAnimInstance* Instance = Cast<UOMAnimInstance>(InAnimInstance))
	{
		SelectAnimations(*Instance);
	}
}

void FOMAnimInstanceProxy::Initialize(UAnimInstance* InAnimInstance)
{
	Super::Initialize(InAnimInstance);
	const FAnimationInitializeContext Context(this);
	PrimaryPlayer.Initialize_AnyThread(Context);
	SecondaryPlayer.Initialize_AnyThread(Context);
}

void FOMAnimInstanceProxy::UpdateAnimationNode(const FAnimationUpdateContext& InContext)
{
	PrimaryPlayer.Update_AnyThread(InContext.FractionalWeight(PrimaryWeight));
	if (PrimaryWeight < 1.0f)
	{
		SecondaryPlayer.Update_AnyThread(InContext.FractionalWeight(1.0f - PrimaryWeight));
	}
}

bool FOMAnimInstanceProxy::Evaluate(FPoseContext& Output)
{
	if (!PrimaryPlayer.GetSequence())
	{
		Output.ResetToRefPose();
		return true;
	}

	if (PrimaryWeight >= 1.0f || !SecondaryPlayer.GetSequence())
	{
		PrimaryPlayer.Evaluate_AnyThread(Output);
		return true;
	}

	FPoseContext PrimaryPose(Output);
	FPoseContext SecondaryPose(Output);
	PrimaryPlayer.Evaluate_AnyThread(PrimaryPose);
	SecondaryPlayer.Evaluate_AnyThread(SecondaryPose);
	FAnimationPoseData PrimaryPoseData(PrimaryPose);
	FAnimationPoseData SecondaryPoseData(SecondaryPose);
	FAnimationPoseData OutputPoseData(Output);
	FAnimationRuntime::BlendTwoPosesTogether(
		PrimaryPoseData,
		SecondaryPoseData,
		PrimaryWeight,
		OutputPoseData);
	return true;
}

FAnimNode_Base* FOMAnimInstanceProxy::GetCustomRootNode()
{
	return &PrimaryPlayer;
}

void FOMAnimInstanceProxy::GetCustomNodes(TArray<FAnimNode_Base*>& OutNodes)
{
	OutNodes.Add(&PrimaryPlayer);
	OutNodes.Add(&SecondaryPlayer);
}

void FOMAnimInstanceProxy::SelectAnimations(const UOMAnimInstance& Instance)
{
	UAnimSequenceBase* Primary = Instance.IdleAnimation;
	UAnimSequenceBase* Secondary = nullptr;
	bool bLoop = true;
	float PrimaryPlayRate = 1.0f;
	float SecondaryPlayRate = 1.0f;
	PrimaryWeight = 1.0f;

	if (Instance.bIsMantling)
	{
		Primary = Instance.FallAnimation ? Instance.FallAnimation : Instance.IdleAnimation;
	}
	else if (Instance.bIsFalling)
	{
		Primary = Instance.bIsAscending ? Instance.JumpAnimation : Instance.FallAnimation;
		bLoop = !Instance.bIsAscending;
	}
	else if (Instance.GetLandingTimeRemaining() > 0.0f)
	{
		Primary = Instance.LandAnimation;
		bLoop = false;
	}
	else if (Instance.bIsCrouched)
	{
		Primary = Instance.CrouchIdleAnimation;
		Secondary = Instance.CrouchWalkAnimation;
		PrimaryWeight = 1.0f - FMath::GetMappedRangeValueClamped(FVector2D(5.0f, 300.0f), FVector2D(0.0f, 1.0f), Instance.GroundSpeed);
		SecondaryPlayRate = FMath::Clamp(Instance.GroundSpeed / 300.0f, 0.75f, 1.5f);
	}
	else if (Instance.GroundSpeed > 5.0f)
	{
		Primary = Instance.WalkAnimation;
		Secondary = Instance.RunAnimation;
		PrimaryWeight = 1.0f - FMath::GetMappedRangeValueClamped(FVector2D(450.0f, 900.0f), FVector2D(0.0f, 1.0f), Instance.GroundSpeed);
		PrimaryPlayRate = FMath::Clamp(Instance.GroundSpeed / 600.0f, 0.75f, 1.25f);
		SecondaryPlayRate = FMath::Clamp(Instance.GroundSpeed / 900.0f, 0.75f, 1.25f);
	}

	ConfigurePlayer(PrimaryPlayer, Primary, bLoop, PrimaryPlayRate);
	ConfigurePlayer(SecondaryPlayer, Secondary, true, SecondaryPlayRate);
}

void FOMAnimInstanceProxy::ConfigurePlayer(
	FAnimNode_SequencePlayer_Standalone& Player,
	UAnimSequenceBase* Sequence,
	bool bLoop,
	float PlayRate)
{
	if (Player.GetSequence() != Sequence)
	{
		Player.SetSequence(Sequence);
		Player.SetAccumulatedTime(0.0f);
	}
	Player.SetLoopAnimation(bLoop);
	Player.SetPlayRate(PlayRate);
}
