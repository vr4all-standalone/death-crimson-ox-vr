# Compiles the real gunModel entry path without an emulator or GL context.
param([string]$Output = '')
$ErrorActionPreference='Stop'
$repo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
if(-not $Output){$Output=Join-Path $repo 'build-win\vrtest\lp-render-cache-unit'}
$out=[IO.Path]::GetFullPath($Output)
$build=[IO.Path]::GetFullPath((Join-Path $repo 'build-win'))+'\'
if(-not $out.StartsWith($build,[StringComparison]::OrdinalIgnoreCase)){throw 'Standalone output must stay under build-win.'}
$null=New-Item -ItemType Directory -Path $out -Force
$text=[IO.File]::ReadAllText((Join-Path $repo 'core\rend\vr\xr_hands.cpp'))
$start=$text.IndexOf('const HandsModel *gunModel(GunKind kind)')
$end=$text.IndexOf('bool gunComplete(GunKind kind)', $start)
if($start -lt 0 -or $end -le $start){throw 'Cannot find exact gunModel function boundaries.'}
[IO.File]::WriteAllText((Join-Path $out 'lupin_gun_model_under_test.inc'),$text.Substring($start,$end-$start),[Text.UTF8Encoding]::new($false))
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsroot=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $vsroot){throw 'MSVC C++ build tools were not found.'}
$envCmd=Join-Path $vsroot 'VC\Auxiliary\Build\vcvars64.bat'
$cpp=Join-Path $PSScriptRoot 'lupin_render_cache_test.cpp'
$builder=Join-Path $repo 'core\rend\vr\hands_build.cpp'
$vr=Join-Path $repo 'core\rend\vr'
$glm=Join-Path $repo 'core\deps\glm'
$exe=Join-Path $out 'lupin_render_cache_test.exe'
$log=Join-Path $out 'build.log'
Push-Location $out
try {
    & $env:ComSpec /d /c "call `"$envCmd`" >nul && cl /nologo /std:c++17 /EHsc /O2 /W3 /I`"$vr`" /I`"$glm`" /I`"$out`" /Fe`"$exe`" `"$cpp`" `"$builder`" >`"$log`" 2>&1"
    if($LASTEXITCODE -ne 0){Get-Content -LiteralPath $log;throw 'Standalone renderer-cache test did not compile.'}
    & $exe
    $code=$LASTEXITCODE
} finally {Pop-Location}
exit $code
