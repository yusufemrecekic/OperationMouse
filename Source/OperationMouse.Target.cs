using UnrealBuildTool;
using System.Collections.Generic;

public class OperationMouseTarget : TargetRules
{
	public OperationMouseTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("OperationMouse");
	}
}
