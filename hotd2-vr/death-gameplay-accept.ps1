<# Private controlled gameplay acceptance, never a shipped configuration.
   Uses owned Japan Rev A data under build-win/game-data. Temporarily raises health
   and skips only the native damage-induced ammo refill in the running test process.
   The harness restores private configuration/saves and stops only its owned PID.
   Do not run concurrently with other death-test scenarios in this checkout. #>
param([ValidateRange(1024,65535)][int]$Port=27116,[switch]$Reuse)
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent $PSScriptRoot
$env:VRLGH_GAMES=Join-Path $repo 'build-win\game-data'
$at=@(); foreach($t in 30..100 | Where-Object {$_%5-eq 0}){$at+="$t=xr left/x 1 800"}
$on=@('VR: P1 magazine 0 -> 10|0|poke 8c03caf6 2 0009','VR: P1 magazine 0 -> 10|0|poke 0c0974ac 4 42c80000','VR: P1 magazine 0 -> 10|0|mute 180')
for($i=0;$i -lt 12;$i++){$t=35+$i*1.3; $on+="VR: P1 magazine 0 -> 10|$t|xr left/trigger 1 400"}
$on+=@('VR: P1 magazine 0 -> 10|51|xr left/y 1 650','VR: P1 magazine 0 -> 10|52.5|xr left/trigger 1 400','VR: P1 magazine 0 -> 10|54|xr right/a 1 600')
$on+=@('VR: P2 magazine 0 -> 10|0|poke 0c0974b0 4 42c80000')
for($i=0;$i -lt 12;$i++){$t=3+$i*1.3; $on+="VR: P2 magazine 0 -> 10|$t|xr right/trigger 1 400"}
$on+=@('VR: P2 magazine 0 -> 10|19|xr right/b 1 650','VR: P2 magazine 0 -> 10|20.5|xr right/trigger 1 400',
 'VR: P2 magazine 0 -> 10|22|shot dual-combat','VR: P2 magazine 0 -> 10|23|xr left/menu 1 250',
 'VR: P2 magazine 0 -> 10|24|shot paused-1','VR: P2 magazine 0 -> 10|25|shot paused-2',
 'VR: P2 magazine 0 -> 10|26|pose head 0 1.6 0 25 0','VR: P2 magazine 0 -> 10|27|shot paused-head',
 'VR: P2 magazine 0 -> 10|28|pose head 0 1.6 0 0 0','VR: P2 magazine 0 -> 10|29|xr right/b 1 250',
 'VR: P2 magazine 0 -> 10|30|shot resumed-1','VR: P2 magazine 0 -> 10|32|shot resumed-2',
 'VR: P2 magazine 0 -> 10|33|xrstate visible','VR: P2 magazine 0 -> 10|34|shot focus-1','VR: P2 magazine 0 -> 10|35|shot focus-2',
 'VR: P2 magazine 0 -> 10|36|xrstate focused','VR: P2 magazine 0 -> 10|37|xr right/trigger 1 5000',
 'VR: P2 magazine 0 -> 10|38|poke 0c0974b0 4 00000000','VR: P2 magazine 0 -> 10|44|xr right/trigger 1 400',
 'VR: P2 magazine 0 -> 10|45.5|xr left/trigger 1 400')
if(-not $Reuse){
& (Join-Path $repo 'hotd2-vr\death-test.ps1') -Game deathcoxj -Name dc-magazines-accept -Port $Port -Seconds 134 -XrSim -Visible `
 -Config 'config:vr.GameHands=no,config:vr.FovScale=2,config:vr.WidenFov=yes,config:vr.Gun=4,config:vr.GunFile=namco,config:vr.WatchRam=0c18bfc9:1,0c097ae0:4,0c18c005:1,0c097ae4:4' `
 -At $at -On $on -Expect @('VR: P1 magazine 0 -> 10','XR: P1 kick:','XR: P2 kick:',
 'VR: P2 magazine 0 -> 10','XR: player 1 reload button \(sensor\)','XR: player 2 reload button \(sensor\)',
 'XR: player 2 joins: a gun in each hand','XR: player 2 is out: one gun again','XR: the gun is in the right hand','XR: the gun is in the left hand') -Reject @('cannot bind','initialisation failed')
if($LASTEXITCODE){exit $LASTEXITCODE}
}
$out=Join-Path $repo 'build-win\vrtest\dc-magazines-accept'
$log=Get-Content -LiteralPath (Join-Path $out 'flycast.log')
function Clock([string]$line){if($line -match '^(\d+):(\d+):(\d+)'){return [double]$Matches[1]*60+[double]$Matches[2]+[double]$Matches[3]/1000};return -1}
foreach($p in 1,2){
 $hand=if($p-eq 1){'left'}else{'right'}
 $address=if($p-eq 1){'0c097ae0'}else{'0c097ae4'}
 $ammo=if($p-eq 1){'0c18bfc9'}else{'0c18c005'}
 $kicks=@($log | Where-Object {$_ -match "XR: P$p kick:"}).Count
 $counters=@($log | Where-Object {$_ -match "VR: ram $address .* -> [0-9a-f]{8}$"})
 $accepted=[Convert]::ToInt32(($counters[-1] -split ' -> ')[-1],16)
 $clicks=@($log | Where-Object {$_ -match "haptic $hand 0\.019 0\.350"}).Count
 if($accepted -ne $kicks -or $accepted -lt 11 -or $clicks -lt 1 -or -not($log -match "VR: ram $ammo 01 -> 00")){throw "P$p magazine/feedback mismatch: accepted=$accepted, kicks=$kicks, empty=$clicks"}
 Write-Host "PASS P$p native accepted rounds=$accepted, matching kicks=$kicks, empty clicks=$clicks"
}
$left=Clock ($log | Where-Object {$_ -match 'XR: player 2 is out:'} | Select-Object -First 1)
if(@($log | Where-Object {(Clock $_)-ge $left -and (Clock $_)-lt ($left+3) -and $_-match 'XR: P1 kick:|XR: player 1 shot at'}).Count){throw 'Held P2 trigger leaked into solo P1'}
Write-Host 'PASS held P2 exit waits for physical trigger release'
function Shot([string]$label){(Get-ChildItem -LiteralPath $out -Filter "*-$label.png" | Select-Object -First 1).FullName}
$vrcheck=Join-Path $PSScriptRoot 're\vrcheck.py'
& python -I $vrcheck same (Shot 'paused-1') (Shot 'paused-2') --max-diff 1.5
if($LASTEXITCODE){throw 'Menu did not freeze the game'}
& python -I $vrcheck differ (Shot 'paused-2') (Shot 'paused-head') --min-diff 2
if($LASTEXITCODE){throw 'Head motion did not redraw the frozen game'}
& python -I $vrcheck differ (Shot 'resumed-1') (Shot 'resumed-2') --min-diff .5
if($LASTEXITCODE){throw 'Game did not resume'}
exit 0
