// Fill out your copyright notice in the Description page of Project Settings.


#include "ClimbNodeGraph.h"
#include "Animation/AnimSequence.h"
#include "Animation/ThirdPersonAnimInstance.h"
#include "AnimationBlueprintLibrary.h"
#include "Components/InteractionComponent.h"
#include "Components/PPCharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "PPCharacter.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Components/BillboardComponent.h"
#include "Components/ClimbNodeGraphRenderingComponent.h"
#include "DrawDebugHelpers.h"
#include "UObject/ConstructorHelpers.h"
#endif

AClimbSegment::AClimbSegment()
{
	LedgeClimbingSpline = CreateDefaultSubobject<USplineComponent>(TEXT("LedgeClimbingSpline"));
	RootComponent = LedgeClimbingSpline;
	MovingRightIsForward = true;

	Climbable = true;
	EntryDistanceAlongSpline = -1.f;
	EntryRadius = -1.f;
	bHidden = false;

	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// create sprite for this actor
#if WITH_EDITOR
	//BillboardComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	//if (!IsRunningCommandlet() && (BillboardComponent != NULL))
	//{

	//	BillboardComponent->Sprite = ConstructorHelpers::FObjectFinderOptional<UTexture2D>(TEXT("/Engine/EditorResources/S_Actor")).Get();
	//	BillboardComponent->RelativeScale3D = FVector(0.5f, 0.5f, 0.5f);
	//	BillboardComponent->bHiddenInGame = true;
	//	BillboardComponent->bVisible = true;
	//	BillboardComponent->SpriteInfo.Category = FName(TEXT("GamePlay"));
	//	// BillboardComponent->SpriteInfo.DisplayName = FText((NSLOCTEXT("SpriteCategory", "Navigation", "Navigation")));
	//	BillboardComponent->SetupAttachment(RootComponent);
	//	BillboardComponent->SetAbsolute(false, false, true);
	//	BillboardComponent->bIsScreenSizeScaled = true;
	//}
#endif
}

void AClimbSegment::OnConstruction(const FTransform& transform)
{
	Super::OnConstruction(transform);

	LedgeClimbingSpline->EditorUnselectedSplineSegmentColor
		= Climbable ? FLinearColor::White : FLinearColor::Blue;
	LedgeClimbingSpline->EditorSelectedSplineSegmentColor
		= Climbable ? FLinearColor::Red : FLinearColor::Yellow;

	AClimbNodeGraph* owner = Cast<AClimbNodeGraph>(GetOwner());

	if (owner && EntryDistanceAlongSpline >= 0.f && EntryRadius > 0.f)
	{
		if (!InteractionCollider)
		{
			InteractionCollider = NewObject<USphereComponent>(this);
			InteractionCollider->RegisterComponent();
		}
		const FVector& colliderLocation = LedgeClimbingSpline->GetLocationAtDistanceAlongSpline(EntryDistanceAlongSpline, ESplineCoordinateSpace::World);
		InteractionCollider->SetWorldLocation(colliderLocation);
		InteractionCollider->SetSphereRadius(EntryRadius);
		InteractionCollider->AttachToComponent(GetRootComponent(), FAttachmentTransformRules(EAttachmentRule::KeepWorld, false));

	}
	else if(InteractionCollider)
	{
		InteractionCollider->DestroyComponent();
		InteractionCollider = nullptr;
	}

	owner->OnSegementConstruction(this);
}

AClimbNodeGraph::AClimbNodeGraph()
{
	USceneComponent* SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("PositionComponent"));
	RootComponent = SceneComponent;

	bHidden = true;

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

#if WITH_EDITOR
	/*EdRenderComp = CreateDefaultSubobject<UClimbNodeGraphRenderingComponent>(TEXT("EdRenderComp"));
	EdRenderComp->SetupAttachment(RootComponent);*/
#endif
}

void AClimbNodeGraph::GenerateClimbSegment()
{
	FActorSpawnParameters spawnParams;
	spawnParams.Owner = this;
	spawnParams.bHideFromSceneOutliner = false;
	// spawnParams.ObjectFlags = EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;
	AClimbSegment* newSegment = GetWorld()->SpawnActor<AClimbSegment>(AClimbSegment::StaticClass(), spawnParams);
	FAttachmentTransformRules attachRule(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, false);
	newSegment->AttachToActor(this, attachRule);
	ClimbSegments.Add(newSegment);
}

// remove this
void AClimbNodeGraph::OnSegementConstruction(AClimbSegment* segmentInConstruction) 
{
	if (segmentInConstruction->InteractionCollider)
	{
		segmentInConstruction->InteractionCollider->OnComponentBeginOverlap.AddUniqueDynamic(this, &AClimbNodeGraph::OnBeginOverlapCollider);
		segmentInConstruction->InteractionCollider->OnComponentEndOverlap.AddUniqueDynamic(this, &AClimbNodeGraph::OnEndOverlapCollider);
	}
	
}

#if WITH_EDITOR
void AClimbNodeGraph::OnConstruction(const FTransform& transform)
{
	ClimbSegments = ClimbSegments.FilterByPredicate([&](AClimbSegment* segment)
	{
		return segment && segment->GetAttachParentActor()->GetUniqueID() == GetUniqueID();
	});

	TArray<AActor*> attachedActors;
	GetAttachedActors(attachedActors);
	for (AActor* attachedActor : attachedActors) 
	{
		if (attachedActor->IsA<AClimbSegment>() && !ClimbSegments.Contains(Cast<AClimbSegment>(attachedActor)))
		{
			attachedActor->Destroy();
		}
	}	
}
#endif

// Called when the game starts or when spawned
void AClimbNodeGraph::BeginPlay()
{
	Super::BeginPlay();

}

void AClimbNodeGraph::OnBeginInteraction()
{
	check(InteractingClimbSegment);
	// remove delegate to avoid reinteraction
	//InteractingComponent->GetInteractionDelegate().RemoveDynamic(this, &AClimbNodeGraph::OnInteraction);
	InteractingCharacter->GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	InteractingCharacter->InputComponent->RemoveActionBinding(CharacterInputActionBindingIdx);
	CharacterInputActionBindingIdx = -1;

	USkeletalMeshComponent * skeletalComponent = InteractingCharacter->GetMesh();
	if (!skeletalComponent)
	{
		return;
	}
	if (!InteractingCharacterAnimInstance)
	{
		return;
	}
	InteractingCharacterAnimInstance->IsHanging = true; 

	// decide hand IK effector location
	// convert from ledge tranform to hand bone location
	// revert direction 
	float splineDirSclar = InteractingClimbSegment->MovingRightIsForward ? 1.f : -1.f;
	float leftHandLedgeDistanceAlongSpline = InteractingClimbSegment->EntryDistanceAlongSpline - 31.403269 * splineDirSclar;
	FTransform leftHandLedgeTransform(
		UKismetMathLibrary::MakeRotFromZY(FVector::UpVector, InteractingClimbSegment->LedgeClimbingSpline->GetTangentAtDistanceAlongSpline(leftHandLedgeDistanceAlongSpline, ESplineCoordinateSpace::World) * splineDirSclar),
		InteractingClimbSegment->LedgeClimbingSpline->GetLocationAtDistanceAlongSpline(leftHandLedgeDistanceAlongSpline, ESplineCoordinateSpace::World),
		FVector(1.f));
	InteractingCharacterAnimInstance->NextLeftHandEffectorLocation = (LedgeToLeftHandTransform * leftHandLedgeTransform).GetLocation();

	float rightHandLedgeDistanceAlongSpline = InteractingClimbSegment->EntryDistanceAlongSpline + 32.96600 * splineDirSclar;
	FTransform rightHandLedgeTransform(
		UKismetMathLibrary::MakeRotFromZY(FVector::UpVector, InteractingClimbSegment->LedgeClimbingSpline->GetTangentAtDistanceAlongSpline(rightHandLedgeDistanceAlongSpline, ESplineCoordinateSpace::World) * splineDirSclar),
		InteractingClimbSegment->LedgeClimbingSpline->GetLocationAtDistanceAlongSpline(rightHandLedgeDistanceAlongSpline, ESplineCoordinateSpace::World),
		FVector(1.f));
	InteractingCharacterAnimInstance->NextRightHandEffectorLocation = (LedgeToRightHandTransform * rightHandLedgeTransform).GetLocation();
	
	UAnimMontage* newMontage = InteractingCharacterAnimInstance->PlaySlotAnimationAsDynamicMontage(
		IdleToHangAnim,
		FName("DefaultSlot"),
		0.25f,
		0.25f);
	FAnimMontageInstance* newMontageInstance = InteractingCharacterAnimInstance->MontageInstances.Last();
	if (newMontage && newMontageInstance && newMontageInstance->IsActive() && newMontageInstance->IsPlaying())
	{
		if (newMontage->GetOuter() == GetTransientPackage())
		{
			if (const FAnimTrack* animTrack = newMontage->GetAnimationData(FName("DefaultSlot")))
			{
				newMontageInstance->OnMontageEnded.BindLambda([=](UAnimMontage* montage, bool interrupted) {
					InteractingCharacterMovement->StopApplyRootMotionSource();
					InteractingCharacterMovement->Velocity = FVector::ZeroVector;
					InteractingCharacterAnimInstance->UpdateMotionDetailStatus();
					InteractingCharacterAnimInstance->ShouldUpdateMotionDetailStatus = false;
					CharacterDistanceAlongSpline = InteractingClimbSegment->EntryDistanceAlongSpline;
				});
			}
		}
	}

	// get diff between transform after applying animation and target transform
	FTransform correctionTransform = MeshComponentToHandCenterTransform.Inverse()
		* TargetEffectorTransform
		* InteractingCharacter->GetMesh()->GetComponentToWorld().Inverse()
		* IdleToHangAnimTransform.Inverse();

	InteractingCharacterMovement = Cast<UPPCharacterMovementComponent>(InteractingCharacter->GetComponentByClass(UPPCharacterMovementComponent::StaticClass()));
	//UE_LOG(LogTemp, Log, TEXT("TargetRootTransform: %s"), *TargetRootTransform.ToString());
	UE_LOG(LogTemp, Log, TEXT("skeletalComponent: %s"), *skeletalComponent->GetComponentToWorld().ToString());
	UE_LOG(LogTemp, Log, TEXT("correctionTransform: %s"), *correctionTransform.ToString());

	FRootMotionSource_FloatCurve* curveMotiomSource = new FRootMotionSource_FloatCurve;
	curveMotiomSource->Duration = -1.f;
	curveMotiomSource->AccumulateMode = ERootMotionAccumulateMode::Override;
	curveMotiomSource->bInLocalSpace = false;
	curveMotiomSource->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity;
	// curveMotiomSource->InitialTransform = InteractingCharacter->GetActorTransform();
	curveMotiomSource->InitialTransform = InteractingCharacter->GetMesh()->GetComponentTransform();
	curveMotiomSource->TranslationCurves = { GetAnimCurve(IdleToHangAnim, FName("translationX")),
		GetAnimCurve(IdleToHangAnim, FName("translationY")),
		GetAnimCurve(IdleToHangAnim, FName("translationZ")) };
	curveMotiomSource->DeltaCorrectionWindow = FVector2D(0.5f, 0.7f);
	curveMotiomSource->DeltaCorrectionTransform = correctionTransform;
	InteractingCharacterMovement->StartApplyRootMotionSource(curveMotiomSource);

	// disable all input movement and gravity, such that the movement component still accepts
	// root motion but not able to move character freely
	InteractingCharacterMovement->GravityScale = 0.f;
	InteractingCharacterMovement->bOrientRotationToMovement = false;
	InteractingCharacterMovement->MaxAcceleration = 0.f;

	check(IdleToHangAnim);
}

// Called every frame
void AClimbNodeGraph::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!InteractingClimbSegment || !InteractingCharacter)
	{
		return;
	}

	// still playing montage or having root motion, skip
	if (InteractingCharacter->GetCharacterMovement()->HasRootMotionSources())
	{
		return;
	}

	// not in climbing state, check able to climb or not
	// this calculation should be in tick if showing climbing point on screen is needed
	if (InteractingCharacterAnimInstance->CurrentMotionState != FName("Climb State"))
	{
		// for now assuming A B are not align with Z axis
		// conversion from hand center transform then to root component
		FVector bestClimbLocation = InteractingClimbSegment->LedgeClimbingSpline->GetLocationAtDistanceAlongSpline(
			InteractingClimbSegment->EntryDistanceAlongSpline,
			ESplineCoordinateSpace::World);
		FVector rightVec = InteractingClimbSegment->LedgeClimbingSpline->GetTangentAtDistanceAlongSpline(
			InteractingClimbSegment->EntryDistanceAlongSpline,
			ESplineCoordinateSpace::World);
		rightVec *= InteractingClimbSegment->MovingRightIsForward ? 1.f : -1.f;
		TargetEffectorTransform = FTransform(UKismetMathLibrary::MakeRotFromZY(FVector::UpVector, rightVec), bestClimbLocation, FVector(1.f));

#if WITH_EDITOR
		FTransform targetRootTransform = (MeshComponentToHandCenterTransform * InteractingCharacter->GetMesh()->GetRelativeTransform()).Inverse() * TargetEffectorTransform;

		DrawDebugSphere(GetWorld(), bestClimbLocation, 10.f, 10, FColor::White, false);

		DrawDebugSphere(GetWorld(), TargetEffectorTransform.GetLocation(), 10.f, 10, FColor::Red, false);
#endif
		if (CharacterInputActionBindingIdx == -1)
		{
			InteractingCharacter->InputComponent->BindAction("Interact", IE_Pressed, this, &AClimbNodeGraph::OnBeginInteraction);
			CharacterInputActionBindingIdx = InteractingCharacter->InputComponent->GetNumActionBindings() - 1;
		}

		return;
	}

	FVector lastMovementInput = InteractingCharacter->GetLastMovementInputVector();
	// handle climb interaction
	if (!lastMovementInput.IsNearlyZero())
	{
		lastMovementInput.Normalize();
		
		const FVector& characterLoc = InteractingCharacter->GetActorLocation();

		// check if is moving up or horizontally
		if (InteractingClimbSegment->UpConnectedSegment && FVector::DotProduct(lastMovementInput, FVector::UpVector) > 0.9f)
		{
			FVector currentLocationOnSpline = InteractingClimbSegment->LedgeClimbingSpline->GetLocationAtDistanceAlongSpline(CharacterDistanceAlongSpline, ESplineCoordinateSpace::World);
			
			float distanceAlongSplineClosestToActorLoc = GetDistanceAlongSplineFromInputKey(
				InteractingClimbSegment->UpConnectedSegment->LedgeClimbingSpline->FindInputKeyClosestToWorldLocation(currentLocationOnSpline),
				InteractingClimbSegment->UpConnectedSegment->LedgeClimbingSpline
			);
			FVector closestLocationOnUpConnectedSegment = InteractingClimbSegment->
				UpConnectedSegment->
				LedgeClimbingSpline->GetLocationAtDistanceAlongSpline(
					CharacterDistanceAlongSpline, 
					ESplineCoordinateSpace::World);
#if WITH_EDITOR

			DrawDebugSphere(GetWorld(), closestLocationOnUpConnectedSegment, 10.f, 10, FColor::Red, false, 4.f);
#endif
			// check if the potential jump location is "vertical" enough, if not, skip
			if (FVector::DotProduct((closestLocationOnUpConnectedSegment - currentLocationOnSpline), FVector::UpVector) < 0.7f)
			{
				return;
			}
			PerformHop(InteractingClimbSegment->UpConnectedSegment,
				HopUpAnim,
				HopUpAnimTransform,
				FVector2D(0.4f, 0.8f),
				distanceAlongSplineClosestToActorLoc,
				InteractingClimbSegment->UpConnectedSegment->MovingRightIsForward ? 1.f : -1.f);

		}
		else  // move horizontally
		{
			// check if character is at near the end of spline
			FVector currentClimbLedgeTangent = InteractingClimbSegment->LedgeClimbingSpline->GetTangentAtDistanceAlongSpline(
				CharacterDistanceAlongSpline, ESplineCoordinateSpace::World).GetSafeNormal();
#if WITH_EDITOR
			// DrawDebugLine(GetWorld(), InteractingCharacter->GetActorLocation(), InteractingCharacter->GetActorLocation() + lastMovementInput * 100.f, FColor::Red, false, -1.f, 0, 10.f);
			// DrawDebugLine(GetWorld(), InteractingCharacter->GetActorLocation(), InteractingCharacter->GetActorLocation() + currentClimbLedgeTangent * 100.f, FColor::Blue, false, -1.f, 0, 10.f);
#endif
			bool movingRight = FVector::DotProduct(InteractingCharacter->GetActorRightVector(), lastMovementInput) >= 0.f;
			bool movingForward = movingRight == InteractingClimbSegment->MovingRightIsForward;
			float splineDirScalar = InteractingClimbSegment->MovingRightIsForward ? 1.f : -1.f;
			float forwardHandDistanceAloneSpline = FMath::Clamp((CharacterDistanceAlongSpline + (movingRight ? 32.96600f : -32.96600f) * splineDirScalar)
				, 0.f
				, InteractingClimbSegment->LedgeClimbingSpline->GetSplineLength());
			bool hasSpaceForShimmy = FMath::Abs(
					forwardHandDistanceAloneSpline
					- (movingForward ? InteractingClimbSegment->LedgeClimbingSpline->GetSplineLength() : 0.f)) 
				> 15.f;
			// bool hasSpaceForShimmy = FMath::Abs(CharacterDistanceAlongSpline - (movingForward ? InteractingClimbSegment->LedgeClimbingSpline->GetSplineLength() : 0.f)) > 10.f;
			// if can shimmy (far enough to the end of spline and input direction is colinear with spline tangent), do it
			if (hasSpaceForShimmy && FMath::Abs(FVector::DotProduct(currentClimbLedgeTangent, lastMovementInput)) > 0.6f)
			{
				UAnimSequence* animToPlay = movingRight ? ShimmyRightAnim : ShimmyLeftAnim;
				UAnimMontage* newMontage = InteractingCharacterAnimInstance->PlaySlotAnimationAsDynamicMontage(
					animToPlay,
					FName("DefaultSlot"),
					0.25f,
					0.25f);
				FAnimMontageInstance* newMontageInstance = InteractingCharacterAnimInstance->MontageInstances.Last();

				// make sure what is active right now is transient that we created by request
				if (newMontage && newMontageInstance && newMontageInstance->IsActive() && newMontageInstance->IsPlaying())
				{
					if (newMontage->GetOuter() == GetTransientPackage())
					{
						if (const FAnimTrack* animTrack = newMontage->GetAnimationData(FName("DefaultSlot")))
						{
							newMontageInstance->OnMontageEnded.BindLambda([=](UAnimMontage* montage, bool interrupted) {
								InteractingCharacterMovement->Velocity = FVector::ZeroVector;
								InteractingCharacterAnimInstance->UpdateMotionDetailStatus();
								InteractingCharacterAnimInstance->ShouldUpdateMotionDetailStatus = false;
								InteractingCharacterMovement->StopApplyRootMotionSource();
								UE_LOG(LogTemp, Log, TEXT("stop blend"));
							});
						}
					}
				}

				FRootMotionSource_Spline* splineMotiomSource = new FRootMotionSource_Spline;
				splineMotiomSource->Duration = -1.f;
				splineMotiomSource->AccumulateMode = ERootMotionAccumulateMode::Override;
				splineMotiomSource->bInLocalSpace = false;
				splineMotiomSource->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity;
				splineMotiomSource->RootFollowingSpline = InteractingClimbSegment->LedgeClimbingSpline;
				splineMotiomSource->EffectorToRootTransform = (MeshComponentToHandCenterTransform * InteractingCharacter->GetMesh()->GetRelativeTransform()).Inverse();
				splineMotiomSource->InteractingNodeGraph = this;
				splineMotiomSource->InitialDistanceAlongSpline = CharacterDistanceAlongSpline;
				TArray<FFloatCurve*, TFixedAllocator<3>> translationCurves = { GetAnimCurve(animToPlay, FName("translationX")),
					GetAnimCurve(animToPlay, FName("translationY")),
					GetAnimCurve(animToPlay, FName("translationZ")) };
				splineMotiomSource->TranslationCurves = translationCurves;
				splineMotiomSource->MovingRightIsForward = InteractingClimbSegment->MovingRightIsForward;
				InteractingCharacterMovement->StartApplyRootMotionSource(splineMotiomSource);

				// calculate hand effector location
				FVector finalTranslation(
					translationCurves[0]->FloatCurve.GetLastKey().Value,
					translationCurves[1]->FloatCurve.GetLastKey().Value,
					translationCurves[2]->FloatCurve.GetLastKey().Value
				);
				// UE_LOG(LogTemp, Log, TEXT("anim: %s"), *curveTranslation.ToString());
				finalTranslation = InteractingCharacter->GetMesh()->GetRelativeTransform().TransformVector(finalTranslation);
				float lastDistanceAlongSpline = FMath::Clamp(CharacterDistanceAlongSpline + finalTranslation.Y * splineDirScalar, 0.f, InteractingClimbSegment->LedgeClimbingSpline->GetSplineLength());
				float leftHandLedgeDistanceAlongSpline = lastDistanceAlongSpline - 31.403269f * splineDirScalar;
				FTransform leftHandLedgeTransform(
					UKismetMathLibrary::MakeRotFromZY(FVector::UpVector, InteractingClimbSegment->LedgeClimbingSpline->GetTangentAtDistanceAlongSpline(leftHandLedgeDistanceAlongSpline, ESplineCoordinateSpace::World) * splineDirScalar),
					InteractingClimbSegment->LedgeClimbingSpline->GetLocationAtDistanceAlongSpline(leftHandLedgeDistanceAlongSpline, ESplineCoordinateSpace::World),
					FVector(1.f));
				InteractingCharacterAnimInstance->NextLeftHandEffectorLocation = (LedgeToLeftHandTransform * leftHandLedgeTransform).GetLocation();

				float rightHandLedgeDistanceAlongSpline = lastDistanceAlongSpline + 32.96600f * splineDirScalar;
				FTransform rightHandLedgeTransform(
					UKismetMathLibrary::MakeRotFromZY(FVector::UpVector, InteractingClimbSegment->LedgeClimbingSpline->GetTangentAtDistanceAlongSpline(rightHandLedgeDistanceAlongSpline, ESplineCoordinateSpace::World) * splineDirScalar),
					InteractingClimbSegment->LedgeClimbingSpline->GetLocationAtDistanceAlongSpline(rightHandLedgeDistanceAlongSpline, ESplineCoordinateSpace::World),
					FVector(1.f));
				InteractingCharacterAnimInstance->NextRightHandEffectorLocation = (LedgeToRightHandTransform * rightHandLedgeTransform).GetLocation();

			}
			else // else check if connecting spline to do hop
			{
				// cannot find connected segment
				if (hasSpaceForShimmy
					|| (movingForward && InteractingClimbSegment->FrontConnectedSegment == nullptr)
					|| (!movingForward && InteractingClimbSegment->RearConnectedSegment == nullptr))
				{
					return;
				}
				// perform hopping
				AClimbSegment* segmenHopTo = movingForward ?
					InteractingClimbSegment->FrontConnectedSegment :
					InteractingClimbSegment->RearConnectedSegment;
				UAnimSequence* animToPlay = movingRight ? HopRightAnim : HopLeftAnim;
				const FTransform& animTransform = movingRight ? HopRightAnimTransform : HopLeftAnimTransform;
				splineDirScalar = segmenHopTo->MovingRightIsForward ? 1.f : -1.f;
				/*float splineDestination =  (movingForward ? InteractingClimbSegment->FrontConnectedToSegmentStart : InteractingClimbSegment->RearConnectedToSegmentStart)
					? 0.f : segmenHopTo->LedgeClimbingSpline->GetSplineLength();*/
				float splineDestination = (movingForward ? InteractingClimbSegment->FrontConnectedToSegmentStart : InteractingClimbSegment->RearConnectedToSegmentStart)
					? 31.403269f : segmenHopTo->LedgeClimbingSpline->GetSplineLength() - 32.96600f;

				PerformHop(segmenHopTo,
					animToPlay, 
					animTransform,
					FVector2D(0.3f, 0.7f), 
					splineDestination, 
					splineDirScalar);
			}
		}
		
	}

}

void AClimbNodeGraph::OnBeginOverlapCollider(
	UPrimitiveComponent* overlappedComp, 
	AActor* other, 
	UPrimitiveComponent* otherComp, 
	int32 otherBodyIndex, 
	bool bFromSweep, 
	const FHitResult& sweepResult)
{
	if (!other->IsA<APPCharacter>())
	{
		return;
	}
	InteractingClimbSegment = Cast<AClimbSegment>(overlappedComp->GetOwner());
	if (!InteractingClimbSegment)
	{
		return;
	}
	InteractingCharacter = Cast<APPCharacter>(other);
	InteractingCharacterAnimInstance = Cast<UThirdPersonAnimInstance>(InteractingCharacter->GetMesh()->GetAnimInstance());
	
}

void AClimbNodeGraph::OnEndOverlapCollider(
	UPrimitiveComponent* overlappedComp, 
	AActor* other, 
	UPrimitiveComponent* otherComp, 
	int32 otherBodyIndex)
{
	InteractingCharacter->InputComponent->RemoveActionBinding(CharacterInputActionBindingIdx);
	CharacterInputActionBindingIdx = -1;
}

void AClimbNodeGraph::PerformHop(AClimbSegment* segmenHopTo,
	UAnimSequence* animToPlay,
	const FTransform& animTransform,
	const FVector2D& deltaCorrectionWindow,
	float splineDestination,
	float splineDirScalar)
{
	float leftHandLedgeDistanceAlongSpline = splineDestination - 31.403269 * splineDirScalar;
	FTransform leftHandLedgeTransform(
		UKismetMathLibrary::MakeRotFromZY(FVector::UpVector, segmenHopTo->LedgeClimbingSpline->GetTangentAtDistanceAlongSpline(leftHandLedgeDistanceAlongSpline, ESplineCoordinateSpace::World) * splineDirScalar),
		segmenHopTo->LedgeClimbingSpline->GetLocationAtDistanceAlongSpline(leftHandLedgeDistanceAlongSpline, ESplineCoordinateSpace::World),
		FVector(1.f));
	InteractingCharacterAnimInstance->NextLeftHandEffectorLocation = (LedgeToLeftHandTransform * leftHandLedgeTransform).GetLocation();

	float rightHandLedgeDistanceAlongSpline = splineDestination + 32.96600 * splineDirScalar;
	FTransform rightHandLedgeTransform(
		UKismetMathLibrary::MakeRotFromZY(FVector::UpVector, segmenHopTo->LedgeClimbingSpline->GetTangentAtDistanceAlongSpline(rightHandLedgeDistanceAlongSpline, ESplineCoordinateSpace::World) * splineDirScalar),
		segmenHopTo->LedgeClimbingSpline->GetLocationAtDistanceAlongSpline(rightHandLedgeDistanceAlongSpline, ESplineCoordinateSpace::World),
		FVector(1.f));
	InteractingCharacterAnimInstance->NextRightHandEffectorLocation = (LedgeToRightHandTransform * rightHandLedgeTransform).GetLocation();
	UAnimMontage* newMontage = InteractingCharacterAnimInstance->PlaySlotAnimationAsDynamicMontage(
		animToPlay,
		FName("DefaultSlot"),
		0.25f,
		0.25f);
	FAnimMontageInstance* newMontageInstance = InteractingCharacterAnimInstance->MontageInstances.Last();
	if (newMontage && newMontageInstance && newMontageInstance->IsActive() && newMontageInstance->IsPlaying())
	{
		if (newMontage->GetOuter() == GetTransientPackage())
		{
			if (const FAnimTrack* animTrack = newMontage->GetAnimationData(FName("DefaultSlot")))
			{
				newMontageInstance->OnMontageEnded.BindLambda([=](UAnimMontage* montage, bool interrupted) {
					InteractingCharacterMovement->StopApplyRootMotionSource();
					InteractingCharacterMovement->Velocity = FVector::ZeroVector;
					InteractingCharacterAnimInstance->UpdateMotionDetailStatus();
					InteractingCharacterAnimInstance->ShouldUpdateMotionDetailStatus = false;
					CharacterDistanceAlongSpline = splineDestination;
					InteractingClimbSegment = segmenHopTo;
				});
			}
		}
	}

	FTransform targetHandCenterTransform(
		UKismetMathLibrary::MakeRotFromZY(FVector::UpVector, segmenHopTo->LedgeClimbingSpline->GetTangentAtDistanceAlongSpline(splineDestination, ESplineCoordinateSpace::World) * splineDirScalar),
		segmenHopTo->LedgeClimbingSpline->GetLocationAtDistanceAlongSpline(splineDestination, ESplineCoordinateSpace::World),
		FVector(1.f));
	// get animation last frame diff transform from first frame (hard coded for now)
	// get diff between transform after applying animation and target transform

	FTransform correctionTransform = MeshComponentToHandCenterTransform.Inverse()
		* targetHandCenterTransform
		* InteractingCharacter->GetMesh()->GetComponentToWorld().Inverse()
		* animTransform.Inverse();

	InteractingCharacterMovement = Cast<UPPCharacterMovementComponent>(InteractingCharacter->GetComponentByClass(UPPCharacterMovementComponent::StaticClass()));
	UE_LOG(LogTemp, Log, TEXT("TargetRootTransform: %s"), *targetHandCenterTransform.ToString());
	// UE_LOG(LogTemp, Log, TEXT("skeletalComponent: %s"), *skeletalComponent->GetComponentToWorld().ToString());
	UE_LOG(LogTemp, Log, TEXT("correctionTransform: %s"), *correctionTransform.ToString());

	FRootMotionSource_FloatCurve* curveMotiomSource = new FRootMotionSource_FloatCurve;
	curveMotiomSource->Duration = -1.f;
	curveMotiomSource->AccumulateMode = ERootMotionAccumulateMode::Override;
	curveMotiomSource->bInLocalSpace = false;
	curveMotiomSource->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity;
	curveMotiomSource->InitialTransform = InteractingCharacter->GetMesh()->GetComponentTransform();
	curveMotiomSource->TranslationCurves = { GetAnimCurve(animToPlay, FName("translationX")),
		GetAnimCurve(animToPlay, FName("translationY")),
		GetAnimCurve(animToPlay, FName("translationZ")) };
	curveMotiomSource->DeltaCorrectionWindow = deltaCorrectionWindow;
	curveMotiomSource->DeltaCorrectionTransform = correctionTransform;
	InteractingCharacterMovement->StartApplyRootMotionSource(curveMotiomSource);
}

FFloatCurve* AClimbNodeGraph::GetAnimCurve(UAnimSequence* anim, const FName& curveName)
{
	const FName containerName = UAnimationBlueprintLibrary::RetrieveContainerNameForCurve(anim, curveName);

	if (containerName != NAME_None)
	{
		// Retrieve smart name for curve
		const FSmartName curveSmartName = UAnimationBlueprintLibrary::RetrieveSmartNameForCurve(anim, curveName, containerName);

		// Retrieve the curve by name
		return  static_cast<FFloatCurve*>(anim->RawCurveData.GetCurveData(curveSmartName.UID, ERawCurveTrackTypes::RCT_Float));
	}
	return nullptr;
}

float AClimbNodeGraph::GetDistanceAlongSplineFromInputKey(float inputKey, USplineComponent* inSplineComponent)
{
	float frac;
	float integer;
	frac = FMath::Modf(inputKey, &integer);
	int index = (int32)integer;

	int maxInputKey = (float)(inSplineComponent->IsClosedLoop() ? inSplineComponent->GetNumberOfSplinePoints() : inSplineComponent->GetNumberOfSplinePoints() - 1);
	check(index <= maxInputKey);
	if (index == maxInputKey)
	{
		check(frac < 0.0001f);
		index = maxInputKey - 1;
		frac = 1.0f;
	}

	return inSplineComponent->GetDistanceAlongSplineAtSplinePoint(index)
		+ inSplineComponent->SplineCurves.GetSegmentLength(index, frac, inSplineComponent->IsClosedLoop(), inSplineComponent->GetComponentTransform().GetScale3D());
}

void FRootMotionSource_Spline::PrepareRootMotion(
	float simulationTime,
	float movementTickTime,
	const ACharacter& character,
	const UCharacterMovementComponent& moveComponent
)
{
	FVector curveTranslation(
		TranslationCurves[0] ? TranslationCurves[0]->Evaluate(CurrentTime + movementTickTime) : 0.f,
		TranslationCurves[1] ? TranslationCurves[1]->Evaluate(CurrentTime + movementTickTime) : 0.f,
		TranslationCurves[2] ? TranslationCurves[2]->Evaluate(CurrentTime + movementTickTime) : 0.f
	);
	curveTranslation = character.GetMesh()->GetRelativeTransform().TransformVector(curveTranslation);
	LastDistanceAlongSpline = InitialDistanceAlongSpline + curveTranslation.Y * (MovingRightIsForward ? 1.f : -1.f);
	LastDistanceAlongSpline = FMath::Clamp(LastDistanceAlongSpline, 0.f, RootFollowingSpline->GetSplineLength());
	// UE_LOG(LogTemp, Log, TEXT("dist %f %f %f"), LastDistanceAlongSpline, InitialDistanceAlongSpline, curveTranslation.Y);

	FTransform targetTransform(
		UKismetMathLibrary::MakeRotFromZY(FVector::UpVector, RootFollowingSpline->GetTangentAtDistanceAlongSpline(LastDistanceAlongSpline, ESplineCoordinateSpace::World) * (MovingRightIsForward ? 1.f : -1.f)).Quaternion(),
		RootFollowingSpline->GetLocationAtDistanceAlongSpline(LastDistanceAlongSpline, ESplineCoordinateSpace::World));

	// UE_LOG(LogTemp, Log, TEXT("target: %s dist %f "), *targetTransform.ToString(), LastDistanceAlongSpline);
	targetTransform = FTransform(FQuat::Identity, FVector(curveTranslation.X, 0.f, curveTranslation.Z)) * targetTransform;
	// UE_LOG(LogTemp, Log, TEXT("target2: %s"), *targetTransform.ToString());
	// add additional transform from delta correction
	targetTransform = EffectorToRootTransform * targetTransform;

	RootMotionParams.Clear();
	if (movementTickTime > SMALL_NUMBER && simulationTime > SMALL_NUMBER)
	{
		FTransform deltaTransform = character.GetActorTransform().Inverse() * targetTransform;

		RootMotionParams.Set(FTransform(deltaTransform.GetRotation(), (deltaTransform.GetTranslation()) / movementTickTime));
#if ROOT_MOTION_DEBUG
#endif
	}
	else
	{
		checkf(Duration > SMALL_NUMBER, TEXT("FRootMotionSource_JumpForce prepared with invalid duration."));
	}

	InteractingNodeGraph->CharacterDistanceAlongSpline = LastDistanceAlongSpline;
	SetTime(GetTime() + simulationTime);
}
