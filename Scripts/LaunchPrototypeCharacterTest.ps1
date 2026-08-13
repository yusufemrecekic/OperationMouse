Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $repositoryRoot "OperationMouse.uproject"
$mapPath = "/Game/OperationMouse/Characters/Prototype/Maps/L_PrototypeCharacterTest"

& (Join-Path $PSScriptRoot "Build.ps1") -Target OperationMouseEditor -Configuration Development

. (Join-Path $PSScriptRoot "Resolve-UnrealEngine.ps1")
$engineRoot = Resolve-UnrealEngineRoot -ProjectFile $projectFile
$editor = Join-Path $engineRoot "Engine\Binaries\Win64\UnrealEditor.exe"

Write-Host "Opening L_PrototypeCharacterTest with the current branch's freshly built Editor module."
Start-Process -FilePath $editor -ArgumentList @($projectFile, $mapPath)
