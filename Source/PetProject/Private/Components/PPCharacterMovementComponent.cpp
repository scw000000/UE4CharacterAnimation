// Fill out your copyright notice in the Description page of Project Settings.


#include "PPCharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"
#include "Engine/NetworkObjectList.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SplineComponent.h"

#include "Kismet/KismetMathLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogCharacterMovement, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogNavMeshMovement, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogCharacterNetSmoothing, Log, All);

/**
 * Character stats
 */
DECLARE_CYCLE_STAT(TEXT("Char Tick"), STAT_CharacterMovementTick, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char NonSimulated Time"), STAT_CharacterMovementNonSimulated, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char Simulated Time"), STAT_CharacterMovementSimulated, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char PerformMovement"), STAT_CharacterMovementPerformMovement, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char ReplicateMoveToServer"), STAT_CharacterMovementReplicateMoveToServer, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char CallServerMove"), STAT_CharacterMovementCallServerMove, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char ServerMove"), STAT_CharacterMovementServerMove, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char ServerForcePositionUpdate"), STAT_CharacterMovementForcePositionUpdate, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char RootMotionSource Calculate"), STAT_CharacterMovementRootMotionSourceCalculate, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char RootMotionSource Apply"), STAT_CharacterMovementRootMotionSourceApply, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char ClientUpdatePositionAfterServerUpdate"), STAT_CharacterMovementClientUpdatePositionAfterServerUpdate, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char CombineNetMove"), STAT_CharacterMovementCombineNetMove, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char NetSmoothCorrection"), STAT_CharacterMovementSmoothCorrection, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char SmoothClientPosition"), STAT_CharacterMovementSmoothClientPosition, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char SmoothClientPosition_Interp"), STAT_CharacterMovementSmoothClientPosition_Interp, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char SmoothClientPosition_Visual"), STAT_CharacterMovementSmoothClientPosition_Visual, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char Physics Interation"), STAT_CharPhysicsInteraction, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char StepUp"), STAT_CharStepUp, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char FindFloor"), STAT_CharFindFloor, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char AdjustFloorHeight"), STAT_CharAdjustFloorHeight, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char Update Acceleration"), STAT_CharUpdateAcceleration, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char MoveUpdateDelegate"), STAT_CharMoveUpdateDelegate, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char PhysWalking"), STAT_CharPhysWalking, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char PhysFalling"), STAT_CharPhysFalling, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char PhysNavWalking"), STAT_CharPhysNavWalking, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char NavProjectPoint"), STAT_CharNavProjectPoint, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char NavProjectLocation"), STAT_CharNavProjectLocation, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char ProcessLanded"), STAT_CharProcessLanded, STATGROUP_Character);


DECLARE_CYCLE_STAT(TEXT("Char HandleImpact"), STAT_CharHandleImpact, STATGROUP_Character);
// Defines for build configs
#if DO_CHECK && !UE_BUILD_SHIPPING // Disable even if checks in shipping are enabled.
#define devCode( Code )		checkCode( Code )
#else
#define devCode(...)
#endif

UPPCharacterMovementComponent::UPPCharacterMovementComponent(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
{
	// SetupWalkMode();
}

void UPPCharacterMovementComponent::StartApplyAnimCurveRootMotionSource()
{
	USkeletalMeshComponent* skeletalComponent = Cast<USkeletalMeshComponent>(GetOwner()->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
	if (!skeletalComponent)
	{
		return;
	}
	FRootMotionSource_AnimationCurve* animMotiomSource = new FRootMotionSource_AnimationCurve;
	animMotiomSource->Duration = -1.f;
	animMotiomSource->AccumulateMode = ERootMotionAccumulateMode::Override;
	animMotiomSource->bInLocalSpace = false;
	/*animMotiomSource->InitialTransform = GetOwner()->GetActorTransform();*/
	animMotiomSource->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity;
	animMotiomSource->CachedMeshComponentTransform = skeletalComponent->GetComponentTransform();
	RootMotionSourceID = ApplyRootMotionSource(animMotiomSource);
}

void UPPCharacterMovementComponent::StopApplyRootMotionSource()
{
	if (RootMotionSourceID == (uint16)ERootMotionSourceID::Invalid)
	{
		return;
	}
	RemoveRootMotionSourceByID(RootMotionSourceID);
	RootMotionSourceID = (uint16)ERootMotionSourceID::Invalid;
}

void UPPCharacterMovementComponent::PerformMovement(float DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_CharacterMovementPerformMovement);

	const UWorld* MyWorld = GetWorld();
	if (!HasValidData() || MyWorld == nullptr)
	{
		return;
	}

	// no movement if we can't move, or if currently doing physical simulation on UpdatedComponent
	if (MovementMode == MOVE_None || UpdatedComponent->Mobility != EComponentMobility::Movable || UpdatedComponent->IsSimulatingPhysics())
	{
		if (!CharacterOwner->bClientUpdating && !CharacterOwner->bServerMoveIgnoreRootMotion && CharacterOwner->IsPlayingRootMotion() && CharacterOwner->GetMesh())
		{
			// Consume root motion
			TickCharacterPose(DeltaSeconds);
			RootMotionParams.Clear();
			CurrentRootMotion.Clear();
		}
		// Clear pending physics forces
		ClearAccumulatedForces();
		return;
	}

	// Force floor update if we've moved outside of CharacterMovement since last update.
	bForceNextFloorCheck |= (IsMovingOnGround() && UpdatedComponent->GetComponentLocation() != LastUpdateLocation);

	// Update saved LastPreAdditiveVelocity with any external changes to character Velocity that happened since last update.
	if (CurrentRootMotion.HasAdditiveVelocity())
	{
		const FVector Adjustment = (Velocity - LastUpdateVelocity);
		CurrentRootMotion.LastPreAdditiveVelocity += Adjustment;

#if ROOT_MOTION_DEBUG
		if (RootMotionSourceDebug::CVarDebugRootMotionSources.GetValueOnGameThread() == 1)
		{
			if (!Adjustment.IsNearlyZero())
			{
				FString AdjustedDebugString = FString::Printf(TEXT("PerformMovement HasAdditiveVelocity LastUpdateVelocityAdjustment LastPreAdditiveVelocity(%s) Adjustment(%s)"),
					*CurrentRootMotion.LastPreAdditiveVelocity.ToCompactString(), *Adjustment.ToCompactString());
				RootMotionSourceDebug::PrintOnScreen(*CharacterOwner, AdjustedDebugString);
			}
		}
#endif
	}

	FVector OldVelocity;
	FVector OldLocation;

	// Scoped updates can improve performance of multiple MoveComponent calls.
	{
		FScopedMovementUpdate ScopedMovementUpdate(UpdatedComponent, bEnableScopedMovementUpdates ? EScopedUpdate::DeferredUpdates : EScopedUpdate::ImmediateUpdates);

		MaybeUpdateBasedMovement(DeltaSeconds);

		// Clean up invalid RootMotion Sources.
		// This includes RootMotion sources that ended naturally.
		// They might want to perform a clamp on velocity or an override, 
		// so we want this to happen before ApplyAccumulatedForces and HandlePendingLaunch as to not clobber these.
		const bool bHasRootMotionSources = HasRootMotionSources();
		if (bHasRootMotionSources && !CharacterOwner->bClientUpdating && !CharacterOwner->bServerMoveIgnoreRootMotion)
		{
			SCOPE_CYCLE_COUNTER(STAT_CharacterMovementRootMotionSourceCalculate);

			const FVector VelocityBeforeCleanup = Velocity;
			CurrentRootMotion.CleanUpInvalidRootMotion(DeltaSeconds, *CharacterOwner, *this);

#if ROOT_MOTION_DEBUG
			if (RootMotionSourceDebug::CVarDebugRootMotionSources.GetValueOnGameThread() == 1)
			{
				if (Velocity != VelocityBeforeCleanup)
				{
					const FVector Adjustment = Velocity - VelocityBeforeCleanup;
					FString AdjustedDebugString = FString::Printf(TEXT("PerformMovement CleanUpInvalidRootMotion Velocity(%s) VelocityBeforeCleanup(%s) Adjustment(%s)"),
						*Velocity.ToCompactString(), *VelocityBeforeCleanup.ToCompactString(), *Adjustment.ToCompactString());
					RootMotionSourceDebug::PrintOnScreen(*CharacterOwner, AdjustedDebugString);
				}
			}
#endif
		}

		OldVelocity = Velocity;
		OldLocation = UpdatedComponent->GetComponentLocation();

		ApplyAccumulatedForces(DeltaSeconds);

		// Update the character state before we do our movement
		UpdateCharacterStateBeforeMovement(DeltaSeconds);

		if (MovementMode == MOVE_NavWalking && bWantsToLeaveNavWalking)
		{
			TryToLeaveNavWalking();
		}

		// Character::LaunchCharacter() has been deferred until now.
		HandlePendingLaunch();
		ClearAccumulatedForces();

#if ROOT_MOTION_DEBUG
		if (RootMotionSourceDebug::CVarDebugRootMotionSources.GetValueOnGameThread() == 1)
		{
			if (OldVelocity != Velocity)
			{
				const FVector Adjustment = Velocity - OldVelocity;
				FString AdjustedDebugString = FString::Printf(TEXT("PerformMovement ApplyAccumulatedForces+HandlePendingLaunch Velocity(%s) OldVelocity(%s) Adjustment(%s)"),
					*Velocity.ToCompactString(), *OldVelocity.ToCompactString(), *Adjustment.ToCompactString());
				RootMotionSourceDebug::PrintOnScreen(*CharacterOwner, AdjustedDebugString);
			}
		}
#endif

		// Update saved LastPreAdditiveVelocity with any external changes to character Velocity that happened due to ApplyAccumulatedForces/HandlePendingLaunch
		if (CurrentRootMotion.HasAdditiveVelocity())
		{
			const FVector Adjustment = (Velocity - OldVelocity);
			CurrentRootMotion.LastPreAdditiveVelocity += Adjustment;

#if ROOT_MOTION_DEBUG
			if (RootMotionSourceDebug::CVarDebugRootMotionSources.GetValueOnGameThread() == 1)
			{
				if (!Adjustment.IsNearlyZero())
				{
					FString AdjustedDebugString = FString::Printf(TEXT("PerformMovement HasAdditiveVelocity AccumulatedForces LastPreAdditiveVelocity(%s) Adjustment(%s)"),
						*CurrentRootMotion.LastPreAdditiveVelocity.ToCompactString(), *Adjustment.ToCompactString());
					RootMotionSourceDebug::PrintOnScreen(*CharacterOwner, AdjustedDebugString);
				}
			}
#endif
		}

		// Prepare Root Motion (generate/accumulate from root motion sources to be used later)
		if (bHasRootMotionSources && !CharacterOwner->bClientUpdating && !CharacterOwner->bServerMoveIgnoreRootMotion)
		{
			// Animation root motion - If using animation RootMotion, tick animations before running physics.
			if (CharacterOwner->IsPlayingRootMotion() && CharacterOwner->GetMesh())
			{
				TickCharacterPose(DeltaSeconds);

				// Make sure animation didn't trigger an event that destroyed us
				if (!HasValidData())
				{
					return;
				}

				// For local human clients, save off root motion data so it can be used by movement networking code.
				if (CharacterOwner->IsLocallyControlled() && (CharacterOwner->Role == ROLE_AutonomousProxy) && CharacterOwner->IsPlayingNetworkedRootMotionMontage())
				{
					CharacterOwner->ClientRootMotionParams = RootMotionParams;
				}
			}

			// Generates root motion to be used this frame from sources other than animation
			{
				SCOPE_CYCLE_COUNTER(STAT_CharacterMovementRootMotionSourceCalculate);
				CurrentRootMotion.PrepareRootMotion(DeltaSeconds, *CharacterOwner, *this, true);
			}

			// For local human clients, save off root motion data so it can be used by movement networking code.
			if (CharacterOwner->IsLocallyControlled() && (CharacterOwner->Role == ROLE_AutonomousProxy))
			{
				CharacterOwner->SavedRootMotion = CurrentRootMotion;
			}
		}

		// Apply Root Motion to Velocity
		if (CurrentRootMotion.HasOverrideVelocity() || HasAnimRootMotion())
		{
			// Animation root motion overrides Velocity and currently doesn't allow any other root motion sources
			if (HasAnimRootMotion())
			{
				// Convert to world space (animation root motion is always local)
				USkeletalMeshComponent * SkelMeshComp = CharacterOwner->GetMesh();
				if (SkelMeshComp)
				{
					// Convert Local Space Root Motion to world space. Do it right before used by physics to make sure we use up to date transforms, as translation is relative to rotation.
					RootMotionParams.Set(ConvertLocalRootMotionToWorld(RootMotionParams.GetRootMotionTransform()));
				}

				// Then turn root motion to velocity to be used by various physics modes.
				if (DeltaSeconds > 0.f)
				{
					AnimRootMotionVelocity = CalcAnimRootMotionVelocity(RootMotionParams.GetRootMotionTransform().GetTranslation(), DeltaSeconds, Velocity);
					Velocity = ConstrainAnimRootMotionVelocity(AnimRootMotionVelocity, Velocity);
				}

				UE_LOG(LogRootMotion, Log, TEXT("PerformMovement WorldSpaceRootMotion Translation: %s, Rotation: %s, Actor Facing: %s, Velocity: %s")
					, *RootMotionParams.GetRootMotionTransform().GetTranslation().ToCompactString()
					, *RootMotionParams.GetRootMotionTransform().GetRotation().Rotator().ToCompactString()
					, *CharacterOwner->GetActorForwardVector().ToCompactString()
					, *Velocity.ToCompactString()
				);
			}
			else
			{
				// We don't have animation root motion so we apply other sources
				if (DeltaSeconds > 0.f)
				{
					SCOPE_CYCLE_COUNTER(STAT_CharacterMovementRootMotionSourceApply);

					const FVector VelocityBeforeOverride = Velocity;
					FVector NewVelocity = Velocity;
					CurrentRootMotion.AccumulateOverrideRootMotionVelocity(DeltaSeconds, *CharacterOwner, *this, NewVelocity);
					Velocity = NewVelocity;

#if ROOT_MOTION_DEBUG
					if (RootMotionSourceDebug::CVarDebugRootMotionSources.GetValueOnGameThread() == 1)
					{
						if (VelocityBeforeOverride != Velocity)
						{
							FString AdjustedDebugString = FString::Printf(TEXT("PerformMovement AccumulateOverrideRootMotionVelocity Velocity(%s) VelocityBeforeOverride(%s)"),
								*Velocity.ToCompactString(), *VelocityBeforeOverride.ToCompactString());
							RootMotionSourceDebug::PrintOnScreen(*CharacterOwner, AdjustedDebugString);
						}
					}
#endif
				}
			}
		}

#if ROOT_MOTION_DEBUG
		if (RootMotionSourceDebug::CVarDebugRootMotionSources.GetValueOnGameThread() == 1)
		{
			FString AdjustedDebugString = FString::Printf(TEXT("PerformMovement Velocity(%s) OldVelocity(%s)"),
				*Velocity.ToCompactString(), *OldVelocity.ToCompactString());
			RootMotionSourceDebug::PrintOnScreen(*CharacterOwner, AdjustedDebugString);
		}
#endif

		// NaN tracking
		devCode(ensureMsgf(!Velocity.ContainsNaN(), TEXT("UCharacterMovementComponent::PerformMovement: Velocity contains NaN (%s)\n%s"), *GetPathNameSafe(this), *Velocity.ToString()));

		// Clear jump input now, to allow movement events to trigger it for next update.
		CharacterOwner->ClearJumpInput(DeltaSeconds);
		NumJumpApexAttempts = 0;

		// change position
		StartNewPhysics(DeltaSeconds, 0);

		if (!HasValidData())
		{
			return;
		}

		// Update character state based on change from movement
		UpdateCharacterStateAfterMovement(DeltaSeconds);

		if ((bAllowPhysicsRotationDuringAnimRootMotion || !HasAnimRootMotion()) && !CharacterOwner->IsMatineeControlled())
		{
			PhysicsRotation(DeltaSeconds);
		}

		// this is the hacking part of applying root motion rotation without animation
		// Apply Root Motion rotation after movement is complete.
		if (CurrentRootMotion.HasActiveRootMotionSources() || HasAnimRootMotion())
		{
			if (HasAnimRootMotion())
			{
				const FQuat OldActorRotationQuat = UpdatedComponent->GetComponentQuat();
				const FQuat RootMotionRotationQuat = RootMotionParams.GetRootMotionTransform().GetRotation();
				if (!RootMotionRotationQuat.IsIdentity())
				{
					const FQuat NewActorRotationQuat = RootMotionRotationQuat * OldActorRotationQuat;
					MoveUpdatedComponent(FVector::ZeroVector, NewActorRotationQuat, true);
				}

#if !(UE_BUILD_SHIPPING)
				// debug
				if (false)
				{
					const FRotator OldActorRotation = OldActorRotationQuat.Rotator();
					const FVector ResultingLocation = UpdatedComponent->GetComponentLocation();
					const FRotator ResultingRotation = UpdatedComponent->GetComponentRotation();

					// Show current position
					DrawDebugCoordinateSystem(MyWorld, CharacterOwner->GetMesh()->GetComponentLocation() + FVector(0, 0, 1), ResultingRotation, 50.f, false);

					// Show resulting delta move.
					DrawDebugLine(MyWorld, OldLocation, ResultingLocation, FColor::Red, false, 10.f);

					// Log details.
					UE_LOG(LogRootMotion, Warning, TEXT("PerformMovement Resulting DeltaMove Translation: %s, Rotation: %s, MovementBase: %s"), //-V595
						*(ResultingLocation - OldLocation).ToCompactString(), *(ResultingRotation - OldActorRotation).GetNormalized().ToCompactString(), *GetNameSafe(CharacterOwner->GetMovementBase()));

					const FVector RMTranslation = RootMotionParams.GetRootMotionTransform().GetTranslation();
					const FRotator RMRotation = RootMotionParams.GetRootMotionTransform().GetRotation().Rotator();
					UE_LOG(LogRootMotion, Warning, TEXT("PerformMovement Resulting DeltaError Translation: %s, Rotation: %s"),
						*(ResultingLocation - OldLocation - RMTranslation).ToCompactString(), *(ResultingRotation - OldActorRotation - RMRotation).GetNormalized().ToCompactString());
				}
#endif // !(UE_BUILD_SHIPPING)

				// Root Motion has been used, clear
				RootMotionParams.Clear();
			}
			else 
			{
				FTransform rootMotionTransform;
				for (const auto& RootMotionSource : CurrentRootMotion.RootMotionSources)
				{
					if (RootMotionSource.IsValid() && RootMotionSource->AccumulateMode == ERootMotionAccumulateMode::Override)
					{
						rootMotionTransform = RootMotionSource->RootMotionParams.GetRootMotionTransform();
						RootMotionSource->RootMotionParams.Clear();
						break;
					}
				}

				const FQuat OldActorRotationQuat = UpdatedComponent->GetComponentQuat();
				const FQuat RootMotionRotationQuat = rootMotionTransform.GetRotation();
				if (!RootMotionRotationQuat.IsIdentity())
				{
					const FQuat NewActorRotationQuat = RootMotionRotationQuat * OldActorRotationQuat;
					MoveUpdatedComponent(FVector::ZeroVector, NewActorRotationQuat, true);
				}
			}
		}

		// consume path following requested velocity
		bHasRequestedVelocity = false;

		OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
	} // End scoped movement update

	// Call external post-movement events. These happen after the scoped movement completes in case the events want to use the current state of overlaps etc.
	CallMovementUpdateDelegate(DeltaSeconds, OldLocation, OldVelocity);

	SaveBaseLocation();
	UpdateComponentVelocity();

	const bool bHasAuthority = CharacterOwner && CharacterOwner->HasAuthority();

	// If we move we want to avoid a long delay before replication catches up to notice this change, especially if it's throttling our rate.
	if (bHasAuthority && UNetDriver::IsAdaptiveNetUpdateFrequencyEnabled() && UpdatedComponent)
	{
		UNetDriver* NetDriver = MyWorld->GetNetDriver();
		if (NetDriver && NetDriver->IsServer())
		{
			FNetworkObjectInfo* NetActor = NetDriver->FindOrAddNetworkObjectInfo(CharacterOwner);

			if (NetActor && MyWorld->GetTimeSeconds() <= NetActor->NextUpdateTime && NetDriver->IsNetworkActorUpdateFrequencyThrottled(*NetActor))
			{
				if (ShouldCancelAdaptiveReplication())
				{
					NetDriver->CancelAdaptiveReplication(*NetActor);
				}
			}
		}
	}

	const FVector NewLocation = UpdatedComponent ? UpdatedComponent->GetComponentLocation() : FVector::ZeroVector;
	const FQuat NewRotation = UpdatedComponent ? UpdatedComponent->GetComponentQuat() : FQuat::Identity;

	if (bHasAuthority && UpdatedComponent && !IsNetMode(NM_Client))
	{
		const bool bLocationChanged = (NewLocation != LastUpdateLocation);
		const bool bRotationChanged = (NewRotation != LastUpdateRotation);
		if (bLocationChanged || bRotationChanged)
		{
			// Update ServerLastTransformUpdateTimeStamp. This is used by Linear smoothing on clients to interpolate positions with the correct delta time,
			// so the timestamp should be based on the client's move delta (ServerAccumulatedClientTimeStamp), not the server time when receiving the RPC.
			const bool bIsRemotePlayer = (CharacterOwner->GetRemoteRole() == ROLE_AutonomousProxy);
			const FNetworkPredictionData_Server_Character* ServerData = bIsRemotePlayer ? GetPredictionData_Server_Character() : nullptr;
			if (bIsRemotePlayer && ServerData)
			{
				ServerLastTransformUpdateTimeStamp = float(ServerData->ServerAccumulatedClientTimeStamp);
			}
			else
			{
				ServerLastTransformUpdateTimeStamp = MyWorld->GetTimeSeconds();
			}
		}
	}

	LastUpdateLocation = NewLocation;
	LastUpdateRotation = NewRotation;
	LastUpdateVelocity = Velocity;
}


void UPPCharacterMovementComponent::StartApplyRootMotionSource(FRootMotionSource* newMotionSource)
{
	RootMotionSourceID = ApplyRootMotionSource(newMotionSource);
}

//void UPPCharacterMovementComponent::StartApplyFloatCurveRootMotionSource(
//	const TArray<FFloatCurve*, TFixedAllocator<3>>& tranlationCurves,
//	const FVector2D& deltaCorrectionWindow,
//	const FTransform& deltaCorrectionTransform)
//{
//	USkeletalMeshComponent* skeletalComponent = Cast<USkeletalMeshComponent>(GetOwner()->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
//	if (!skeletalComponent)
//	{
//		return;
//	}
//	FRootMotionSource_FloatCurveReader* curveMotiomSource = new FRootMotionSource_FloatCurveReader;
//	curveMotiomSource->Duration = -1.f;
//	curveMotiomSource->AccumulateMode = ERootMotionAccumulateMode::Override;
//	curveMotiomSource->bInLocalSpace = false;
//	curveMotiomSource->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity;
//	curveMotiomSource->InitialTransform = GetOwner()->GetActorTransform();
//	curveMotiomSource->CachedMeshComponentTransform = skeletalComponent->GetComponentTransform();
//	curveMotiomSource->TranslationCurves = tranlationCurves;
//	curveMotiomSource->DeltaCorrectionWindow = deltaCorrectionWindow;
//	curveMotiomSource->DeltaCorrectionTransform = deltaCorrectionTransform;
//}

void FRootMotionSource_AnimationCurve::PrepareRootMotion
(
	float simulationTime,
	float movementTickTime,
	const ACharacter& character,
	const UCharacterMovementComponent& moveComponent
)
{
	FVector currentTranslation(
		character.GetMesh()->GetAnimInstance()->GetCurveValue(FName("translationX")),
		character.GetMesh()->GetAnimInstance()->GetCurveValue(FName("translationY")),
		character.GetMesh()->GetAnimInstance()->GetCurveValue(FName("translationZ"))
	);
	UE_LOG(LogTemp, Log, TEXT("anim: %s"), *currentTranslation.ToString());
	FQuat currentRotation(FRotator(0.f, character.GetMesh()->GetAnimInstance()->GetCurveValue(FName("Rot(yaw)")), 0.f));
	currentTranslation = CachedMeshComponentTransform.TransformVector(currentTranslation);
	// root motion only accepts diff, so need to calculate rotation diff for each frame
	FQuat rotationDiff = currentRotation * PreviousRotation.Inverse();

	RootMotionParams.Clear();
	if (movementTickTime > SMALL_NUMBER && simulationTime > SMALL_NUMBER)
	{
		// float CurrentTimeFraction = GetTime() / Duration;
		// float TargetTimeFraction = (GetTime() + simulationTime) / Duration;
		// FTransform newTransform((currentTranslation - PreviousTranslation) / movementTickTime);
		auto newVelocity = (currentTranslation - PreviousTranslation) / movementTickTime;
		// newVelocity = (currentTranslation - PreviousTranslation) / movementTickTime;
		//newTransform = character.GetMesh()->GetComponentToWorld();
		// newTransform.SetTranslation((currentTranslation - PreviousTranslation) / movementTickTime);
		RootMotionParams.Set(FTransform (rotationDiff, newVelocity));
		// UE_LOG(LogTemp, Log, TEXT("curve local translation val: %s"), *RootMotionParams.GetRootMotionTransform().GetLocation().ToString());
		//// If we're beyond specified duration, we need to re-map times so that
		//// we continue our desired ending velocity
		//if (TargetTimeFraction > 1.f)
		//{
		//	float TimeFractionPastAllowable = TargetTimeFraction - 1.0f;
		//	TargetTimeFraction -= TimeFractionPastAllowable;
		//	CurrentTimeFraction -= TimeFractionPastAllowable;
		//}

		//float CurrentMoveFraction = CurrentTimeFraction;
		//float TargetMoveFraction = TargetTimeFraction;

		//if (TimeMappingCurve)
		//{
		//	CurrentMoveFraction = TimeMappingCurve->GetFloatValue(CurrentTimeFraction);
		//	TargetMoveFraction = TimeMappingCurve->GetFloatValue(TargetTimeFraction);
		//}

		//const FVector CurrentRelativeLocation = GetRelativeLocation(CurrentMoveFraction);
		//const FVector TargetRelativeLocation = GetRelativeLocation(TargetMoveFraction);

		//const FVector Force = (TargetRelativeLocation - CurrentRelativeLocation) / movementTickTime;

		// Debug
#if ROOT_MOTION_DEBUG
		//if (RootMotionSourceDebug::CVarDebugRootMotionSources.GetValueOnGameThread() != 0)
		//{
		//	const FVector CurrentLocation = character.GetActorLocation();
		//	const FVector CurrentTargetLocation = CurrentLocation + (TargetRelativeLocation - CurrentRelativeLocation);
		//	const FVector LocDiff = moveComponent.UpdatedComponent->GetComponentLocation() - CurrentLocation;
		//	const float DebugLifetime = CVarDebugRootMotionSourcesLifetime.GetValueOnGameThread();

		//	// Current
		//	DrawDebugCapsule(character.GetWorld(), moveComponent.UpdatedComponent->GetComponentLocation(), character.GetSimpleCollisionHalfHeight(), character.GetSimpleCollisionRadius(), FQuat::Identity, FColor::Red, true, DebugLifetime);

		//	// Current Target
		//	DrawDebugCapsule(character.GetWorld(), CurrentTargetLocation + LocDiff, character.GetSimpleCollisionHalfHeight(), character.GetSimpleCollisionRadius(), FQuat::Identity, FColor::Green, true, DebugLifetime);

		//	// Target
		//	DrawDebugCapsule(character.GetWorld(), CurrentTargetLocation + LocDiff, character.GetSimpleCollisionHalfHeight(), character.GetSimpleCollisionRadius(), FQuat::Identity, FColor::Blue, true, DebugLifetime);

		//	// Force
		//	DrawDebugLine(character.GetWorld(), CurrentLocation, CurrentLocation + Force, FColor::Blue, true, DebugLifetime);

		//	// Halfway point
		//	const FVector HalfwayLocation = CurrentLocation + (GetRelativeLocation(0.5f) - CurrentRelativeLocation);
		//	if (SavedHalfwayLocation.IsNearlyZero())
		//	{
		//		SavedHalfwayLocation = HalfwayLocation;
		//	}
		//	if (FVector::DistSquared(SavedHalfwayLocation, HalfwayLocation) > 50.f*50.f)
		//	{
		//		UE_LOG(LogRootMotion, Verbose, TEXT("RootMotion JumpForce drifted from saved halfway calculation!"));
		//		SavedHalfwayLocation = HalfwayLocation;
		//	}
		//	DrawDebugCapsule(character.GetWorld(), HalfwayLocation + LocDiff, character.GetSimpleCollisionHalfHeight(), character.GetSimpleCollisionRadius(), FQuat::Identity, FColor::White, true, DebugLifetime);

		//	// Destination point
		//	const FVector DestinationLocation = CurrentLocation + (GetRelativeLocation(1.0f) - CurrentRelativeLocation);
		//	DrawDebugCapsule(character.GetWorld(), DestinationLocation + LocDiff, character.GetSimpleCollisionHalfHeight(), character.GetSimpleCollisionRadius(), FQuat::Identity, FColor::White, true, DebugLifetime);

		//	UE_LOG(LogRootMotion, VeryVerbose, TEXT("RootMotionJumpForce %s %s preparing from %f to %f from (%s) to (%s) resulting force %s"),
		//		character.Role == ROLE_AutonomousProxy ? TEXT("AUTONOMOUS") : TEXT("AUTHORITY"),
		//		character.bClientUpdating ? TEXT("UPD") : TEXT("NOR"),
		//		GetTime(), GetTime() + simulationTime,
		//		*CurrentLocation.ToString(), *CurrentTargetLocation.ToString(),
		//		*Force.ToString());

		//	{
		//		FString AdjustedDebugString = FString::Printf(TEXT("    FRootMotionSource_JumpForce::Prep Force(%s) SimTime(%.3f) MoveTime(%.3f) StartP(%.3f) EndP(%.3f)"),
		//			*Force.ToCompactString(), simulationTime, movementTickTime, CurrentMoveFraction, TargetMoveFraction);
		//		RootMotionSourceDebug::PrintOnScreen(character, AdjustedDebugString);
		//	}
		//}
#endif
	}
	else
	{
		checkf(Duration > SMALL_NUMBER, TEXT("FRootMotionSource_JumpForce prepared with invalid duration."));
	}

	PreviousTranslation = currentTranslation;
	PreviousRotation = currentRotation;

	SetTime(GetTime() + simulationTime);
}

//UScriptStruct* FRootMotionSource_AnimationCurveReader::GetScriptStruct() const
//{
//	return FRootMotionSource_AnimationCurveReader::StaticStruct();
//}

//FString FRootMotionSource_AnimationCurveReader::ToSimpleString() const
//{
//	return FString::Printf(TEXT("[ID:%u]FRootMotionSource_AnimationCurveReader %s"), LocalID, *InstanceName.GetPlainNameString());
//}

//void FRootMotionSource_AnimationCurveReader::AddReferencedObjects(class FReferenceCollector& Collector)
//{
//	Collector.AddReferencedObject(PathOffsetCurve);
//	Collector.AddReferencedObject(TimeMappingCurve);
//
//	FRootMotionSource::AddReferencedObjects(Collector);
//}

void FRootMotionSource_FloatCurve::PrepareRootMotion(
	float simulationTime,
	float movementTickTime,
	const ACharacter& character,
	const UCharacterMovementComponent& moveComponent
) 
{
	FVector currentTranslation(
		TranslationCurves[0] ? TranslationCurves[0]->Evaluate(CurrentTime + movementTickTime) : 0.f,
		TranslationCurves[1] ? TranslationCurves[1]->Evaluate(CurrentTime + movementTickTime) : 0.f,
		TranslationCurves[2] ? TranslationCurves[2]->Evaluate(CurrentTime + movementTickTime) : 0.f
	);
	// UE_LOG(LogTemp, Log, TEXT("anim: %s"), *currentTranslation.ToString());
	// FQuat currentRotation(FRotator(0.f, character.GetMesh()->GetAnimInstance()->GetCurveValue(FName("Rot(yaw)")), 0.f));
	FTransform targetTransform(FQuat::Identity, currentTranslation);
	// add additional transform from delta correction
	if (DeltaCorrectionWindow.X != DeltaCorrectionWindow.Y
		&& DeltaCorrectionWindow.X <= CurrentTime + movementTickTime) 
	{
		// calculate how much accumlated translation and rotation are needed
		if (CurrentTime + movementTickTime < DeltaCorrectionWindow.Y)
		{
			// const FVector& totalTranslation = DeltaCorrectionTransform.GetTranslation();
			const float windowDuration = (DeltaCorrectionWindow.Y - DeltaCorrectionWindow.X);
			const float elapseTime = FMath::Min(DeltaCorrectionWindow.Y, CurrentTime + movementTickTime) - DeltaCorrectionWindow.X;
			float alpha = elapseTime / windowDuration;
			FTransform correctionTransform = FTransform::Identity;
			correctionTransform.BlendWith(DeltaCorrectionTransform, alpha);
			targetTransform = correctionTransform * targetTransform;
		}
		else 
		{
			targetTransform = DeltaCorrectionTransform * targetTransform;
		}
		//// for x and y movement, it's uniform translation
		//currentTranslation += FVector(totalTranslation.X, totalTranslation.Y, 0.f) * fraction;
		//currentTranslation.Z += totalTranslation.Z * fraction;
		//// for z, it will be dicated by the current time
		//// v0 = -gt, v0t + 0.5gt^2 = h, -gt^2 + 0.5gt^2 = h, g = -2h / t^2
		//// const float gravity = -2.f * totalTranslation.Z / (windowDuration * windowDuration);
		//// v0t + 1/2 g t^2 = h
		//// currentTranslation.Z += -gravity * windowDuration * elapseTime + 0.5f * gravity * elapseTime * elapseTime;
		//// FTransform::ler
		//currentRotation = FQuat::FastLerp(FQuat::Identity, DeltaCorrectionTransform.GetRotation(), fraction).GetNormalized() * currentRotation;
	}
	// root motion only accepts diff, so need to calculate rotation diff for each frame
	targetTransform = character.GetMesh()->GetRelativeTransform().Inverse() * targetTransform * InitialTransform;
	RootMotionParams.Clear();
	if (movementTickTime > SMALL_NUMBER && simulationTime > SMALL_NUMBER)
	{
		FTransform deltaTransform = character.GetActorTransform().Inverse() * targetTransform;
		/*UE_LOG(LogTemp, Log, TEXT("%f target: %s"), CurrentTime + movementTickTime, *targetTransform.ToString());
		UE_LOG(LogTemp, Log, TEXT("delta: %s"), *deltaTransform.ToString());*/

		RootMotionParams.Set(FTransform(deltaTransform.GetRotation(), (deltaTransform.GetTranslation()) / movementTickTime));
#if ROOT_MOTION_DEBUG
#endif
	}
	else
	{
		checkf(Duration > SMALL_NUMBER, TEXT("FRootMotionSource_JumpForce prepared with invalid duration."));
	}

	SetTime(GetTime() + simulationTime);
}

