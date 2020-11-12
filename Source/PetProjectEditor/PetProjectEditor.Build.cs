// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class PetProjectEditor : ModuleRules
{
	public PetProjectEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "Slate", "SlateCore", "EditorStyle", "AnimationModifiers" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Blutility", "PetProject" });
		
		PrivateIncludePaths.AddRange(
             new string[] {
				 // Path.GetFullPath(Target.RelativeEnginePath) + "Source/Editor/Blutility/Private" 
                 // "/Editor/Blutility/Private"
                 // ... add other private include paths required here ...
             }
             );
	}
}
