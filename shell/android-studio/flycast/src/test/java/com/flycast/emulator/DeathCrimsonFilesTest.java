package com.flycast.emulator;

import org.junit.*;
import org.junit.rules.TemporaryFolder;
import static org.junit.Assert.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.zip.*;

/** Synthetic CRC-matched stand-ins only; never reads or stores game data. */
public class DeathCrimsonFilesTest {
    @Rule public TemporaryFolder tmp = new TemporaryFolder();
    static final String[] NAMES = { "epr-23524a.ic22", "mpr-23514.ic1", "mpr-23515.ic2", "mpr-23516.ic3", "mpr-23517.ic4", "mpr-23518.ic5", "mpr-23519.ic6", "mpr-23520.ic7", "mpr-23521.ic8", "mpr-23522.ic9", "mpr-23523.ic10" };
    static final long[] CRCS = {0xb60e113eL,0x1f2b090eL,0xdc8557ebL,0x94494cbbL,0x69ba6a41L,0x49882766L,0xcdc82805L,0x1a268360L,0xcf8674b8L,0x7ae6716eL,0xc91efb67L};
    static final Map<String,byte[]> DATA = new LinkedHashMap<>();
    @BeforeClass public static void fixtures() {
        for(int i=0;i<NAMES.length;i++) DATA.put(NAMES[i], forge(i==0?0x400000:0x800000,CRCS[i]));
        DATA.put("epr-21576h.ic27", forge(0x200000,0xd4895685L));
    }
    // Solve the 32 CRC influence bits of the final four bytes over GF(2).
    static byte[] forge(int size,long wanted) {
        byte[] bytes=new byte[size]; CRC32 crc=new CRC32(); crc.update(bytes); int base=(int)crc.getValue();
        int[] values=new int[32], masks=new int[32];
        for(int bit=0;bit<32;bit++) {
            bytes[size-4+bit/8]=(byte)(1<<(bit%8)); crc.reset(); crc.update(bytes); int value=(int)crc.getValue()^base, mask=1<<bit;
            bytes[size-4+bit/8]=0;
            for(int b=31;b>=0;b--) if((value&(1<<b))!=0) {
                if(values[b]==0) {values[b]=value;masks[b]=mask;break;} value^=values[b];mask^=masks[b];
            }
        }
        int value=(int)wanted^base, mask=0;
        for(int b=31;b>=0;b--) if((value&(1<<b))!=0) {value^=values[b];mask^=masks[b];}
        for(int bit=0;bit<32;bit++) if((mask&(1<<bit))!=0) bytes[size-4+bit/8]|=(byte)(1<<(bit%8));
        crc.reset();crc.update(bytes);assertEquals(wanted,crc.getValue());return bytes;
    }
    File zip(File root,String name,Map<String,byte[]> entries)throws Exception {
        File file=new File(root,name); root.mkdirs();
        try(ZipOutputStream out=new ZipOutputStream(new FileOutputStream(file))) {
            out.setLevel(1);
            for(Map.Entry<String,byte[]> e:entries.entrySet()) {out.putNextEntry(new ZipEntry(e.getKey()));out.write(e.getValue());out.closeEntry();}
        }return file;
    }
    Map<String,byte[]> game() {Map<String,byte[]> files=new LinkedHashMap<>(DATA);files.remove("epr-21576h.ic27");return files;}
    File complete()throws Exception {File dir=tmp.newFolder();zip(dir,"deathcoxj.zip",game());bios(dir);return dir;}
    void bios(File dir)throws Exception {zip(dir,"naomi.zip",Collections.singletonMap("epr-21576h.ic27",DATA.get("epr-21576h.ic27")));}
    DeathCrimsonFiles.Result find(File dir){return DeathCrimsonFiles.find(Collections.singletonList(dir));}
    @Test public void completeJapanRevACartridgeNeedsNoDisc()throws Exception {File dir=complete();assertTrue(find(dir).missing.toString(),find(dir).missing.isEmpty());assertNotNull(DeathCrimsonFiles.launchPath(null,Collections.singletonList(dir)));}
    @Test public void allElevenChipsRequired()throws Exception {
        File dir=tmp.newFolder();bios(dir);
        for(String name:NAMES) {
            Map<String,byte[]> f=game();f.remove(name);zip(dir,"deathcoxj.zip",f);
            assertNull(name,DeathCrimsonFiles.launchPath(new File(dir,"deathcoxj.zip"),Collections.emptyList()));
        }
    }
    @Test public void exactLengthRequired()throws Exception {File dir=tmp.newFolder();Map<String,byte[]> f=game();f.put("epr-23524a.ic22",new byte[16]);zip(dir,"deathcoxj.zip",f);bios(dir);assertFalse(find(dir).missing.isEmpty());}
    @Test public void correctlyNamedWrongProgramCrcRejected()throws Exception {File dir=tmp.newFolder();Map<String,byte[]> f=game();f.put("epr-23524a.ic22",new byte[0x400000]);zip(dir,"deathcoxj.zip",f);bios(dir);assertFalse(find(dir).missing.isEmpty());}
    @Test public void correctlyNamedWrongSharedChipCrcRejected()throws Exception {File dir=tmp.newFolder();Map<String,byte[]> f=game();f.put("mpr-23519.ic6",new byte[0x800000]);zip(dir,"deathcoxj.zip",f);bios(dir);assertFalse(find(dir).missing.isEmpty());}
    @Test public void mergedOldNameExplainsCorrectCopy()throws Exception {File dir=tmp.newFolder();zip(dir,"deathcox.zip",game());bios(dir);DeathCrimsonFiles.Result r=find(dir);assertFalse(r.missing.isEmpty());assertTrue(r.missing.toString(),r.missing.toString().contains("deathcoxj.zip"));assertNull(DeathCrimsonFiles.launchPath(new File(dir,"deathcox.zip"),Collections.emptyList()));}
    @Test public void originalJapanRevisionNotClaimed()throws Exception {File dir=tmp.newFolder();Map<String,byte[]> f=game();f.remove("epr-23524a.ic22");f.put("epr-23524.ic22",forge(0x400000,0xedc20e44L));zip(dir,"deathcoxo.zip",f);bios(dir);assertFalse(find(dir).missing.isEmpty());}
    @Test public void biosCanBeMergedInsideGame()throws Exception {File dir=tmp.newFolder();zip(dir,"deathcoxj.zip",DATA);assertTrue(find(dir).missing.toString(),find(dir).missing.isEmpty());}
    @Test public void biosMissingPreventsLaunch()throws Exception {File dir=tmp.newFolder();zip(dir,"deathcoxj.zip",game());assertFalse(find(dir).bios);assertNull(DeathCrimsonFiles.launchPath(null,Collections.singletonList(dir)));}
    @Test public void knownBiosNameWithWrongCrcRejected()throws Exception {File dir=tmp.newFolder();zip(dir,"deathcoxj.zip",game());zip(dir,"naomi.zip",Collections.singletonMap("epr-21576h.ic27",new byte[0x200000]));assertFalse(find(dir).bios);}
    @Test public void knownBiosCrcWithWrongLengthRejected()throws Exception {File dir=tmp.newFolder();zip(dir,"deathcoxj.zip",game());zip(dir,"naomi.zip",Collections.singletonMap("epr-21576h.ic27",forge(16,0xd4895685L)));assertFalse(find(dir).bios);}
    @Test public void corruptArchivePreventsLaunch()throws Exception {File dir=complete();File f=new File(dir,"deathcoxj.zip");try(RandomAccessFile out=new RandomAccessFile(f,"rw")){out.setLength(f.length()/2);}assertNull(DeathCrimsonFiles.launchPath(f,Collections.emptyList()));}
    @Test public void foreignExplicitLaunchDoesNotFallback()throws Exception {File dir=complete();File foreign=zip(dir,"lupinshoo.zip",game());assertNull(DeathCrimsonFiles.launchPath(foreign,Collections.singletonList(dir)));}
    @Test public void uppercaseNameCannotShowReadyOnCaseSensitiveStorage()throws Exception {File dir=tmp.newFolder();zip(dir,"DEATHCOXJ.ZIP",game());bios(dir);assertFalse(find(dir).missing.isEmpty());}
    @Test public void nestedCompleteSetWinsOverIncompleteSet()throws Exception {File root=tmp.newFolder();zip(root,"deathcoxj.zip",Collections.emptyMap());File nested=new File(root,"set");zip(nested,"deathcoxj.zip",game());bios(nested);assertEquals(new File(nested,"deathcoxj.zip").getAbsolutePath(),DeathCrimsonFiles.launchPath(null,Collections.singletonList(root)));}
    @Test public void traversalDepthBounded()throws Exception {File root=tmp.newFolder(),nested=new File(root,"a/b/c/d");zip(nested,"deathcoxj.zip",game());bios(nested);assertNull(DeathCrimsonFiles.launchPath(null,Collections.singletonList(root)));}
    @Test public void corruptedPayloadWithKnownMetadataRejected()throws Exception {File dir=complete();File f=new File(dir,"deathcoxj.zip");try(RandomAccessFile out=new RandomAccessFile(f,"rw")){out.seek(30+NAMES[0].length()+12);int b=out.read();out.seek(out.getFilePointer()-1);out.write(b^0x80);}assertNull(DeathCrimsonFiles.launchPath(f,Collections.emptyList()));}
    @Test public void moreThanEntryBudgetRejected()throws Exception {File dir=tmp.newFolder();Map<String,byte[]> f=new LinkedHashMap<>();for(int i=0;i<150;i++)f.put("filler"+i,new byte[0]);f.putAll(game());zip(dir,"deathcoxj.zip",f);bios(dir);assertFalse(find(dir).missing.isEmpty());}
    @Test public void corruptPreferredBiosCannotHideBehindValidFallback()throws Exception {
        File dir=tmp.newFolder();Map<String,byte[]> f=game();f.put("epr-21576h.ic27",new byte[0x200000]);zip(dir,"deathcoxj.zip",f);
        zip(dir,"naomi.zip",Collections.singletonMap("epr-21576g.ic27",forge(0x200000,0xd2a1c6bfL)));
        assertFalse("native loader selects the corrupt preferred h BIOS by name before the g fallback",find(dir).bios);
    }    @Test public void sharedTraversalVisitBudgetIsBounded()throws Exception {
        File root=tmp.newFolder(),nested=new File(root,"zz-valid-set");
        for(int i=0;i<DeathCrimsonFiles.WALK_LIMIT;i++) Files.createFile(new File(root,String.format(Locale.ROOT,"f%05d",i)).toPath());
        zip(nested,"deathcoxj.zip",game());bios(nested);
        assertNull(DeathCrimsonFiles.launchPath(null,Collections.singletonList(root)));
    }}