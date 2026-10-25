$ErrorActionPreference='Stop'
$repo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$out=Join-Path $repo 'build-win\vrtest\death-prep-unit'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsroot=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $vsroot){throw 'MSVC build tools not found.'}
$envCmd=Join-Path $vsroot 'VC\Auxiliary\Build\vcvars64.bat'
$cpp=Join-Path $PSScriptRoot 'death_prep_test.cpp'
$exe=Join-Path $out 'death_prep_test.exe'
$log=Join-Path $out 'build.log'
Push-Location $out
try{
 & $env:ComSpec /d /c "call `"$envCmd`" >nul && cl /nologo /std:c++17 /EHsc /O2 /W3 /I`"$repo\core\rend\vr`" /Fe`"$exe`" `"$cpp`" >`"$log`" 2>&1"
 if($LASTEXITCODE -ne 0){Get-Content -LiteralPath $log;throw 'Gun choices test failed to compile.'}
 & $exe
 $code=$LASTEXITCODE
}finally{Pop-Location}
exit $code