Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $repositoryRoot "OperationMouse.uproject"
. (Join-Path $PSScriptRoot "Resolve-UnrealEngine.ps1")

$engineRoot = Resolve-UnrealEngineRoot -ProjectFile $projectFile
$unrealBuildTool = Join-Path $engineRoot "Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"

if (-not (Test-Path -LiteralPath $unrealBuildTool)) {
    throw "UnrealBuildTool was not found at '$unrealBuildTool'."
}

& $unrealBuildTool -ProjectFiles -Project="$projectFile" -Game -Rocket -Progress -VSCode
if ($LASTEXITCODE -ne 0) {
    throw "Unreal project file generation failed with exit code $LASTEXITCODE."
}
