// Fill out your copyright notice in the Description page of Project Settings.


#include "FootAnimationModifier.h"
#include "Animation/AnimSequence.h"
#include "Animation/PoseAsset.h"
#include "AnimationBlueprintLibrary.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/KismetMathLibrary.h"
#include <functional>


void UFloatCurveGenerator::TryInitCurve(UAnimSequence* animSequence)
{
	if (!Enabled || CurveName.IsNone())
	{
		return;
	}

	if (UAnimationBlueprintLibrary::DoesCurveExist(animSequence, CurveName, ERawCurveTrackTypes::RCT_Float))
	{
		if (ResetCurveBeforeGeneration)
		{
			UAnimationBlueprintLibrary::RemoveCurve(animSequence, CurveName);
			UAnimationBlueprintLibrary::AddCurve(animSequence, CurveName, ERawCurveTrackTypes::RCT_Float);
		}
	}
	else 
	{
		UAnimationBlueprintLibrary::AddCurve(animSequence, CurveName, ERawCurveTrackTypes::RCT_Float);
	}
}

void UVelocityCurveGenerator::CreateKey(UAnimSequence* outputAnimSequence,
	UAnimSequence* referenceAnimSequence,
	int frame,
	const TArray<FTransform>& initialBoneTransforms,
	const TArray<FTransform>& previousBoneTransforms,
	const TArray<FTransform>& currentBoneTransforms)
{
	if (!Enabled) 
	{
		return;
	}
	static FVector lastVelocity = FVector::ZeroVector;
	float speed = frame > 0 ? FVector::DistXY(currentBoneTransforms[0].GetLocation(), previousBoneTransforms[0].GetLocation()) * referenceAnimSequence->GetFrameRate() : 0.f;
	// UAnimationBlueprintLibrary::AddFloatCurveKey(outputAnimSequence, CurveName, float(frame) / outputAnimSequence->GetFrameRate(), speed);

	FVector velocity = (currentBoneTransforms[0].GetLocation() - previousBoneTransforms[0].GetLocation() ) * referenceAnimSequence->GetFrameRate();
	FVector accel = frame > 1 ? (velocity - lastVelocity) * referenceAnimSequence->GetFrameRate() : FVector::ZeroVector;
	lastVelocity = velocity;
	UE_LOG(LogTemp, Log, TEXT("%d vecl %s accel %s "), frame, *velocity.ToString(), *accel.ToString());


	FVector leftHandLocation = currentBoneTransforms[1].GetLocation();

	FVector rightHandLocation = currentBoneTransforms[2].GetLocation();

	FVector vLR = (rightHandLocation - leftHandLocation).GetSafeNormal();
	// project root to left-right hand center
	FVector handCenterLocation = leftHandLocation + FVector::DotProduct(currentBoneTransforms[0].GetLocation() - leftHandLocation, vLR) * vLR;
	// hand center transform in WS
	FTransform handCenterTransform(UKismetMathLibrary::MakeRotFromZY(FVector::UpVector, vLR), handCenterLocation, FVector(1.f));
	UE_LOG(LogTemp, Log, TEXT("%d LH %s"), frame, *(currentBoneTransforms[1] * handCenterTransform.Inverse()).ToString());
	UE_LOG(LogTemp, Log, TEXT("%d RH %s"), frame, *(currentBoneTransforms[2] * handCenterTransform.Inverse()).ToString());
	UE_LOG(LogTemp, Log, TEXT("%d center transform %s"), frame, *(handCenterTransform).ToString());
	UE_LOG(LogTemp, Log, TEXT("%d anim transform %s"), frame, *(currentBoneTransforms[0] * initialBoneTransforms[0].Inverse()).ToString());
	// UE_LOG(LogTemp, Log, TEXT("%d root relative transform %s"), frame,  *(handCenterTransform * currentBoneTransforms[0].Inverse()).ToString());
}

UVectorCurveGenerator::UVectorCurveGenerator() 
{
	ChannelEnabled.AddDefaulted(3);

	CurveChannelNames.AddDefaulted(3);
}

void UVectorCurveGenerator::TryInitCurve(UAnimSequence* animSequence)
{
	if (!Enabled)
	{
		return;
	}

	for (int i = 0; i < 3; ++i) 
	{
		if (!ChannelEnabled[i] || CurveChannelNames[i].IsNone())
		{
			continue;
		}

		if (UAnimationBlueprintLibrary::DoesCurveExist(animSequence, CurveChannelNames[i], ERawCurveTrackTypes::RCT_Float))
		{
			if (ResetCurveBeforeGeneration)
			{
				UAnimationBlueprintLibrary::RemoveCurve(animSequence, CurveChannelNames[i]);
				UAnimationBlueprintLibrary::AddCurve(animSequence, CurveChannelNames[i], ERawCurveTrackTypes::RCT_Float);
			}
		}
		else
		{
			UAnimationBlueprintLibrary::AddCurve(animSequence, CurveChannelNames[i], ERawCurveTrackTypes::RCT_Float);
		}
	}

}

void UTranslationCurveGenerator::CreateKey(UAnimSequence* outputAnimSequence,
	UAnimSequence* referenceAnimSequence,
	int frame,
	const TArray<FTransform>& initialBoneTransforms,
	const TArray<FTransform>& previousBoneTransforms,
	const TArray<FTransform>& currentBoneTransforms)
{
	if (!Enabled)
	{
		return;
	}
	const FVector& translation = (currentBoneTransforms[0] * initialBoneTransforms[0].Inverse()).GetTranslation();
	for (int i = 0; i < 3; ++i)
	{
		if (!ChannelEnabled[i])
		{
			continue;
		}

		UAnimationBlueprintLibrary::AddFloatCurveKey(outputAnimSequence, CurveChannelNames[i], float(frame) / outputAnimSequence->GetFrameRate(), translation[i]);
	}
}

void URotationCurveGenerator::CreateKey(UAnimSequence* outputAnimSequence,
	UAnimSequence* referenceAnimSequence,
	int frame,
	const TArray<FTransform>& initialBoneTransforms,
	const TArray<FTransform>& previousBoneTransforms,
	const TArray<FTransform>& currentBoneTransforms)
{
	if (!Enabled)
	{
		return;
	}
	const FRotator& rotation = (currentBoneTransforms[0] * initialBoneTransforms[0].Inverse()).Rotator();
	const FVector output(rotation.Pitch, rotation.Yaw, rotation.Roll);
	for (int i = 0; i < 3; ++i)
	{
		if (!ChannelEnabled[i])
		{
			continue;
		}

		UAnimationBlueprintLibrary::AddFloatCurveKey(outputAnimSequence, CurveChannelNames[i], float(frame) / outputAnimSequence->GetFrameRate(), output[i]);
	}
}

#define ADD_BUTTON_TO_CATEGORY(DetailLayout, CategoryName, NewRowFilterString, TextLeftToButton, ButtonText, ObjectPtr, FunctionPtr) \
	{ \
		DetailLayout.EditCategory((CategoryName)) \
			.AddCustomRow((NewRowFilterString)) \
			.NameContent() \
			[ \
				SNew(STextBlock) \
				.Font(IDetailLayoutBuilder::GetDetailFont()) \
				.Text((TextLeftToButton)) \
			] \
			.ValueContent() \
			.MaxDesiredWidth(125.f) \
			.MinDesiredWidth(125.f) \
			[ \
				SNew(SButton) \
				.ContentPadding(2) \
				.VAlign(VAlign_Center) \
				.HAlign(HAlign_Center) \
				.OnClicked((ObjectPtr), (FunctionPtr)) \
				[ \
					SNew(STextBlock) \
					.Font(IDetailLayoutBuilder::GetDetailFont()) \
					.Text((ButtonText)) \
				] \
			]; \
	}

TSharedRef<IDetailCustomization> FFootAnimationModifierDetails::MakeInstance()
{
	return MakeShareable(new FFootAnimationModifierDetails());
}

void FFootAnimationModifierDetails::CustomizeDetails(IDetailLayoutBuilder& detailBuilder)
{
	/*MyLayout = &detailBuilder;*/

	TArray< TWeakObjectPtr<UObject> > selectedObjects;
	detailBuilder.GetObjectsBeingCustomized(selectedObjects);
	// don't want to hanle mutiple selections, it's not necessarry for this data
	if (selectedObjects.Num() != 1 || !selectedObjects[0].IsValid())
	{
		return;
	}
	this->ReferencingObject = Cast<UFootAnimationModifier >(selectedObjects[0]);
	UFootAnimationModifier* animModifier = Cast<UFootAnimationModifier>(ReferencingObject.Get());

	ADD_BUTTON_TO_CATEGORY(detailBuilder,
		"Output",
		FText::FromString("Filter"),
		FText::FromString("Output"),
		FText::FromString("Start Extraction"),
		this,
		&FFootAnimationModifierDetails::GenerateMarkers);

	ADD_BUTTON_TO_CATEGORY(detailBuilder,
		"Curve Generation",
		FText::FromString("Filter"),
		FText::FromString("Curve Generator"),
		FText::FromString("Create Curve Generator"),
		this,
		&FFootAnimationModifierDetails::GenerateCurveGenerator);
}


FReply FFootAnimationModifierDetails::GenerateMarkers()
{
	UFootAnimationModifier* animModifier = Cast<UFootAnimationModifier>(ReferencingObject.Get());
	if (!animModifier->ReferenceAnimSequence)
	{
		return FReply::Handled();
	}
	bool resetOutpout = false;
	if (!animModifier->OutputAnimSequence)
	{
		animModifier->OutputAnimSequence = animModifier->ReferenceAnimSequence;
		resetOutpout = true;
	}

	UAnimSequence* refAnimSequence = ReferencingObject->ReferenceAnimSequence;
	const USkeleton* skeleton = refAnimSequence->GetSkeleton();
	const FReferenceSkeleton& refSkeleton = skeleton->GetReferenceSkeleton();

	const TArray<FMeshBoneInfo>& meshBoneInfo = refSkeleton.GetRefBoneInfo();
	TArray<FName> boneNames;
	boneNames.Reserve(meshBoneInfo.Num());
	for (const auto& boneInfo : meshBoneInfo)
	{
		boneNames.Add(boneInfo.Name);
	}

	float lowestZ = FLT_MAX;
	int32 lowestZFrame = -1;

	// marker group, socket group, transform each frame
	TArray<TArray<TArray<FTransform>>> markerGroupTransforms;
	markerGroupTransforms.AddDefaulted(animModifier->MarkerGroups.Num());
	for (int markerGroupIdx = 0; markerGroupIdx < animModifier->MarkerGroups.Num(); ++markerGroupIdx)
	{
		markerGroupTransforms[markerGroupIdx].AddDefaulted(animModifier->MarkerGroups[markerGroupIdx].SocketNames.Num());
	}

	for (UCurveGenerator* curveGenerator : animModifier->CurveGenerators)
	{
		curveGenerator->TryInitCurve(animModifier->OutputAnimSequence);
	}

	TArray<FTransform> initialFrameposes;
	TMap<FName, FTransform> initialFrameBoneTransformsInWorldSpace;

	TArray<FTransform> previousFrameposes;
	TMap<FName, FTransform> previousFrameBoneTransformsInWorldSpace;

	TArray<FTransform> currentFrameposes;
	TMap<FName, FTransform> currentFrameBoneTransformsInWorldSpace;
	/*float velocitySum = 0.f;*/
	float chestLeanSum = 0.f;
	// first round determine lowest point
	for (int frame = 0; frame < refAnimSequence->GetNumberOfFrames(); ++frame)
	{
		currentFrameBoneTransformsInWorldSpace.Reset();
		UAnimationBlueprintLibrary::GetBonePosesForFrame(refAnimSequence, boneNames, frame, false, currentFrameposes);
		
		// const auto& ddd = getBoneTransformInWorldSpace(FName("foot_r"));

		// calculate socket transform in every frame, and cache it
		for(int markerGroupIdx = 0; markerGroupIdx < animModifier->MarkerGroups.Num(); ++markerGroupIdx)
		{
			for (int socketGroupIdx = 0; socketGroupIdx < animModifier->MarkerGroups[markerGroupIdx].SocketNames.Num(); ++socketGroupIdx)
			{
				USkeletalMeshSocket* socket = skeleton->FindSocket(animModifier->MarkerGroups[markerGroupIdx].SocketNames[socketGroupIdx]);
				check(socket);
				const FTransform& socketTransform = socket->GetSocketLocalTransform() * 
					GetBoneTransformInWorldSpace(
						refSkeleton,
						currentFrameposes,
						currentFrameBoneTransformsInWorldSpace,
						socket->BoneName);
				markerGroupTransforms[markerGroupIdx][socketGroupIdx].Add(socketTransform);
				if (socketTransform.GetLocation().Z <= lowestZ)
				{
					lowestZ = socketTransform.GetLocation().Z;
					lowestZFrame = frame;
				}
			}
		}

		const FTransform& currentRootTransform = GetBoneTransformInWorldSpace(
			refSkeleton,
			currentFrameposes,
			currentFrameBoneTransformsInWorldSpace,
			animModifier->RootBoneName);


		const FTransform& currentChestTransform = GetBoneTransformInWorldSpace(
			refSkeleton,
			currentFrameposes,
			currentFrameBoneTransformsInWorldSpace,
			animModifier->ChestBoneName);

		FVector rootToChest = (currentChestTransform.GetLocation() - currentRootTransform.GetLocation()).GetSafeNormal();
		check(rootToChest != FVector::ZeroVector);
		chestLeanSum += FMath::Acos(rootToChest.Z);
		if (frame == 0)
		{
			initialFrameposes = currentFrameposes;
			initialFrameBoneTransformsInWorldSpace = currentFrameBoneTransformsInWorldSpace;

			previousFrameposes = currentFrameposes;
			previousFrameBoneTransformsInWorldSpace = currentFrameBoneTransformsInWorldSpace;
		}

		for (UCurveGenerator* curveGenerator : animModifier->CurveGenerators)
		{
			curveGenerator->CreateKey(animModifier->OutputAnimSequence,
				animModifier->ReferenceAnimSequence,
				frame, 
				GetBoneTransformsInWorldSpace(
					refSkeleton, 
					initialFrameposes,
					initialFrameBoneTransformsInWorldSpace,
					curveGenerator->TrackingBoneNames),
				GetBoneTransformsInWorldSpace(
					refSkeleton,
					previousFrameposes,
					previousFrameBoneTransformsInWorldSpace,
					curveGenerator->TrackingBoneNames),
				GetBoneTransformsInWorldSpace(
					refSkeleton,
					currentFrameposes,
					currentFrameBoneTransformsInWorldSpace,
					curveGenerator->TrackingBoneNames)
				);
		}

		previousFrameposes = currentFrameposes;
		previousFrameBoneTransformsInWorldSpace = currentFrameBoneTransformsInWorldSpace;
	}

	// last frame
	const FTransform& currentRootTransform = GetBoneTransformInWorldSpace(
		refSkeleton,
		currentFrameposes,
		currentFrameBoneTransformsInWorldSpace,
		animModifier->RootBoneName);

	const FTransform& initialRootTransform = GetBoneTransformInWorldSpace(
		refSkeleton,
		initialFrameposes,
		initialFrameBoneTransformsInWorldSpace,
		animModifier->RootBoneName);
	float averageVelocity = FVector::DistXY(currentRootTransform.GetLocation(), initialRootTransform.GetLocation()) / refAnimSequence->GetPlayLength();


	UE_LOG(LogTemp, Log, TEXT("Name %s Average Velocity: %f"), *refAnimSequence->GetFullName(), averageVelocity);

	UE_LOG(LogTemp, Log, TEXT("Name %s Average Chest Lean: %f in rad %f in degree"),
		*refAnimSequence->GetFullName(),
		chestLeanSum / float(refAnimSequence->GetNumberOfFrames()),
		FMath::RadiansToDegrees(chestLeanSum / float(refAnimSequence->GetNumberOfFrames()))
	);

	// test code for hanging animation
	USkeletalMeshSocket* leftHandSocket = skeleton->FindSocket(FName("hand_l_grab_socket"));
	check(leftHandSocket);
	FTransform leftHandTransfom = GetBoneTransformInWorldSpace(
		refSkeleton,
		currentFrameposes,
		currentFrameBoneTransformsInWorldSpace,
		leftHandSocket->BoneName);
	const FTransform& leftHandSocketTransform = leftHandSocket->GetSocketLocalTransform() * leftHandTransfom;

	FVector leftHandLocation = leftHandSocketTransform.GetLocation();


	USkeletalMeshSocket* rightHandSocket = skeleton->FindSocket(FName("hand_r_grab_socket"));
	check(rightHandSocket);
	FTransform righHandTransfom = GetBoneTransformInWorldSpace(
		refSkeleton,
		currentFrameposes,
		currentFrameBoneTransformsInWorldSpace,
		rightHandSocket->BoneName);
	const FTransform& rightHandSocketTransform = rightHandSocket->GetSocketLocalTransform() * righHandTransfom;
	FVector rightHandLocation = rightHandSocketTransform.GetLocation();

	FVector vLR = (rightHandLocation - leftHandLocation).GetSafeNormal();
	// project root to left-right hand center
	FVector handCenterLocation = leftHandLocation + FVector::DotProduct(currentRootTransform.GetLocation() - leftHandLocation, vLR) * vLR;
	// hand center transform in WS
	FTransform handCenterTransform(UKismetMathLibrary::MakeRotFromZY(FVector::UpVector, vLR), handCenterLocation, FVector(1.f));
	UE_LOG(LogTemp, Log, TEXT("%d hand center transform %s"), 0, *(handCenterTransform).ToString());
	UE_LOG(LogTemp, Log, TEXT("%d anim transform %s"), 0, *(initialRootTransform.Inverse() * currentRootTransform).ToString());

	FTransform leftHandEffectorTransform(UKismetMathLibrary::MakeRotFromZY(FVector::UpVector, vLR), leftHandLocation, FVector(1.f));
	UE_LOG(LogTemp, Log, TEXT("transform from ledge to hand bone %s"), *(leftHandEffectorTransform * leftHandTransfom.Inverse()).Inverse().ToString());
	
	FTransform rightHandEffectorTransform(UKismetMathLibrary::MakeRotFromZY(FVector::UpVector, vLR), rightHandLocation, FVector(1.f));
	UE_LOG(LogTemp, Log, TEXT("transform from ledge to hand bone %s"), *(rightHandEffectorTransform * righHandTransfom.Inverse()).Inverse().ToString());

	// end of test code for hanging animation


	TArray<bool> MarkerAvailable;
	MarkerAvailable.AddDefaulted(animModifier->MarkerGroups.Num());
	for (bool& availability : MarkerAvailable)
	{
		availability = true;
	}
	for (int frame = 0; frame < refAnimSequence->GetRawNumberOfFrames(); ++frame)
	{
		for (int markerGroupIdx = 0; markerGroupIdx < animModifier->MarkerGroups.Num(); ++markerGroupIdx)
		{
			bool isOnGround = false;
			for (int socketGroupIdx = 0; 
				socketGroupIdx < animModifier->MarkerGroups[markerGroupIdx].SocketNames.Num() && !isOnGround;
				++socketGroupIdx)
			{
				isOnGround |= markerGroupTransforms[markerGroupIdx][socketGroupIdx][frame].GetLocation().Z
					<= lowestZ + animModifier->ZThreshold ;
			}
			if (isOnGround && MarkerAvailable[markerGroupIdx] && animModifier->EnableMarkerGeneration)
			{
				if (UAnimationBlueprintLibrary::GetTrackIndexForAnimationNotifyTrackName(
					animModifier->OutputAnimSequence,
					animModifier->MarkerGroups[markerGroupIdx].NotifyTrackName) == INDEX_NONE)
				{
					UAnimationBlueprintLibrary::AddAnimationNotifyTrack(
						animModifier->OutputAnimSequence,
						animModifier->MarkerGroups[markerGroupIdx].NotifyTrackName
					);
				}

				UAnimationBlueprintLibrary::AddAnimationSyncMarker(
					animModifier->OutputAnimSequence,
					animModifier->MarkerGroups[markerGroupIdx].MarkerName,
					(float)frame / refAnimSequence->GetFrameRate(),
					animModifier->MarkerGroups[markerGroupIdx].NotifyTrackName
				);
				MarkerAvailable[markerGroupIdx] = false;
			}
			else if (!isOnGround)
			{
				MarkerAvailable[markerGroupIdx] = true;
			}
		}
	}

	bool animCurveGenerated = false;

	for (UCurveGenerator* curveGenerator : animModifier->CurveGenerators)
	{
		if (curveGenerator->Enabled) 
		{
			animCurveGenerated = true;
			break;
		}
	}
	if (animCurveGenerated || animModifier->EnableMarkerGeneration)
	{
		animModifier->OutputAnimSequence->PostProcessSequence();
		animModifier->OutputAnimSequence->PostEditChange();
	}
	if (resetOutpout)
	{
		animModifier->OutputAnimSequence = nullptr;
	}
	return FReply::Handled();
}

FTransform FFootAnimationModifierDetails::GetBoneTransformInWorldSpace(const FReferenceSkeleton& refSkeleton, const TArray<FTransform>& poseTransforms, TMap<FName, FTransform>& cache, const FName& boneName)
{
	if (FTransform* foundTransform = cache.Find(boneName))
	{
		return *foundTransform;
	}
	
	int32 boneIndex = refSkeleton.FindBoneIndex(boneName);
	const FTransform& transformLocalSpace = poseTransforms[boneIndex];
	const int32 parentBoneIndex = refSkeleton.GetParentIndex(boneIndex);
	if (parentBoneIndex == INDEX_NONE)
	{
		// boneTransformsInCompSpace.Add(boneName, FTransform::Identity);
		cache.Add(boneName, transformLocalSpace);
		return FTransform::Identity;
	}
	const TArray<FMeshBoneInfo>& meshBoneInfo = refSkeleton.GetRefBoneInfo();
	const FTransform& ret = transformLocalSpace * GetBoneTransformInWorldSpace(
		refSkeleton,
		poseTransforms,
		cache,
		meshBoneInfo[parentBoneIndex].Name);
	cache.Add(boneName, ret);
	return ret;
}

TArray<FTransform> FFootAnimationModifierDetails::GetBoneTransformsInWorldSpace(const FReferenceSkeleton& refSkeleton, const TArray<FTransform>& poseTransforms, TMap<FName, FTransform>& cache, TArray<FName> boneNames)
{
	TArray<FTransform> ret;
	ret.Reserve(boneNames.Num());
	for (const FName& boneName : boneNames)
	{
		ret.Add(GetBoneTransformInWorldSpace(refSkeleton, poseTransforms, cache, boneName));
	}

	return ret;
}


FReply FFootAnimationModifierDetails::GenerateCurveGenerator()
{
	UFootAnimationModifier* animModifier = Cast<UFootAnimationModifier>(ReferencingObject.Get());
	animModifier->CurveGenerators.Add(NewObject<UCurveGenerator>(animModifier, animModifier->CurveGeneratorClass));
	return FReply::Handled();
}