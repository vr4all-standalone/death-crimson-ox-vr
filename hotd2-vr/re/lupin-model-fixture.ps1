<# Private runtime fixture. Default Plan performs no writes or launches. Coordinate exclusive access to the clone first.
   Install/Restore only touch this Lupin clone; neither starts nor stops any process.
   A Restore preserves the installed fixture in its backup folder for inspection.
#>
param(
    [ValidateSet('Plan','Install','Restore')][string]$Mode = 'Plan',
    [string]$Backup = ''
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
if ((Split-Path -Leaf $repo) -ne 'lupin-vr') { throw 'This fixture is scoped to a checkout named lupin-vr.' }
$build = [IO.Path]::GetFullPath((Join-Path $repo 'build-win'))
$private = [IO.Path]::GetFullPath((Join-Path $build 'vrtest\model-fixtures'))
$source = [IO.Path]::GetFullPath((Join-Path $repo 'hotd2-vr\models'))
$relativeTargets = @('guns','controllers','data\models')
function Assert-Under([string]$Path,[string]$Root) {
    $p = [IO.Path]::GetFullPath($Path)
    $r = [IO.Path]::GetFullPath($Root).TrimEnd('\') + '\'
    if (-not $p.StartsWith($r,[StringComparison]::OrdinalIgnoreCase)) { throw "Unsafe path outside expected root: $p" }
    return $p
}
function Assert-Plain([string]$Path) {
    if (Test-Path -LiteralPath $Path) {
        $item = Get-Item -LiteralPath $Path -Force
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw "Reparse point is not a fixture target: $Path" }
        foreach ($child in @(Get-ChildItem -LiteralPath $Path -Force -Recurse)) {
            if (($child.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw "Reparse point under fixture target: $($child.FullName)" }
        }
    }
}
function Assert-Idle {
    $players = @(Get-CimInstance Win32_Process -Filter "Name = 'lupin-vr-player.exe'")
    foreach ($p in $players) {
        if (-not $p.ExecutablePath -or [IO.Path]::GetFullPath($p.ExecutablePath).Equals((Join-Path $build 'lupin-vr-player.exe'),[StringComparison]::OrdinalIgnoreCase)) {
            throw 'The Lupin clone harness is active. Coordinate with root before installing/restoring this fixture.'
        }
    }
}
function Write-State($State,[string]$Dir) {
    $State | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $Dir 'manifest.json') -Encoding utf8
}
function Restore-State($State,[string]$Dir) {
    if ($State.Restored) { throw "Fixture already restored: $Dir" }
    $target = Assert-Under (Join-Path $build 'data\models') $build
    if ($State.FixtureInstalled -and (Test-Path -LiteralPath $target)) {
        Assert-Plain $target
        $after = Assert-Under (Join-Path $Dir 'fixture-after') $Dir
        if (Test-Path -LiteralPath $after) { throw "Restore destination already exists: $after" }
        Move-Item -LiteralPath $target -Destination $after
        $State.FixtureInstalled = $false
        Write-State $State $Dir
    }
    foreach ($entry in $State.Targets) {
        if (-not $entry.Moved) { continue }
        $target = Assert-Under (Join-Path $build $entry.Relative) $build
        $saved = Assert-Under (Join-Path $Dir $entry.Saved) $Dir
        Assert-Plain $saved
        if (Test-Path -LiteralPath $target) { throw "Refusing to overwrite a new path while restoring: $target" }
        Move-Item -LiteralPath $saved -Destination $target
        $entry.Moved = $false
        Write-State $State $Dir
    }
    $State.Restored = $true
    Write-State $State $Dir
    Write-Output "Restored original model folders. Fixture retained at $Dir"
}
foreach ($rel in $relativeTargets) { $null = Assert-Under (Join-Path $build $rel) $build }
if ($Mode -eq 'Plan') {
    Write-Output 'Plan only: no model folder changes and no process launch.'
    foreach ($rel in $relativeTargets) { Write-Output "Back up and restore: $(Join-Path $build $rel)" }
    Write-Output "Copy licensed bundle: $source -> $(Join-Path $build 'data\models')"
    Write-Output 'Existing executable-directory models are moved aside so they cannot shadow the actual bundle.'
    return
}
Assert-Idle
if ($Mode -eq 'Restore') {
    if (-not $Backup) { throw 'Restore requires the exact -Backup folder printed by Install.' }
    $Backup = Assert-Under $Backup $private
    Assert-Plain $Backup
    $state = Get-Content -LiteralPath (Join-Path $Backup 'manifest.json') -Raw | ConvertFrom-Json
    if ($state.Repo -ne $repo -or $state.Build -ne $build) { throw 'Manifest does not belong to this clone.' }
    Restore-State $state $Backup
    return
}
if ($Backup) { throw 'Install generates a unique backup; do not supply -Backup.' }
Assert-Plain $source
foreach ($rel in $relativeTargets) { Assert-Plain (Join-Path $build $rel) }
$files = @('guns\namco.glb','guns\namco.json','guns\zapper.glb','guns\zapper.json','controllers\left.glb','controllers\left.json','controllers\right.glb','controllers\right.json','CREDITS.txt')
foreach ($rel in $files) {
    $path = Assert-Under (Join-Path $source $rel) $source
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing licensed bundle file: $path" }
}
$Backup = Assert-Under (Join-Path $private ('model-fixture-' + [guid]::NewGuid().ToString('N'))) $private
$null = New-Item -ItemType Directory -Path $Backup -Force
$state = [pscustomobject]@{ Repo=$repo; Build=$build; Created=[DateTime]::UtcNow.ToString('o'); Restored=$false; FixtureInstalled=$false; Targets=@(); Files=@() }
for ($i=0; $i -lt $relativeTargets.Count; $i++) {
    $state.Targets += [pscustomobject]@{ Relative=$relativeTargets[$i]; Saved="original-$i"; Existed=(Test-Path -LiteralPath (Join-Path $build $relativeTargets[$i])); Moved=$false }
}
Write-State $state $Backup
try {
    foreach ($entry in $state.Targets) {
        if (-not $entry.Existed) { continue }
        $target = Assert-Under (Join-Path $build $entry.Relative) $build
        $saved = Assert-Under (Join-Path $Backup $entry.Saved) $Backup
        Move-Item -LiteralPath $target -Destination $saved
        $entry.Moved = $true
        Write-State $state $Backup
    }
    $target = Assert-Under (Join-Path $build 'data\models') $build
    $state.FixtureInstalled = $true
    Write-State $state $Backup
    $null = New-Item -ItemType Directory -Path $target
    foreach ($rel in $files) {
        $src = Assert-Under (Join-Path $source $rel) $source
        $dst = Assert-Under (Join-Path $target $rel) $target
        $null = New-Item -ItemType Directory -Path (Split-Path -Parent $dst) -Force
        Copy-Item -LiteralPath $src -Destination $dst
        $srcHash = (Get-FileHash -LiteralPath $src -Algorithm SHA256).Hash
        $dstHash = (Get-FileHash -LiteralPath $dst -Algorithm SHA256).Hash
        if ($srcHash -ne $dstHash) { throw "Copy differs from licensed source: $rel" }
        $state.Files += [pscustomobject]@{ Relative=$rel; Bytes=(Get-Item -LiteralPath $dst).Length; SHA256=$dstHash }
    }
    Write-State $state $Backup
} catch {
    Write-Warning "Install failed; restoring original folders: $($_.Exception.Message)"
    Restore-State $state $Backup
    throw
}
Write-Output "Installed independent copies of $($files.Count) licensed bundle files."
Write-Output "Backup: $Backup"
Write-Output "Restore: & '$PSCommandPath' -Mode Restore -Backup '$Backup'"
