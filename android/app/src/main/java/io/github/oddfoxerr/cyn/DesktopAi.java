// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;
import android.content.Context;
import android.os.SystemClock;
import ai.onnxruntime.*;
import java.io.*;
import java.nio.FloatBuffer;
import java.security.MessageDigest;
import java.util.*;
import java.util.concurrent.*;

final class DesktopAi {
    static final class Result {
        final long at;final float[] features,scene,decision,apex;final long milliseconds;
        Result(float[] f,float[] s,float[] d,float[] a,long ms){features=f;scene=s;decision=d;apex=a;milliseconds=ms;at=SystemClock.elapsedRealtime();}
    }
    final long engine=NativeEngine.create();
    volatile boolean active,apexEnabled,movie;
    volatile Result latest;
    volatile String status="Ready for library playback";
    private final Context context;
    private final ScheduledExecutorService worker=Executors.newSingleThreadScheduledExecutor(r->{Thread t=new Thread(r,"CYN neural worker");t.setPriority(Thread.MIN_PRIORITY);return t;});
    private OrtSession yamnet,apex;
    private OrtEnvironment environment;
    private final ApexFeatures stft=new ApexFeatures();
    private float[] previousScene, previousEmbedding;
    private volatile long generation;
    private long workerGeneration=-1;
    void reset(){generation++;latest=null;}
    private long retryAt;
    private volatile boolean closed;
    DesktopAi(Context context){this.context=context.getApplicationContext();worker.scheduleWithFixedDelay(this::infer,0,650,TimeUnit.MILLISECONDS);}
    private String model(String name,String hash)throws Exception{
        File directory=new File(context.getFilesDir(),"models");if(!directory.isDirectory()&&!directory.mkdirs())throw new IOException("Model folder unavailable");
        File target=new File(directory,name);
        if(target.isFile()&&hash.equals(digest(target)))return target.getAbsolutePath();
        File temporary=new File(directory,name+".part");
        try(InputStream in=context.getAssets().open("models/"+name);OutputStream out=new FileOutputStream(temporary)){byte[] b=new byte[65536];int n;while((n=in.read(b))!=-1)out.write(b,0,n);}
        if(!hash.equals(digest(temporary))){temporary.delete();throw new IOException("Model checksum mismatch");}
        if(target.exists()&&!target.delete())throw new IOException("Could not replace model");
        if(!temporary.renameTo(target))throw new IOException("Could not save model");return target.getAbsolutePath();
    }
    static String digest(File file)throws Exception{MessageDigest digest=MessageDigest.getInstance("SHA-256");try(InputStream in=new FileInputStream(file)){byte[]b=new byte[65536];int n;while((n=in.read(b))!=-1)digest.update(b,0,n);}StringBuilder s=new StringBuilder();for(byte b:digest.digest())s.append(String.format(Locale.ROOT,"%02x",b&255));return s.toString();}
    private OrtSession open(String name,String hash)throws Exception{
        if(environment==null)environment=OrtEnvironment.getEnvironment();
        try(OrtSession.SessionOptions options=new OrtSession.SessionOptions()){options.setIntraOpNumThreads(2);options.setInterOpNumThreads(1);options.setOptimizationLevel(OrtSession.SessionOptions.OptLevel.ALL_OPT);return environment.createSession(model(name,hash),options);}
    }
    private static float finite(float value){return Float.isFinite(value)?Math.max(0,Math.min(1,value)):0;}
    private static float max(float[] s,int... indices){float v=0;for(int i:indices)v=Math.max(v,finite(s[i]));return v;}
    private float[] scene(float[] scores,float[] embedding){
        float[] s={max(scores,132,133),max(scores,0,2,3),max(scores,156,157,158,159),max(scores,163,238),max(scores,234,235,236,240,212,237),max(scores,24,249),max(scores,420,421,430,487),max(scores,265,267),finite(scores[494]),0,0};
        float total=0,count=0;for(float score:scores){float p=finite(score);total+=p;if(p>.15)count++;}s[9]=Math.min(1,.5f*total/20+.5f*count/40);
        float[] projected=new float[24];double norm=0;
        for(int i=0;i<1024;i++){float v=Float.isFinite(embedding[i])?Math.max(-100,Math.min(100,embedding[i])):0;long code=(i*2654435761L+1013904223L)&0xffffffffL;int bucket=(int)((code>>>12)%24);projected[bucket]+=(code&0x80000000L)!=0?v:-v;}
        for(float v:projected)norm+=v*v;if(norm>1e-12)for(int i=0;i<24;i++)projected[i]/=(float)Math.sqrt(norm);
        if(previousEmbedding!=null){float similarity=0;for(int i=0;i<24;i++)similarity+=projected[i]*previousEmbedding[i];s[10]=Math.max(0,Math.min(1,(1-similarity)*.8f));}previousEmbedding=projected;
        if(previousScene!=null)for(int i=0;i<11;i++)s[i]=previousScene[i]*.55f+s[i]*.45f;
        previousScene=s.clone();return s;
    }
    private void infer(){
        if(closed)return;
        if(!active){latest=null;previousScene=null;previousEmbedding=null;return;}
        if(SystemClock.elapsedRealtime()<retryAt)return;
        try{
            long epoch=generation;
            if(workerGeneration!=epoch){previousScene=null;previousEmbedding=null;workerGeneration=epoch;}
            float[] f=NativeEngine.features(engine);if(f[11]<.0001f){latest=null;status="Waiting for audible audio";return;}
            float[] wave=NativeEngine.yamnetWindow(engine);if(wave==null)return;
            long started=SystemClock.elapsedRealtime();status="Listening with YAMNet";
            if(yamnet==null)yamnet=open("yamnet-16k-0p975s.onnx","d3c173d26202dd94bf5532795a0f6e3090610a70adb2a773faa0ee635ec6acbe");
            float[] scores=null,embedding=null;
            try(OnnxTensor input=OnnxTensor.createTensor(environment,FloatBuffer.wrap(wave),new long[]{15600});OrtSession.Result out=yamnet.run(Collections.singletonMap(yamnet.getInputNames().iterator().next(),input))){
                for(Map.Entry<String,OnnxValue> entry:out){Object v=entry.getValue().getValue();if(v instanceof float[][]&&((float[][])v).length==1){if(((float[][])v)[0].length==521)scores=((float[][])v)[0].clone();else if(((float[][])v)[0].length==1024)embedding=((float[][])v)[0].clone();}}
            }
            if(scores==null||embedding==null)throw new IOException("YAMNet output shape mismatch");
            float[] s=scene(scores,embedding),decisions=NativeEngine.decide(f,s,movie),goals=new float[8];
            if(apexEnabled){
                float[] full=NativeEngine.apexWindow(engine);
                if(full!=null){
                    if(apex==null)apex=open("apex6m-225.onnx","89f1f3eecd7fc488afe770e8bb64666bad98c6dad6cf4ea4d03e713293c6365f");
                    stft.compute(full);
                    try(OnnxTensor input=OnnxTensor.createTensor(environment,FloatBuffer.wrap(stft.features),new long[]{1,513,189});OrtSession.Result out=apex.run(Collections.singletonMap(apex.getInputNames().iterator().next(),input))){
                        OnnxValue mask=out.get("mask").orElseThrow(()->new IOException("APEX mask missing"));Object value=mask.getValue();
                        if(!(value instanceof float[][][]))throw new IOException("APEX mask is not rank 3");float[][][] m=(float[][][])value;
                        if(m.length!=1||m[0].length!=513)throw new IOException("APEX mask dimensions mismatch");float[] flat=new float[513*189];
                        for(int k=0;k<513;k++){if(m[0][k].length!=189)throw new IOException("APEX frame count mismatch");System.arraycopy(m[0][k],0,flat,k*189,189);}goals=stft.goals(flat);
                    }
                }
            }
            if(active&&!closed&&epoch==generation){latest=new Result(f,s,decisions,goals,SystemClock.elapsedRealtime()-started);status=(apexEnabled?"APEX 6M + YAMNet":"YAMNet")+" · running locally";}
        }catch(Exception|LinkageError e){latest=null;status="AI unavailable: "+e.getClass().getSimpleName()+" · manual sound remains active";retryAt=SystemClock.elapsedRealtime()+15000;}
    }
    void closeAfterPlayer(){closed=true;active=false;worker.execute(()->{try{if(yamnet!=null)yamnet.close();if(apex!=null)apex.close();}catch(Exception ignored){}finally{NativeEngine.destroy(engine);}});worker.shutdown();}
}
