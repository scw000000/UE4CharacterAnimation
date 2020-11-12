// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifyState_HandIKWeightModifier.h"
#include "Animation/ThirdPersonAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotifyState_HandIKWeightModifier::NotifyBegin(USkeletalMeshComponent * meshComp, UAnimSequenceBase * animation, float totalDuration)
{
	AnimInstance = Cast<UThirdPersonAnimInstance>(meshComp->GetAnimInstance());
	if (!AnimInstance)
	{
		return;
	}
	if (ControlLeftHand)
	{
		AnimInstance->LeftHandIKWeight = BeginIKWeight;
	}
	if (ControlRightHand)
	{
		AnimInstance->RightHandIKWeight = BeginIKWeight;
	}

	if (BeginIKWeight < EndIKWeight)
	{
		if (ControlLeftHand)
		{
			AnimInstance->LeftHandEffectorLocation = AnimInstance->NextLeftHandEffectorLocation;
		}

		if (ControlRightHand)
		{
			AnimInstance->RightHandEffectorLocation = AnimInstance->NextRightHandEffectorLocation;
		}
	}

	TotalDuration = totalDuration;
	ElapsedTime = 0.f;
}

void UAnimNotifyState_HandIKWeightModifier::NotifyTick(USkeletalMeshComponent * meshComp, UAnimSequenceBase * animation, float frameDeltaTime)
{
	if (!AnimInstance)
	{
		return;
	}

	ElapsedTime += frameDeltaTime;
	float newIKWeight = FMath::GetMappedRangeValueClamped(FVector2D(0.f, TotalDuration), FVector2D(BeginIKWeight, EndIKWeight), ElapsedTime);

	if (ControlLeftHand)
	{
		AnimInstance->LeftHandIKWeight = newIKWeight;
	}

	if (ControlRightHand)
	{
		AnimInstance->RightHandIKWeight = newIKWeight;
	}
}

void UAnimNotifyState_HandIKWeightModifier::NotifyEnd(USkeletalMeshComponent * meshComp, UAnimSequenceBase * animation)
{
	if (!AnimInstance)
	{
		return;
	}

	if (ControlLeftHand)
	{
		AnimInstance->LeftHandIKWeight = EndIKWeight;
	}

	if (ControlRightHand)
	{
		AnimInstance->RightHandIKWeight = EndIKWeight;
	}
}