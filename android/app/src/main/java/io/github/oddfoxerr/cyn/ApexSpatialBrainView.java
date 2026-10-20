// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.os.SystemClock;
import android.view.View;

/**
 * Cinematic, brain-shaped 3D neural-network mesh for APEX 10M's mobile AI Hub.
 *
 * Nodes, branching connections and hues are an artistic representation, NOT
 * telemetry from an untrained reasoning network. Only wave emission,
 * brightness and rotation respond to measured native stereo-spatial DSP output.
 *
 * Fixed-size arrays are allocated once; onDraw has no bitmap/mesh allocations.
 */
final class ApexSpatialBrainView extends View {
    interface Signal { float activity(); boolean enabled(); boolean reduced(); }

    private static final int HEMI_NODES=150;
    private static final int N=HEMI_NODES*2;
    private static final int MAX_EDGES=1300;
    private static final int MAX_NEIGHBORS=14;
    private static final int WAVE_COUNT=14;
    private static final int ROUTE_LEN=7;
    private static final float GOLDEN=.61803398875f;
    private static final int[] NEON={
        0xff55f0ff,0xffff55d9,0xff9fff66,0xffffbb55,
        0xff8f8bff,0xffff7580,0xff50ffc3,0xff87b7ff
    };
    private final float[] modelX=new float[N],modelY=new float[N],modelZ=new float[N];
    private final float[] screenX=new float[N],screenY=new float[N],depth=new float[N];
    private final int[] edgeA=new int[MAX_EDGES],edgeB=new int[MAX_EDGES];
    private final int[][] adjacent=new int[N][MAX_NEIGHBORS];
    private final int[] degrees=new int[N];
    private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Wave[] waves=new Wave[WAVE_COUNT];
    private final Signal source;
    private final int accent,secondary;
    private int edgeCount,randomState=0x3a14f1c7,colourCursor=0;
    private float angle=.37f,tilt=.08f,energy,lastMeasured,travelTime;
    private long lastFrame,lastWave;

    private static final class Wave {
        final int[] route=new int[ROUTE_LEN];
        int hue;
        float distance=99f,rate=1f,power;
    }
    ApexSpatialBrainView(Context context,Signal feed,int mainColour,int secondaryColour){
        super(context);
        source=feed;accent=mainColour;secondary=secondaryColour;
        setContentDescription("CYN APEX 10M holographic brain mesh with interconnected nodes and neon signal paths. Coloured thoughts are illustrative; their pulse intensity follows spatial audio processing.");
        setImportantForAccessibility(IMPORTANT_FOR_ACCESSIBILITY_NO);
        paint.setStrokeCap(Paint.Cap.ROUND);
        for(int i=0;i<WAVE_COUNT;i++)waves[i]=new Wave();
        generateBrain();
        generateGraph();
    }
    private static float clamp(float v,float a,float b){return Math.max(a,Math.min(b,v));}
    private static float fract(float v){return v-(float)Math.floor(v);}
    private int rand(){
        int s=randomState;
        s^=s<<13;s^=s>>>17;s^=s<<5;
        randomState=s;
        return s&0x7fffffff;
    }
    private static float sq(float x){return x*x;}
    private void generateBrain(){
        // Two organic hemispheres with central fissure, frontal/occipital lobes,
        // folds and irregular 3D placement: unlike a latitude/longitude wire ball.
        for(int h=0;h<2;h++) for(int i=0;i<HEMI_NODES;i++){
            int n=h*HEMI_NODES+i;
            float q=(i+.5f)/HEMI_NODES;
            float latitude=(float)Math.asin(clamp(2*q-1,-1,1));
            float longitude=(float)Math.PI*fract((i+.45f)*GOLDEN+h*.237f);
            float radius=(float)Math.cos(latitude);
            float fold=.048f*(float)Math.sin(longitude*12+latitude*9+h);
            float asymmetry=.027f*(float)Math.sin(i*1.73f+h*3.3f);
            float side=h==0?-1:1;
            float sagittal=(float)Math.sin(longitude);
            float silhouette=.51f+fold+asymmetry;
            modelX[n]=side*(.095f+radius*silhouette*sagittal);
            modelY[n]=.76f*(float)Math.sin(latitude)
                +.035f*(float)Math.sin(longitude*7+latitude*13);
            modelZ[n]=.72f*radius*(float)Math.cos(longitude)
                +.027f*(float)Math.sin(latitude*11+longitude*8);
            // Subtle temporal-lobe fullness and non-spherical back contour.
            if(modelY[n]<-.23f)modelX[n]*=1.06f;
        }
    }
    private void addEdge(int a,int b){
        if(a==b||edgeCount>=MAX_EDGES)return;
        for(int i=0;i<degrees[a];i++)if(adjacent[a][i]==b)return;
        if(degrees[a]>=MAX_NEIGHBORS||degrees[b]>=MAX_NEIGHBORS)return;
        edgeA[edgeCount]=a;edgeB[edgeCount]=b;edgeCount++;
        adjacent[a][degrees[a]++]=b;
        adjacent[b][degrees[b]++]=a;
    }
    private void generateGraph(){
        // k-nearest-neighbor dendrite graph, giving branching multi-connected
        // routes rather than predictable squares in a rotating latitude mesh.
        for(int i=0;i<N;i++){
            int start=(i/HEMI_NODES)*HEMI_NODES;
            int end=start+HEMI_NODES;
            int[] nearest={-1,-1,-1,-1,-1};
            float[] distances={Float.MAX_VALUE,Float.MAX_VALUE,
                Float.MAX_VALUE,Float.MAX_VALUE,Float.MAX_VALUE};
            for(int j=start;j<end;j++){
                if(i==j)continue;
                float d=sq(modelX[i]-modelX[j])
                    +sq(modelY[i]-modelY[j])+sq(modelZ[i]-modelZ[j]);
                for(int slot=0;slot<nearest.length;slot++)if(d<distances[slot]){
                    for(int k=nearest.length-1;k>slot;k--){
                        distances[k]=distances[k-1];nearest[k]=nearest[k-1];
                    }
                    distances[slot]=d;nearest[slot]=j;break;
                }
            }
            for(int id:nearest)if(id>=0)addEdge(i,id);
        }
        // An understated corpus-callosum bridge, not an artificial sphere seam.
        for(int i=0;i<HEMI_NODES;i++){
            int right=HEMI_NODES+i;
            if(Math.abs(modelY[i])<.29f&&Math.abs(modelZ[i])<.39f
                    &&Math.abs(modelX[i])<.34f){
                int opposite=-1;
                float best=Float.MAX_VALUE;
                for(int j=HEMI_NODES;j<N;j++){
                    float d=sq(modelY[i]-modelY[j])
                        +sq(modelZ[i]-modelZ[j])
                        +.4f*sq(Math.abs(modelX[i])-Math.abs(modelX[j]));
                    if(d<best){best=d;opposite=j;}
                }
                if(opposite>=0 && (i%3==0))addEdge(i,opposite);
            }
        }
    }
    private void project(float cx,float cy,float scale){
        float ca=(float)Math.cos(angle),sa=(float)Math.sin(angle);
        float ct=(float)Math.cos(tilt),st=(float)Math.sin(tilt);
        for(int i=0;i<N;i++){
            float x=modelX[i]*ca-modelZ[i]*sa;
            float z=modelX[i]*sa+modelZ[i]*ca;
            float y=modelY[i]*ct-z*st;
            z=modelY[i]*st+z*ct;
            float perspective=2.8f/(2.8f+.43f*z);
            screenX[i]=cx+x*scale*perspective;
            screenY[i]=cy+y*scale*perspective;
            depth[i]=z;
        }
    }
    private void ink(int color,int opacity,Paint.Style style,float size){
        paint.setStyle(style);
        paint.setColor(color);
        paint.setAlpha(Math.max(0,Math.min(255,opacity)));
        paint.setStrokeWidth(size);
    }
    private void line(Canvas c,int a,int b,int hue,int alpha,float width){
        ink(hue,alpha,Paint.Style.STROKE,width);
        c.drawLine(screenX[a],screenY[a],screenX[b],screenY[b],paint);
    }
    private void circle(Canvas c,float x,float y,float radius,int hue,int opacity){
        ink(hue,opacity,Paint.Style.FILL,0);
        c.drawCircle(x,y,radius,paint);
    }
    private void launch(float measured,long now){
        if(measured<.00003f)return;
        // Deterministic spatial-energy-based cadence. Randomness only chooses
        // the visual hue and route, NOT fake neural inference events.
        long interval=(long)(790-620*clamp((float)Math.sqrt(measured)*4,0,1));
        boolean rise=measured>lastMeasured*1.45f&&measured>.00012f;
        if(now-lastWave<interval&&!rise)return;
        lastWave=now;
        int from=rand()%N;
        for(int attempt=0;attempt<8&&depth[from]<-.30f;attempt++)from=rand()%N;
        for(Wave wave:waves){
            if(wave.distance<ROUTE_LEN+.8f)continue;
            wave.route[0]=from;
            int previous=-1,current=from;
            for(int k=1;k<ROUTE_LEN;k++){
                int count=degrees[current],next=current;
                if(count>0){
                    int choice=rand()%count;
                    next=adjacent[current][choice];
                    if(next==previous&&count>1)
                        next=adjacent[current][(choice+1)%count];
                }
                wave.route[k]=next;
                previous=current;current=next;
            }
            wave.distance=0f;
            wave.hue=NEON[(colourCursor+++(rand()%3))%NEON.length];
            wave.rate=2.2f+4.6f*clamp((float)Math.sqrt(measured)*5,0,1);
            wave.power=clamp((float)Math.sqrt(measured)*4.4f,.2f,1f);
            break;
        }
    }
    private void drawGraph(Canvas c,float scale,float strength,int pass){
        // Rear connections first; front routes are brighter and legible.
        for(int i=0;i<edgeCount;i++){
            int a=edgeA[i],b=edgeB[i];
            boolean forward=(depth[a]+depth[b])>=0;
            if(forward!=(pass==1))continue;
            int hue=NEON[((a/17)+(b/23))%NEON.length];
            int alpha=(forward?92:38)+Math.round(strength*(forward?78:30));
            line(c,a,b,hue,alpha,forward?1.45f:0.85f);
        }
        // Fine cross-connection web at a lighter opacity: distinguishes
        // dendrites from the dominant nearest-neighbour pathways.
        if(pass==1)for(int i=0;i<N;i+=3){
            if(degrees[i]<3||depth[i]<-.08f)continue;
            int b=adjacent[i][2];
            line(c,i,b,secondary,35+Math.round(strength*60),.85f);
        }
        for(int i=0;i<N;i++){
            boolean front=depth[i]>=0;
            if(front!=(pass==1))continue;
            boolean hub=(i%17==0||degrees[i]>=9);
            int hue=hub?secondary:NEON[(i*7+i/11)%NEON.length];
            float r=front?(hub?3.7f:2.15f):(hub?2.1f:1.35f);
            if(front && (hub||i%4==0)){
                circle(c,screenX[i],screenY[i],r*3.2f,hue,
                    16+Math.round(strength*62));
                circle(c,screenX[i],screenY[i],r*1.8f,hue,
                    33+Math.round(strength*88));
            }
            circle(c,screenX[i],screenY[i],r,hue,
                (front?200:80)+Math.round(strength*(front?52:34)));
            if(hub&&front)circle(c,screenX[i],screenY[i],1.1f,Color.WHITE,
                146+Math.round(strength*95));
        }
    }
    private void drawWaves(Canvas c,float strength){
        for(Wave wave:waves){
            if(wave.distance>ROUTE_LEN-1+.5f)continue;
            float position=clamp(wave.distance,0,ROUTE_LEN-1.001f);
            int index=(int)position;
            int start=wave.route[index],end=wave.route[index+1];
            float fraction=position-index;
            int lead=Math.min(255,Math.round(165+90*strength));
            // Glowing full-path segment, as in cinematic signal propagation.
            for(int k=0;k<index;k++){
                line(c,wave.route[k],wave.route[k+1],wave.hue,
                    Math.max(23,Math.round(105*wave.power)),2.35f);
            }
            line(c,start,end,wave.hue,65+Math.round(strength*125),2.8f);
            float x=screenX[start]+(screenX[end]-screenX[start])*fraction;
            float y=screenY[start]+(screenY[end]-screenY[start])*fraction;
            circle(c,x,y,13.0f,wave.hue,30+Math.round(strength*68));
            circle(c,x,y,7.1f,wave.hue,90+Math.round(strength*85));
            circle(c,x,y,3.5f,Color.WHITE,lead);
            // Illuminate the destination and a connected branch node.
            circle(c,screenX[end],screenY[end],7.0f,wave.hue,
                34+Math.round(strength*118));
            if(degrees[end]>2){
                int fork=adjacent[end][2];
                line(c,end,fork,wave.hue,35+Math.round(strength*116),1.5f);
            }
        }
    }
    @Override protected void onDraw(Canvas canvas){
        super.onDraw(canvas);
        int w=getWidth(),h=getHeight();
        if(w<80||h<80)return;
        long now=SystemClock.elapsedRealtime();
        float dt=lastFrame==0?0:Math.min(.06f,(now-lastFrame)*.001f);
        lastFrame=now;
        boolean playing=source.enabled(),reduced=source.reduced();
        float measured=playing?clamp(source.activity(),0,1):0;
        if(!Float.isFinite(measured))measured=0;
        energy+=(measured-energy)*Math.min(1,dt*(measured>energy?9:4));
        float level=clamp((float)Math.sqrt(energy)*5.0f,0,1);
        if(playing&&!reduced){
            // Slow autonomous orientation; brightness and energy paths remain
            // faithful to the available measured stereo signal.
            angle+=dt*(.08f+level*.74f);
            tilt=.08f+.12f*(float)Math.sin(angle*.35f);
            travelTime+=dt;
            launch(measured,now);
            for(Wave wave:waves)if(wave.distance<ROUTE_LEN+.8f)
                wave.distance+=dt*wave.rate;
        }
        lastMeasured=measured;
        float cx=w*.5f,cy=h*.445f;
        float scale=Math.min(w*.59f,h*.405f);
        project(cx,cy,scale);
        // Background rings suggest a holographic diagnostic rather than a ball.
        ink(accent,48+Math.round(level*86),Paint.Style.STROKE,1.15f);
        canvas.drawOval(cx-scale*1.34f,cy-scale*1.13f,
            cx+scale*1.34f,cy+scale*1.13f,paint);
        ink(secondary,30+Math.round(level*82),Paint.Style.STROKE,.9f);
        canvas.drawOval(cx-scale*1.16f,cy-scale*.91f,
            cx+scale*1.16f,cy+scale*.91f,paint);

        drawGraph(canvas,scale,level,0);
        drawGraph(canvas,scale,level,1);
        if(playing)drawWaves(canvas,level);

        paint.setTypeface(Typeface.create("sans-serif-medium",Typeface.BOLD));
        paint.setTextSize(Math.max(12,h*.050f));
        ink(accent,245,Paint.Style.FILL,0);
        canvas.drawText(playing?"SPATIAL NETWORK  /  LIVE":"SPATIAL NETWORK  /  STANDBY",
            16,h-44,paint);
        paint.setTypeface(Typeface.create("sans-serif",Typeface.NORMAL));
        paint.setTextSize(Math.max(10,h*.035f));
        ink(secondary,220,Paint.Style.FILL,0);
        canvas.drawText("Measured spatial energy: "+Math.round(measured*100)+"%",16,h-26,paint);
        canvas.drawText("Coloured pathways are artistic, not trained neuron activations",
            16,h-9,paint);
        if(playing&&!reduced&&isAttachedToWindow())postInvalidateDelayed(48);
    }
}
