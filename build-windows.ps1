<#
.SYNOPSIS
    THE APP STUB -- canonical template: MTEngineSDL\tools\appbuild\stubs\.
.DESCRIPTION
    An app owns two files: mtengine.caps and mtengine-app.conf. The build
    flow -- and the MTENGINE_REF verification -- lives in the engine's
    app-build driver; this stub keeps only the clone-when-absent job. The
    driver warns when this file drifts from the template.
#>
param(
    [ValidateSet('x64','ARM64')]
    [string]$Platform,
    [ValidateSet('Debug','Release')]
    [string]$Configuration = 'Release',
    [ValidateSet('Clang','MSVC')]
    [string]$Compiler = 'Clang',
    [switch]$SkipCuda,
    [switch]$SkipDeps,
    [switch]$Prod,
    [ValidateSet('on','off')]
    [string]$Logs,
    [ValidateSet('on','off')]
    [string]$Symbols,
    [ValidateSet('dev','commercial')]
    [string]$Tier,
    [switch]$Clean,
    [string[]]$Set,
    [switch]$Gc,
    [switch]$Help,
    [Parameter(ValueFromRemainingArguments)][string[]]$GcArgs
)
$ErrorActionPreference = 'Stop'
$mtDir = Join-Path (Split-Path -Parent $PSScriptRoot) 'MTEngineSDL'
if (-not (Test-Path $mtDir)) {
    $ref = (Get-Content (Join-Path $PSScriptRoot 'MTENGINE_REF') |
            Where-Object { $_ -notmatch '^\s*#' -and $_.Trim() -ne '' } |
            Select-Object -First 1).Trim()
    Write-Host "Cloning MTEngineSDL at $ref" -ForegroundColor Cyan
    git clone https://github.com/slajerek/MTEngineSDL.git $mtDir
    if ($LASTEXITCODE -ne 0) { throw "git clone failed" }
    git -C $mtDir checkout ($ref -replace '^origin/', '') 2>$null
    if ($LASTEXITCODE -ne 0) { git -C $mtDir checkout --detach $ref }
}
$driver = Join-Path $mtDir 'tools\appbuild\app-build-windows.ps1'
if (-not (Test-Path $driver)) {
    throw "$driver not found -- the engine checkout predates the app-build driver. Run: git -C `"$mtDir`" pull"
}
if ($Help) { Get-Help $driver -Detailed; exit 0 }
$driverArgs = @{
    AppDir        = $PSScriptRoot
    Configuration = $Configuration
    Compiler      = $Compiler
    SkipCuda      = $SkipCuda
    SkipDeps      = $SkipDeps
    Prod          = $Prod
    Clean         = $Clean
    Gc            = $Gc
}
if ($Set) { $driverArgs.Set = $Set }
if ($Logs) { $driverArgs.Logs = $Logs }
if ($Symbols) { $driverArgs.Symbols = $Symbols }
if ($Tier) { $driverArgs.Tier = $Tier }
if ($GcArgs) { $driverArgs.GcArgs = $GcArgs }
if ($Platform) { $driverArgs.Platform = $Platform }
& $driver @driverArgs
exit $LASTEXITCODE
