# Production adapter end-to-end test. Game data and generated dumps stay local/ignored.
param([switch]$Stock, [switch]$Reuse)
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent $PSScriptRoot
$name=if($Stock){'lp-aim-stock'}else{'lp-aim-wide'}
$wide=if($Stock){'no'}else{'yes'}
$key='VR: ram 0ca4c808 .* -> 40e00000'
$on=@()
$points=@(@(128,96),@(320,96),@(512,96),@(128,240),@(320,240),@(512,240),@(128,384),@(320,384),@(512,384),@(0,0),@(639,479))
for($i=0;$i -lt $points.Count;$i++) {
    $t=1+$i*.7; if($i -ge 6){$t+=.5}
    $x=[int][Math]::Ceiling($points[$i][0]*10000/640); $y=[int][Math]::Ceiling($points[$i][1]*10000/480)
    $on+="$key|$t|fire 0 $x $y"
    $on+="$key|$($t+.18)|rip 1 1 1"
}
$on+="$key|4.8|button 0 2 200"
$on+="$key|9.2|button 1 4 200"
$p2=@(@(128,96),@(320,240),@(512,384),@(0,0),@(639,479))
for($i=0;$i -lt $p2.Count;$i++) {
    $t=10.4+$i*.7
    $x=[int][Math]::Ceiling($p2[$i][0]*10000/640); $y=[int][Math]::Ceiling($p2[$i][1]*10000/480)
    $on+="$key|$t|fire 1 $x $y"
    $on+="$key|$($t+.18)|rip 1 1 1"
}
if(-not $Reuse) {
    & "$PSScriptRoot\vr-test.ps1" -Game lupinshoo -Name $name -Port 27036 -Seconds 120 -Visible `
        -Config "config:vr.GameHands=no,config:vr.FovScale=2,config:vr.WidenFov=$wide,config:vr.WatchRam=0ca4c808:4,0ca4c828:4,0ca4c82c:4" `
        -At @('60=start','64=button 0 4 1000','70=start','100=shot floor','101=rip 1 1 1','115=shot dual') -On $on `
        -Expect @('VR: P1 calibration inverse','VR: P2 calibration inverse','VR: P1 active on','VR: P2 active on')
    if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
}
& python -I "$PSScriptRoot\re\lupin_aim_check.py" (Join-Path $repo "build-win\vrtest\$name") --focal $(if($Stock){879.4259033203125}else{381.4189147949219})
exit $LASTEXITCODE
