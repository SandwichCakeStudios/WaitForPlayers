// Copyright 2026 Sandwich Cake Studios. All Rights Reserved.

using UnrealBuildTool;

public class WaitForPlayersOnlineSubsystem : ModuleRules
{
	public WaitForPlayersOnlineSubsystem(ReadOnlyTargetRules Target) : base(Target)
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
				"Engine",
				"OnlineSubsystem",
				"OnlineSubsystemUtils",
			}
			);
	}
}
