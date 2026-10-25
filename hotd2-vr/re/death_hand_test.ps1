# Build real hand policy and trigger/reload input blocks with runtime/pose boundaries replaced.
param([string]$Output = '')
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
if (-not $Output) { $Output = Join-Path $repo 'build-win\vrtest\death-hand-unit' }
$out = [IO.Path]::GetFullPath($Output)
$build = [IO.Path]::GetFullPath((Join-Path $repo 'build-win')) + '\'
if (-not $out.StartsWith($build, [StringComparison]::OrdinalIgnoreCase)) { throw 'Test output must stay under build-win.' }
$null = New-Item -ItemType Directory -Path $out -Force
function Read-Source([string]$name) { [IO.File]::ReadAllText((Join-Path $repo $name)).Replace("`r`n", "`n") }
function Function-Text([string]$source, [string]$signature) {
    $start = $source.IndexOf($signature, [StringComparison]::Ordinal)
    if ($start -lt 0) { throw "Missing production function: $signature" }
    $open = $source.IndexOf('{', $start)
    $depth = 1
    $end = $open + 1
    while ($depth -gt 0 -and $end -lt $source.Length) {
        if ($source[$end] -eq '{') { $depth++ }
        if ($source[$end] -eq '}') { $depth-- }
        $end++
    }
    if ($depth) { throw "Unbalanced production function: $signature" }
    $source.Substring($start, $end - $start)
}
function Write-Include([string]$name, [string]$source) { [IO.File]::WriteAllText((Join-Path $out $name), $source, [Text.UTF8Encoding]::new($false)) }
$iniH = Read-Source 'core\cfg\ini.h'
$iniCpp = Read-Source 'core\cfg\ini.cpp'
$iniH = [regex]::Replace($iniH, '(?m)^#include "oslib/storage.h"\n', '')
$iniCpp = [regex]::Replace($iniCpp, '(?m)^#include .*\n', '')
foreach ($signature in @('void IniFile::load(hostfs::File *file, bool cEscape)', 'void IniFile::save(hostfs::File *file) const')) {
    $iniCpp = $iniCpp.Replace((Function-Text $iniCpp $signature), '')
}
Write-Include 'death_ini_under_test.inc' ($iniH + "`n" + $iniCpp)
$optionH = Read-Source 'core\cfg\option.h'
$base = $optionH.Substring($optionH.IndexOf('class BaseOption'), $optionH.IndexOf('#ifdef LIBRETRO', $optionH.IndexOf('class BaseOption')) - $optionH.IndexOf('class BaseOption'))
$start = $optionH.IndexOf('class Settings')
$end = $optionH.IndexOf("`n#endif", $start)
$options = $base + $optionH.Substring($start, $end - $start)
$optionCpp = Read-Source 'core\cfg\option.cpp'
$setting = [regex]::Match($optionCpp, '(?m)^Option<[^\n]+ VrSwapP1P2\([^\n]+').Value
if ($setting) { $feature = '#define DEATH_SWAP_OPTION 1' } else {
    $setting = [regex]::Match($optionCpp, '(?m)^Option<[^\n]+ VrP1Hand\([^\n]+').Value
    $feature = '#define DEATH_SWAP_OPTION 0'
}
if (-not $setting) { throw 'Missing hand option declaration.' }
Write-Include 'death_option_under_test.inc' ($feature + "`nnamespace config {`n" + $options + "`n" + $setting + "`n}")
$hostSource = Read-Source 'core\rend\vr\xr_host.cpp'
$gunStart = $hostSource.IndexOf("struct Gun`n")
$gunEnd = $hostSource.IndexOf('bool dualWield;', $gunStart)
Write-Include 'death_gun_under_test.inc' ($hostSource.Substring($gunStart, $gunEnd - $gunStart))
$updateGun = Function-Text $hostSource 'void updateGun(Gun& g, XrTime time'
$begin = $updateGun.IndexOf('const float pull = pulls[g.hand];')
$end = $updateGun.IndexOf('glm::vec2 screen', $begin)
$head = $updateGun.Substring($begin, $end - $begin)
$begin = $updateGun.IndexOf('const bool outside = !onScreen && aimOutside;')
$end = $updateGun.IndexOf('// Trigger + B + Start', $begin)
$inputBody = $updateGun.Substring($begin, $end - $begin)
$begin = $updateGun.LastIndexOf('g.triggerWas = trigger;')
$end = $updateGun.IndexOf('const glm::ivec2 pos', $begin)
$tail = $updateGun.Substring($begin, $end - $begin)
$inputBoundary = 'void updateGunBoundary(Gun& g, XrTime time, const float *pulls, u32 extra) {' + "`n" + $head + "`n" + 'const bool onScreen = testAimOnScreen, aimOutside = testAimOutside;' + "`n" + $inputBody + "`n" + $tail + "`nsentButtons[g.player] = buttons;`n}"
Write-Include 'death_update_gun_input_under_test.inc' $inputBoundary
$releaseDeclaration = [regex]::Match($hostSource, '(?m)^bool triggerReleaseWait\[2\][^\n]*').Value
Write-Include 'death_physical_trigger_under_test.inc' $(if ($releaseDeclaration) { "#define PHYSICAL_RELEASE_LATCH 1`n" + $releaseDeclaration } else { '#define PHYSICAL_RELEASE_LATCH 0' })
$hostFunctions = ''
foreach ($signature in @('bool triggerHeld(const Gun& g, float pull)', 'Holds holds(int hand)', 'int p1Hand()')) {
    $hostFunctions += (Function-Text $hostSource $signature) + "`n"
}
# Optional production helpers introduced when the policy is separated from runtime calls.
foreach ($signature in @('void suppressGunInput(Gun& g)', 'void swapGunHands()', 'void resetPlayerHands()')) {
    if ($hostSource.Contains($signature)) { $hostFunctions += (Function-Text $hostSource $signature) + "`n" }
}
$hostFunctions += Function-Text $hostSource 'void updateLightgun(XrTime time)'
Write-Include 'death_host_under_test.inc' $hostFunctions
$menuSource = Read-Source 'core\rend\vr\xr_menu.cpp'
$changeAt = $menuSource.IndexOf('void change(int row, int dir, int hand)')
$caseAt = $menuSource.IndexOf("`tcase Hands:", $changeAt)
$caseEnd = $menuSource.IndexOf("`tcase Laser:", $caseAt)
Write-Include 'death_menu_under_test.inc' ("void toggleHands() { switch (0) {`n" + $menuSource.Substring($caseAt, $caseEnd - $caseAt).Replace('case Hands:', 'case 0:') + "`n} }")
$labelsSource = Read-Source 'core\rend\vr\xr_labels.cpp'
$labelStart = $labelsSource.IndexOf('using controllers::Button;')
$labelEnd = $labelsSource.IndexOf('// The log', $labelStart)
Write-Include 'death_labels_under_test.inc' ($labelsSource.Substring($labelStart, $labelEnd - $labelStart) + (Function-Text $labelsSource 'const Words& wordsFor(const char *profile)'))
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsroot = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsroot) { throw 'MSVC C++ build tools not found.' }
$envCmd = Join-Path $vsroot 'VC\Auxiliary\Build\vcvars64.bat'
$cpp = Join-Path $PSScriptRoot 'death_hand_test.cpp'
$glm = Join-Path $repo 'core\deps\glm'
$exe = Join-Path $out 'death_hand_test.exe'
$log = Join-Path $out 'build.log'
Push-Location $out
try {
    & $env:ComSpec /d /c "call `"$envCmd`" >nul && cl /nologo /std:c++17 /EHsc /O2 /W3 /I`"$glm`" /I`"$out`" /Fe`"$exe`" `"$cpp`" >`"$log`" 2>&1"
    if ($LASTEXITCODE -ne 0) { Get-Content -LiteralPath $log; throw 'Death hand test did not compile.' }
    & $exe
    $code = $LASTEXITCODE
} finally { Pop-Location }
exit $code