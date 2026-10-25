// Lupin standalone setup, derived from Confidential Mission VR's setup.
package com.flycast.emulator;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

/** The player's original Lupin MAME set. Checks layout and metadata, without reading the whole
 * disc or claiming a full integrity check. Plain Java so the setup rules run in JVM tests. */
final class LupinFiles
{
    private LupinFiles() {}

    static final String ZIP = "lupinshoo.zip";
    static final String DISC = "lupinshoo/gds-0018.chd";
    static final String BIOS_ZIP = "naomi.zip";
    private static final String REV_A_ZIP = "lupinsho.zip";
    private static final String PIC = "317-0325-jpn.pic";
    private static final long PIC_CRC = 0xf71cb2fcL;
    private static final String ORIGINAL_SHA1 = "3b7d5dd917b9a2cc9b7b51aad22f8e75383c3301";
    // The supplied original's raw-data SHA1, for CHD v3 (v4/v5 use the combined SHA1 above).
    private static final String ORIGINAL_RAW_SHA1 = "ded9d1aa791e64b0db962a5ea762f4ff28b2e76a";
    private static final List<String> BIOS_FILES = Arrays.asList("epr-21576h.ic27", "epr-21576g.ic27",
            "epr-21578h.ic27", "epr-21578g.ic27", "epr-21577h.ic27", "epr-21577g.ic27", "epr-21579d.ic27",
            "epr-21576h_multi.ic27");
    private static final long[] BIOS_CRCS = { 0xd4895685L, 0xd2a1c6bfL, 0x7b452946L, 0x55413214L,
            0xfdf17452L, 0x25f64af7L, 0x33513691L, 0xcce01f1fL };
    private static final int DEPTH = 3, DESCRIPTOR_LIMIT = 65536;
    static final int WALK_LIMIT = 5000;

    static final class Result
    {
        File zip;
        /** A structurally valid, correctly paired disc; compatible BIOS metadata. */
        boolean chd, bios;
        File disc;
        /** One line per missing/invalid component, prefixed by its expected path. */
        List<String> missing = new ArrayList<>();
    }

    /** First complete set wins, otherwise the set needing the fewest corrections. */
    static Result find(List<File> roots)
    {
        List<File[]> found = new ArrayList<>();
        for (File root : roots)
            if (root != null)
                search(root, root, DEPTH, found, new int[] { WALK_LIMIT });
        Result best = null;
        for (File[] f : found)
        {
            Result r = check(f[0], f[1]);
            if (best == null || r.missing.size() < best.missing.size())
                best = r;
            if (best.missing.isEmpty())
                break;
        }
        return best != null ? best : withoutZip(roots);
    }

    /** The standalone app validates explicit launches just like the setup panel. An invalid
     * request never falls through to another game or bypasses the required ZIP/disc/BIOS. */
    static String launchPath(File requested, List<File> roots)
    {
        Result r;
        if (requested == null)
            r = find(roots);
        else
        {
            requested = requested.getAbsoluteFile();
            if (!requested.getName().equalsIgnoreCase(ZIP) || !nonEmpty(requested))
                return null;
            r = check(requested, requested.getParentFile());
        }
        return r.missing.isEmpty() && r.zip != null ? r.zip.getAbsolutePath() : null;
    }

    private static Result withoutZip(List<File> roots)
    {
        Result r = new Result();
        File at = null;
        for (File root : roots)
        {
            if (root == null || !root.isDirectory())
                continue;
            File disc = findDisc(root);
            boolean bios = hasEntry(new File(root, BIOS_ZIP), true);
            if ((disc != null ? 1 : 0) + (bios ? 1 : 0) > (r.chd ? 1 : 0) + (r.bios ? 1 : 0))
            {
                r.disc = disc;
                r.chd = disc != null;
                r.bios = bios;
                at = root;
            }
        }
        String where = at != null ? at.getName() : "Download";
        r.missing.add(ZIP + " (the original game, a MAME set with " + PIC + " inside): in " + where
                + (at == null ? "" : ", next to " + (r.bios ? BIOS_ZIP : "the lupinshoo folder")));
        if (!r.chd)
            r.missing.add(DISC + " (the original GDS-0018 disc): in a folder named lupinshoo in " + where + ", next to " + ZIP);
        if (!r.bios)
            r.missing.add(BIOS_ZIP + " (the NAOMI BIOS): in " + where + ", next to " + ZIP);
        return r;
    }

    /** Shallow entries first, then folders by name, with one shared visit budget per root. */
    private static void search(File dir, File root, int depth, List<File[]> out, int[] left)
    {
        if (left[0] <= 0)
            return;
        File[] files = dir.listFiles();
        if (files == null)
            return;
        Arrays.sort(files);
        int count = Math.min(left[0], files.length);
        left[0] -= count;
        for (int i = 0; i < count; i++)
        {
            File f = files[i];
            if ((f.getName().equalsIgnoreCase(ZIP) || f.getName().equalsIgnoreCase(REV_A_ZIP)) && nonEmpty(f))
                out.add(new File[] { f, root });
        }
        if (depth > 0)
            for (int i = 0; i < count && left[0] > 0; i++)
                if (!files[i].getName().startsWith(".") && files[i].isDirectory())
                    search(files[i], root, depth - 1, out, left);
    }

    private static Result check(File found, File root)
    {
        Result r = new Result();
        File dir = found.getParentFile();
        String where = place(dir, root);
        // Flycast's FindGame is case sensitive. Shared storage accepts the canonical lowercase
        // path; on a case-sensitive filesystem the player must correct the ZIP name themselves.
        File exact = new File(dir, ZIP);
        r.zip = exact.isFile() ? exact : found;
        if (found.getName().equalsIgnoreCase(REV_A_ZIP))
            r.missing.add(ZIP + ": " + REV_A_ZIP + " is Rev A (GDS-0018A). This app supports original GDS-0018: "
                    + "use " + ZIP + " with " + DISC + " (in " + where + "). Keep a separate copy of your original files.");
        else if (!exact.isFile())
            r.missing.add(ZIP + ": rename " + found.getName() + " to " + ZIP + ", in small letters (in " + where + ")");
        else if (!hasEntry(r.zip, false))
            r.missing.add(ZIP + ": needs its " + PIC + " key (16384 bytes) inside a readable ZIP (in " + where + ")");
        r.disc = findDisc(dir);
        r.chd = r.disc != null;
        if (!r.chd)
            r.missing.add(DISC + " (the original GDS-0018 disc): in a folder named lupinshoo next to " + ZIP
                    + " (in " + where + "). Use a matching CHD, or a complete same-named CUE/GDI with its tracks. "
                    + "Rev A (gds-0018a) is not supported; do not rename that disc to gds-0018.");
        r.bios = hasEntry(new File(dir, BIOS_ZIP), true) || hasEntry(r.zip, true);
        if (!r.bios)
            r.missing.add(BIOS_ZIP + " (a supported NAOMI BIOS): in " + where + ", next to " + ZIP
                    + ", or its BIOS file inside the game ZIP");
        return r;
    }

    private static File findDisc(File dir)
    {
        File base = new File(dir, "lupinshoo");
        File chd = new File(base, "gds-0018.chd");
        // A present CHD is what the engine opens first. Never hide its wrong revision behind a
        // fallback descriptor that the engine would not use.
        if (nonEmpty(chd))
            return originalChd(chd) ? chd : null;
        for (String ext : new String[] { ".cue", ".gdi" })
        {
            File f = new File(base, "gds-0018" + ext);
            // A stale CUE can parse as a CD and shadow the GDI in the native loader.
            // Validate the first present candidate instead of selecting a different disc.
            if (f.isFile())
                return validDescriptor(f, ext.equals(".gdi")) ? f : null;
        }
        return null;
    }

    private static boolean nonEmpty(File f)
    {
        return f.isFile() && f.length() > 0;
    }

    /** Matches the engine's entry lookup (CRC then name), and the required entry length. */
    private static boolean hasEntry(File archive, boolean bios)
    {
        try (ZipFile z = new ZipFile(archive)) {
            Enumeration<? extends ZipEntry> entries = z.entries();
            for (int n = 0; n < WALK_LIMIT && entries.hasMoreElements(); n++)
            {
                ZipEntry e = entries.nextElement();
                if (e.isDirectory() || e.getSize() != (bios ? 0x200000 : 0x4000))
                    continue;
                String name = e.getName();
                if (!bios && (name.equals(PIC) || e.getCrc() == PIC_CRC))
                    return true;
                if (bios)
                {
                    if (BIOS_FILES.contains(name))
                        return true;
                    for (long crc : BIOS_CRCS)
                        if (e.getCrc() == crc)
                            return true;
                }
            }
        } catch (IOException | RuntimeException e) {
            // Missing, unreadable or invalid ZIP.
        }
        return false;
    }

    /** Bounded CHD/map headers check geometry and distinguish original GDS-0018 from Rev A. */
    private static boolean originalChd(File disc)
    {
        try (RandomAccessFile in = new RandomAccessFile(disc, "r")) {
            byte[] start = new byte[16];
            in.readFully(start);
            ByteBuffer b = ByteBuffer.wrap(start);
            byte[] tag = new byte[8];
            b.get(tag);
            int length = b.getInt(), version = b.getInt();
            int expected = version == 3 ? 120 : version == 4 ? 108 : version == 5 ? 124 : 0;
            if (!Arrays.equals(tag, "MComprHD".getBytes(StandardCharsets.US_ASCII)) || expected == 0
                    || length != expected || disc.length() <= length)
                return false;
            byte[] header = new byte[length];
            System.arraycopy(start, 0, header, 0, start.length);
            in.readFully(header, start.length, length - start.length);
            b = ByteBuffer.wrap(header);
            long fileSize = disc.length();
            long hunk = Integer.toUnsignedLong(b.getInt(version == 3 ? 76 : version == 4 ? 44 : 56));
            long logical = b.getLong(version < 5 ? 28 : 32);
            long maxHunk = version < 5 ? 16L * 1024 * 1024 : 128L * 1024 * 1024;
            // libchdr bounds allocation sizes, and the disc reader requires whole CD frames.
            if (hunk == 0 || hunk >= maxHunk || hunk % 2448 != 0 || logical <= 0)
                return false;
            long hunks = version < 5 ? Integer.toUnsignedLong(b.getInt(24)) : 1 + (logical - 1) / hunk;
            if (hunks == 0 || hunks > (10L * 1024 * 1024 * 1024 - 1) / hunk
                    || (version == 5 && b.getInt(60) == 0))
                return false;
            long metadata = b.getLong(version < 5 ? 36 : 48);
            if (metadata != 0 && (metadata < length || metadata > fileSize - 16))
                return false;
            if (version < 5)
            {
                // Legacy maps follow the header and include a 16-byte end cookie.
                if ((b.getInt(16) & ~2) != 0 || length + (hunks + 1) * 16 > fileSize)
                    return false;
            }
            else
            {
                long map = b.getLong(40);
                boolean compressed = b.getInt(16) != 0;
                long mapSize = hunks * (compressed ? 12 : 4);
                if (map < length || mapSize > Integer.MAX_VALUE || map > fileSize - (compressed ? 16 : mapSize + 1))
                    return false;
                if (compressed)
                {
                    // Read only the fixed map header, never the map/disc payload.
                    in.seek(map);
                    long compressedSize = Integer.toUnsignedLong(in.readInt());
                    if (compressedSize == 0 || compressedSize > fileSize - map - 16)
                        return false;
                }
            }
            int sha = version == 3 ? 80 : version == 4 ? 48 : 84;
            int parent = version == 3 ? 100 : version == 4 ? 68 : 104;
            for (int i = parent; i < parent + 20; i++)
                if (header[i] != 0)
                    return false;
            if (version < 5 && (header[19] & 1) != 0)
                return false;
            String wanted = version == 3 ? ORIGINAL_RAW_SHA1 : ORIGINAL_SHA1;
            for (int i = 0; i < 20; i++)
                if ((header[sha + i] & 0xff) != Integer.parseInt(wanted.substring(2 * i, 2 * i + 2), 16))
                    return false;
            return true;
        } catch (IOException | RuntimeException e) {
            return false;
        }
    }

    private static final Pattern GDI_TRACK = Pattern.compile("^(\\d+)\\s+(\\d+)\\s+([04])\\s+(2048|2352)\\s+(?:\"([^\"]+)\"|(\\S+))\\s+(\\d+)\\s*$");
    private static final Pattern CUE_FILE = Pattern.compile("^FILE\\s+(?:\"([^\"]+)\"|(\\S+))\\s+BINARY\\s*$");
    private static final Pattern CUE_TRACK = Pattern.compile("^TRACK\\s+(\\d+)\\s+(AUDIO|CDG|MODE1/(?:2048|2352)|MODE2/(?:2336|2352)|CDI/(?:2336|2352))\\s*$");

    private static final Pattern CUE_INDEX = Pattern.compile("^INDEX\\s+(00|01)\\s+(\\d+):([0-5]\\d):([0-6]\\d|7[0-4])\\s*$");
    private static final Pattern CUE_SESSION = Pattern.compile("^REM SESSION (\\d+)$");

    /** CUE/GDI has no CHD identity header. Require the original set/disc names and every track;
     * raw-disc revision and physical headset boot remain unverified. */
    private static boolean validDescriptor(File descriptor, boolean gdi)
    {
        if (!nonEmpty(descriptor) || descriptor.length() >= (gdi ? 16384 : DESCRIPTOR_LIMIT))
            return false;
        try (BufferedReader in = new BufferedReader(new InputStreamReader(new FileInputStream(descriptor), StandardCharsets.UTF_8))) {
            int expected = 0, tracks = 0, files = 0, indexes = 0;
            boolean highDensity = false, highDensityTrack = false, firstTrackOfSession = false;
            long currentFad = 150, fileStartFad = 0, indexZero = -1;
            int sectorSize = 0, session = 0;
            File cueFile = null;
            String line;
            while ((line = in.readLine()) != null)
            {
                line = line.trim();
                if (line.isEmpty())
                    continue;
                if (gdi)
                {
                    if (expected == 0)
                    {
                        expected = Integer.parseInt(line);
                        if (expected < 3 || expected > 99)
                            return false;
                        continue;
                    }
                    Matcher m = GDI_TRACK.matcher(line);
                    if (!m.matches() || Integer.parseInt(m.group(1)) != ++tracks
                            || !trackExists(descriptor, m.group(5) != null ? m.group(5) : m.group(6)))
                        return false;
                    int type = Integer.parseInt(m.group(3));
                    if (((tracks == 1 || tracks == 3) && type != 4) || (tracks == 2 && type != 0)
                            || (tracks == 3 && Long.parseLong(m.group(2)) != 45000))
                        return false;
                }
                else if (line.equals("REM HIGH-DENSITY AREA") || line.equals("REM SINGLE-DENSITY AREA")
                        || CUE_SESSION.matcher(line).matches())
                {
                    if (tracks != indexes)
                        return false;
                    if (line.equals("REM HIGH-DENSITY AREA"))
                    {
                        highDensity = true;
                        currentFad = 45150;
                    }
                    else
                    {
                        Matcher m = CUE_SESSION.matcher(line);
                        if (m.matches())
                        {
                            int next = Integer.parseInt(m.group(1));
                            if (next == 0)
                                return false;
                            if (next != session && next == 2)
                            {
                                currentFad = Math.addExact(currentFad, 6750 + 4500 + 150);
                                firstTrackOfSession = true;
                                highDensity = true;
                            }
                            session = next;
                        }
                    }
                    // The native parser clears FILE/TRACK context at these REM boundaries.
                    cueFile = null;
                    sectorSize = 0;
                }
                else if (line.startsWith("FILE"))
                {
                    Matcher m = CUE_FILE.matcher(line);
                    if (!m.matches() || tracks != indexes
                            || !trackExists(descriptor, m.group(1) != null ? m.group(1) : m.group(2)))
                        return false;
                    cueFile = new File(descriptor.getParentFile(), m.group(1) != null ? m.group(1) : m.group(2));
                    fileStartFad = currentFad;
                    sectorSize = 0;
                    files++;
                }
                else if (line.startsWith("TRACK"))
                {
                    Matcher m = CUE_TRACK.matcher(line);
                    if (cueFile == null || tracks != indexes || !m.matches() || Integer.parseInt(m.group(1)) != ++tracks)
                        return false;
                    boolean audio = m.group(2).equals("AUDIO") || m.group(2).equals("CDG");
                    if (((tracks == 1 || tracks == 3) && audio) || (tracks == 2 && !audio))
                        return false;
                    int nextSize = audio ? 2352 : Integer.parseInt(m.group(2).substring(m.group(2).indexOf('/') + 1));
                    if (sectorSize == 0)
                        currentFad = Math.addExact(currentFad, cueFile.length() / nextSize);
                    sectorSize = nextSize;
                    indexZero = -1;
                    if (tracks == 3)
                        highDensityTrack = highDensity;
                }
                else if (line.startsWith("INDEX"))
                {
                    Matcher m = CUE_INDEX.matcher(line);
                    if (!m.matches() || cueFile == null || indexes + 1 != tracks)
                        return false;
                    long frame = Math.addExact(Math.multiplyExact(Long.parseLong(m.group(2)), 4500),
                            Integer.parseInt(m.group(3)) * 75L + Integer.parseInt(m.group(4)));
                    if (m.group(1).equals("00"))
                    {
                        if (indexZero >= 0)
                            return false;
                        indexZero = frame;
                    }
                    else
                    {
                        if (indexZero > frame)
                            return false;
                        if (firstTrackOfSession && frame > 0)
                        {
                            long pregap = Math.min(frame, 150);
                            fileStartFad -= pregap;
                            currentFad -= pregap;
                        }
                        firstTrackOfSession = false;
                        if (tracks == 3 && (!highDensityTrack || Math.addExact(fileStartFad, frame) != 45150))
                            return false;
                        indexes++;
                    }
                }
                if (tracks > 99 || files > 99)
                    return false;
            }
            return gdi ? expected >= 3 && tracks == expected : tracks >= 3 && tracks == indexes && highDensityTrack;
        } catch (IOException | RuntimeException e) {
            return false;
        }
    }

    private static boolean trackExists(File descriptor, String path)
    {
        return nonEmpty(new File(descriptor.getParentFile(), path));
    }

    private static String place(File dir, File root)
    {
        String path = dir.getAbsolutePath(), top = root.getAbsolutePath();
        if (path.startsWith(top))
            return (root.getName() + path.substring(top.length())).replace(File.separatorChar, '/');
        return dir.getName();
    }
}
