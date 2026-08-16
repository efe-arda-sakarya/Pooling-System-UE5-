// Copyright Efe Arda Sakarya. All Rights Reserved.

using UnrealBuildTool;

public class PoolingSystemDemo : ModuleRules
{
	public PoolingSystemDemo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"PoolingSystem"
			}
		);
	}
}
