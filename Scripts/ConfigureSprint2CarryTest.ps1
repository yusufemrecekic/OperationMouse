Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $repositoryRoot "OperationMouse.uproject"
$configurationScript = Join-Path $PSScriptRoot "Editor\configure_sprint2_carry_test.py"

. (Join-Path $PSScriptRoot "Resolve-UnrealEngine.ps1")
$engineRoot = Resolve-UnrealEngineRoot -ProjectFile $projectFile
$editorCmd = Join-Path $engineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

& $editorCmd $projectFile -run=pythonscript "-script=$configurationScript" -unattended -nop4 -nosplash -nullrhi
if ($LASTEXITCODE -ne 0) {
    throw "Sprint 2 test-map configuration failed with exit code $LASTEXITCODE."
}

Write-Host "Sprint 2 carry test map configured."
