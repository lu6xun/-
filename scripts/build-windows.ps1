param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [switch]$Headless,
    [string]$RaylibSource = ''
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
$cmakePath = if ($cmakeCommand) { $cmakeCommand.Source } else { $null }

# Visual Studio often bundles CMake without adding it to PATH.
if (-not $cmakePath) {
    $vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path -LiteralPath $vswherePath) {
        $installations = & $vswherePath -products '*' -all -property installationPath
        foreach ($installation in $installations) {
            $candidate = Join-Path $installation 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
            if (Test-Path -LiteralPath $candidate) { $cmakePath = $candidate; break }
        }
    }
}
if (-not $cmakePath) { throw 'CMake 3.20+ is required. Install the Visual Studio C++ CMake tools component or add CMake to PATH.' }

$buildName = if ($Headless) { 'build-headless' } else { 'build' }
$buildPath = Join-Path $projectRoot $buildName
$configureArguments = @('-S', $projectRoot, '-B', $buildPath, '-G', 'Visual Studio 17 2022', '-A', 'x64')
$configureArguments += if ($Headless) { '-DSNAKE_BUILD_GAME=OFF' } else { '-DSNAKE_BUILD_GAME=ON' }
if ($RaylibSource) {
    $sourcePath = (Resolve-Path -LiteralPath $RaylibSource).Path
    if (-not (Test-Path -LiteralPath (Join-Path $sourcePath 'src\raylib.h'))) {
        throw 'RaylibSource must point to the extracted raylib 5.5 source root.'
    }
    $configureArguments += "-DFETCHCONTENT_SOURCE_DIR_RAYLIB=$sourcePath"
}

& $cmakePath @configureArguments
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }
& $cmakePath --build $buildPath --config $Configuration --parallel
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
$ctestPath = Join-Path (Split-Path -Parent $cmakePath) 'ctest.exe'
& $ctestPath --test-dir $buildPath -C $Configuration --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Tests failed.' }
if (-not $Headless) {
    Write-Host "Run: & '$buildPath\$Configuration\grid_snake.exe'"
}
