// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;
import android.content.Context;
import android.os.SystemClock;
import org.json.JSONObject;
import java.nio.charset.StandardCharsets;
import ai.onnxruntime.*;
import java.io.*;
import java.nio.FloatBuffer;
import java.security.MessageDigest;
import java.util.*;
import java.util.concurrent.*;

final class DesktopAi {
    // The beta neural head is a submode of the one selected APEX 10M profile.
    // Policy decisions are short-lived and only ever REDUCE existing spatial DSP.
    static final class PartialPolicy {
        final long at;
        final float gain,spatial,bass,kick;
        PartialPolicy(float gain,float spatial,float bass,float kick) {
            this.gain=gain;this.spatial=spatial;this.bass=bass;this.kick=kick;
            this.at=SystemClock.elapsedRealtime();
        }
    }
    static final class AudonicPolicy {
        final long at;
        final float[] effects;
        AudonicPolicy(float[] six){
            effects=six.clone();
            at=SystemClock.elapsedRealtime();
        }
    }
    volatile boolean audonicEnabled;
    volatile AudonicPolicy audonicPolicy;
    volatile String audonicStatus="Creative engine ready";
    private OrtSession audonicSession;
    private long audonicRetryAt;
    volatile boolean partialEnabled;
    volatile PartialPolicy partialPolicy;
    volatile String partialStatus="Ready for APEX 10M neural control";
    private OrtSession partialSession;
    private String partialHash;
    private long partialRetryAt;

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
    void reset(){generation++;latest=null;partialPolicy=null;audonicPolicy=null;}
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
    private void inferAudonic(long epoch){
        if(!audonicEnabled||!active){audonicPolicy=null;return;}
        if(SystemClock.elapsedRealtime()<audonicRetryAt)return;
        try{
            float[] features=NativeEngine.spatialPolicyWindow(engine);
            if(features==null||features.length!=160){
                audonicPolicy=null;
                audonicStatus="Listening for the next musical moment";
                return;
            }
            for(float value:features)if(!Float.isFinite(value))
                throw new IOException("Invalid musical feature window");
            if(audonicSession==null){
                JSONObject manifest;
                try(InputStream in=context.getAssets().open("models/audonic-manifest.json")){
                    ByteArrayOutputStream output=new ByteArrayOutputStream();
                    byte[] buffer=new byte[4096];int read;
                    while((read=in.read(buffer))!=-1){
                        if(output.size()+read>8192)throw new IOException("Invalid AUDONIC manifest");
                        output.write(buffer,0,read);
                    }
                    manifest=new JSONObject(output.toString(StandardCharsets.UTF_8.name()));
                }
                if(!"AUDONIC neural creative-FX student".equals(manifest.optString("model"))
                    ||manifest.optInt("gradient_updates")<100
                    ||!manifest.optBoolean("apex10m_teacher_distillation",false)
                    ||manifest.optBoolean("three_b_teacher_trained",true)
                    ||manifest.optBoolean("real_music_quality_verified",true))
                    throw new IOException("Unverified experimental AUDONIC release");
                String hash=manifest.optString("onnx_sha256");
                if(!hash.matches("[0-9a-fA-F]{64}"))
                    throw new IOException("Missing AUDONIC signature");
                audonicSession=open("audonic-student.onnx",hash);
            }
            long begin=SystemClock.elapsedRealtime();
            float[][][] controls;
            try(OnnxTensor input=OnnxTensor.createTensor(environment,
                        FloatBuffer.wrap(features),new long[]{1,8,20});
                OrtSession.Result results=audonicSession.run(
                        Collections.singletonMap("features",input))){
                OnnxValue output=results.get("fx_controls")
                    .orElseThrow(()->new IOException("AUDONIC output unavailable"));
                if(!(output.getValue() instanceof float[][][]))
                    throw new IOException("AUDONIC output shape changed");
                controls=(float[][][])output.getValue();
                if(controls.length!=1||controls[0].length!=6||
                   controls[0][0].length!=20)
                    throw new IOException("AUDONIC control shape invalid");
            }
            float[] six=new float[6];
            for(int i=0;i<6;i++){
                six[i]=controls[0][i][19];
                if(!Float.isFinite(six[i]))throw new IOException("Invalid FX output");
                six[i]=finite(six[i]);
            }
            // Width and harmonic excitement require independent headroom,
            // bass-preservation and phase evidence from native PCM analysis.
            float coherence=Math.max(0f,Math.min(1f,(features[3*20+19]+.25f)/.8f));
            float headroom=Math.max(0f,Math.min(1f,features[6*20+19]));
            float vocal=Math.max(0f,Math.min(1f,features[5*20+19]));
            float bass=Math.max(0f,Math.min(1f,features[19]));
            six[0]*=headroom*(1-.65f*vocal);
            six[1]*=headroom;
            six[2]*=coherence*headroom*(1-.6f*vocal);
            six[3]*=headroom*(1-.6f*vocal);
            six[4]*=headroom*(1-.7f*vocal);
            six[5]*=headroom*(1-.55f*bass);
            if(audonicEnabled&&active&&!closed&&epoch==generation){
                audonicPolicy=new AudonicPolicy(six);
                audonicStatus="Creative AI live · "+(SystemClock.elapsedRealtime()-begin)+" ms";
            }
        }catch(Exception|LinkageError problem){
            audonicPolicy=null;
            audonicStatus="Creative engine unavailable · safe bypass";
            audonicRetryAt=SystemClock.elapsedRealtime()+15000;
        }
    }
    private void inferPartial(long epoch) {
        if(!partialEnabled||!active) {partialPolicy=null;return;}
        if(SystemClock.elapsedRealtime()<partialRetryAt)return;
        try {
            float[] window=NativeEngine.spatialPolicyWindow(engine);
            if(window==null||window.length!=160) {
                partialPolicy=null;
                partialStatus="Collecting 2 seconds of real stereo audio…";
                return;
            }
            for(float value:window)if(!Float.isFinite(value))
                throw new IOException("Invalid spatial-feature window");
            if(partialSession==null) {
                JSONObject manifest;
                try(InputStream in=context.getAssets().open("models/apex10m-partial-manifest.json")){
                    ByteArrayOutputStream out=new ByteArrayOutputStream();
                    byte[] b=new byte[4096];int n;
                    while((n=in.read(b))!=-1){if(out.size()+n>8192)throw new IOException("Manifest too large");out.write(b,0,n);}
                    manifest=new JSONObject(out.toString(StandardCharsets.UTF_8.name()));
                }
                if(!"APEX 10M experimental spatial-policy head (synthetic-only)".equals(manifest.optString("name"))
                    ||manifest.optBoolean("trained_on_real_music",true)
                    ||manifest.optBoolean("neural_playback_approved",true)
                    ||manifest.optDouble("frozen_percent_of_15h")<66.66
                    ||manifest.optDouble("frozen_percent_of_15h")>100.0
                    ||manifest.optLong("trained_spatial_head_parameters")!=3693992L)
                    throw new IOException("APEX release checkpoint metadata verification failed");
                partialHash=manifest.getString("onnx_sha256");
                if(!partialHash.matches("[0-9a-fA-F]{64}"))
                    throw new IOException("Unverified partial ONNX checksum");
                partialSession=open("apex10m-partial-policy.onnx",partialHash);
            }
            long started=SystemClock.elapsedRealtime();
            float[][][] controls;
            try(OnnxTensor tensor=OnnxTensor.createTensor(environment,FloatBuffer.wrap(window),new long[]{1,8,20});
                OrtSession.Result out=partialSession.run(Collections.singletonMap("features",tensor))){
                OnnxValue value=out.get("policy_controls").orElseThrow(()->new IOException("Partial policy output missing"));
                if(!(value.getValue() instanceof float[][][]))
                    throw new IOException("Partial policy output must have rank 3");
                controls=(float[][][])value.getValue();
                if(controls.length!=1||controls[0].length!=8||controls[0][0].length!=20)
                    throw new IOException("Partial policy output dimensions changed");
            }
            float spatial=controls[0][2][19],kick=controls[0][3][19],bass=controls[0][4][19];
            if(!Float.isFinite(spatial)||!Float.isFinite(kick)||!Float.isFinite(bass))
                throw new IOException("Non-finite partial policy output");
            spatial=finite(spatial);bass=finite(bass);kick=finite(kick);
            float correlation=window[3*20+19];
            // Independent safety gate: no new widening for anti-phase material.
            float coherence=Math.max(0f,Math.min(1f,(correlation+.3f)/.65f));
            float gain=Math.min(.70f,(.10f+.60f*spatial)*(1-.45f*bass)*(1-.45f*kick)*coherence);
            if(partialEnabled&&active&&!closed&&epoch==generation) {
                partialPolicy=new PartialPolicy(gain,spatial,bass,kick);
                partialStatus="Inference "+(SystemClock.elapsedRealtime()-started)+
                    " ms · spatial "+Math.round(spatial*100)+"% · control "+Math.round(gain*100)+"%";
            }
        }catch(Exception|LinkageError ex){
            partialPolicy=null;
            partialStatus="Neural engine unavailable: "+ex.getClass().getSimpleName()+" · safe bypass";
            partialRetryAt=SystemClock.elapsedRealtime()+15000;
        }
    }
    private void infer(){
        if(closed)return;
        if(!active){latest=null;partialPolicy=null;audonicPolicy=null;previousScene=null;previousEmbedding=null;return;}
        // APEX 10M inference is independent of YAMNet availability. The
        // previous placement after scene classification delayed or prevented
        // neural-triggered animations when scene analysis was unavailable.
        final long epoch=generation;
        inferPartial(epoch);
        inferAudonic(epoch);
        if(SystemClock.elapsedRealtime()<retryAt)return;
        try{
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
        }catch(Exception|LinkageError e){latest=null;partialPolicy=null;status="AI unavailable: "+e.getClass().getSimpleName()+" · manual sound remains active";retryAt=SystemClock.elapsedRealtime()+15000;}
    }
    void closeAfterPlayer(){closed=true;active=false;worker.execute(()->{try{if(yamnet!=null)yamnet.close();if(apex!=null)apex.close();if(partialSession!=null)partialSession.close();if(audonicSession!=null)audonicSession.close();}catch(Exception ignored){}finally{NativeEngine.destroy(engine);}});worker.shutdown();}
}
