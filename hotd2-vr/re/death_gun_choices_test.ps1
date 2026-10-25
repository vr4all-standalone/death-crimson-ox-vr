# Isolated real gunChoices() test; model/source/directory metadata are boundary fixtures.
param([string]$Output='')
$ErrorActionPreference='Stop'
$repo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
if(-not $Output){$Output=Join-Path $repo 'build-win\vrtest\death-gun-choices-unit'}
$out=[IO.Path]::GetFullPath($Output)
$build=[IO.Path]::GetFullPath((Join-Path $repo 'build-win'))+'\'
if(-not $out.StartsWith($build,[StringComparison]::OrdinalIgnoreCase)){throw 'Test output must stay under build-win.'}
$null=New-Item -ItemType Directory -Force -Path $out
$source=[IO.File]::ReadAllText((Join-Path $repo 'core\rend\vr\xr_hands.cpp'))
$start=$source.IndexOf('std::vector<GunChoice> gunChoices()')
$end=$source.IndexOf('void drawHands(', $start)
if($start -lt 0 -or $end -le $start){throw 'Missing real gunChoices boundaries.'}
[IO.File]::WriteAllText((Join-Path $out 'death_gun_choices_under_test.inc'),$source.Substring($start,$end-$start),[Text.UTF8Encoding]::new($false))
$header=[IO.File]::ReadAllText((Join-Path $repo 'core\rend\vr\xr_hands.h'))
$start=$header.IndexOf('struct GunChoice')
$end=$header.IndexOf('std::vector<GunChoice> gunChoices();', $start)
if($start -lt 0 -or $end -le $start){throw 'Missing real GunChoice boundaries.'}
[IO.File]::WriteAllText((Join-Path $out 'death_gun_choice_type_under_test.inc'),$header.Substring($start,$end-$start),[Text.UTF8Encoding]::new($false))
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsroot=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $vsroot){throw 'MSVC build tools not found.'}
$envCmd=Join-Path $vsroot 'VC\Auxiliary\Build\vcvars64.bat'
$cpp=Join-Path $PSScriptRoot 'death_gun_choices_test.cpp'
$exe=Join-Path $out 'death_gun_choices_test.exe'
$log=Join-Path $out 'build.log'
Push-Location $out
try{
 & $env:ComSpec /d /c "call `"$envCmd`" >nul && cl /nologo /std:c++17 /EHsc /O2 /W3 /I`"$out`" /Fe`"$exe`" `"$cpp`" >`"$log`" 2>&1"
 if($LASTEXITCODE -ne 0){Get-Content -LiteralPath $log;throw 'Gun choices test failed to compile.'}
 & $exe
 $code=$LASTEXITCODE
}finally{Pop-Location}
exit $code