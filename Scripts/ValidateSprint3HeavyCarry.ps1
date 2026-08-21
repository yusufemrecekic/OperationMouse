param(
    [switch]$Full
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $repositoryRoot "OperationMouse.uproject"
$validationScript = Join-Path $PSScriptRoot "Editor\validate_sprint3_heavy_carry.py"

if ($Full) {
    & (Join-Path $PSScriptRoot "Build.ps1") -Target OperationMouseEditor -Configuration Development
    & (Join-Path $PSScriptRoot "Build.ps1") -Target OperationMouse -Configuration Development
}

. (Join-Path $PSScriptRoot "Resolve-UnrealEngine.ps1")
$engineRoot = Resolve-UnrealEngineRoot -ProjectFile $projectFile
$editorCmd = Join-Path $engineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

& $editorCmd $projectFile -run=pythonscript "-script=$validationScript" -unattended -nop4 -nosplash -nullrhi
if ($LASTEXITCODE -ne 0) {
    throw "Sprint 3 Heavy Carry validation failed with exit code $LASTEXITCODE."
}

Write-Host "Sprint 3 Heavy Carry automated validation PASSED."
