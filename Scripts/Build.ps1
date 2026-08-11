param(
    [ValidateSet("DebugGame", "Development", "Shipping")]
    [string]$Configuration = "Development",

    [string]$Target = "OperationMouseEditor"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $repositoryRoot "OperationMouse.uproject"
. (Join-Path $PSScriptRoot "Resolve-UnrealEngine.ps1")

$engineRoot = Resolve-UnrealEngineRoot -ProjectFile $projectFile
$buildScript = Join-Path $engineRoot "Engine\Build\BatchFiles\Build.bat"

if (-not (Test-Path -LiteralPath $buildScript)) {
    throw "Unreal build script was not found at '$buildScript'."
}

& $buildScript $Target Win64 $Configuration "-Project=$projectFile" -WaitMutex -FromMsBuild
if ($LASTEXITCODE -ne 0) {
    throw "Unreal build failed with exit code $LASTEXITCODE."
}
