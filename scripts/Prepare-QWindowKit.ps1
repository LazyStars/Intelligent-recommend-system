[CmdletBinding()]
param(
    [ValidateSet("All", "Debug", "Release")]
    [string]$Configuration = "All",
    [switch]$Force
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$thirdPartyRoot = Join-Path $repositoryRoot "3rdParty"
$manifestPath = Join-Path $thirdPartyRoot "qwindowkit-manifest.json"

if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    throw "QWindowKit manifest is missing: $manifestPath"
}

$manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
$configurations = if ($Configuration -eq "All") {
    @("Debug", "Release")
} else {
    @($Configuration)
}

foreach ($currentConfiguration in $configurations) {
    $artifact = $manifest.artifacts.$currentConfiguration
    if ($null -eq $artifact) {
        throw "Manifest has no $currentConfiguration artifact."
    }

    $archivePath = Join-Path $thirdPartyRoot $artifact.file
    if (-not (Test-Path -LiteralPath $archivePath -PathType Leaf)) {
        throw "Artifact is missing: $archivePath. Run 'git lfs pull'."
    }

    $archiveInfo = Get-Item -LiteralPath $archivePath
    if ($archiveInfo.Length -lt 1024) {
        $archiveHeader = Get-Content -LiteralPath $archivePath -TotalCount 1
        if ($archiveHeader -eq "version https://git-lfs.github.com/spec/v1") {
            throw "Artifact is still a Git LFS pointer: $archivePath. Run 'git lfs pull'."
        }
    }

    $actualHash = (Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actualHash -ne $artifact.sha256.ToLowerInvariant()) {
        throw "SHA-256 mismatch for $archivePath. Expected $($artifact.sha256), got $actualHash."
    }

    $configurationDirectory = $currentConfiguration.ToLowerInvariant()
    $targetPath = Join-Path $thirdPartyRoot "$configurationDirectory/qwindowkit"
    $stagingPath = Join-Path $thirdPartyRoot ".qwindowkit-prepare-$configurationDirectory"

    $targetExists = Test-Path -LiteralPath $targetPath
    if ($targetExists -and -not $Force) {
        throw "$targetPath already exists. Re-run with -Force to replace it."
    }
    if (Test-Path -LiteralPath $stagingPath) {
        Remove-Item -LiteralPath $stagingPath -Recurse -Force
    }

    try {
        Expand-Archive -LiteralPath $archivePath -DestinationPath $stagingPath

        $packageConfig = Join-Path $stagingPath "lib/cmake/QWindowKit/QWindowKitConfig.cmake"
        if (-not (Test-Path -LiteralPath $packageConfig -PathType Leaf)) {
            throw "Artifact does not contain a QWindowKit CMake package: $archivePath"
        }

        if ($targetExists) {
            Remove-Item -LiteralPath $targetPath -Recurse -Force
        }
        $targetParent = Split-Path -Parent $targetPath
        New-Item -ItemType Directory -Path $targetParent -Force | Out-Null
        Move-Item -LiteralPath $stagingPath -Destination $targetPath
    } finally {
        if (Test-Path -LiteralPath $stagingPath) {
            Remove-Item -LiteralPath $stagingPath -Recurse -Force
        }
    }

    Write-Host "Prepared QWindowKit $currentConfiguration at $targetPath"
}
