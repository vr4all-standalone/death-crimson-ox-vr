// SPDX-License-Identifier: GPL-2.0-or-later
package com.flycast.emulator;

import java.io.*;
import java.util.*;
import java.util.zip.*;

/** Verified Japan Rev A cartridge set. Checks all required chip lengths and streamed CRCs.
 * This app needs no CHD. Plain Java so the setup and explicit-launch policy share JVM tests. */
final class DeathCrimsonFiles {
    private DeathCrimsonFiles() {}
    static final String ZIP = "deathcoxj.zip", BIOS_ZIP = "naomi.zip";
    static final int WALK_LIMIT = 5000;
    private static final int DEPTH = 3, ZIP_ENTRY_LIMIT = 128;
    private static final String[] CHIPS = {"epr-23524a.ic22","mpr-23514.ic1","mpr-23515.ic2","mpr-23516.ic3","mpr-23517.ic4","mpr-23518.ic5","mpr-23519.ic6","mpr-23520.ic7","mpr-23521.ic8","mpr-23522.ic9","mpr-23523.ic10"};
    private static final long[] CRCS = {0xb60e113eL,0x1f2b090eL,0xdc8557ebL,0x94494cbbL,0x69ba6a41L,0x49882766L,0xcdc82805L,0x1a268360L,0xcf8674b8L,0x7ae6716eL,0xc91efb67L};
    // Supported h/g Japanese/US/export BIOSes and the native loader's multi-region BIOS.
    private static final String[] BIOS_NAMES = {"epr-21576h.ic27","epr-21576g.ic27","epr-21578h.ic27","epr-21578g.ic27","epr-21577h.ic27","epr-21577g.ic27","epr-21579d.ic27","epr-21576h_multi.ic27"};
    private static final long[] BIOS_CRCS = {0xd4895685L,0xd2a1c6bfL,0x7b452946L,0x55413214L,0xfdf17452L,0x25f64af7L,0x33513691L,0xcce01f1fL};
    static final class Result {
        File zip;
        boolean bios;
        final List<String> missing = new ArrayList<>();
    }
    static Result find(List<File> roots) {
        List<File[]> candidates = new ArrayList<>();
        for(File root:roots) if(root!=null) search(root,root,DEPTH,candidates,new int[]{WALK_LIMIT});
        Result best=null;
        for(File[] candidate:candidates) {
            Result r=check(candidate[0],candidate[1]);
            if(best==null || r.missing.size()<best.missing.size()) best=r;
            if(r.missing.isEmpty()) break;
        }
        if(best!=null) return best;
        Result r=new Result();
        for(File root:roots) if(root!=null && biosState(new File(root,BIOS_ZIP))==1) {r.bios=true;break;}
        r.missing.add(ZIP+": the complete Death Crimson OX Japan Rev A MAME cartridge set, in Download. No CHD is needed.");
        if(!r.bios) r.missing.add(BIOS_ZIP+": a supported NAOMI BIOS beside "+ZIP+", or its BIOS file inside the game ZIP");
        return r;
    }
    static String launchPath(File requested,List<File> roots) {
        Result r;
        if(requested==null) r=find(roots);
        else {
            requested=requested.getAbsoluteFile();
            if(!requested.getName().equals(ZIP) || !requested.isFile()) return null;
            r=check(requested,requested.getParentFile());
        }
        return r.zip!=null && r.missing.isEmpty()?r.zip.getAbsolutePath():null;
    }
    private static void search(File dir,File root,int depth,List<File[]> out,int[] left) {
        if(left[0]<=0) return;
        File[] files=dir.listFiles();if(files==null)return;Arrays.sort(files);
        int count=Math.min(left[0],files.length);left[0]-=count;
        for(int i=0;i<count;i++) {
            String name=files[i].getName();
            if((name.equalsIgnoreCase(ZIP)||name.equalsIgnoreCase("deathcox.zip")||name.equalsIgnoreCase("deathcoxo.zip"))&&files[i].isFile()) out.add(new File[]{files[i],root});
        }
        if(depth>0) for(int i=0;i<count&&left[0]>0;i++) if(!files[i].getName().startsWith(".")&&files[i].isDirectory()) search(files[i],root,depth-1,out,left);
    }
    private static Result check(File found,File root) {
        Result r=new Result();r.zip=found;
        String where=found.getParentFile().getAbsolutePath();
        if(!found.getName().equals(ZIP)) {
            if(found.getName().equalsIgnoreCase(ZIP))
                r.missing.add(ZIP+": rename "+found.getName()+" using lowercase letters (in "+where+")");
            else
                r.missing.add(ZIP+": this app verifies Japan Rev A only. An older merged deathcox.zip containing epr-23524a.ic22 and all ten mpr chips needs a separate copy named deathcoxj.zip. Current deathcox.zip is USA; deathcoxo.zip is original Japan and is not verified. Keep your originals unchanged.");
        } else {
            String error=gameError(found);
            if(error!=null) r.missing.add(ZIP+": "+error+" (in "+where+")");
        }
        int mergedBios=biosState(found), besideBios=biosState(new File(found.getParentFile(),BIOS_ZIP));
        r.bios=mergedBios>=0 && besideBios>=0 && (mergedBios==1 || besideBios==1);
        if(!r.bios)r.missing.add(BIOS_ZIP+": a supported NAOMI BIOS beside "+ZIP+" (in "+where+"), or its BIOS file inside the game ZIP");
        return r;
    }
    private static List<? extends ZipEntry> entries(ZipFile zip) throws IOException {
        if(zip.size()>ZIP_ENTRY_LIMIT) throw new IOException("too many ZIP entries (limit "+ZIP_ENTRY_LIMIT+")");
        return Collections.list(zip.entries());
    }
    private static String gameError(File archive) {
        try(ZipFile zip=new ZipFile(archive)) {
            List<? extends ZipEntry> entries=entries(zip);
            for(int i=0;i<CHIPS.length;i++) {
                long length=i==0?0x400000:0x800000;ZipEntry match=null;
                // Match the native loader's CRC lookup, including renamed chip files.
                for(ZipEntry e:entries) if(!e.isDirectory()&&e.getCrc()==CRCS[i]&&e.getSize()==length) {match=e;break;}
                if(match==null) return "needs "+CHIPS[i]+" ("+length+" bytes, CRC "+String.format(Locale.ROOT,"%08x",CRCS[i])+") in the complete Japan Rev A set";
                if(!validPayload(zip,match,length,CRCS[i])) return "corrupt or truncated chip "+CHIPS[i];
            }
            return null;
        } catch(IOException|RuntimeException e) {return "unreadable or invalid ZIP; use a complete Japan Rev A cartridge archive";}
    }
    /** -1 means a corrupt known BIOS that the native filename fallback could select;
     * 0 means absent; 1 means a verified BIOS. Corrupt preferred files cannot hide behind
     * another valid revision in the game or beside archive. */
    private static int biosState(File archive) {
        try(ZipFile zip=new ZipFile(archive)) {
            boolean usable=false;
            for(ZipEntry entry:entries(zip)) {
                if(entry.isDirectory()) continue;
                boolean verified=false;
                for(int i=0;i<BIOS_NAMES.length;i++) if(entry.getName().equals(BIOS_NAMES[i])) {
                    if(entry.getSize()!=0x200000 || entry.getCrc()!=BIOS_CRCS[i]
                            || !validPayload(zip,entry,0x200000,BIOS_CRCS[i])) return -1;
                    usable=true;verified=true;break;
                }
                if(!verified && entry.getSize()==0x200000)
                    for(long crc:BIOS_CRCS) if(entry.getCrc()==crc && validPayload(zip,entry,0x200000,crc)) {usable=true;break;}
            }
            return usable?1:0;
        } catch(IOException|RuntimeException e) {return archive.isFile()?-1:0;}
    }

    private static boolean validPayload(ZipFile zip,ZipEntry entry,long length,long crc) throws IOException {
        CRC32 actual=new CRC32();byte[] buffer=new byte[65536];long total=0;
        try(InputStream in=zip.getInputStream(entry)) {
            for(int n;(n=in.read(buffer))!=-1;) {
                total+=n;if(total>length)return false;actual.update(buffer,0,n);
            }
        }
        return total==length&&actual.getValue()==crc;
    }
}