# SPDX-License-Identifier: GPL-2.0-or-later
# Read-only APK verification. No signing-key reads, builds, installs, or publishing.
param([string]$Apk)
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent $PSScriptRoot
if(-not $Apk){$Apk=Join-Path $repo 'shell\android-studio\flycast\build\outputs\apk\vr\flycast-vr.apk'}
$Apk=[IO.Path]::GetFullPath($Apk)
$packageScript=Join-Path $PSScriptRoot 'package-release.ps1'
$tokens=$null;$errors=$null
$ast=[Management.Automation.Language.Parser]::ParseFile($packageScript,[ref]$tokens,[ref]$errors)
if($errors.Count){throw ($errors|Out-String)}
foreach($statement in $ast.EndBlock.Statements){
    if($statement -is [Management.Automation.Language.FunctionDefinitionAst]){
        . ([scriptblock]::Create($statement.Extent.Text))
    }elseif($statement -is [Management.Automation.Language.AssignmentStatementAst] -and
            $statement.Left.VariablePath.UserPath -in @('appId','appLabel','appVersion','appLauncher','playCmd','bannedNames','bannedDirs')){
        . ([scriptblock]::Create($statement.Extent.Text))
    }
}
Add-Type -AssemblyName System.IO.Compression.FileSystem
$taskSdk=if($env:ANDROID_HOME){$env:ANDROID_HOME}else{'E:\AndroidSdk'}
$taskJdk=if($env:JAVA_HOME){$env:JAVA_HOME}else{'E:\.gradle\jdks\eclipse_adoptium-21-amd64-windows.2'}
$tools=Get-ChildItem (Join-Path $taskSdk 'build-tools') -Directory |
    Where-Object {Test-Path (Join-Path $_.FullName 'aapt2.exe')} |
    Sort-Object {[version]$_.Name} -Descending | Select-Object -First 1
$env:JAVA_HOME=$taskJdk
& (Join-Path $tools.FullName 'apksigner.bat') verify $Apk
if($LASTEXITCODE){throw 'APK signature verification failed'}
$badging=@(& (Join-Path $tools.FullName 'aapt2.exe') dump badging $Apk)
if($LASTEXITCODE){throw 'aapt2 badging failed'}
$tree=@(& (Join-Path $tools.FullName 'aapt2.exe') dump xmltree --file AndroidManifest.xml $Apk)
if($LASTEXITCODE){throw 'aapt2 manifest read failed'}
$faults=@(Find-AppIdentityFault $badging $tree '' $true)
if($faults.Count){throw ($faults -join "`n")}
if(-not (Test-StandalonePolicy (Read-ZipEntry $Apk 'lib/arm64-v8a/libflycast.so'))){throw 'APK native standalone policy missing'}
if(-not (Test-Archives @($Apk))){throw 'APK content/license checks failed'}
if(-not (Test-ModelsUntracked)){throw 'Bundled models unexpectedly tracked'}
$zip=[IO.Compression.ZipFile]::OpenRead($Apk)
try {
    $inside=@($zip.Entries | Where-Object {$_.Name -and $_.FullName.StartsWith('assets/models/')} |
        ForEach-Object {$_.FullName.Substring('assets/models/'.Length)} | Sort-Object)
}finally{$zip.Dispose()}
$models=Join-Path $PSScriptRoot 'models'
$expected=@()
if(Test-Path -LiteralPath $models){
    $expected=@(Get-ChildItem -LiteralPath $models -Recurse -File | Where-Object {$_.Extension -in @('.glb','.json') -or $_.Name -eq 'CREDITS.txt'} |
        ForEach-Object {$_.FullName.Substring($models.Length+1).Replace('\','/')} | Sort-Object)
}
if(($inside -join '|') -ne ($expected -join '|')){throw 'APK model assets differ from the independently licensed source folder'}
$badging | Where-Object {$_ -match '^package:|^application-label:'} | Write-Host
Write-Host "APK verified: identity, launcher, signature, native policy, game-data exclusions, licenses, and $($inside.Count) model files."
Get-FileHash -LiteralPath $Apk -Algorithm SHA256 | Format-List