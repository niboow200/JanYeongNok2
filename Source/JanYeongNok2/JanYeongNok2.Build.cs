// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class JanYeongNok2 : ModuleRules
{
	public JanYeongNok2(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"JanYeongNok2",
			// JYN 클래스
			"JanYeongNok2/Character",
			"JanYeongNok2/Components",
			"JanYeongNok2/Gameplay",
			"JanYeongNok2/Abilities",
			"JanYeongNok2/AI",
			"JanYeongNok2/Data",
			"JanYeongNok2/UI",
			"JanYeongNok2/Weapons",
			// 프로토타입 레퍼런스 (레거시)
			"JanYeongNok2/Variant_Strategy",
			"JanYeongNok2/Variant_Strategy/UI",
			"JanYeongNok2/Variant_TwinStick",
			"JanYeongNok2/Variant_TwinStick/AI",
			"JanYeongNok2/Variant_TwinStick/Gameplay",
			"JanYeongNok2/Variant_TwinStick/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
