param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Debug",
    [string]$Target = "coin_runner",
    [switch]$Clean,
    [string]$Generator
)

# Prerequisites:
#   1. CMake must be in PATH (or set CMAKE_PATH environment variable)
#   2. Visual Studio 2022 with C++ workload installed

$SCRIPT_DIR = $PSScriptRoot
if (-not $SCRIPT_DIR) { $SCRIPT_DIR = Split-Path -Parent $MyInvocation.MyCommand.Path }

if ($env:CMAKE_PATH) {
    $CMAKE_PATH = $env:CMAKE_PATH
}
else {
    $CMAKE_PATH = (Get-Command cmake -ErrorAction SilentlyContinue).Source
    if (-not $CMAKE_PATH) {
        Write-Host "Error: CMake not found in PATH. Install CMake or set CMAKE_PATH environment variable." -ForegroundColor Red
        exit 1
    }
}

# Ensure child tools use the same cmake path for this run.
$env:CMAKE_PATH = $CMAKE_PATH

if (-not $Generator) {
    if (Get-Command cl.exe -ErrorAction SilentlyContinue) {
        $Generator = "Visual Studio 17 2022"
    }
    elseif (Get-Command mingw32-make.exe -ErrorAction SilentlyContinue) {
        $Generator = "MinGW Makefiles"
    }
    elseif (Get-Command ninja.exe -ErrorAction SilentlyContinue) {
        $Generator = "Ninja"
    }
    else {
        $Generator = "Visual Studio 17 2022"
    }
}

$BIN_DIR = Join-Path $SCRIPT_DIR "bin"

Write-Host "Using CMake: $CMAKE_PATH" -ForegroundColor Gray
Write-Host "Configuration: $Config" -ForegroundColor Gray
Write-Host "Target: $Target" -ForegroundColor Gray
Write-Host "Generator: $Generator" -ForegroundColor Gray

$buildDirSuffix = $Generator -replace '[^A-Za-z0-9]+', '_'
$BUILD_DIR = Join-Path $SCRIPT_DIR ("out\build_" + $buildDirSuffix)
Write-Host "Build directory: $BUILD_DIR" -ForegroundColor Gray

if ($Clean -and (Test-Path $BUILD_DIR)) {
    Write-Host "Cleaning build directory..." -ForegroundColor Cyan
    Remove-Item -Recurse -Force $BUILD_DIR
}

New-Item -ItemType Directory -Force -Path $BUILD_DIR | Out-Null
New-Item -ItemType Directory -Force -Path $BIN_DIR | Out-Null

Write-Host "Configuring CMake..." -ForegroundColor Cyan
$configureArgs = @("-S", $SCRIPT_DIR, "-B", $BUILD_DIR, "-G", $Generator)

if ($Generator -like "Visual Studio*") {
    $configureArgs += @("-A", "x64", "-T", "v143")
}
elseif ($Generator -eq "MinGW Makefiles") {
    $gccPath = (Get-Command gcc.exe -ErrorAction SilentlyContinue).Source
    $gppPath = (Get-Command g++.exe -ErrorAction SilentlyContinue).Source
    $makePath = (Get-Command mingw32-make.exe -ErrorAction SilentlyContinue).Source
    if (-not $gccPath -or -not $gppPath -or -not $makePath) {
        Write-Host "Error: MinGW generator selected, but gcc/g++/mingw32-make not found in PATH." -ForegroundColor Red
        exit 1
    }
    $gccPath = $gccPath -replace '\\', '/'
    $gppPath = $gppPath -replace '\\', '/'
    $makePath = $makePath -replace '\\', '/'
    $configureArgs += @("-DCMAKE_MAKE_PROGRAM=$makePath", "-DCMAKE_C_COMPILER=$gccPath", "-DCMAKE_CXX_COMPILER=$gppPath")
}

& $CMAKE_PATH @configureArgs
if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed!" -ForegroundColor Red
    exit 1
}

Write-Host "Building $Target..." -ForegroundColor Cyan
$buildArgs = @("--build", $BUILD_DIR, "--target", $Target)
if ($Generator -like "Visual Studio*" -or $Generator -eq "Ninja Multi-Config") {
    $buildArgs += @("--config", $Config)
}

& $CMAKE_PATH @buildArgs
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    exit 1
}

$EXE_PATH = Join-Path $BIN_DIR "$Target.exe"
Write-Host "`nBuild complete!" -ForegroundColor Green
Write-Host "Executable: $EXE_PATH" -ForegroundColor Yellow
Write-Host "Run: `"$EXE_PATH`"" -ForegroundColor Yellow
