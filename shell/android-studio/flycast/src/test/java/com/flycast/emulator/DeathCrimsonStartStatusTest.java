package com.flycast.emulator;
import org.junit.Test;
import static org.junit.Assert.*;
import java.util.Arrays;
public class DeathCrimsonStartStatusTest {
    @Test public void completeCartridgeMarksTwoFilesWithoutChd() {DeathCrimsonFiles.Result r=new DeathCrimsonFiles.Result();DeathCrimsonStartStatus s=DeathCrimsonStartStatus.from(r,true);assertTrue(s.ready);assertEquals(Arrays.asList("deathcoxj.zip ✓","naomi.zip ✓"),s.fileMarks);}
    @Test public void missingChipCannotShowReady() {DeathCrimsonFiles.Result r=new DeathCrimsonFiles.Result();r.missing.add("deathcoxj.zip: needs mpr-23523.ic10");DeathCrimsonStartStatus s=DeathCrimsonStartStatus.from(r,true);assertFalse(s.ready);assertEquals("MISSING FILES",s.badge);assertEquals(1,s.missing.size());assertTrue(s.missing.get(0).contains("mpr-23523.ic10"));}
    @Test public void missingBiosExplainsLocation() {DeathCrimsonFiles.Result r=new DeathCrimsonFiles.Result();r.missing.add("naomi.zip: beside deathcoxj.zip or inside it");DeathCrimsonStartStatus s=DeathCrimsonStartStatus.from(r,true);assertFalse(s.ready);assertTrue(s.missing.get(0).contains("beside deathcoxj.zip"));}
    @Test public void ownStorageCanBeReadyWithoutSharedAccess(){assertTrue(DeathCrimsonStartStatus.from(new DeathCrimsonFiles.Result(),false).ready);}
    @Test public void sharedAccessRequestedOnlyWhenFilesIncomplete(){DeathCrimsonFiles.Result r=new DeathCrimsonFiles.Result();r.missing.add("deathcoxj.zip: missing");assertEquals("ALLOW ACCESS",DeathCrimsonStartStatus.from(r,false).badge);}
    @Test public void initialStatusIsChecking(){assertEquals("CHECKING",DeathCrimsonStartStatus.from(null,true).badge);assertEquals("ALLOW ACCESS",DeathCrimsonStartStatus.from(null,false).badge);}
    @Test public void ownVersionIsReadable(){assertEquals("v1.0",DeathCrimsonStartStatus.shortVersion("1.0.0-gabc123"));assertEquals("",DeathCrimsonStartStatus.shortVersion(null));}
}