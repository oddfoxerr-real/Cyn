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
    interface Signal {
        // Cached real audio + inference telemetry, updated by AudioService.
        float activity();
        float[] channels(); // bass, kick, spatial DSP, neural decision
        long decisionAt();
        boolean enabled();
        boolean connected();
        boolean external();
        float[] hidden();
        float[] connections();
        float[] outputs();
        boolean reduced();
        // Fresh six-channel AUDONIC policy only; never substitute generic AI inference.
        float[] creative();
        float creativeStrength();
    }

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
    private static final int[] CREATIVE_HUES={
        0xffff815a,0xffffc15d,0xff8e86ff,0xff51d8e9,0xffe57cdd,0xff69edaa
    };
    private static final String[] CREATIVE_NAMES={
        "HARMONICS","ATTACK","WIDTH","COLOUR","RHYTHM","OVERTONES"
    };
    private static final int[] CREATIVE_ANCHORS={29,81,127,176,230,279};
    private static final Typeface HEADING_FACE=Typeface.create("sans-serif-medium",Typeface.BOLD);
    private static final Typeface BODY_FACE=Typeface.create("sans-serif",Typeface.NORMAL);
    private final float[] modelX=new float[N],modelY=new float[N],modelZ=new float[N];
    private final float[] screenX=new float[N],screenY=new float[N],depth=new float[N];
    private final int[] edgeA=new int[MAX_EDGES],edgeB=new int[MAX_EDGES];
    private final int[][] adjacent=new int[N][MAX_NEIGHBORS];
    private final int[] degrees=new int[N];
    private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Wave[] waves=new Wave[WAVE_COUNT];
    private final Signal source;
    private final int accent,secondary;
    private final boolean audonic;
    // Material 3 colour roles come from the active CYN palette, not fixed neon ink.
    private final int surfaceLow,surfaceHigh,onSurface,onSurfaceVariant,outlineVariant,primaryContainer;
    private final int[] materialCreativeHues=new int[6];
    private final float density,scaledDensity;
    private final android.graphics.RectF viewBounds=new android.graphics.RectF();
    private final android.graphics.RectF stageBounds=new android.graphics.RectF();
    private final android.graphics.RectF tileBounds=new android.graphics.RectF();
    private final android.graphics.RectF tileProgress=new android.graphics.RectF();
    private float stageBottom;
    private final float[] creativeLevel=new float[6],creativePrevious=new float[6];
    private final long[] lastCreativeWave=new long[6];
    private final android.graphics.RectF creativeOrbit=new android.graphics.RectF();
    private int edgeCount,randomState=0x3a14f1c7,colourCursor=0;
    private float angle=.37f,tilt=.08f,energy,lastMeasured,travelTime;
    private float lastBass,lastKick,lastSpatial,bassFlash,kickFlash,spatialFlash,neuralFlash;
    private long lastFrame,lastWave,lastBassWave,lastKickWave,lastSpatialWave,lastDecisionAt;

    private static final class Wave {
        final int[] route=new int[ROUTE_LEN];
        int hue;
        float distance=99f,rate=1f,power;
    }
    ApexSpatialBrainView(Context context,Signal feed,int mainColour,int secondaryColour){
        this(context,feed,mainColour,secondaryColour,false);
    }
    ApexSpatialBrainView(Context context,Signal feed,int mainColour,int secondaryColour,boolean creativeMode){
        super(context);
        source=feed;accent=mainColour;secondary=secondaryColour;audonic=creativeMode;
        density=getResources().getDisplayMetrics().density;
        scaledDensity=getResources().getDisplayMetrics().scaledDensity;
        surfaceLow=com.google.android.material.color.MaterialColors.getColor(
            context,com.google.android.material.R.attr.colorSurfaceContainerLow,0xff101723);
        surfaceHigh=com.google.android.material.color.MaterialColors.getColor(
            context,com.google.android.material.R.attr.colorSurfaceContainerHigh,0xff223044);
        onSurface=com.google.android.material.color.MaterialColors.getColor(
            context,com.google.android.material.R.attr.colorOnSurface,Color.WHITE);
        onSurfaceVariant=com.google.android.material.color.MaterialColors.getColor(
            context,com.google.android.material.R.attr.colorOnSurfaceVariant,0xffb9c7da);
        outlineVariant=com.google.android.material.color.MaterialColors.getColor(
            context,com.google.android.material.R.attr.colorOutlineVariant,0xff405066);
        primaryContainer=com.google.android.material.color.MaterialColors.getColor(
            context,com.google.android.material.R.attr.colorPrimaryContainer,0xff2b405d);
        for(int i=0;i<materialCreativeHues.length;i++)
            materialCreativeHues[i]=androidx.core.graphics.ColorUtils.blendARGB(
                CREATIVE_HUES[i],accent,.23f);
        setContentDescription(audonic
            ? "AUDONIC six-channel creative neural mesh; each labelled effect responds to recent model decisions."
            : "CYN APEX 10M active neural network mesh with rotating lights and audio-reactive pathways.");
        // One calm accessibility node, not a new spoken event on each 40-ms frame.
        setImportantForAccessibility(IMPORTANT_FOR_ACCESSIBILITY_YES);
        paint.setStrokeCap(Paint.Cap.ROUND);
        for(int i=0;i<WAVE_COUNT;i++)waves[i]=new Wave();
        generateBrain();
        generateGraph();
    }
    private float dp(float value){return value*density;}
    private float sp(float value){return value*scaledDensity;}
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
    private void launch(float measured,long now,int channel){
        if(measured<=0f)return;
        // Deterministic spatial-energy-based cadence. Randomness only chooses
        // the visual hue and route, NOT fake neural inference events.
        if(now-lastWave<85)return;
        lastWave=now;
        int from=rand()%N;
        for(int attempt=0;attempt<20;attempt++){
            int candidate=rand()%N;
            // Each real signal lights an appropriate physical region.
            float y=modelY[candidate];
            int region=Math.floorMod(channel,4);
            boolean inRegion=region==0?y>.17f:
                region==1?Math.abs(y)<.35f:
                region==2?Math.abs(modelX[candidate])>.29f:
                y<-.10f;
            if(inRegion&&depth[candidate]>-.3f){from=candidate;break;}
        }
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
            // Each measured event gets a different neon colour; variation is
            // cosmetic, while the event, amplitude and speed are real.
            wave.hue=audonic?materialCreativeHues[Math.floorMod(channel,materialCreativeHues.length)]
                :NEON[(colourCursor+++(rand()%NEON.length))%NEON.length];
            wave.rate=3.5f+4.5f*clamp(measured,0,1);
            wave.power=clamp(measured,.25f,1f);
            break;
        }
    }
    private void drawGraph(Canvas c,float scale,float strength,int pass){
        // Rear connections first; front routes are brighter and legible.
        for(int i=0;i<edgeCount;i++){
            int a=edgeA[i],b=edgeB[i];
            boolean forward=(depth[a]+depth[b])>=0;
            if(forward!=(pass==1))continue;
            int hue=audonic?materialCreativeHues[((a/17)+(b/23))%materialCreativeHues.length]
                :NEON[((a/17)+(b/23))%NEON.length];
            float region=(modelY[a]+modelY[b])*.5f;
            float channelLight=region>.18f?bassFlash:
                region<-.24f?neuralFlash:kickFlash;
            channelLight=Math.max(channelLight,spatialFlash*
                Math.max(0f,Math.abs(modelX[a]+modelX[b])));
            if(audonic)channelLight=Math.max(channelLight,creativeLevel[(a+b)%6]);
            int alpha=(forward?92:38)+Math.round(strength*(forward?65:24))
                +Math.round(channelLight*(forward?65:20));
            line(c,a,b,hue,alpha,forward?1.45f+channelLight*.85f:0.85f);
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
            int hue=audonic?materialCreativeHues[i%6]
                :hub?secondary:NEON[(i*7+i/11)%NEON.length];
            float region=modelY[i];
            float signal=region>.18f?bassFlash:region<-.24f?neuralFlash:kickFlash;
            signal=Math.max(signal,spatialFlash*Math.abs(modelX[i]));
            if(audonic)signal=Math.max(signal,creativeLevel[i%6]);
            float r=(front?(hub?3.7f:2.15f):(hub?2.1f:1.35f))
                *(1f+.37f*signal);
            if(front && (hub||i%4==0)){
                circle(c,screenX[i],screenY[i],r*3.2f,hue,
                    16+Math.round(strength*50)+Math.round(signal*80));
                circle(c,screenX[i],screenY[i],r*1.8f,hue,
                    33+Math.round(strength*70)+Math.round(signal*105));
            }
            circle(c,screenX[i],screenY[i],r,hue,
                (front?170:80)+Math.round(strength*(front?35:25))
                +Math.round(signal*(front?55:30)));
            if(hub&&front)circle(c,screenX[i],screenY[i],1.1f,Color.WHITE,
                146+Math.round(strength*95));
        }
    }
    // Connected playback exposes the actual personal model's 12 hidden
    // activations and 60 output contributions. Highlight those weights on
    // top of the illustrative 3D mesh instead of inventing activation values.
    private void drawNeuralPathways(Canvas c,float[] hidden,float[] links,
            float[] outputs) {
        if(hidden==null||hidden.length!=12||links==null||links.length!=60||
                outputs==null||outputs.length!=5)return;
        for(int o=0;o<5;o++){
            int destination=19+o*54;
            for(int j=0;j<12;j++){
                float weight=links[o*12+j];
                if(!Float.isFinite(weight))continue;
                int origin=7+j*23;
                float strength=clamp(Math.abs(weight)*7f,0f,1f);
                int hue=weight>=0?NEON[(o+j)%NEON.length]:secondary;
                line(c,origin,destination,hue,
                    24+Math.round(strength*195),.9f+strength*1.65f);
            }
        }
        for(int j=0;j<12;j++){
            int node=7+j*23;
            float strength=Float.isFinite(hidden[j])
                ?clamp(Math.abs(hidden[j])*5f,0f,1f):0f;
            int hue=NEON[(j*3)%NEON.length];
            circle(c,screenX[node],screenY[node],6f+strength*4f,
                hue,20+Math.round(strength*76));
            circle(c,screenX[node],screenY[node],2.5f+strength*1.8f,
                hue,85+Math.round(strength*155));
        }
        for(int o=0;o<5;o++){
            int node=19+o*54;
            float strength=Float.isFinite(outputs[o])
                ?clamp(outputs[o],0f,1f):0f;
            circle(c,screenX[node],screenY[node],8.0f,
                accent,20+Math.round(strength*90));
            circle(c,screenX[node],screenY[node],3.3f,
                Color.WHITE,80+Math.round(strength*165));
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

    /**
     * AUDONIC's creative rings and satellites reflect the six actual, recent
     * ONNX effect controls scaled by the user's creative-intensity setting.
     * Missing/stale decisions fade to zero: no synthetic pulses or fake values.
     */
    private void updateCreative(float dt,boolean live,long now,boolean reduced){
        if(!audonic)return;
        float[] decisions=live?source.creative():null;
        float intensity=clamp(source.creativeStrength(),0,1);
        for(int i=0;i<6;i++){
            float raw=decisions!=null&&decisions.length==6&&Float.isFinite(decisions[i])
                ?decisions[i]:0f;
            float target=clamp(raw,0,1)*intensity;
            float previous=creativePrevious[i];
            if(live&&!reduced&&target>.07f
                    &&Math.abs(target-previous)>.095f
                    &&now-lastCreativeWave[i]>180){
                launch(target,now,i);
                lastCreativeWave[i]=now;
            }
            creativePrevious[i]=target;
            creativeLevel[i]+=(target-creativeLevel[i])
                *Math.min(1f,Math.max(.016f,dt)*(target>creativeLevel[i]?11f:6f));
        }
    }
    /**
     * Material 3 Expressive scan: six tonal telemetry tiles plus measured arcs.
     * The cards remain informational; no fabricated meters or touch controls.
     */
    private void drawCreative(Canvas canvas,float cx,float cy,float scale){
        creativeOrbit.set(cx-scale*1.10f,cy-scale*.83f,
            cx+scale*1.10f,cy+scale*.83f);
        for(int i=0;i<6;i++){
            float intensity=clamp(creativeLevel[i],0,1);
            int hue=materialCreativeHues[i];
            ink(outlineVariant,106,Paint.Style.STROKE,dp(2.5f));
            canvas.drawArc(creativeOrbit,-90+i*60,52,false,paint);
            if(intensity>.001f){
                ink(hue,120+Math.round(intensity*125),Paint.Style.STROKE,
                    dp(2.5f+intensity*1.5f));
                canvas.drawArc(creativeOrbit,-90+i*60,52*intensity,false,paint);
            }
            float x=stageBounds.left+dp(7)+i%3*
                ((stageBounds.width()-dp(8))/3f);
            float space=(getWidth()-dp(24)-dp(12))/3f;
            x=dp(12)+(i%3)*(space+dp(6));
            float y=stageBottom+dp(10)+(i/3)*dp(49);
            tileBounds.set(x,y,x+space,y+dp(43));
            int fill=androidx.core.graphics.ColorUtils.blendARGB(
                surfaceHigh,primaryContainer,.16f+intensity*.30f);
            ink(fill,255,Paint.Style.FILL,0);
            canvas.drawRoundRect(tileBounds,dp(15),dp(15),paint);
            ink(outlineVariant,115,Paint.Style.STROKE,dp(1));
            canvas.drawRoundRect(tileBounds,dp(15),dp(15),paint);
            circle(canvas,x+dp(11),y+dp(12),dp(3.5f),hue,180+Math.round(intensity*75));
            paint.setTypeface(HEADING_FACE);
            paint.setTextAlign(Paint.Align.LEFT);
            float maxTextWidth=space-dp(20);
            float textSize=Math.min(sp(10f),Math.max(sp(8f),
                maxTextWidth/Math.max(1f,CREATIVE_NAMES[i].length()*.64f)));
            paint.setTextSize(textSize);
            ink(onSurface,235,Paint.Style.FILL,0);
            canvas.drawText(CREATIVE_NAMES[i],x+dp(19),y+dp(16),paint);
            float trackLeft=x+dp(10),trackRight=x+space-dp(10);
            tileProgress.set(trackLeft,y+dp(30),trackRight,y+dp(33));
            ink(outlineVariant,180,Paint.Style.FILL,0);
            canvas.drawRoundRect(tileProgress,dp(2),dp(2),paint);
            if(intensity>.001f){
                tileProgress.right=trackLeft+(trackRight-trackLeft)*intensity;
                ink(hue,255,Paint.Style.FILL,0);
                canvas.drawRoundRect(tileProgress,dp(2),dp(2),paint);
            }
        }
    }
    private void drawMaterialSurface(Canvas canvas,int w,int h){
        viewBounds.set(dp(2),dp(2),w-dp(2),h-dp(2));
        ink(surfaceLow,255,Paint.Style.FILL,0);
        canvas.drawRoundRect(viewBounds,dp(28),dp(28),paint);
        ink(outlineVariant,135,Paint.Style.STROKE,dp(1));
        canvas.drawRoundRect(viewBounds,dp(28),dp(28),paint);
        stageBottom=h-(audonic?dp(153):dp(79));
        stageBounds.set(dp(12),dp(49),w-dp(12),stageBottom);
        ink(surfaceHigh,255,Paint.Style.FILL,0);
        canvas.drawRoundRect(stageBounds,dp(24),dp(24),paint);
        ink(outlineVariant,90,Paint.Style.STROKE,dp(1));
        canvas.drawRoundRect(stageBounds,dp(24),dp(24),paint);
    }
    private void drawMaterialHeader(Canvas canvas,boolean playing,boolean connected){
        paint.setTypeface(HEADING_FACE);
        paint.setTextSize(sp(12));
        paint.setTextAlign(Paint.Align.LEFT);
        ink(onSurface,245,Paint.Style.FILL,0);
        canvas.drawText(audonic?"CREATIVE BRAINSCAN":"NEURAL BRAINSCAN",
            dp(19),dp(31),paint);
        final String status=playing?"LIVE":connected?"WAITING":"OFFLINE";
        final int stateColour=playing?accent:onSurfaceVariant;
        final float right=getWidth()-dp(22);
        paint.setTextSize(sp(10));
        paint.setTextAlign(Paint.Align.RIGHT);
        ink(stateColour,245,Paint.Style.FILL,0);
        canvas.drawText(status,right,dp(30),paint);
        circle(canvas,right-paint.measureText(status)-dp(11),
            dp(26),dp(3),stateColour,255);
        paint.setTextAlign(Paint.Align.LEFT);
    }
    private void drawMaterialFooter(Canvas canvas,boolean playing,
            boolean connected,boolean external,float bass,float kick,
            float spatial,float decision){
        paint.setTextAlign(Paint.Align.LEFT);
        paint.setTypeface(BODY_FACE);
        paint.setTextSize(sp(11));
        ink(onSurfaceVariant,240,Paint.Style.FILL,0);
        final String status=audonic
            ?(playing?"6 CREATIVE EFFECTS  ·  LIVE DSP POLICY":
                connected?"WAITING FOR AUDONIC DECISIONS":"CONNECT CYN TO VIEW ACTIVITY")
            :playing?"BASS "+Math.round(bass*100)+"%  ·  KICK "+Math.round(kick*100)
                 +"%  ·  "+(external?"SPECTRUM ":"SPATIAL ")+Math.round(spatial*100)+"%"
             :external?"CYN NEURAL MIND  /  CONNECTED":
               connected?"WAITING FOR AUDIO":"CONNECT CYN TO VIEW ACTIVITY";
        canvas.drawText(status,dp(18),getHeight()-dp(17),paint);
    }
    @Override protected void onAttachedToWindow(){
        super.onAttachedToWindow();
        lastFrame=0;
        postInvalidateOnAnimation();
    }
    @Override protected void onWindowVisibilityChanged(int visibility){
        super.onWindowVisibilityChanged(visibility);
        lastFrame=0;
        if(visibility==VISIBLE)postInvalidateOnAnimation();
    }
    @Override protected void onDraw(Canvas canvas){
        super.onDraw(canvas);
        int w=getWidth(),h=getHeight();
        if(w<80||h<80)return;
        long now=SystemClock.elapsedRealtime();
        float dt=lastFrame==0?0:Math.min(.06f,(now-lastFrame)*.001f);
        lastFrame=now;
        boolean playing=source.enabled(),reduced=source.reduced();
        boolean connected=source.connected(),external=source.external();
        float[] measuredChannels=source.channels();
        float bass=playing&&measuredChannels!=null&&measuredChannels.length>=4
            ?clamp(measuredChannels[0],0,1):0;
        float kick=playing&&measuredChannels!=null&&measuredChannels.length>=4
            ?clamp(measuredChannels[1],0,1):0;
        float spatial=playing&&measuredChannels!=null&&measuredChannels.length>=4
            ?clamp(measuredChannels[2],0,1):0;
        float decision=playing&&measuredChannels!=null&&measuredChannels.length>=4
            ?clamp(measuredChannels[3],0,1):0;
        float measured=playing?clamp(source.activity(),0,1):0;
        if(!Float.isFinite(measured))measured=0;
        if(!Float.isFinite(bass))bass=0;
        if(!Float.isFinite(kick))kick=0;
        if(!Float.isFinite(spatial))spatial=0;
        if(!Float.isFinite(decision))decision=0;
        energy+=(measured-energy)*Math.min(1f,dt*(measured>energy?9f:4f));
        float level=clamp((float)Math.sqrt(energy)*5f,0,1);
        bassFlash=Math.max(0,bassFlash-dt*1.0f);
        kickFlash=Math.max(0,kickFlash-dt*3.4f);
        spatialFlash=Math.max(0,spatialFlash-dt*1.5f);
        neuralFlash=Math.max(0,neuralFlash-dt*2.8f);
        updateCreative(dt,playing,now,reduced);

        if(!reduced){
            // Always rotate while the AI page is visible. Active spatial or
            // decision signals noticeably accelerate the 3D rotation.
            angle+=dt*(playing?.58f+level*.90f+decision*.38f:.42f);
            tilt=.09f+.13f*(float)Math.sin(angle*.55f);
            travelTime+=dt;
            if(playing){
                final long decisionTime=source.decisionAt();
                if(decisionTime>0&&decisionTime!=lastDecisionAt){
                    lastDecisionAt=decisionTime;
                    neuralFlash=Math.max(neuralFlash,.38f+.62f*decision);
                    if(!audonic){
                        launch(Math.max(.45f,decision),now,3);
                        launch(Math.max(.30f,decision*.7f),now+1,3);
                    }else{
                        int strongest=0;
                        for(int i=1;i<6;i++)
                            if(creativeLevel[i]>creativeLevel[strongest])strongest=i;
                        if(creativeLevel[strongest]>.01f)
                            launch(creativeLevel[strongest],now,strongest);
                    }
                }
                if(kick>.11f && kick-lastKick>.065f && now-lastKickWave>140){
                    kickFlash=Math.max(kickFlash,clamp(kick*2.1f,.4f,1f));
                    launch(kickFlash,now,1);
                    lastKickWave=now;
                }
                if(bass>.20f && bass-lastBass>.04f && now-lastBassWave>270){
                    bassFlash=Math.max(bassFlash,clamp(bass*1.25f,.25f,1f));
                    launch(bassFlash,now,0);
                    lastBassWave=now;
                }
                if(spatial>.00025f &&
                    (spatial>lastSpatial*1.35f || now-lastSpatialWave>450)
                    && now-lastSpatialWave>180){
                    spatialFlash=Math.max(spatialFlash,clamp((float)Math.sqrt(spatial)*4f,.2f,1f));
                    launch(spatialFlash,now,2);
                    lastSpatialWave=now;
                }
                for(Wave wave:waves)if(wave.distance<ROUTE_LEN+.8f)
                    wave.distance+=dt*wave.rate;
            }
        }
        lastBass=bass;lastKick=kick;lastSpatial=spatial;lastMeasured=measured;
        drawMaterialSurface(canvas,w,h);
        float cx=w*.5f,cy=(stageBounds.top+stageBottom)*.5f;
        float scale=Math.min((w-dp(42))*.57f,
            (stageBottom-stageBounds.top)*.56f);
        project(cx,cy,scale);
        // Delicate tonal orbit; its opacity only follows observed signal.
        ink(outlineVariant,52+Math.round(level*75),Paint.Style.STROKE,dp(1.1f));
        canvas.drawOval(cx-scale*1.25f,cy-scale*1.06f,
            cx+scale*1.25f,cy+scale*1.06f,paint);
        ink(accent,32+Math.round(level*65),Paint.Style.STROKE,dp(1));
        canvas.drawOval(cx-scale*1.07f,cy-scale*.88f,
            cx+scale*1.07f,cy+scale*.88f,paint);

        drawGraph(canvas,scale,level,0);
        drawGraph(canvas,scale,level,1);
        if(external&&!audonic)drawNeuralPathways(canvas,source.hidden(),
            source.connections(),source.outputs());
        if(playing)drawWaves(canvas,level);
        if(audonic)drawCreative(canvas,cx,cy,scale);

        drawMaterialHeader(canvas,playing,connected);
        drawMaterialFooter(canvas,playing,connected,external,bass,kick,spatial,decision);
        if(!reduced&&isAttachedToWindow()&&isShown()){
            // This is the missing continuous frame loop. Previously no redraw
            // was scheduled at all when paused, so rotation appeared frozen.
            postInvalidateDelayed(40);
        }
    }
}
