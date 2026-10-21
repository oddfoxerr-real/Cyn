// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;
import java.util.regex.*;

/** Studio channel only. Legacy Android and Windows releases have different identities. */
public final class AndroidRelease {
    private static final String TAG_PREFIX="android-studio-v";
    private static final Pattern VERSION=Pattern.compile("^(\\d{1,6})\\.(\\d{1,6})\\.(\\d{1,6})-preview$");
    public static long number(String version) {
        Matcher m=VERSION.matcher(version); if(!m.matches()) return -1;
        return Long.parseLong(m.group(1))*1_000_000_000_000L+Long.parseLong(m.group(2))*1_000_000L+Long.parseLong(m.group(3));
    }
    public static String newerVersion(String tag,String current) {
        if(tag==null||!tag.startsWith(TAG_PREFIX)) return null;
        String v=tag.substring(TAG_PREFIX.length()); return number(v)>number(current)&&number(current)>=0?v:null;
    }
    public static String assetUrl(String version) {
        if(number(version)<0) throw new IllegalArgumentException("Invalid Android release version");
        return "https://github.com/oddfoxerr-real/Cyn/releases/download/"+TAG_PREFIX+version+"/Cyn-Studio-Android-"+version+".apk";
    }
}
