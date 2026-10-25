# Safety contract for a harness that runs alongside another game's final tests.
param([string]$Harness = (Join-Path $PSScriptRoot 'vr-test.ps1'))
$ErrorActionPreference = 'Stop'
$tokens = $null; $parseErrors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile($Harness, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw "Harness syntax errors: $parseErrors" }
$failures = @()
$globalStops = @($ast.FindAll({ param($node)
    $node -is [Management.Automation.Language.CommandAst] -and
    $node.GetCommandName() -eq 'Get-Process' -and
    $node.Extent.Text -match '\bflycast\b'
}, $true))
if ($globalStops.Count) { $failures += 'Harness enumerates every Flycast process instead of owning only its launched PID.' }
$port = @($ast.ParamBlock.Parameters | Where-Object { $_.Name.VariablePath.UserPath -eq 'Port' })
if ($port.Count -ne 1) { $failures += 'Harness has no configurable private UDP port.' }
$text = [IO.File]::ReadAllText($Harness)
if ($text -match '(?m)^\$port = 27016') { $failures += 'Harness hardcodes Confidential Mission test port 27016.' }
if ($text -notmatch '\[IO\.Path\]::GetFullPath') { $failures += 'Harness does not verify resolved run-folder containment before recursive cleanup.' }
if ($failures.Count) { $failures | ForEach-Object { Write-Output "FAIL $_" }; exit 1 }
Write-Output 'PASS safe harness: private configurable port, owned process cleanup, resolved output containment, valid syntax.'
exit 0
