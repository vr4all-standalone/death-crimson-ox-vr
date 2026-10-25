$ErrorActionPreference='Stop'
$repo='E:\___flattovrdev\death-crimson-ox-vr'
$out=Join-Path $repo 'build-win\vrtest\death-feedback-unit'
New-Item -ItemType Directory -Path $out -Force|Out-Null
$s=[IO.File]::ReadAllText((Join-Path $repo 'core\rend\vr_reproject.cpp')).Replace("`r`n","`n")
$start=$s.IndexOf("`t`tnow.fired = was.fired;")
$finish=$s.IndexOf('now.fired += rises;',$start)+'now.fired += rises;'.Length
if($start-lt 0 -or $finish-lt $start){throw 'Production feedback snapshot block missing'}
[IO.File]::WriteAllText((Join-Path $out 'feedback_under_test.inc'),$s.Substring($start,$finish-$start)+"`n}")
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsroot=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$envCmd=Join-Path $vsroot 'VC\Auxiliary\Build\vcvars64.bat'
$cpp=Join-Path $repo 'hotd2-vr\re\death_feedback_test.cpp'
Push-Location $out
try{& $env:ComSpec /d /c "call `"$envCmd`" >nul && cl /nologo /std:c++17 /EHsc /O2 /I`"$out`" /Fefeedback.exe `"$cpp`" >build.log 2>&1";if($LASTEXITCODE-ne 0){Get-Content build.log;throw 'Feedback regression did not compile'};& .\feedback.exe;$code=$LASTEXITCODE}finally{Pop-Location}
exit $code
