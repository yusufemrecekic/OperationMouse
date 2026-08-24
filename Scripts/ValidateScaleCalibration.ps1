Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $repositoryRoot "OperationMouse.uproject"
$validationScript = Join-Path $PSScriptRoot "Editor\validate_scale_calibration.py"
. (Join-Path $PSScriptRoot "Resolve-UnrealEngine.ps1")
$engineRoot = Resolve-UnrealEngineRoot -ProjectFile $projectFile
$editorCmd = Join-Path $engineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

& $editorCmd $projectFile -run=pythonscript "-script=$validationScript" -unattended -nop4 -nosplash -nullrhi
if ($LASTEXITCODE -ne 0) {
    throw "Scale Calibration validation failed with exit code $LASTEXITCODE."
}

Write-Host "Scale Calibration targeted validation PASSED."
