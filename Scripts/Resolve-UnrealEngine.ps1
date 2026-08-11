Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-UnrealEngineRoot {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ProjectFile
    )

    if ($env:UE_ENGINE_ROOT -and (Test-Path -LiteralPath $env:UE_ENGINE_ROOT)) {
        return (Resolve-Path -LiteralPath $env:UE_ENGINE_ROOT).Path
    }

    $project = Get-Content -Raw -LiteralPath $ProjectFile | ConvertFrom-Json
    $association = [string]$project.EngineAssociation

    if (-not $association) {
        throw "The .uproject file does not define EngineAssociation. Set UE_ENGINE_ROOT locally."
    }

    $versionKey = $association -replace '^UE_', ''
    $machineKey = "HKLM:\SOFTWARE\EpicGames\Unreal Engine\$versionKey"
    if (Test-Path -LiteralPath $machineKey) {
        $installedDirectory = (Get-ItemProperty -LiteralPath $machineKey).InstalledDirectory
        if ($installedDirectory -and (Test-Path -LiteralPath $installedDirectory)) {
            return (Resolve-Path -LiteralPath $installedDirectory).Path
        }
    }

    $userKey = "HKCU:\Software\Epic Games\Unreal Engine\Builds"
    if (Test-Path -LiteralPath $userKey) {
        $builds = Get-ItemProperty -LiteralPath $userKey
        $customPath = $builds.PSObject.Properties[$association].Value
        if ($customPath -and (Test-Path -LiteralPath $customPath)) {
            return (Resolve-Path -LiteralPath $customPath).Path
        }
    }

    $launcherPath = "C:\Program Files\Epic Games\UE_$versionKey"
    if (Test-Path -LiteralPath $launcherPath) {
        return (Resolve-Path -LiteralPath $launcherPath).Path
    }

    throw "Unreal Engine '$association' was not found. Set the local UE_ENGINE_ROOT environment variable."
}
