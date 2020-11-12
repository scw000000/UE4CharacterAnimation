// Fill out your copyright notice in the Description page of Project Settings.


#include "ThirdPersonAnimInstance.h"
#include "Animation/AnimNode_StateMachine.h"
#include "Camera/CameraComponent.h"
#include "Components/PPCharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework//CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "PPCharacter.h"
// #include "Animation/AnimSequence.h"

#include "DrawDebugHelpers.h"

UThirdPersonAnimInstance::UThirdPersonAnimInstance(const FObjectInitializer& objectInitializer) : Super(objectInitializer)
{
}

void UThirdPersonAnimInstance::NativeInitializeAnimation()
{
	MotionStateMachine = GetStateMachineInstance(GetStateMachineIndex(FName("Motion State")));
	PPCharacterMovement = Cast<UPPCharacterMovementComponent>(GetOwningActor()->GetComponentByClass(UPPCharacterMovementComponent::StaticClass()));
}

void UThirdPersonAnimInstance::NativeUpdateAnimation(float deltaSeconds)
{
	// auto ddd = GFrameCounter;
	APPCharacter* owner = Cast<APPCharacter>(this->GetOwningActor());
	if (!owner || !MotionStateMachine)
	{
		return;
	}
	// UCharacterMovementComponent* characterMovement = owner->GetCharacterMovement();
	const FName& newState = MotionStateMachine->GetCurrentStateName();
	if (newState != CurrentMotionState)
	{
		PreviousMotionState = CurrentMotionState;
		CurrentMotionState = newState;
	}
	
	const FVector& actorForward = owner->GetActorForwardVector();
	const FVector& aimingDir = owner->AimingLocation - owner->GetActorLocation();
	
	AimingYaw = CalculateYaw(actorForward, aimingDir);
	AimingVSVelocityYaw = CalculateYaw(aimingDir, owner->GetVelocity());

	const FVector& velocity = owner->GetVelocity();
	FVector clampedStrafeVector = FVector::ZeroVector;
	if (IsStrafing())
	{
		// calculate yaw between moving and aiming direction
		float aimToVelocityYaw = CalculateYaw(aimingDir, velocity);
		// UE_LOG(LogTemp, Log, TEXT("aim yaw %f"), aimToVelocityYaw);
		// clamp the yaw based on strafe type
		// farward: [-45 + -(45), 90 + (45)]
		// backward: [-90 + (45), -180], [135 + (-45), 180]
		switch (StrafeType)
		{
		case EStrafeType::StrafeForward:
			aimToVelocityYaw = FMath::Clamp(aimToVelocityYaw, -45.f, 90.f);
			break;
		case EStrafeType::StrafeBackward:
			aimToVelocityYaw = aimToVelocityYaw >= 0.f ? FMath::Clamp(aimToVelocityYaw, 135.f, 180.f) : FMath::Clamp(aimToVelocityYaw, -180.f, -90.f);
			break;
		default:
			checkNoEntry();
			break;
		};
		clampedStrafeVector = FRotator(0.f, -aimToVelocityYaw, 0.f).RotateVector(velocity.GetSafeNormal2D());

		// UE_LOG(LogTemp, Log, TEXT("aim yaw after clamp %f"), aimToVelocityYaw);
		// rotate in reversed direction
		// const FVector& clampedStrafeVector = GetClampedStrafeForwardVector((owner->AimingLocation - owner->GetActorLocation()), owner->GetVelocity());
		if (!clampedStrafeVector.IsNearlyZero())
		{
			owner->GetCharacterMovement()->bOrientRotationToMovement = false;
			const FVector& dir2DLocal = owner->GetActorTransform().InverseTransformVectorNoScale(clampedStrafeVector);
			float maxTurnRad = FMath::Acos(dir2DLocal.X);

			//UE_LOG(LogTemp, Log, TEXT("dir 2d local %s turn deg %f"), *dir2DLocal.ToString(), FMath::RadiansToDegrees(maxTurnRad));
			owner->AddActorLocalRotation(
				FQuat(
					FVector::UpVector,
					FMath::Min(maxTurnRad * deltaSeconds * TurnToAimingLocationRate, maxTurnRad) * (dir2DLocal.Y >= 0.f ? 1.f : -1.f)
				)
			);
		}
	}
	

	// AimingYaw = calculateYaw(owner->AimingLocation - owner->GetActorLocation());
	// UE_LOG(LogTemp, Log, TEXT("aim yaw after clamp%f"), AimingYaw);
	if (!ShouldUpdateMotionDetailStatus) 
	{
		return;
	}
	UpdateMotionDetailStatus();
	
}

void UThirdPersonAnimInstance::NativePostEvaluateAnimation()
{
	StrafeType = EStrafeType::Invalid;
	FName internalStateName;

	if (MotionStateMachine->GetCurrentStateName() == FName("Jog State"))
	{
		internalStateName = GetStateMachineInstance(GetStateMachineIndex(FName("Jog State")))->GetCurrentStateName();
	}
	else if (MotionStateMachine->GetCurrentStateName() == FName("Crouch State"))
	{
		internalStateName = GetStateMachineInstance(GetStateMachineIndex(FName("Crouch State")))->GetCurrentStateName();
	}

	StrafeType = internalStateName == FName("Strafe Forward") ? EStrafeType::StrafeForward :
		internalStateName == FName("Strafe Backward") ? EStrafeType::StrafeBackward : EStrafeType::Invalid;
}

float UThirdPersonAnimInstance::GetClampedJogSpeed(bool isRunning)
{
	float ret = isRunning ? 647.48f : 174.f;
	// this also applies to crouch state, if it's crouching, isRunning should be false
	if (MotionStateMachine->GetCurrentStateName() != FName("Jog State") || !isRunning)
	{
		return ret;
	}
	// return ret;

	auto getVelocityYaw = [&]() 
	{
		APPCharacter* owner = Cast<APPCharacter>(this->GetOwningActor());
		const FVector& acotrForward = owner->GetActorForwardVector();
		return CalculateYaw(acotrForward, owner->GetVelocity());
	};

	// if it's strafing, based on strafe mode and velocity yaw, setup max velocity
	FName jobStateName = GetStateMachineInstance(GetStateMachineIndex(FName("Jog State")))->GetCurrentStateName();
	if (jobStateName == FName("Strafe Forward"))
	{
		float velocityYaw = getVelocityYaw();
		float absVelocityYaw = FMath::Abs(velocityYaw);
		ret = absVelocityYaw <= 45.f ?
			FMath::GetMappedRangeValueClamped(FVector2D(0.f, 45.f), FVector2D(647.48f, 379.f), absVelocityYaw) :
			(velocityYaw > 45.f ?
				FMath::GetMappedRangeValueClamped(FVector2D(45.f, 90.f), FVector2D(379.f, 174.f), velocityYaw) :
				379.f);
		//UE_LOG(LogTemp, Log, TEXT("yaw %f ret %f"), velocityYaw, ret);
		return ret;
	}

	if (jobStateName == FName("Strafe Backward"))
	{
		float velocityYaw = getVelocityYaw();
		float absVelocityYaw = FMath::Abs(velocityYaw);
		ret = absVelocityYaw >= 135.f ?
			231.7f :
			(velocityYaw > -135.f && velocityYaw < 0.f?
				FMath::GetMappedRangeValueClamped(FVector2D(-135.f, -90.f), FVector2D(231.7f, 174.f), velocityYaw) :
				231.f);
		return ret;
	}
	return ret;
}

void UThirdPersonAnimInstance::UpdateMotionDetailStatus()
{
	APPCharacter* owner = Cast<APPCharacter>(this->GetOwningActor());
	if (!owner)
	{
		return;
	}

	UCharacterMovementComponent* characterMovement = owner->GetCharacterMovement();

	Acceleration = characterMovement->GetCurrentAcceleration();
	ForwardDirection = owner->GetActorForwardVector();
	Velocity = owner->GetVelocity();

	IsOnGround = characterMovement->IsMovingOnGround();
	IsCrouched = owner->bIsCrouched;
	IsAiming = owner->IsAiming;
	if (IsOnGround)
	{
		const FVector& actorLocation = owner->GetActorLocation();
		StandingLocationZOffset = actorLocation.Z - LastStandingLocation.Z;
		LastStandingLocation = actorLocation;
	}

	const FVector& acotrForward = owner->GetActorForwardVector();
	AccelerationYaw = CalculateYaw(acotrForward, Acceleration);
	VelocityYaw = CalculateYaw(acotrForward, Velocity);
	// AiminigYaw = calculateYaw(owner->AimingLocation - owner->GetActorLocation());

	// will need to be updated when adding more movement modes
	MaxSpeed = characterMovement->MaxWalkSpeed;


	FMarkerSyncAnimPosition marker = this->GetSyncGroupPosition(FName("Foot Phase"));
	NextFootMarkerName = marker.NextMarkerName;
}

float UThirdPersonAnimInstance::CalculateYaw(const FVector& va, const FVector& vb)
{
	const FVector& vaXY = va.GetSafeNormal2D();
	if (vaXY.IsNearlyZero())
	{
		return 0.f;
	}
	
	const FVector& vbXY = vb.GetSafeNormal2D();
	if (vbXY.IsNearlyZero())
	{
		return 0.f;
	}

	return FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(vaXY, vbXY))) *
		(FVector2D::CrossProduct(FVector2D(vaXY), FVector2D(vbXY)) >= 0.f ? 1.f : -1.f);

}
