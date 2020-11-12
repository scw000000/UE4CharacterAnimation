// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PetProject : ModuleRules
{
	public PetProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "RHI", "RenderCore" });
		
		if (Target.bBuildEditor)
		{
		//PublicDependencyModuleNames.AddRange(new string[] { "UnrealEd" });
		}
	}
}
