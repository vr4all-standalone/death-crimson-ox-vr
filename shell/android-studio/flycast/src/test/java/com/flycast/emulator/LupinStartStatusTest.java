// hotd2-vr: added in 2026 for Lupin the Third VR.
package com.flycast.emulator;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

/** LupinStartStatus turns LupinFiles' check into what the start screen shows (made-up stand-in files, no game data). */
public class LupinStartStatusTest
{
    private static final String OK = " ✓";

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

    private void file(File dir, String path) throws IOException
    {
        File f = new File(dir, path);
        f.getParentFile().mkdirs();
        try (FileOutputStream out = new FileOutputStream(f)) {
            if (path.endsWith(".chd")) {
                byte[] header = new byte[125];
                java.nio.ByteBuffer b = java.nio.ByteBuffer.wrap(header);
                b.put("MComprHD".getBytes(java.nio.charset.StandardCharsets.US_ASCII));
                b.putInt(124).putInt(5);
                b.position(84);
                for (int i = 0; i < "3b7d5dd917b9a2cc9b7b51aad22f8e75383c3301".length(); i += 2)
                    b.put((byte) Integer.parseInt("3b7d5dd917b9a2cc9b7b51aad22f8e75383c3301".substring(i, i + 2), 16));
                out.write(header);
            } else {
                out.write(new byte[] { 1, 2, 3, 4 });
            }
        }
    }

    private void zip(File dir, String path, String... entries) throws IOException
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
    }

    private void allThree(File dir) throws IOException
    {
        zip(dir, "lupinshoo.zip", "317-0325-jpn.pic");
        file(dir, "lupinshoo/gds-0018.chd");
        zip(dir, "naomi.zip", "epr-21578h.ic27");
    }

    @Test
    public void allFoundWithAccessIsReady() throws IOException
    {
        allThree(download);
        LupinStartStatus s = LupinStartStatus.from(LupinFiles.find(roots()), true);
        assertTrue(s.ready);
        assertEquals("READY", s.badge);
        assertTrue(s.missing.toString(), s.missing.isEmpty());
        assertEquals(Arrays.asList("lupinshoo.zip" + OK, "naomi.zip" + OK, "CHD" + OK), s.fileMarks);
    }

    @Test
    public void missingBiosNamesIt() throws IOException
    {
        zip(download, "lupinshoo.zip", "317-0325-jpn.pic");
        file(download, "lupinshoo/gds-0018.chd");
        LupinStartStatus s = LupinStartStatus.from(LupinFiles.find(roots()), true);
        assertFalse(s.ready);
        assertEquals("MISSING FILES", s.badge);
        assertEquals(s.missing.toString(), 1, s.missing.size());
        String line = s.missing.get(0);
        assertTrue(line, line.startsWith("naomi.zip missing"));
        // and the folder it goes in, from LupinFiles' line
        assertTrue(line, line.contains("in Download, next to lupinshoo.zip"));
        assertEquals(Arrays.asList("lupinshoo.zip" + OK, "naomi.zip missing", "CHD" + OK), s.fileMarks);
    }

    @Test
    public void missingDiscAndZip() throws IOException
    {
        zip(download, "naomi.zip", "epr-21578h.ic27");
        LupinStartStatus s = LupinStartStatus.from(LupinFiles.find(roots()), true);
        assertFalse(s.ready);
        assertEquals("MISSING FILES", s.badge);
        assertEquals(s.missing.toString(), 2, s.missing.size());
        assertTrue(s.missing.get(0), s.missing.get(0).startsWith("lupinshoo.zip missing"));
        assertTrue(s.missing.get(0), s.missing.get(0).contains("in Download"));
        assertTrue(s.missing.get(1), s.missing.get(1).startsWith("lupinshoo/gds-0018.chd missing"));
        assertTrue(s.missing.get(1), s.missing.get(1).contains("a folder named lupinshoo in Download"));
        assertEquals(Arrays.asList("lupinshoo.zip missing", "naomi.zip" + OK, "CHD missing"), s.fileMarks);
    }

    @Test
    public void ownFolderNeedsNoAccess() throws IOException
    {
        // all three in the app's own folder, which needs no access: ready without it
        allThree(games);
        LupinFiles.Result r = LupinFiles.find(roots());
        assertTrue(r.missing.toString(), r.missing.isEmpty());
        LupinStartStatus s = LupinStartStatus.from(r, false);
        assertTrue(s.ready);
        assertEquals("READY", s.badge);
        assertEquals(Arrays.asList("lupinshoo.zip" + OK, "naomi.zip" + OK, "CHD" + OK), s.fileMarks);
    }

    @Test
    public void missingWithoutAccessAsksForIt() throws IOException
    {
        // nothing found and no access: the files may be in Download, so the badge asks for the access
        LupinFiles.Result r = LupinFiles.find(roots());
        LupinStartStatus s = LupinStartStatus.from(r, false);
        assertFalse(s.ready);
        assertEquals("ALLOW ACCESS", s.badge);
    }

    @Test
    public void beforeFirstCheck()
    {
        LupinStartStatus s = LupinStartStatus.from(null, true);
        assertFalse(s.ready);
        assertEquals("CHECKING", s.badge);
        assertEquals(Collections.emptyList(), s.fileMarks);
        assertEquals(Collections.emptyList(), s.missing);
        // without the access, that's what the badge asks for first
        LupinStartStatus noAccess = LupinStartStatus.from(null, false);
        assertFalse(noAccess.ready);
        assertEquals("ALLOW ACCESS", noAccess.badge);
    }

    @Test
    public void shortVersionIsMajorMinor()
    {
        // the app's versionName is "<hubVersion>-g<commit>"
        assertEquals("v1.1", LupinStartStatus.shortVersion("1.1.0-g1a2b3c4"));
        assertEquals("v1.2", LupinStartStatus.shortVersion("1.2.0"));
        assertEquals("v10.4", LupinStartStatus.shortVersion("10.4.2-gabc-vr"));
        assertEquals("", LupinStartStatus.shortVersion(null));
        assertEquals("", LupinStartStatus.shortVersion("gabc123"));
    }
}
