// hotd2-vr: added in 2026 for Lupin the Third VR.
package com.flycast.emulator;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.List;
import java.util.zip.CRC32;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

/** LupinFiles finds Lupin the Third's three files (made-up stand-ins here, no game data). */
public class LupinFilesTest
{
    @Rule
    public TemporaryFolder tmp = new TemporaryFolder();

    private File games, download;

    @Before
    public void setUp() throws IOException
    {
        games = tmp.newFolder("files", "games");
        download = tmp.newFolder("Download");
    }

    private List<File> roots()
    {
        // as the start screen passes them: the app's own folders (one not there), then Download
        return Arrays.asList(games, new File(tmp.getRoot(), "external/games"), download);
    }

    private File file(File dir, String path) throws IOException
    {
        File f = new File(dir, path);
        f.getParentFile().mkdirs();
        try (FileOutputStream out = new FileOutputStream(f)) {
            if (path.toLowerCase(java.util.Locale.ROOT).endsWith(".chd")) {
                out.write(chdHeader(5));
            } else if (path.endsWith(".gdi")) {
                out.write("3\n1 0 4 2048 track.bin 0\n2 450 0 2352 track.bin 0\n3 45000 4 2048 track.bin 0\n".getBytes(java.nio.charset.StandardCharsets.US_ASCII));
                file(f.getParentFile(), "track.bin");
            } else {
                out.write(new byte[] { 1, 2, 3, 4 });
            }
        }
        return f;
    }

    private File zip(File dir, String path, String... entries) throws IOException
    {
        File f = new File(dir, path);
        f.getParentFile().mkdirs();
        try (ZipOutputStream out = new ZipOutputStream(new FileOutputStream(f))) {
            for (String e : entries) {
                out.putNextEntry(new ZipEntry(e));
                out.write(new byte[e.endsWith(".pic") ? 0x4000 : 0x200000]);
                out.closeEntry();
            }
        }
        return f;
    }

    private static boolean names(List<String> missing, String what)
    {
        for (String line : missing)
            if (line.contains(what))
                return true;
        return false;
    }

    @Test
    public void allThreeThere() throws IOException
    {
        File z = zip(download, "lupinshoo.zip", "317-0325-jpn.pic");
        file(download, "lupinshoo/gds-0018.chd");
        zip(download, "naomi.zip", "epr-21578h.ic27");
        LupinFiles.Result r = LupinFiles.find(roots());
        assertEquals(z.getCanonicalFile(), r.zip.getCanonicalFile());
        assertTrue(r.chd);
        assertEquals("gds-0018.chd", r.disc == null ? null : r.disc.getName());
        assertTrue(r.bios);
        assertTrue(r.missing.toString(), r.missing.isEmpty());
    }

    @Test
    public void noDiscFolder() throws IOException
    {
        zip(download, "lupinshoo.zip", "317-0325-jpn.pic");
        zip(download, "naomi.zip", "epr-21578h.ic27");
        LupinFiles.Result r = LupinFiles.find(roots());
        assertNotNull(r.zip);
        assertFalse(r.chd);
        assertTrue(r.bios);
        assertEquals(r.missing.toString(), 1, r.missing.size());
        assertTrue(r.missing.toString(), names(r.missing, "lupinshoo/gds-0018.chd"));
    }

    @Test
    public void discNotInItsFolder() throws IOException
    {
        zip(download, "lupinshoo.zip", "317-0325-jpn.pic");
        file(download, "lupinshoo.chd");
        zip(download, "naomi.zip", "epr-21578h.ic27");
        LupinFiles.Result r = LupinFiles.find(roots());
        assertFalse(r.chd);
        assertTrue(r.missing.toString(), names(r.missing, "lupinshoo/gds-0018.chd"));
    }

    @Test
    public void discAsCueOrGdi() throws IOException
    {
        zip(download, "lupinshoo.zip", "317-0325-jpn.pic");
        file(download, "lupinshoo/gds-0018.gdi");
        zip(download, "naomi.zip", "epr-21578h.ic27");
        LupinFiles.Result r = LupinFiles.find(roots());
        assertTrue(r.chd);
        // the start screen names the disc that's there
        assertEquals("gds-0018.gdi", r.disc == null ? null : r.disc.getName());
        assertTrue(r.missing.toString(), r.missing.isEmpty());
    }

    @Test
    public void biosInsideTheGameZip() throws IOException
    {
        zip(download, "lupinshoo.zip", "317-0325-jpn.pic", "epr-21577h.ic27");
        file(download, "lupinshoo/gds-0018.chd");
        LupinFiles.Result r = LupinFiles.find(roots());
        assertTrue(r.bios);
        assertTrue(r.missing.toString(), r.missing.isEmpty());
    }

    @Test
    public void noBios() throws IOException
    {
        zip(download, "lupinshoo.zip", "317-0325-jpn.pic");
        file(download, "lupinshoo/gds-0018.chd");
        LupinFiles.Result r = LupinFiles.find(roots());
        assertFalse(r.bios);
        assertEquals(r.missing.toString(), 1, r.missing.size());
        assertTrue(r.missing.toString(), names(r.missing, "naomi.zip"));
    }

    @Test
    public void notAZip() throws IOException
    {
        file(download, "lupinshoo.zip");
        file(download, "lupinshoo/gds-0018.chd");
        LupinFiles.Result r = LupinFiles.find(roots());
        assertNotNull(r.zip);
        assertFalse(r.bios);
        assertTrue(r.missing.toString(), names(r.missing, "naomi.zip"));
    }

    @Test
    public void zipNameInAnyCase() throws IOException
    {
        zip(download, "Lupinshoo.ZIP", "317-0325-jpn.pic");
        LupinFiles.Result r = LupinFiles.find(roots());
        assertNotNull(r.zip);
        assertTrue(r.zip.getName().equalsIgnoreCase("lupinshoo.zip"));
    }

    @Test
    public void threeFoldersDeep() throws IOException
    {
        zip(download, "a/b/lupinshoo.zip", "317-0325-jpn.pic");
        file(download, "a/b/lupinshoo/gds-0018.chd");
        zip(download, "a/b/naomi.zip", "epr-21578h.ic27");
        LupinFiles.Result r = LupinFiles.find(roots());
        assertNotNull(r.zip);
        assertTrue(r.missing.toString(), r.missing.isEmpty());
    }

    @Test
    public void tooDeep() throws IOException
    {
        zip(download, "a/b/c/d/lupinshoo.zip", "317-0325-jpn.pic");
        LupinFiles.Result r = LupinFiles.find(roots());
        assertNull(r.zip);
        assertFalse(r.chd);
        assertFalse(r.bios);
        assertEquals(r.missing.toString(), 3, r.missing.size());
        assertTrue(names(r.missing, "lupinshoo.zip"));
        assertTrue(names(r.missing, "lupinshoo/gds-0018.chd"));
        assertTrue(names(r.missing, "naomi.zip"));
    }

    @Test
    public void nothingThere()
    {
        LupinFiles.Result r = LupinFiles.find(roots());
        assertNull(r.zip);
        assertEquals(r.missing.toString(), 3, r.missing.size());
    }

    @Test
    public void theCompleteSetWins() throws IOException
    {
        zip(download, "lupinshoo.zip", "317-0325-jpn.pic");      // alone, nothing next to it
        File full = zip(download, "cm/lupinshoo.zip", "317-0325-jpn.pic");
        file(download, "cm/lupinshoo/gds-0018.chd");
        zip(download, "cm/naomi.zip", "epr-21578h.ic27");
        LupinFiles.Result r = LupinFiles.find(roots());
        assertEquals(full.getCanonicalFile(), r.zip.getCanonicalFile());
        assertTrue(r.missing.toString(), r.missing.isEmpty());
    }

    @Test
    public void theAppsOwnFolder() throws IOException
    {
        zip(games, "lupinshoo.zip", "317-0325-jpn.pic", "epr-21578g.ic27");
        file(games, "lupinshoo/gds-0018.chd");
        LupinFiles.Result r = LupinFiles.find(roots());
        assertNotNull(r.zip);
        assertTrue(r.missing.toString(), r.missing.isEmpty());
    }

    @Test
    public void missingLinesSayWhere() throws IOException
    {
        zip(download, "lupinshoo.zip", "317-0325-jpn.pic");
        LupinFiles.Result r = LupinFiles.find(roots());
        assertEquals(r.missing.toString(), 2, r.missing.size());
        for (String line : r.missing)
            assertTrue(line, line.contains("Download") && line.contains("next to lupinshoo.zip"));
    }

    @Test
    public void noZipButTheOtherTwo() throws IOException
    {
        // the disc and the BIOS are there, only the game's zip isn't: one line, for the zip
        file(download, "lupinshoo/gds-0018.chd");
        zip(download, "naomi.zip", "epr-21578h.ic27");
        LupinFiles.Result r = LupinFiles.find(roots());
        assertNull(r.zip);
        assertTrue(r.chd);
        assertTrue(r.bios);
        assertEquals(r.missing.toString(), 1, r.missing.size());
        assertTrue(r.missing.toString(), r.missing.get(0).startsWith("lupinshoo.zip"));
        assertTrue(r.missing.toString(), r.missing.get(0).contains("Download"));
    }

    @Test
    public void noZipButTheBios() throws IOException
    {
        zip(download, "naomi.zip", "epr-21578h.ic27");
        LupinFiles.Result r = LupinFiles.find(roots());
        assertFalse(r.chd);
        assertTrue(r.bios);
        assertEquals(r.missing.toString(), 2, r.missing.size());
        assertTrue(names(r.missing, "lupinshoo.zip"));
        assertTrue(names(r.missing, "lupinshoo/gds-0018.chd"));
    }

    @Test
    public void biosUnderAnotherNameByItsCrc() throws IOException
    {
        // the emulator finds the BIOS by its CRC first (naomi_cart.cpp): an entry renamed, with
        // epr-21576h.ic27's CRC (synthetic bytes of the expected size, no BIOS data)
        File z = new File(download, "lupinshoo.zip");
        try (ZipOutputStream out = new ZipOutputStream(new FileOutputStream(z))) {
            out.putNextEntry(new ZipEntry("317-0325-jpn.pic"));
            out.write(new byte[0x4000]);
            out.closeEntry();
            out.putNextEntry(new ZipEntry("bios.bin"));
            out.write(withCrc(0xd4895685L, 0x200000));
            out.closeEntry();
        }
        file(download, "lupinshoo/gds-0018.chd");
        LupinFiles.Result r = LupinFiles.find(roots());
        assertTrue(r.bios);
        assertTrue(r.missing.toString(), r.missing.isEmpty());
    }

    @Test
    public void theWalkStopsInAHugeTree() throws IOException
    {
        File crowd = new File(download, "a");
        assertTrue(crowd.mkdirs());
        for (int i = 0; i < LupinFiles.WALK_LIMIT; i++)
            assertTrue(new File(crowd, "f" + i).createNewFile());
        zip(download, "b/lupinshoo.zip", "317-0325-jpn.pic");    // after the crowd: past the cap
        assertNull(LupinFiles.find(roots()).zip);
        zip(download, "lupinshoo.zip", "317-0325-jpn.pic");      // at the top: still found
        assertNotNull(LupinFiles.find(roots()).zip);
    }

    /** Synthetic bytes whose CRC-32 is crc (the final four bytes depend only on the four
     *  table indices: found from the top byte down, then the bytes that give them). */
    private static byte[] withCrc(long crc, int length)
    {
        int[] table = new int[256];
        for (int n = 0; n < 256; n++) {
            int c = n;
            for (int k = 0; k < 8; k++)
                c = (c & 1) != 0 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1;
            table[n] = c;
        }
        // state = T[m3] ^ T[m2] >>> 8 ^ T[m1] >>> 16 ^ T[m0] >>> 24
        int rest = ~(int) crc;
        int[] m = new int[4];
        for (int i = 3; i >= 0; i--) {
            int shift = 8 * (3 - i), top = (rest >>> (24 - shift)) & 0xFF;
            for (int n = 0; n < 256; n++)
                if ((table[n] >>> 24) == top) { m[i] = n; break; }
            rest ^= table[m[i]] >>> shift;
        }
        byte[] out = new byte[length];
        int s = 0xFFFFFFFF;
        for (int i = 0; i < length - 4; i++)
            s = table[s & 0xFF] ^ (s >>> 8);
        for (int i = 0; i < 4; i++) {
            out[length - 4 + i] = (byte) (m[i] ^ (s & 0xFF));
            s = table[m[i]] ^ (s >>> 8);
        }
        CRC32 check = new CRC32();
        check.update(out);
        assertEquals(crc, check.getValue());
        return out;
    }
    private static final String ORIGINAL_SHA1 = "3b7d5dd917b9a2cc9b7b51aad22f8e75383c3301";

    /** Plausible bounded headers/maps, made up without disc payload or game data. */
    private byte[] chdHeader(int version)
    {
        byte[] header = new byte[512];
        java.nio.ByteBuffer b = java.nio.ByteBuffer.wrap(header);
        b.put("MComprHD".getBytes(java.nio.charset.StandardCharsets.US_ASCII));
        b.putInt(version == 3 ? 120 : version == 4 ? 108 : 124).putInt(version);
        if (version == 5) {
            b.putLong(32, 9792).putLong(40, 124);
            b.putInt(56, 9792).putInt(60, 2448);
        } else {
            b.putInt(20, 1).putInt(24, 1).putLong(28, 9792);
            b.putInt(version == 3 ? 76 : 44, 9792);
        }
        int sha = version == 3 ? 80 : version == 4 ? 48 : 84;
        String identity = version == 3 ? "ded9d1aa791e64b0db962a5ea762f4ff28b2e76a" : ORIGINAL_SHA1;
        b.position(sha);
        for (int i = 0; i < identity.length(); i += 2)
            b.put((byte) Integer.parseInt(identity.substring(i, i + 2), 16));
        return header;
    }

    private void chdBytes(byte[] header) throws IOException
    {
        java.nio.file.Files.write(new File(download, "lupinshoo/gds-0018.chd").toPath(), header);
    }

    private String cue() throws IOException
    {
        return new String(java.nio.file.Files.readAllBytes(new File(download, "lupinshoo/gds-0018.cue").toPath()),
                java.nio.charset.StandardCharsets.UTF_8);
    }

    private void complete() throws IOException
    {
        zip(download, "lupinshoo.zip", "317-0325-jpn.pic");
        file(download, "lupinshoo/gds-0018.chd");
        zip(download, "naomi.zip", "epr-21576h.ic27");
    }

    private void text(String path, String value) throws IOException
    {
        File f = new File(download, path);
        f.getParentFile().mkdirs();
        java.nio.file.Files.write(f.toPath(), value.getBytes(java.nio.charset.StandardCharsets.UTF_8));
    }

    @Test public void misleadingRevANameExplainsOriginalLayout() throws IOException
    {
        zip(download, "lupinsho.zip", "317-0325-jpn.pic");
        file(download, "lupinsho/gds-0018.chd");
        zip(download, "naomi.zip", "epr-21576h.ic27");
        LupinFiles.Result r = LupinFiles.find(roots());
        assertFalse(r.missing.isEmpty());
        assertTrue(r.missing.toString(), names(r.missing, "lupinsho.zip is Rev A"));
        assertTrue(r.missing.toString(), names(r.missing, "lupinshoo/gds-0018.chd"));
    }

    @Test public void originalSetRejectsRevADiscName() throws IOException
    {
        zip(download, "lupinshoo.zip", "317-0325-jpn.pic");
        file(download, "lupinshoo/gds-0018a.chd");
        zip(download, "naomi.zip", "epr-21576h.ic27");
        LupinFiles.Result r = LupinFiles.find(roots());
        assertFalse(r.chd);
        assertTrue(r.missing.toString(), names(r.missing, "Rev A"));
    }

    @Test public void wrongChdRevisionNeverReady() throws IOException
    {
        complete();
        File f = new File(download, "lupinshoo/gds-0018.chd");
        try (java.io.RandomAccessFile out = new java.io.RandomAccessFile(f, "rw")) {
            out.seek(84);
            out.write(0x7f);
        }
        LupinFiles.Result r = LupinFiles.find(roots());
        assertFalse(r.chd);
        assertTrue(r.missing.toString(), names(r.missing, "original GDS-0018"));
    }

    @Test public void invalidChdNeverReady() throws IOException
    {
        complete();
        text("lupinshoo/gds-0018.chd", "not a CHD");
        assertFalse(LupinFiles.find(roots()).chd);
    }

    @Test public void parentDependentChdNeverReady() throws IOException
    {
        complete();
        File f = new File(download, "lupinshoo/gds-0018.chd");
        try (java.io.RandomAccessFile out = new java.io.RandomAccessFile(f, "rw")) {
            out.seek(104);
            out.write(1);
        }
        assertFalse(LupinFiles.find(roots()).chd);
    }

    @Test public void missingPicNeverReady() throws IOException
    {
        complete();
        zip(download, "lupinshoo.zip", "other.bin");
        LupinFiles.Result r = LupinFiles.find(roots());
        assertTrue(r.missing.toString(), names(r.missing, "317-0325-jpn.pic"));
    }

    @Test public void truncatedPicNeverReady() throws IOException
    {
        complete();
        File f = new File(download, "lupinshoo.zip");
        try (ZipOutputStream out = new ZipOutputStream(new FileOutputStream(f))) {
            out.putNextEntry(new ZipEntry("317-0325-jpn.pic"));
            out.write(1);
            out.closeEntry();
        }
        assertFalse(LupinFiles.find(roots()).missing.isEmpty());
    }

    @Test public void arbitraryBiosZipNeverReady() throws IOException
    {
        complete();
        text("naomi.zip", "not a ZIP");
        assertFalse(LupinFiles.find(roots()).bios);
    }

    @Test public void unsupportedBiosNeverReady() throws IOException
    {
        complete();
        zip(download, "naomi.zip", "epr-21576e.ic27");
        assertFalse(LupinFiles.find(roots()).bios);
    }

    @Test public void emptyBiosEntryNeverReady() throws IOException
    {
        complete();
        try (ZipOutputStream out = new ZipOutputStream(new FileOutputStream(new File(download, "naomi.zip")))) {
            out.putNextEntry(new ZipEntry("epr-21576h.ic27"));
            out.closeEntry();
        }
        assertFalse(LupinFiles.find(roots()).bios);
    }

    @Test public void completeCueUsesAllTracks() throws IOException
    {
        zip(download, "lupinshoo.zip", "317-0325-jpn.pic", "epr-21576h.ic27");
        text("lupinshoo/gds-0018.cue", "REM SINGLE-DENSITY AREA\nFILE \"track 1.bin\" BINARY\n TRACK 01 MODE1/2048\n INDEX 01 00:00:00\nFILE \"track 2.bin\" BINARY\n TRACK 02 AUDIO\n INDEX 01 00:00:00\nREM HIGH-DENSITY AREA\nFILE \"track 3.bin\" BINARY\n TRACK 03 MODE1/2048\n INDEX 01 00:00:00\n");
        file(download, "lupinshoo/track 1.bin");
        file(download, "lupinshoo/track 2.bin");
        file(download, "lupinshoo/track 3.bin");
        assertTrue(LupinFiles.find(roots()).missing.toString(), LupinFiles.find(roots()).missing.isEmpty());
    }

    @Test public void incompleteCueNeverReady() throws IOException
    {
        completeCueUsesAllTracks();
        assertTrue(new File(download, "lupinshoo/track 2.bin").delete());
        assertFalse(LupinFiles.find(roots()).chd);
    }

    @Test public void incompleteGdiNeverReady() throws IOException
    {
        discAsCueOrGdi();
        assertTrue(new File(download, "lupinshoo/track.bin").delete());
        assertFalse(LupinFiles.find(roots()).chd);
    }

    @Test public void wrongGdiTrackCountNeverReady() throws IOException
    {
        discAsCueOrGdi();
        text("lupinshoo/gds-0018.gdi", "2\n1 45000 4 2048 track.bin 0\n");
        assertFalse(LupinFiles.find(roots()).chd);
    }

    @Test public void malformedCueNeverReady() throws IOException
    {
        complete();
        assertTrue(new File(download, "lupinshoo/gds-0018.chd").delete());
        text("lupinshoo/gds-0018.cue", "not a cue");
        assertFalse(LupinFiles.find(roots()).chd);
    }

    @Test public void revisionFolderCannotSupplyOriginalDisc() throws IOException
    {
        zip(download, "lupinshoo.zip", "317-0325-jpn.pic", "epr-21576h.ic27");
        file(download, "lupinsho/gds-0018a.chd");
        assertFalse(LupinFiles.find(roots()).chd);
    }

    @Test public void aHugeSingleDirectoryStopsAtLimit() throws IOException
    {
        for (int i = 0; i < LupinFiles.WALK_LIMIT; i++)
            assertTrue(new File(download, String.format(java.util.Locale.ROOT, "a%05d", i)).createNewFile());
        zip(download, "lupinshoo.zip", "317-0325-jpn.pic");
        assertNull(LupinFiles.find(roots()).zip);
    }    @Test public void explicitOriginalLaunchIsValidated() throws IOException
    {
        complete();
        assertEquals(new File(download, "lupinshoo.zip").getAbsolutePath(),
                LupinFiles.launchPath(new File(download, "lupinshoo.zip"), roots()));
    }

    @Test public void explicitForeignGameCannotBypassSetup() throws IOException
    {
        complete();
        File other = zip(download, "confmiss.zip", "317-0325-jpn.pic");
        assertNull(LupinFiles.launchPath(other, roots()));
    }

    @Test public void explicitDiscCannotBypassGameZip() throws IOException
    {
        complete();
        assertNull(LupinFiles.launchPath(new File(download, "lupinshoo/gds-0018.chd"), roots()));
    }

    @Test public void explicitMissingFilesCannotBypassSetup() throws IOException
    {
        File incomplete = zip(games, "lupinshoo.zip", "317-0325-jpn.pic");
        complete();
        assertNull(LupinFiles.launchPath(incomplete, roots()));
    }

    @Test public void launchWithoutExplicitPathDiscoversOriginal() throws IOException
    {
        complete();
        assertEquals(new File(download, "lupinshoo.zip").getAbsolutePath(), LupinFiles.launchPath(null, roots()));
    }

    @Test public void caseVariantsResolveToEngineNames() throws IOException
    {
        zip(download, "LUPINSHOO.ZIP", "317-0325-jpn.pic", "epr-21576h.ic27");
        file(download, "LUPINSHOO/GDS-0018.CHD");
        // The shared-storage layout is case insensitive; a case-sensitive filesystem must instead
        // explain the required canonical names and keep Play disabled.
        LupinFiles.Result r = LupinFiles.find(roots());
        assertNotNull(r.zip);
        if (new File(download, "lupinshoo/gds-0018.chd").exists())
            assertTrue(r.missing.toString(), r.missing.isEmpty());
        else
            assertFalse(r.missing.isEmpty());
    }    @Test public void gdiNeedsThreeTracksLikeEngine() throws IOException
    {
        discAsCueOrGdi();
        text("lupinshoo/gds-0018.gdi", "1\n1 0 4 2048 track.bin 0\n");
        assertFalse(LupinFiles.find(roots()).chd);
    }

    @Test public void gdiNeedsAudioSecondTrack() throws IOException
    {
        discAsCueOrGdi();
        text("lupinshoo/gds-0018.gdi", "3\n1 0 4 2048 track.bin 0\n2 450 4 2048 track.bin 0\n3 45000 4 2048 track.bin 0\n");
        assertFalse(LupinFiles.find(roots()).chd);
    }

    @Test public void gdiNeedsHighDensityTrackAt45000() throws IOException
    {
        discAsCueOrGdi();
        text("lupinshoo/gds-0018.gdi", "3\n1 0 4 2048 track.bin 0\n2 450 0 2352 track.bin 0\n3 450 4 2048 track.bin 0\n");
        assertFalse(LupinFiles.find(roots()).chd);
    }

    @Test public void cueNeedsIndexForEachTrack() throws IOException
    {
        completeCueUsesAllTracks();
        File f = new File(download, "lupinshoo/gds-0018.cue");
        String cue = new String(java.nio.file.Files.readAllBytes(f.toPath()), java.nio.charset.StandardCharsets.UTF_8);
        text("lupinshoo/gds-0018.cue", cue.replace(" INDEX 01 00:00:00", ""));
        assertFalse(LupinFiles.find(roots()).chd);
    }

    @Test public void cueNeedsHighDensityAreaForNaomi() throws IOException
    {
        completeCueUsesAllTracks();
        File f = new File(download, "lupinshoo/gds-0018.cue");
        String cue = new String(java.nio.file.Files.readAllBytes(f.toPath()), java.nio.charset.StandardCharsets.UTF_8);
        text("lupinshoo/gds-0018.cue", cue.replace("REM HIGH-DENSITY AREA", ""));
        assertFalse(LupinFiles.find(roots()).chd);
    }    @Test public void biosEntryNameIsCaseSensitiveLikeEngineUnlessCrcMatches() throws IOException
    {
        complete();
        zip(download, "naomi.zip", "EPR-21576H.IC27");
        assertFalse(LupinFiles.find(roots()).bios);
    }

    @Test public void picEntryNameIsCaseSensitiveLikeEngineUnlessCrcMatches() throws IOException
    {
        complete();
        File f = new File(download, "lupinshoo.zip");
        try (ZipOutputStream out = new ZipOutputStream(new FileOutputStream(f))) {
            out.putNextEntry(new ZipEntry("317-0325-JPN.PIC"));
            out.write(new byte[0x4000]);
            out.closeEntry();
        }
        assertFalse(LupinFiles.find(roots()).missing.isEmpty());
    }
    @Test public void supportedChdHeaderVersionsRemainReady() throws IOException
    {
        complete();
        for (int version : new int[] { 3, 4, 5 }) {
            chdBytes(chdHeader(version));
            assertTrue("CHD v" + version, LupinFiles.find(roots()).chd);
        }
    }

    @Test public void zeroChdHunkSizeNeverReady() throws IOException
    {
        complete();
        for (int version : new int[] { 3, 4, 5 }) {
            byte[] header = chdHeader(version);
            java.nio.ByteBuffer.wrap(header).putInt(version == 3 ? 76 : version == 4 ? 44 : 56, 0);
            chdBytes(header);
            assertFalse("CHD v" + version, LupinFiles.find(roots()).chd);
        }
    }

    @Test public void chdHunksMustContainWholeCdFrames() throws IOException
    {
        complete();
        for (int version : new int[] { 3, 4, 5 }) {
            byte[] header = chdHeader(version);
            java.nio.ByteBuffer.wrap(header).putInt(version == 3 ? 76 : version == 4 ? 44 : 56, 9793);
            chdBytes(header);
            assertFalse("partial CD frame v" + version, LupinFiles.find(roots()).chd);
        }
    }

    @Test public void zeroChdUnitSizeNeverReady() throws IOException
    {
        complete();
        byte[] header = chdHeader(5);
        java.nio.ByteBuffer.wrap(header).putInt(60, 0);
        chdBytes(header);
        assertFalse(LupinFiles.find(roots()).chd);
    }

    @Test public void impossibleChdCountsAndSizesNeverReady() throws IOException
    {
        complete();
        for (int version : new int[] { 3, 4, 5 }) {
            byte[] header = chdHeader(version);
            java.nio.ByteBuffer.wrap(header).putInt(version == 3 ? 76 : version == 4 ? 44 : 56,
                    version < 5 ? 16 * 1024 * 1024 : 128 * 1024 * 1024);
            chdBytes(header);
            assertFalse("oversized hunk v" + version, LupinFiles.find(roots()).chd);
            header = chdHeader(version);
            java.nio.ByteBuffer.wrap(header).putLong(version < 5 ? 28 : 32, 0);
            chdBytes(header);
            assertFalse("empty logical disc v" + version, LupinFiles.find(roots()).chd);
            if (version < 5) {
                header = chdHeader(version);
                java.nio.ByteBuffer.wrap(header).putInt(24, 0);
                chdBytes(header);
                assertFalse("zero hunks v" + version, LupinFiles.find(roots()).chd);
            }
        }
    }

    @Test public void truncatedOrOutOfFileChdMapsNeverReady() throws IOException
    {
        complete();
        byte[] header = chdHeader(5);
        java.nio.ByteBuffer.wrap(header).putLong(40, 510);
        chdBytes(header);
        assertFalse("v5 map beyond file", LupinFiles.find(roots()).chd);
        chdBytes(Arrays.copyOf(chdHeader(5), 128));
        assertFalse("v5 truncated at map end", LupinFiles.find(roots()).chd);
        header = chdHeader(5);
        java.nio.ByteBuffer.wrap(header).putInt(16, 0x63646c7a).putInt(124, 500);
        chdBytes(header);
        assertFalse("v5 compressed map beyond file", LupinFiles.find(roots()).chd);
        for (int version : new int[] { 3, 4 }) {
            header = chdHeader(version);
            java.nio.ByteBuffer.wrap(header).putInt(24, 40);
            chdBytes(header);
            assertFalse("legacy map beyond file v" + version, LupinFiles.find(roots()).chd);
        }
    }

    @Test public void outOfFileChdMetadataNeverReady() throws IOException
    {
        complete();
        for (int version : new int[] { 3, 4, 5 }) {
            byte[] header = chdHeader(version);
            java.nio.ByteBuffer.wrap(header).putLong(version < 5 ? 36 : 48, 510);
            chdBytes(header);
            assertFalse("metadata beyond file v" + version, LupinFiles.find(roots()).chd);
        }
    }

    @Test public void cueHighDensityIndexMustStartAt45150() throws IOException
    {
        completeCueUsesAllTracks();
        text("lupinshoo/gds-0018.cue", cue().replace("TRACK 03 MODE1/2048\n INDEX 01 00:00:00",
                "TRACK 03 MODE1/2048\n INDEX 01 00:01:00"));
        assertFalse(LupinFiles.find(roots()).chd);
    }

    @Test public void cueIndexFramesMustBeUnder75() throws IOException
    {
        completeCueUsesAllTracks();
        String original = cue();
        for (String index : new String[] { "00:00:75", "00:00:79", "00:60:00" }) {
            text("lupinshoo/gds-0018.cue", original.replace("INDEX 01 00:00:00", "INDEX 01 " + index));
            assertFalse(index, LupinFiles.find(roots()).chd);
        }
    }

    @Test public void cueHighDensityMarkerMustPrecedeNewFileContext() throws IOException
    {
        completeCueUsesAllTracks();
        String original = cue();
        text("lupinshoo/gds-0018.cue", original.replace("REM HIGH-DENSITY AREA\nFILE \"track 3.bin\" BINARY",
                "FILE \"track 3.bin\" BINARY\nREM HIGH-DENSITY AREA"));
        assertFalse("REM clears the native FILE context", LupinFiles.find(roots()).chd);
    }

    @Test public void cueIndexZeroNeedsPendingTrackAndCorrectOrder() throws IOException
    {
        completeCueUsesAllTracks();
        String original = cue();
        for (String invalid : new String[] {
                "INDEX 00 00:00:00\n" + original,
                original.replace("INDEX 01 00:00:00", "INDEX 01 00:00:00\n INDEX 00 00:00:00"),
                original.replace("INDEX 01 00:00:00", "INDEX 00 00:00:01\n INDEX 01 00:00:00") }) {
            text("lupinshoo/gds-0018.cue", invalid);
            assertFalse(LupinFiles.find(roots()).chd);
        }
        text("lupinshoo/gds-0018.cue", original.replace("INDEX 01 00:00:00", "INDEX 00 00:00:00\n INDEX 01 00:00:00"));
        assertTrue("valid INDEX 00/01 ordering", LupinFiles.find(roots()).chd);
    }

    @Test public void staleCueMustNotHideBehindValidGdi() throws IOException
    {
        discAsCueOrGdi();
        text("lupinshoo/gds-0018.cue", "FILE track.bin BINARY\n TRACK 01 MODE1/2048\n INDEX 01 00:00:00\n");
        LupinFiles.Result r = LupinFiles.find(roots());
        assertFalse("the engine chooses the stale parseable CUE first", r.chd);
        assertNull(LupinFiles.launchPath(null, roots()));
    }
}
