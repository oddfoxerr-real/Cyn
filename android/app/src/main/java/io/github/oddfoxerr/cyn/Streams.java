// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;
import java.io.*;
import java.nio.charset.StandardCharsets;
final class Streams {
    static String text(InputStream in) throws IOException {
        ByteArrayOutputStream out=new ByteArrayOutputStream(); byte[] buffer=new byte[4096]; int count;
        while((count=in.read(buffer))!=-1) out.write(buffer,0,count);
        return out.toString(StandardCharsets.UTF_8.name());
    }
}
