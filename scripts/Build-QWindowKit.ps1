[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$QtRoot,
    [string]$CMakeCommand = "cmake",
    [string]$GitCommand = "git",
    [string]$NinjaCommand = "ninja",
    [string]$Generator = "Ninja",
    [string]$WorkRoot = (Join-Path ([System.IO.Path]::GetTempPath()) "intelligent-recommend-system-qwindowkit")
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$qwindowKitVersion = "1.5.1"
$qwindowKitCommit = "5626b3ae5f22508d77361495f589abedb39c2ee4"
$qwindowKitRepository = "https://github.com/stdware/qwindowkit.git"
$artifactBaseName = "qwindowkit-$qwindowKitVersion-qt6.8.3-msvc-x64"

function Invoke-Checked {
    param(
        [Parameter(Mandatory)]
        [string]$Command,
        [Parameter(Mandatory)]
        [string[]]$Arguments
    )

    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code ${LASTEXITCODE}: $Command $($Arguments -join ' ')"
    }
}

function Remove-BuildDirectory {
    param(
        [Parameter(Mandatory)]
        [string]$Path,
        [Parameter(Mandatory)]
        [string]$ExpectedRoot
    )

    $resolvedPath = [System.IO.Path]::GetFullPath($Path)
    $resolvedRoot = [System.IO.Path]::GetFullPath($ExpectedRoot).TrimEnd([System.IO.Path]::DirectorySeparatorChar)
    if (-not $resolvedPath.StartsWith($resolvedRoot + [System.IO.Path]::DirectorySeparatorChar,
            [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove a path outside the build root: $resolvedPath"
    }
    if (Test-Path -LiteralPath $resolvedPath) {
        Remove-Item -LiteralPath $resolvedPath -Recurse -Force
    }
}

$QtRoot = [System.IO.Path]::GetFullPath($QtRoot)
$qtConfigPath = Join-Path $QtRoot "lib/cmake/Qt6/Qt6Config.cmake"
$qmakePath = Join-Path $QtRoot "bin/qmake.exe"
if (-not (Test-Path -LiteralPath $qtConfigPath -PathType Leaf)) {
    throw "Qt 6 CMake package was not found under $QtRoot."
}
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    throw "cl.exe is unavailable. Run this script from an x64 Visual Studio Developer PowerShell."
}
if (-not (Get-Command $CMakeCommand -ErrorAction SilentlyContinue)) {
    throw "CMake command was not found: $CMakeCommand"
}
if (-not (Get-Command $GitCommand -ErrorAction SilentlyContinue)) {
    throw "Git command was not found: $GitCommand"
}
$ninjaPath = $null
if ($Generator -eq "Ninja") {
    $ninjaCommandInfo = Get-Command $NinjaCommand -ErrorAction SilentlyContinue
    if (-not $ninjaCommandInfo) {
        throw "Ninja is unavailable: $NinjaCommand"
    }
    $ninjaPath = $ninjaCommandInfo.Source
}

$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$thirdPartyRoot = Join-Path $repositoryRoot "3rdParty"
$artifactRoot = Join-Path $thirdPartyRoot "artifacts"
$manifestPath = Join-Path $thirdPartyRoot "qwindowkit-manifest.json"
$workRootPath = [System.IO.Path]::GetFullPath($WorkRoot)
$sourcePath = Join-Path $workRootPath "source"

New-Item -ItemType Directory -Path $workRootPath -Force | Out-Null
New-Item -ItemType Directory -Path $artifactRoot -Force | Out-Null

if (-not (Test-Path -LiteralPath (Join-Path $sourcePath ".git"))) {
    Invoke-Checked -Command $GitCommand -Arguments @(
        "clone", "--filter=blob:none", "--no-checkout", $qwindowKitRepository, $sourcePath
    )
}

Invoke-Checked -Command $GitCommand -Arguments @(
    "-C", $sourcePath, "fetch", "origin", $qwindowKitCommit, "--depth", "1"
)
Invoke-Checked -Command $GitCommand -Arguments @(
    "-C", $sourcePath, "checkout", "--detach", $qwindowKitCommit
)
Invoke-Checked -Command $GitCommand -Arguments @(
    "-C", $sourcePath, "submodule", "update", "--init", "--recursive", "--depth", "1"
)

$artifactData = [ordered]@{}
foreach ($configuration in @("Debug", "Release")) {
    $configurationName = $configuration.ToLowerInvariant()
    $buildPath = Join-Path $workRootPath "build-$configurationName"
    $installPath = Join-Path $workRootPath "install-$configurationName"
    Remove-BuildDirectory -Path $buildPath -ExpectedRoot $workRootPath
    Remove-BuildDirectory -Path $installPath -ExpectedRoot $workRootPath

    $configureArguments = @(
        "-S", $sourcePath,
        "-B", $buildPath,
        "-G", $Generator,
        "-DCMAKE_BUILD_TYPE=$configuration",
        "-DCMAKE_PREFIX_PATH=$QtRoot",
        "-DCMAKE_INSTALL_PREFIX=$installPath",
        "-DQWINDOWKIT_BUILD_WIDGETS=ON",
        "-DQWINDOWKIT_BUILD_QUICK=ON",
        "-DQWINDOWKIT_BUILD_EXAMPLES=OFF",
        "-DQWINDOWKIT_BUILD_TESTS=OFF",
        "-DQWINDOWKIT_BUILD_DOCUMENTATIONS=OFF",
        "-DQWINDOWKIT_INSTALL=ON"
    )
    if ($ninjaPath) {
        $configureArguments += "-DCMAKE_MAKE_PROGRAM=$ninjaPath"
    }
    Invoke-Checked -Command $CMakeCommand -Arguments $configureArguments
    Invoke-Checked -Command $CMakeCommand -Arguments @(
        "--build", $buildPath, "--target", "install", "--config", $configuration, "--parallel"
    )

    Copy-Item -LiteralPath (Join-Path $sourcePath "LICENSE") -Destination (Join-Path $installPath "LICENSE") -Force
    if ($configuration -eq "Debug") {
        foreach ($pdbName in @("QWKCored.pdb", "QWKWidgetsd.pdb", "QWKQuickd.pdb")) {
            $pdbPath = Join-Path $buildPath "out/bin/$pdbName"
            if (-not (Test-Path -LiteralPath $pdbPath -PathType Leaf)) {
                throw "Expected debug symbols were not produced: $pdbPath"
            }
            Copy-Item -LiteralPath $pdbPath -Destination (Join-Path $installPath "bin/$pdbName") -Force
        }
    }

    $archiveName = "$artifactBaseName-$configurationName.zip"
    $archivePath = Join-Path $artifactRoot $archiveName
    if (Test-Path -LiteralPath $archivePath) {
        Remove-Item -LiteralPath $archivePath -Force
    }
    Compress-Archive -Path (Join-Path $installPath "*") -DestinationPath $archivePath -CompressionLevel Optimal
    $archiveHash = (Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash.ToLowerInvariant()

    $artifactData[$configuration] = [ordered]@{
        file = "artifacts/$archiveName"
        sha256 = $archiveHash
        installPath = "$configurationName/qwindowkit"
    }
}

$qtVersion = if (Test-Path -LiteralPath $qmakePath -PathType Leaf) {
    (& $qmakePath -query QT_VERSION).Trim()
} else {
    "unknown"
}
$compilerBanner = (& cl.exe 2>&1 | Select-Object -First 1).ToString()
$compilerVersion = if ($compilerBanner -match "Version\s+([0-9.]+)") {
    $Matches[1]
} else {
    "unknown"
}
$toolsetVersion = if ($env:VCToolsVersion) { $env:VCToolsVersion.TrimEnd("\") } else { "unknown" }

$manifest = [ordered]@{
    name = "QWindowKit"
    version = $qwindowKitVersion
    source = $qwindowKitRepository
    commit = $qwindowKitCommit
    license = "Apache-2.0"
    build = [ordered]@{
        qt = $qtVersion
        compiler = "MSVC $compilerVersion (toolset $toolsetVersion)"
        architecture = "x64"
        linkage = "shared"
        modules = @("Core", "Widgets", "Quick")
    }
    artifacts = $artifactData
}

$manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $manifestPath -Encoding utf8
Write-Host "Created QWindowKit artifacts in $artifactRoot"
Write-Host "Updated manifest: $manifestPath"
