param(
    [ValidateSet("Quick", "Full")]
    [string]$Mode = "Quick"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $repositoryRoot "OperationMouse.uproject"
$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$logDirectory = Join-Path $repositoryRoot "Saved\Validation\$timestamp-$($Mode.ToLowerInvariant())"
$summaryPath = Join-Path $logDirectory "validation-summary.txt"
$results = New-Object System.Collections.Generic.List[object]

New-Item -ItemType Directory -Path $logDirectory -Force | Out-Null

function Add-ValidationResult {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,

        [Parameter(Mandatory = $true)]
        [bool]$Passed,

        [Parameter(Mandatory = $true)]
        [string]$Details
    )

    $results.Add([PSCustomObject]@{
        Name = $Name
        Passed = $Passed
        Details = $Details
    })
}

function Invoke-GitLines {
    param([string[]]$Arguments)

    $output = @(& git -C $repositoryRoot @Arguments 2>&1)
    if ($LASTEXITCODE -ne 0) {
        throw "git $($Arguments -join ' ') failed: $($output -join [Environment]::NewLine)"
    }
    return @($output | ForEach-Object { [string]$_ })
}

function Get-RelativeLogPath {
    param([string]$Path)
    return $Path.Substring($repositoryRoot.Length + 1)
}

function Get-FirstMeaningfulError {
    param([string]$LogPath)

    $match = Select-String -LiteralPath $LogPath -Pattern @(
        "fatal error",
        "error C[0-9]+:",
        "error LNK[0-9]+:",
        "UnrealBuildTool.*Error",
        "BUILD FAILED",
        "Result: Failed"
    ) -CaseSensitive:$false | Select-Object -First 1

    if ($match) {
        return $match.Line.Trim()
    }
    return "See the detailed log for the first meaningful failure."
}

function Test-RepositoryHealth {
    $problems = New-Object System.Collections.Generic.List[string]
    $gitIgnorePath = Join-Path $repositoryRoot ".gitignore"
    $gitAttributesPath = Join-Path $repositoryRoot ".gitattributes"

    if (-not (Test-Path -LiteralPath $gitIgnorePath -PathType Leaf)) {
        $problems.Add(".gitignore is missing")
    }
    else {
        $ignoreLines = @(Get-Content -LiteralPath $gitIgnorePath | ForEach-Object { $_.Trim() })
        foreach ($requiredPattern in @("Binaries/", "DerivedDataCache/", "Intermediate/", "Saved/", ".vs/")) {
            if ($ignoreLines -notcontains $requiredPattern) {
                $problems.Add(".gitignore does not contain $requiredPattern")
            }
        }
    }

    if (-not (Test-Path -LiteralPath $gitAttributesPath -PathType Leaf)) {
        $problems.Add(".gitattributes is missing")
    }

    $generatedRoots = @("Binaries", "Intermediate", "Saved", "DerivedDataCache", ".vs")
    $trackedGenerated = @(Invoke-GitLines -Arguments (@("ls-files", "--") + ($generatedRoots | ForEach-Object { "$_/**" })))
    if ($trackedGenerated.Count -gt 0) {
        $problems.Add("Generated files are tracked: $($trackedGenerated -join ', ')")
    }

    Add-ValidationResult -Name "Repository hygiene" -Passed ($problems.Count -eq 0) -Details $(
        if ($problems.Count -eq 0) {
            ".gitignore/.gitattributes exist and no generated Unreal/IDE folders are tracked."
        }
        else {
            $problems -join "; "
        }
    )

    $workingTree = @(Invoke-GitLines -Arguments @("status", "--porcelain", "--untracked-files=all"))
    Add-ValidationResult -Name "Working tree" -Passed ($workingTree.Count -eq 0) -Details $(
        if ($workingTree.Count -eq 0) { "Clean." } else { "Uncommitted paths: $($workingTree -join ', ')" }
    )
}

function Test-GitLfs {
    $problems = New-Object System.Collections.Generic.List[string]
    try {
        $null = Invoke-GitLines -Arguments @("lfs", "version")
    }
    catch {
        $problems.Add("Git LFS is unavailable: $($_.Exception.Message)")
    }

    $representativePaths = @(
        "Content/Test.uasset",
        "Content/Test.umap",
        "SourceAssets/Test.fbx"
    )
    foreach ($path in $representativePaths) {
        try {
            $attribute = (Invoke-GitLines -Arguments @("check-attr", "filter", "--", $path)) -join " "
            if ($attribute -notmatch "filter:\s+lfs$") {
                $problems.Add("LFS filter is not configured for $path")
            }
        }
        catch {
            $problems.Add($_.Exception.Message)
        }
    }

    try {
        $trackedFiles = @(Invoke-GitLines -Arguments @("ls-files"))
        $trackedBinaryFiles = @($trackedFiles | Where-Object {
            [IO.Path]::GetExtension($_).ToLowerInvariant() -in @(".uasset", ".umap", ".fbx")
        })
        $lfsFiles = @(Invoke-GitLines -Arguments @("lfs", "ls-files", "--name-only") | ForEach-Object {
            $_.Replace("\", "/")
        })
        $missingLfs = @($trackedBinaryFiles | Where-Object {
            $lfsFiles -notcontains $_.Replace("\", "/")
        })
        if ($missingLfs.Count -gt 0) {
            $problems.Add("Tracked Unreal/source binaries outside LFS: $($missingLfs -join ', ')")
        }
    }
    catch {
        $problems.Add($_.Exception.Message)
    }

    Add-ValidationResult -Name "Git LFS" -Passed ($problems.Count -eq 0) -Details $(
        if ($problems.Count -eq 0) {
            "Git LFS is available; tracked .uasset, .umap, and .fbx files are LFS-managed."
        }
        else {
            $problems -join "; "
        }
    )
}

function Test-CoreProjectFiles {
    $requiredFiles = @(
        "OperationMouse.uproject",
        "Source/OperationMouse/Core/OMGameMode.h",
        "Source/OperationMouse/Core/OMGameState.h",
        "Source/OperationMouse/Core/OMPlayerState.h",
        "Source/OperationMouse/Core/OMPlayerController.h",
        "Source/OperationMouse/Characters/OMMouseCharacter.h",
        "Source/OperationMouse/Traversal/OMTraversalComponent.h"
    )
    $missing = @($requiredFiles | Where-Object {
        -not (Test-Path -LiteralPath (Join-Path $repositoryRoot $_) -PathType Leaf)
    })

    $projectProblem = $null
    if ($missing.Count -eq 0) {
        try {
            $project = Get-Content -Raw -LiteralPath $projectFile | ConvertFrom-Json
            $runtimeModule = @($project.Modules | Where-Object { $_.Name -eq "OperationMouse" -and $_.Type -eq "Runtime" })
            $enhancedInput = @($project.Plugins | Where-Object { $_.Name -eq "EnhancedInput" -and $_.Enabled })
            if ($runtimeModule.Count -ne 1) {
                $projectProblem = "OperationMouse Runtime module declaration is missing or duplicated."
            }
            elseif ($enhancedInput.Count -ne 1) {
                $projectProblem = "EnhancedInput plugin is not enabled."
            }
        }
        catch {
            $projectProblem = "OperationMouse.uproject is invalid JSON: $($_.Exception.Message)"
        }
    }

    $passed = $missing.Count -eq 0 -and -not $projectProblem
    $details = if ($passed) {
        "Approved main project file, module, framework headers, Character, and traversal component exist."
    }
    elseif ($missing.Count -gt 0) {
        "Missing: $($missing -join ', ')"
    }
    else {
        $projectProblem
    }
    Add-ValidationResult -Name "Core project files" -Passed $passed -Details $details
}

function Invoke-UnrealContentValidation {
    . (Join-Path $PSScriptRoot "Resolve-UnrealEngine.ps1")
    $engineRoot = Resolve-UnrealEngineRoot -ProjectFile $projectFile
    $unrealEditorCmd = Join-Path $engineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
    $validationScript = Join-Path $PSScriptRoot "Editor\validate_project_content.py"
    $resultPath = Join-Path $logDirectory "unreal-content-result.json"
    $logPath = Join-Path $logDirectory "unreal-content.log"

    if (-not (Test-Path -LiteralPath $unrealEditorCmd -PathType Leaf)) {
        Add-ValidationResult -Name "Headless project boot" -Passed $false -Details "UnrealEditor-Cmd.exe was not found."
        return
    }

    $previousResultPath = $env:OM_VALIDATION_RESULT_PATH
    try {
        $env:OM_VALIDATION_RESULT_PATH = $resultPath
        & $unrealEditorCmd $projectFile "-run=pythonscript" "-script=$validationScript" -unattended -nop4 -nosplash -nullrhi *> $logPath
        $exitCode = $LASTEXITCODE
    }
    finally {
        $env:OM_VALIDATION_RESULT_PATH = $previousResultPath
    }

    $resultExists = Test-Path -LiteralPath $resultPath -PathType Leaf
    Add-ValidationResult -Name "Headless project boot" -Passed ($exitCode -eq 0 -and $resultExists) -Details $(
        if ($exitCode -eq 0 -and $resultExists) {
            "OperationMouse module loaded and the read-only Unreal validation script completed."
        }
        else {
            "Unreal validation failed with exit code $exitCode. Log: $(Get-RelativeLogPath $logPath)"
        }
    )

    if ($resultExists) {
        $unrealResult = Get-Content -Raw -LiteralPath $resultPath | ConvertFrom-Json
        foreach ($check in $unrealResult.checks) {
            Add-ValidationResult -Name ([string]$check.name) -Passed ([bool]$check.passed) -Details ([string]$check.details)
        }
    }
}

function Invoke-BuildValidation {
    param([string]$Target)

    $logPath = Join-Path $logDirectory "$Target-build.log"
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "Build.ps1") -Target $Target -Configuration Development *> $logPath
    $exitCode = $LASTEXITCODE
    $passed = $exitCode -eq 0
    $details = if ($passed) {
        "Win64 Development build succeeded. Log: $(Get-RelativeLogPath $logPath)"
    }
    else {
        "$(Get-FirstMeaningfulError $logPath) Log: $(Get-RelativeLogPath $logPath)"
    }
    Add-ValidationResult -Name "$Target build" -Passed $passed -Details $details
}

Write-Host "Operation Mouse Project Validation"
Write-Host "Mode: $($Mode.ToUpperInvariant())"
Write-Host ""

try {
    Test-RepositoryHealth
    Test-GitLfs
    Test-CoreProjectFiles
    Invoke-UnrealContentValidation

    if ($Mode -eq "Full") {
        Invoke-BuildValidation -Target "OperationMouseEditor"
        Invoke-BuildValidation -Target "OperationMouse"
    }
}
catch {
    Add-ValidationResult -Name "Validator execution" -Passed $false -Details $_.Exception.Message
}

$summaryLines = New-Object System.Collections.Generic.List[string]
$summaryLines.Add("Operation Mouse Project Validation")
$summaryLines.Add("Mode: $($Mode.ToUpperInvariant())")
$summaryLines.Add("")
foreach ($result in $results) {
    $status = if ($result.Passed) { "PASS" } else { "FAIL" }
    $line = "[$status] $($result.Name) - $($result.Details)"
    $summaryLines.Add($line)
    Write-Host $line
}

$allPassed = @($results | Where-Object { -not $_.Passed }).Count -eq 0
$finalResult = if ($allPassed) { "PASSED" } else { "FAILED" }
$summaryLines.Add("")
$summaryLines.Add("FINAL RESULT: $finalResult")
$summaryLines.Add("Detailed logs: $(Get-RelativeLogPath $logDirectory)")
$summaryLines | Set-Content -LiteralPath $summaryPath -Encoding UTF8

Write-Host ""
Write-Host "FINAL RESULT: $finalResult"
Write-Host "Detailed logs: $(Get-RelativeLogPath $logDirectory)"

if (-not $allPassed) {
    exit 1
}
