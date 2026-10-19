// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;
import java.io.*;
import java.nio.*;
import java.nio.file.*;
import java.util.*;
public final class ApexFeaturesTest {
    public static void main(String[]args)throws Exception{
        ApexFeatures f=new ApexFeatures();float[]wave=new float[48000];for(int i=0;i<wave.length;i++)wave[i]=(float)(.2*Math.sin(2*Math.PI*440*i/48000)+.07*Math.sin(2*Math.PI*87*i/48000));f.compute(wave);
        float[]mask=new float[513*189];Arrays.fill(mask,1);for(float g:f.goals(mask))if(Math.abs(g)>1e-6)throw new AssertionError("Unity mask must have no EQ coloration");
        for(float x:f.features)if(!Float.isFinite(x)||x<0)throw new AssertionError("Invalid model feature");
        ByteBuffer out=ByteBuffer.allocate(f.features.length*4).order(ByteOrder.LITTLE_ENDIAN);for(float x:f.features)out.putFloat(x);Files.write(Path.of(args[0]),out.array());
        System.out.println("PASS: APEX dimensions, finite input, unity-mask neutrality; wrote numerical parity fixture");
    }
}
