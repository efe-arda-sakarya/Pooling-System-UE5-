// Copyright Efe Arda Sakarya. All Rights Reserved.

using UnrealBuildTool;

public class PoolingSystem : ModuleRules
{
	public PoolingSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine"
			}
		);
	}
}
