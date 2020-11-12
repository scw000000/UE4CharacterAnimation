#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/PrimitiveComponent.h"
#include "ClimbNodeGraphRenderingComponent.generated.h"

class FPrimitiveSceneProxy;
struct FConvexVolume;
struct FEngineShowFlags;

UCLASS(BlueprintType)
class PETPROJECT_API UClimbNodeGraphRenderingComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()
public:
		//~ Begin UPrimitiveComponent Interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;

	/** Should recreate proxy one very update */
	virtual bool ShouldRecreateProxyOnUpdateTransform() const override { return true; }
#if WITH_EDITOR
	virtual bool ComponentIsTouchingSelectionBox(const FBox& InSelBBox, const FEngineShowFlags& ShowFlags, const bool bConsiderOnlyBSP, const bool bMustEncompassEntireComponent) const override;
	virtual bool ComponentIsTouchingSelectionFrustum(const FConvexVolume& InFrustum, const FEngineShowFlags& ShowFlags, const bool bConsiderOnlyBSP, const bool bMustEncompassEntireComponent) const override;
	virtual bool IgnoreBoundsForEditorFocus() const override { return true; }
#endif
	virtual FBoxSphereBounds CalcBounds(const FTransform &LocalToWorld) const override;


};


class PETPROJECT_API FClimbNodeGraphRenderingProxy : public FPrimitiveSceneProxy
{

public:

	FClimbNodeGraphRenderingProxy(const UPrimitiveComponent* InComponent);

public:
	uint32 GetAllocatedSize(void) const;


	SIZE_T GetTypeHash() const override;
	/** Initialization constructor. */
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;
	virtual uint32 GetMemoryFootprint(void) const override;
	/*uint32 GetAllocatedSize(void) const;
	void StorePointLinks(const FTransform& LocalToWorld, const TArray<FNavigationLink>& LinksArray);
	void StoreSegmentLinks(const FTransform& LocalToWorld, const TArray<FNavigationSegmentLink>& LinksArray);

	static void GetLinkMeshes(const TArray<FNavLinkDrawing>& OffMeshPointLinks, const TArray<FNavLinkSegmentDrawing>& OffMeshSegmentLinks, TArray<float>& StepHeights, FMaterialRenderProxy* const MeshColorInstance, int32 ViewIndex, FMeshElementCollector& Collector, uint32 AgentMask);
*/
	/** made static to allow consistent navlinks drawing even if something is drawing links without FNavLinkRenderingProxy */
	// static void DrawLinks(FPrimitiveDrawInterface* PDI, TArray<FNavLinkDrawing>& OffMeshPointLinks, TArray<FNavLinkSegmentDrawing>& OffMeshSegmentLinks, TArray<float>& StepHeights, FMaterialRenderProxy* const MeshColorInstance, uint32 AgentMask);
	TArray<FTransform> GraphNodes;
	TArray<TPair<int32, int32>> GraphEdges;

private:
	class AClimbNodeGraph* OwnerNodeGraph;
};