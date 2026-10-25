# Standalone native model parser tests; the game fixture stays private under build-win.
param([string]$Output='', [string]$OwnedRomZip='')
$ErrorActionPreference='Stop'
$repo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
if(-not $Output){$Output=Join-Path $repo 'build-win\vrtest\death-model-unit'}
$out=[IO.Path]::GetFullPath($Output)
$build=[IO.Path]::GetFullPath((Join-Path $repo 'build-win'))+'\'
if(-not $out.StartsWith($build,[StringComparison]::OrdinalIgnoreCase)){throw 'Test output must stay under build-win.'}
if(-not $OwnedRomZip){$OwnedRomZip=Join-Path $repo 'build-win\game-data\deathcoxj.zip'}
$null=New-Item -ItemType Directory -Force -Path $out
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip=[IO.Compression.ZipFile]::OpenRead($OwnedRomZip)
try{
 $entry=$zip.GetEntry('mpr-23515.ic2')
 if(-not $entry -or $entry.Length -ne 0x800000){throw 'Expected owned 8 MiB mpr-23515.ic2'}
 $fixture=Join-Path $out 'owned-mpr-23515.ic2'
 [IO.Compression.ZipFileExtensions]::ExtractToFile($entry,$fixture,$true)
}finally{$zip.Dispose()}
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsroot=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $vsroot){throw 'MSVC build tools not found.'}
$envCmd=Join-Path $vsroot 'VC\Auxiliary\Build\vcvars64.bat'
$cpp=Join-Path $PSScriptRoot 'death_model_test.cpp'
$impl=Join-Path $repo 'core\rend\vr\death_model.cpp'
$inc=Join-Path $repo 'core\rend\vr'
$exe=Join-Path $out 'death_model_test.exe'
$log=Join-Path $out 'build.log'
Push-Location $out
try{
 & $env:ComSpec /d /c "call `"$envCmd`" >nul && cl /nologo /std:c++17 /EHsc /O2 /W4 /I`"$inc`" /Fe`"$exe`" `"$cpp`" `"$impl`" >`"$log`" 2>&1"
 if($LASTEXITCODE -ne 0){Get-Content -LiteralPath $log;throw 'Death model tests failed to compile.'}
 & $exe $fixture (Join-Path $out 'gun-dc-pistol.bin')
 $code=$LASTEXITCODE
}finally{Pop-Location}
exit $code
