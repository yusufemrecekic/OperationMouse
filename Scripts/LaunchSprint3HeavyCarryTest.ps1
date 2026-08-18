Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $repositoryRoot "OperationMouse.uproject"
$mapPath = "/Game/OperationMouse/Tests/Maps/L_Sprint3_HeavyCarryTest"

& (Join-Path $PSScriptRoot "Build.ps1") -Target OperationMouseEditor -Configuration Development

. (Join-Path $PSScriptRoot "Resolve-UnrealEngine.ps1")
$engineRoot = Resolve-UnrealEngineRoot -ProjectFile $projectFile
$editor = Join-Path $engineRoot "Engine\Binaries\Win64\UnrealEditor.exe"

Write-Host "Opening the Sprint 3 Heavy Carry gameplay test map."
Start-Process -FilePath $editor -ArgumentList @($projectFile, $mapPath)
