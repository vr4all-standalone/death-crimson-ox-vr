param([switch]$LocalFixture)
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent $PSScriptRoot
$jdk = if($env:JAVA_HOME){$env:JAVA_HOME}else{'E:\.gradle\jdks\eclipse_adoptium-21-amd64-windows.2'}
$gradle=if($env:GRADLE_USER_HOME){$env:GRADLE_USER_HOME}else{'E:\.gradle'}
$junit=Get-ChildItem "$gradle\caches\modules-2\files-2.1\junit\junit" -Recurse -Filter '*.jar' | Select-Object -First 1 -ExpandProperty FullName
$hamcrest=Get-ChildItem "$gradle\caches\modules-2\files-2.1\org.hamcrest\hamcrest-core" -Recurse -Filter '*.jar' | Select-Object -First 1 -ExpandProperty FullName
$out=Join-Path $repo 'build-win\vrtest\death-setup-unit'
New-Item -ItemType Directory -Path $out -Force | Out-Null
$java=Join-Path $repo 'shell\android-studio\flycast\src\main\java\com\flycast\emulator'
$tests=Join-Path $repo 'shell\android-studio\flycast\src\test\java\com\flycast\emulator'
$files=@("$java\DeathCrimsonFiles.java","$java\DeathCrimsonStartStatus.java","$tests\DeathCrimsonFilesTest.java","$tests\DeathCrimsonStartStatusTest.java","$tests\DeathCrimsonLocalFixtureTest.java")
& "$jdk\bin\javac.exe" --release 11 -cp "$junit;$hamcrest" -d $out @files
if($LASTEXITCODE){throw 'javac failed'}
if($LocalFixture){$fixture=@("-Ddeath.fixture=$(Join-Path $repo 'build-win\game-data')")}else{$fixture=@()}
& "$jdk\bin\java.exe" @fixture -Xmx512m -cp "$out;$junit;$hamcrest" org.junit.runner.JUnitCore com.flycast.emulator.DeathCrimsonFilesTest com.flycast.emulator.DeathCrimsonStartStatusTest com.flycast.emulator.DeathCrimsonLocalFixtureTest
exit $LASTEXITCODE