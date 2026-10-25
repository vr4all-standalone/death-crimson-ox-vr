package com.flycast.emulator;
import org.junit.Test;
import org.junit.Assume;
import static org.junit.Assert.*;
import java.io.File;
import java.util.Collections;
/** Opt-in validation of user-supplied data; never copied into test artifacts or distributions. */
public class DeathCrimsonLocalFixtureTest {
    @Test public void suppliedCartridgePassesTheRealLaunchValidator() {
        String path=System.getProperty("death.fixture");Assume.assumeNotNull(path);
        File dir=new File(path);
        DeathCrimsonFiles.Result r=DeathCrimsonFiles.find(Collections.singletonList(dir));
        assertTrue(r.missing.toString(),r.missing.isEmpty());
        assertEquals(new File(dir,"deathcoxj.zip").getAbsolutePath(),DeathCrimsonFiles.launchPath(null,Collections.singletonList(dir)));
    }
}