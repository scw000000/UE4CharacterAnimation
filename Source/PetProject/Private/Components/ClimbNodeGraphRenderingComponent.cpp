
#include "ClimbNodeGraphRenderingComponent.h"
#include "EngineGlobals.h"
#include "PrimitiveViewRelevance.h"
#include "PrimitiveSceneProxy.h"
#include "Engine/Engine.h"
#include "MaterialShared.h"
#include "Materials/Material.h"
#include "Engine/CollisionProfile.h"
#include "SceneManagement.h"
#include "Actors/ClimbNodeGraph.h"
//----------------------------------------------------------------------//
// UNavLinkRenderingComponent
//----------------------------------------------------------------------//
UClimbNodeGraphRenderingComponent::UClimbNodeGraphRenderingComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// properties

	// Allows updating in game, while optimizing rendering for the case that it is not modified
	Mobility = EComponentMobility::Stationary;

	BodyInstance.SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	bIsEditorOnly = true;

	SetGenerateOverlapEvents(false);
}

FBoxSphereBounds UClimbNodeGraphRenderingComponent::CalcBounds(const FTransform& InLocalToWorld) const
{
	AActor* ownerActor = GetOwner();
	if (ownerActor == nullptr)
	{
		return FBoxSphereBounds(ForceInitToZero);
	}

	AClimbNodeGraph* climbNodeGraph = Cast<AClimbNodeGraph>(GetOwner());
	if (climbNodeGraph == nullptr)
	{
		return FBoxSphereBounds(ForceInitToZero);
	}
	FBox boundingBox(ForceInit);
	const FTransform& localToWorld = ownerActor->ActorToWorld();

	/*for (const auto& climbNode : climbNodeGraph->ClimbNodes)
	{
		boundingBox += climbNode->GetRootComponent()->GetRelativeTransform().GetLocation();
	}*/

	return FBoxSphereBounds(boundingBox).TransformBy(localToWorld);

}

FPrimitiveSceneProxy* UClimbNodeGraphRenderingComponent::CreateSceneProxy()
{
	return new FClimbNodeGraphRenderingProxy(this);
}

#if WITH_EDITOR
bool UClimbNodeGraphRenderingComponent::ComponentIsTouchingSelectionBox(const FBox& InSelBBox, const FEngineShowFlags& ShowFlags, const bool bConsiderOnlyBSP, const bool bMustEncompassEntireComponent) const
{
	return false;
}

bool UClimbNodeGraphRenderingComponent::ComponentIsTouchingSelectionFrustum(const FConvexVolume& InFrustum, const FEngineShowFlags& ShowFlags, const bool bConsiderOnlyBSP, const bool bMustEncompassEntireComponent) const
{
	return false;
}
#endif

FClimbNodeGraphRenderingProxy::FClimbNodeGraphRenderingProxy(const UPrimitiveComponent* inComponent)
	: FPrimitiveSceneProxy(inComponent)
{
	AActor* ownerActor = inComponent->GetOwner();
	if (ownerActor == nullptr)
	{
		return;
	}
	OwnerNodeGraph = Cast<AClimbNodeGraph>(ownerActor);
	if (OwnerNodeGraph == nullptr)
	{
		return;
	}
	////const FTransform& localToWord = OwnerNodeGraph->ActorToWorld();
	//GraphNodes.Reserve(OwnerNodeGraph->ClimbNodes.Num());
	//TMap<AClimbNode*, int32> nodeToIndex;
	//nodeToIndex.Reserve(OwnerNodeGraph->ClimbNodes.Num());
	//for (int i = 0; i < OwnerNodeGraph->ClimbNodes.Num(); ++i)
	//{
	//	const auto& node = OwnerNodeGraph->ClimbNodes[i];
	//	if (node) 
	//	{
	//		nodeToIndex.Add(node, GraphNodes.Num());
	//		GraphNodes.Add(node->GetActorTransform());
	//	}
	//}
	//GraphEdges.Reserve(OwnerNodeGraph->ClimbEdges.Num());
	//for (const auto& edge : OwnerNodeGraph->ClimbEdges)
	//{
	//	if (edge.LeftNode && edge.RightNode)
	//	{
	//		check(nodeToIndex.Contains(edge.LeftNode) && nodeToIndex.Contains(edge.RightNode));
	//		GraphEdges.Add(TPair<int, int>(nodeToIndex[edge.LeftNode], nodeToIndex[edge.RightNode]));
	//	}
	//}
}

SIZE_T FClimbNodeGraphRenderingProxy::GetTypeHash() const
{
	static size_t UniquePointer;
	return reinterpret_cast<size_t>(&UniquePointer);
}

void FClimbNodeGraphRenderingProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& views, const FSceneViewFamily& viewFamily, uint32 visibilityMap, FMeshElementCollector& collector) const
{
	static const FColor RadiusColor(150, 160, 150, 48);
	FMaterialRenderProxy* const MeshColorInstance = new(FMemStack::Get()) FColoredMaterialRenderProxy(GEngine->DebugMeshMaterial->GetRenderProxy(), RadiusColor);

	for (int32 viewIndex = 0; viewIndex < views.Num(); viewIndex++)
	{
		if ((visibilityMap & (1 << viewIndex)) == 0)
		{
			continue;
		}
		//FNavLinkRenderingProxy::GetLinkMeshes(OffMeshPointLinks, OffMeshSegmentLinks, StepHeights, MeshColorInstance, ViewIndex, Collector, AgentMask);
		FPrimitiveDrawInterface* PDI = collector.GetPDI(viewIndex);

		for (int32 edgeIndex = 0; edgeIndex < GraphEdges.Num(); ++edgeIndex)
		{
			const auto& edge = GraphEdges[edgeIndex];
			const FVector& leftPoint = GraphNodes[edge.Key].GetLocation();
			const FVector& rightPoint = GraphNodes[edge.Value].GetLocation();
			PDI->DrawLine(leftPoint, rightPoint, FColor::White, SDPG_World, 1.f);
			DrawArrowHead(PDI, leftPoint, rightPoint, 0.5f, FColor::White, SDPG_World, 3.5f);
			DrawArrowHead(PDI, rightPoint, leftPoint, 0.5f, FColor::White, SDPG_World, 3.5f);
		}
	}
}

FPrimitiveViewRelevance FClimbNodeGraphRenderingProxy::GetViewRelevance(const FSceneView* view) const
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(view) /*&& IsSelected()*/ && (view && view->Family && view->Family->EngineShowFlags.Navigation);
	Result.bDynamicRelevance = true;
	// ideally the TranslucencyRelevance should be filled out by the material, here we do it conservative
	Result.bSeparateTranslucencyRelevance = Result.bNormalTranslucencyRelevance = IsShown(view);
	Result.bShadowRelevance = IsShadowCast(view);
	Result.bEditorPrimitiveRelevance = UseEditorCompositing(view);
	return Result;
}

uint32 FClimbNodeGraphRenderingProxy::GetMemoryFootprint(void) const
{
	return(sizeof(*this) + GetAllocatedSize());
}

uint32 FClimbNodeGraphRenderingProxy::GetAllocatedSize(void) const
{
	return FPrimitiveSceneProxy::GetAllocatedSize() + GraphNodes.GetAllocatedSize() + GraphEdges.GetAllocatedSize();
}
