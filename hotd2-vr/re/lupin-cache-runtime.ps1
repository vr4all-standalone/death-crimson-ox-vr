# Private clone-only cache/runtime check. Default describes work; -Run requires root's exclusive slot.
param([switch]$Run,[string]$Games = $env:VRLGH_GAMES,[switch]$RendererRejectedOnly)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$repo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
if((Split-Path -Leaf $repo) -ne 'lupin-vr'){throw 'This runtime suite is scoped to a checkout named lupin-vr.'}
$private=Join-Path $repo 'build-win\vrtest'
$fixture=Join-Path $PSScriptRoot 'lupin-model-fixture.ps1'
$cache=[IO.Path]::GetFullPath((Join-Path $repo 'build-win\data\gun-lp-pistol.bin'))
$part=$cache+'.part'
$build=[IO.Path]::GetFullPath((Join-Path $repo 'build-win'))+'\'
if (-not $cache.StartsWith($build,[StringComparison]::OrdinalIgnoreCase)) { throw 'Cache is outside clone.' }
if (-not $Run) {
    Write-Output $(if($RendererRejectedOnly){'Plan: licensed fixture; flags1+skip7s; NaN-muzzle+skip7s; valid-pistol7s; restore original cache/models in finally.'}else{'Plan: licensed fixture; valid orphan .part + skip86s; retry35s; cached Zapper7s; truncated cache recovery35s; restore original cache/models in finally.'})
    return
}
if(-not $Games -or -not(Test-Path -LiteralPath (Join-Path $Games 'lupinshoo.zip') -PathType Leaf)){throw 'Supply -Games (or VRLGH_GAMES) with the supported original Lupin set and BIOS.'}
$players=@(Get-CimInstance Win32_Process -Filter "Name = 'lupin-vr-player.exe'")
if ($players.Count) { throw 'A clone player is running; obtain the exclusive slot first.' }
$out=Join-Path $private ('cache-runtime-'+[guid]::NewGuid().ToString('N'))
$null=New-Item -ItemType Directory -Path $out
$original=Join-Path $out 'original-cache.bin';$originalPart=Join-Path $out 'original-cache.part'
$hadCache=Test-Path -LiteralPath $cache;$hadPart=Test-Path -LiteralPath $part
$originalHash=if($hadCache){(Get-FileHash -LiteralPath $cache -Algorithm SHA256).Hash}else{''}
$fixtureBackup='';$cacheMoved=$false;$partMoved=$false;$cacheScopeStarted=$false
$oldGames=$env:VRLGH_GAMES
$env:VRLGH_GAMES=(Resolve-Path -LiteralPath $Games).Path
$harness=Join-Path $repo 'hotd2-vr\vr-test.ps1'
$baseConfig='config:vr.HandsPrep=yes,config:vr.GameHands=yes,config:vr.Gun=0'
function Invoke-Case([string]$Name,[double]$Seconds,[string]$Extra,[string[]]$At,[string[]]$On,[string[]]$Expect,[string[]]$Reject) {
    $harnessArgs=@{Game='lupinshoo';Name=$Name;XrSim=$true;Visible=$true;Seconds=$Seconds;Config=$baseConfig+$Extra;At=$At;On=$On;Expect=$Expect;Reject=$Reject;Port=27028}
    Write-Output "RUN $Name ($Seconds seconds)"
    & $harness @harnessArgs *> (Join-Path $out ($Name+'.txt'))
    $code=$LASTEXITCODE
    Get-Content -LiteralPath (Join-Path $out ($Name+'.txt')) -Tail 12 | Write-Output
    if ($code -ne 0) { throw "$Name failed, harness exit $code" }
}
try {
    $messages=@(& $fixture -Mode Install)
    $messages | Write-Output
    $line=@($messages | Where-Object { $_ -is [string] -and $_.StartsWith('Backup: ') })
    if($line.Count -ne 1){throw 'Fixture did not return one backup manifest.'}
    $fixtureBackup=$line[0].Substring(8)
    if($hadCache){Move-Item -LiteralPath $cache -Destination $original;$cacheMoved=$true}
    if($hadPart){Move-Item -LiteralPath $part -Destination $originalPart;$partMoved=$true}
    $cacheScopeStarted=$true
    if($hadCache){Copy-Item -LiteralPath $original -Destination $part}else{[IO.File]::WriteAllBytes($part,[byte[]](72,78,68,49))}
    if($RendererRejectedOnly) {
        if(-not $hadCache){throw 'Rejected-renderer cases require a complete preexisting Lupin pistol cache.'}
        $good=[IO.File]::ReadAllBytes($original)
        if($good.Length -lt 112 -or $good.Length -gt 16777216 -or [Text.Encoding]::ASCII.GetString($good,0,4) -ne 'HND1' -or [BitConverter]::ToUInt32($good,4) -ne 2 -or [BitConverter]::ToUInt32($good,100) -ne 3 -or [BitConverter]::ToUInt32($good,104) -ne 24){throw 'Baseline cache does not have the expected complete Lupin header.'}
        $bad=[byte[]]$good.Clone()
        [Array]::Copy([BitConverter]::GetBytes([uint32]1),0,$bad,100,4)
        [IO.File]::WriteAllBytes($cache,$bad)
        $expectRejected=@('VR hands: no model yet, watching the attract demo','VR hands: skipped after','XR: gun: Arcade gun')
        $rejectLoaded=@("XR: the game's own model from .*gun-lp-pistol.bin",'XR: gun: in-game pistol','VR hands: done after')
        $skipActions=@('2=xr right/b 1 200','3=xr right/trigger/value 1 200','5=shot rejected-arcade')
        Invoke-Case 'lp-cache-incomplete-skip-real' 7 '' $skipActions @() $expectRejected $rejectLoaded
        $bad=[byte[]]$good.Clone()
        [Array]::Copy([BitConverter]::GetBytes([uint32]2143289344),0,$bad,8,4) # quiet NaN in muzzle.x
        [IO.File]::WriteAllBytes($cache,$bad)
        Invoke-Case 'lp-cache-nan-skip-real' 7 '' $skipActions @() $expectRejected $rejectLoaded
        [IO.File]::WriteAllBytes($cache,$good)
        Invoke-Case 'lp-cache-valid-pistol-real' 7 '' @('2=xr right/trigger/value 1 200','4=shot valid-pistol') @() @('VR hands: .*made before:','XR: gun: in-game pistol') @('VR hands: no model yet,','VR hands: done after')
        Write-Output 'PASS renderer rejects incomplete/NaN Lupin caches after skip and accepts the complete pistol.'
    } else {
    Invoke-Case 'lp-cache-skip-real' 86 '' @('2=xr right/b 1 200','3=shot skipped','83=xr right/trigger/value 1 200','85=shot arcade-real') @() @('VR hands: no model yet, watching the attract demo','VR hands: skipped after','XR: own gun bundled models/guns/namco.glb offered:') @('the in-game pistol from this game,','VR hands: done after')
    if(Test-Path -LiteralPath $cache){throw 'Skip unexpectedly generated a final cache.'}
    if(-not(Test-Path -LiteralPath $part)){throw 'Orphan .part unexpectedly treated as final or removed during skip.'}
    Invoke-Case 'lp-cache-retry-real' 35 '' @() @('VR hands: done after|0.5|xr right/trigger/value 1 200','VR hands: done after|5|shot pistol-real') @('VR hands: no model yet, watching the attract demo','the in-game pistol from this game,','VR hands: done after','XR: gun: in-game pistol') @()
    if(-not(Test-Path -LiteralPath $cache)){throw 'Retry did not produce a final cache.'}
    $madeBytes=(Get-Item -LiteralPath $cache).Length
    if($madeBytes -lt 128){throw "Retry cache is too short: $madeBytes"}
    if(Test-Path -LiteralPath $part){throw 'Successful atomic save left .part behind.'}
    Invoke-Case 'lp-cache-valid-zapper' 7 ',config:vr.Gun=4,config:vr.GunFile=zapper' @('2=xr right/trigger/value 1 200','4=shot zapper-real','5=xr left/menu 1 200','6=shot menu-real') @() @('VR hands: .*made before:','XR: gun: Zapper','XR: own gun bundled models/guns/namco.glb offered:') @('VR hands: no model yet,','VR hands: done after')
    Copy-Item -LiteralPath $cache -Destination (Join-Path $out 'regenerated-before-corrupt.bin')
    [IO.File]::WriteAllBytes($cache,[byte[]](72,78,68,49))
    Invoke-Case 'lp-cache-corrupt-real' 35 '' @() @('VR hands: done after|0.5|xr right/trigger/value 1 200','VR hands: done after|5|shot recovered-pistol') @('VR hands: no model yet, watching the attract demo','the in-game pistol from this game,','VR hands: done after','XR: gun: in-game pistol') @('VR hands: .*made before:')
    if((Get-Item -LiteralPath $cache).Length -ne $madeBytes){throw 'Recovered cache size differs from preceding complete cache.'}
    if(Test-Path -LiteralPath $part){throw 'Replacement left .part behind.'}
    Write-Output "PASS four runtime cases, regenerated cache bytes=$madeBytes"
    }
 } finally {
    # Every restore is attempted even when another fails; preserve generated files for diagnosis.
    $restoreErrors=@()
    try { if($cacheScopeStarted -and (Test-Path -LiteralPath $cache)){Move-Item -LiteralPath $cache -Destination (Join-Path $out 'final-after.bin')} } catch {$restoreErrors += "final cache: $_"}
    try { if($cacheScopeStarted -and (Test-Path -LiteralPath $part)){Move-Item -LiteralPath $part -Destination (Join-Path $out 'part-after.bin')} } catch {$restoreErrors += "orphan part: $_"}
    try { if($cacheMoved){Move-Item -LiteralPath $original -Destination $cache} } catch {$restoreErrors += "original cache: $_"}
    try { if($partMoved){Move-Item -LiteralPath $originalPart -Destination $part} } catch {$restoreErrors += "original part: $_"}
    try { if($hadCache -and (Get-FileHash -LiteralPath $cache -Algorithm SHA256).Hash -ne $originalHash){throw 'Hash differs from original.'} } catch {$restoreErrors += "cache hash: $_"}
    try { if($fixtureBackup){& $fixture -Mode Restore -Backup $fixtureBackup | Write-Output} } catch {$restoreErrors += "model folders: $_"}
    $env:VRLGH_GAMES=$oldGames
    Write-Output "Runtime output: $out"
    $remaining=@(Get-CimInstance Win32_Process -Filter "Name = 'lupin-vr-player.exe'")
    Write-Output "Clone player processes remaining: $($remaining.Count)"
    if($restoreErrors.Count){throw "Restore needs attention: $($restoreErrors -join '; ')"}
}
