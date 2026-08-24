Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $repositoryRoot "OperationMouse.uproject"
. (Join-Path $PSScriptRoot "Resolve-UnrealEngine.ps1")
$engineRoot = Resolve-UnrealEngineRoot -ProjectFile $projectFile
$editor = Join-Path $engineRoot "Engine\Binaries\Win64\UnrealEditor.exe"

& (Join-Path $PSScriptRoot "Build.ps1") -Target OperationMouseEditor -Configuration Development
& $editor $projectFile "/Game/OperationMouse/Tests/Scale/L_ScaleCalibration"
