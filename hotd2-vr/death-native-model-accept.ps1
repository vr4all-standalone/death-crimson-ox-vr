# Native first-run/cache integration using private owned cartridge files. Restores the initial cache.
$ErrorActionPreference='Stop'
 $repo=Split-Path -Parent $PSScriptRoot
$env:VRLGH_GAMES=Join-Path $repo 'build-win\game-data'
$cache=[IO.Path]::GetFullPath((Join-Path $repo 'build-win\data\gun-dc-pistol.bin'))
$allowed=[IO.Path]::GetFullPath((Join-Path $repo 'build-win\data'))+'\'
if(-not$cache.StartsWith($allowed,[StringComparison]::OrdinalIgnoreCase)){throw 'Cache target escaped private data folder'}
if(Test-Path -LiteralPath $cache -PathType Container){throw 'Initial cache path must not be a directory'}
$had=Test-Path -LiteralPath $cache
$before=if($had){[IO.File]::ReadAllBytes($cache)}else{$null}
function Clear-Cache {
 if(Test-Path -LiteralPath $cache -PathType Container){if(@(Get-ChildItem -LiteralPath $cache -Force).Count){throw 'Refusing to delete a nonempty fixture directory'};[IO.Directory]::Delete($cache,$false)}
 elseif(Test-Path -LiteralPath $cache){[IO.File]::Delete($cache)}
}
function Run([string]$name,[string[]]$Expect,[string[]]$Reject=@(),[string[]]$On=@(),[string[]]$At=@()){
 & (Join-Path $repo 'hotd2-vr\death-test.ps1') -Game deathcoxj -Name $name -Seconds 8 -XrSim -Visible -Config 'config:vr.GameHands=yes,config:vr.HandsPrep=yes,config:vr.Gun=1,config:vr.BuiltInGun=no' -Expect $Expect -Reject $Reject -On $On -At $At
 if($LASTEXITCODE){throw "$name failed"}
}
try {
 Run 'dc-native-reuse' @('VR hands: the in-game soldier pistol, made before:','XR: gun: in-game pistol') @('VR hands: no model yet') -At @('5=xr left/trigger 0.9 250','6=shot native-left')
 $expected=(Get-FileHash -LiteralPath $cache).Hash
 [IO.File]::WriteAllBytes($cache,[byte[]](1,2,3))
 Run 'dc-native-corrupt' @('VR hands: complete in-game soldier pistol from your cartridge saved') @('native pistol unavailable')
 if((Get-FileHash -LiteralPath $cache).Hash -ne $expected){throw 'Corrupt cache did not rebuild deterministically'}
 Clear-Cache
 Run 'dc-native-skip' @('VR hands: skipped','XR: gun: Arcade gun','XR: menu closed \(resumed\)') @('complete in-game soldier pistol') @('VR hands: no model yet, reading|0|xr right/b 1 1200') @('4=xr left/menu 1 150','5=xr right/b 1 150')
 if(Test-Path -LiteralPath $cache){throw 'Skipped preparation wrote a native cache'}
 $lines=Get-Content -LiteralPath (Join-Path $repo 'build-win\vrtest\dc-native-skip\flycast.log')
 if(@($lines|Where-Object {$_ -match 'VR hands: no model yet'}).Count -ne 1){throw 'Pause/resume retried a skipped extraction'}
 New-Item -ItemType Directory -Path $cache | Out-Null
 Run 'dc-native-write-failure' @('native pistol unavailable: can.t write','XR: gun: Arcade gun') @('complete in-game soldier pistol') -At @('4=xr left/menu 1 150','5=xr right/b 1 150')
 if(Test-Path -LiteralPath ($cache+'.part')){throw 'Failed write left a partial cache'}
 Clear-Cache
 Run 'dc-native-retry' @('VR hands: complete in-game soldier pistol from your cartridge saved','XR: gun: in-game pistol') @('native pistol unavailable')
 if((Get-FileHash -LiteralPath $cache).Hash -ne $expected){throw 'New launch failed to retry native extraction'}
 Write-Host 'PASS native cache reuse, corrupt rebuild, skip/resume, write-failure fallback and next-launch retry'
} finally {
 Clear-Cache
 if($had){[IO.File]::WriteAllBytes($cache,$before)}
}
