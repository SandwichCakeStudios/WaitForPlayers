// Copyright 2026 Sandwich Cake Studios. All Rights Reserved.

using UnrealBuildTool;

public class WaitForPlayersOnlineServices : ModuleRules
{
	public WaitForPlayersOnlineServices(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"WaitForPlayers",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreOnline",
				"Engine",
				"OnlineServicesInterface",
			}
			);
	}
}
