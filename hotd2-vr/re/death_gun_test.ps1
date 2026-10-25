param([string]$Output='')
$ErrorActionPreference='Stop'
$repo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
if(-not $Output){$Output=Join-Path $repo 'build-win\vrtest\death-gun-unit'}
$out=[IO.Path]::GetFullPath($Output)
$build=[IO.Path]::GetFullPath((Join-Path $repo 'build-win'))+'\'
if(-not $out.StartsWith($build,[StringComparison]::OrdinalIgnoreCase)){throw 'Test output must stay under build-win.'}
$null=New-Item -ItemType Directory -Path $out -Force
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsroot=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $vsroot){throw 'MSVC C++ build tools not found.'}
$envCmd=Join-Path $vsroot 'VC\Auxiliary\Build\vcvars64.bat'
$cpp=Join-Path $PSScriptRoot 'death_gun_test.cpp'
$exe=Join-Path $out 'death_gun_test.exe'
$log=Join-Path $out 'build.log'
Push-Location $out
try{
 & $env:ComSpec /d /c "call `"$envCmd`" >nul && cl /nologo /std:c++17 /EHsc /O2 /W4 /Fe`"$exe`" `"$cpp`" >`"$log`" 2>&1"
 if($LASTEXITCODE -ne 0){Get-Content -LiteralPath $log;throw 'Death gun test did not compile.'}
 & $exe
 $code=$LASTEXITCODE
}finally{Pop-Location}
exit $code