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
 * Readable 3D-style *illustrative* network for the APEX 10M spatial DSP.
 * Wires and nodes are always visible; the illuminated pulse amplitude comes
 * exclusively from actual stereo-side processing. No trained neural layer runs.
 *
 * Reuses projection buffers and Paint objects to avoid GC on every audio frame.
 */
final class ApexSpatialBrainView extends View {
    interface Signal { float activity(); boolean enabled(); boolean reduced(); }

    private static final int ROWS=10;
    private static final int SEGMENTS=18;
    private static final int POINTS=SEGMENTS+1;
    private final float[][][] projectedX=new float[2][ROWS+1][POINTS];
    private final float[][][] projectedY=new float[2][ROWS+1][POINTS];
    private final float[][][] depth=new float[2][ROWS+1][POINTS];
    private final Paint p=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Signal source;
    private final int accent, secondary;
    private float angle=.42f, envelope=0f, orbitPhase=0f;
    private long previousTick;

    ApexSpatialBrainView(Context context,Signal signal,int primary,int secondary){
        super(context);
        source=signal;
        accent=primary;
        this.secondary=secondary;
        setContentDescription("APEX 10M three-dimensional illustrative spatial network. Bright nodes and moving pulses indicate measured stereo DSP output, not artificial neuron activations.");
        setImportantForAccessibility(IMPORTANT_FOR_ACCESSIBILITY_NO);
        // Simple Canvas primitives and cached coordinate buffers; no GPU dependency.
    }

    private void color(int rgb,int alpha) {
        p.setColor(rgb);
        p.setAlpha(Math.max(0,Math.min(255,alpha)));
    }

    private void wire(Canvas canvas,float x1,float y1,float x2,float y2,
                      int tint,int opacity,float width){
        p.setStyle(Paint.Style.STROKE);
        p.setStrokeWidth(width);
        p.setStrokeCap(Paint.Cap.ROUND);
        color(tint,opacity);
        canvas.drawLine(x1,y1,x2,y2,p);
    }

    private void dot(Canvas canvas,float x,float y,float radius,int tint,int alpha){
        p.setStyle(Paint.Style.FILL);
        color(tint,alpha);
        canvas.drawCircle(x,y,radius,p);
    }

    private void project(float cx,float cy,float scale){
        float ca=(float)Math.cos(angle),sa=(float)Math.sin(angle);
        for(int hemi=0;hemi<2;hemi++){
            int side=hemi==0?-1:1;
            for(int row=0;row<=ROWS;row++){
                float lat=(float)(-Math.PI/2+Math.PI*row/ROWS);
                float radius=(float)Math.cos(lat);
                for(int col=0;col<=SEGMENTS;col++){
                    float lon=(float)(Math.PI*col/SEGMENTS);
                    // Folds are fixed geometry; not invented neuron readings.
                    float crease=.034f*(float)Math.sin(lon*7+lat*9+side*1.1f);
                    float x=side*(.12f+radius*(.46f+crease)*(float)Math.sin(lon));
                    float y=.74f*(float)Math.sin(lat)
                        +.048f*(float)Math.sin(lon*6.5f+lat*8.3f);
                    float z=.67f*radius*(float)Math.cos(lon);
                    float rx=x*ca-z*sa;
                    float dz=x*sa+z*ca;
                    float perspective=2.55f/(2.55f+.38f*dz);
                    projectedX[hemi][row][col]=cx+rx*scale*perspective;
                    projectedY[hemi][row][col]=cy+y*scale*perspective;
                    depth[hemi][row][col]=dz;
                }
            }
        }
    }

    private void drawBrain(Canvas canvas,float scale,float intensity,float now){
        // Render the rear lines first so foreground neurons appear in front.
        for(int pass=0;pass<2;pass++){
            for(int hemi=0;hemi<2;hemi++){
                for(int row=0;row<=ROWS;row++){
                    for(int col=0;col<=SEGMENTS;col++){
                        float z=depth[hemi][row][col];
                        boolean front=z>=0f;
                        if((pass==1)!=front)continue;
                        float x=projectedX[hemi][row][col];
                        float y=projectedY[hemi][row][col];
                        int base=front?114:48;
                        float width=front?1.45f:1.0f;
                        if(col>0){
                            wire(canvas,x,y,projectedX[hemi][row][col-1],
                                projectedY[hemi][row][col-1],accent,
                                base+(int)(intensity*(front?74:36)),width);
                        }
                        if(row>0 && (col%2==0)){
                            wire(canvas,x,y,projectedX[hemi][row-1][col],
                                projectedY[hemi][row-1][col],secondary,
                                (front?88:37)+(int)(intensity*80),front?1.25f:.8f);
                        }
                    }
                }
            }
        }
        // Illuminated "neurons": static topology, always readable at rest.
        // Front dots carry halos, rear dots remain dim to convey depth.
        for(int hemi=0;hemi<2;hemi++){
            for(int row=1;row<ROWS;row+=2){
                for(int col=2;col<SEGMENTS;col+=2){
                    float x=projectedX[hemi][row][col];
                    float y=projectedY[hemi][row][col];
                    boolean front=depth[hemi][row][col]>=0;
                    // Region-specific pulses visualise ONE measured DSP level;
                    // no randomly invented neuron-by-neuron activity.
                    float modulation=.64f+.36f*(float)Math.sin(
                        now*1.75f+row*.81f+col*.56f+hemi*2.1f);
                    float lit=intensity*modulation;
                    float radius=front?3.0f:1.7f;
                    if(front){
                        dot(canvas,x,y,radius*3.25f,secondary,
                            16+Math.round(lit*75));
                        dot(canvas,x,y,radius*2.05f,accent,
                            28+Math.round(lit*100));
                    }
                    dot(canvas,x,y,radius,front?secondary:accent,
                        front?190+Math.round(lit*65):75+Math.round(lit*55));
                    if(front && ((row+col+hemi)%4==0)){
                        dot(canvas,x,y,1.25f,Color.WHITE,125+Math.round(lit*130));
                    }
                }
            }
        }
        // Signal travelling along genuine projected mesh links. Never moves
        // when there is no measured stereo side enhancement.
        if(intensity>.015f) {
            int streams=4+Math.round(intensity*10f);
            for(int i=0;i<streams;i++){
                int hemi=i%2;
                int row=1+(i*3)%(ROWS-1);
                int col=(i*7)%(SEGMENTS-1);
                float phase=now*(.38f+intensity*1.5f)+i*.231f;
                float progress=phase-(float)Math.floor(phase);
                float x0=projectedX[hemi][row][col],y0=projectedY[hemi][row][col];
                float x1=projectedX[hemi][row][col+1],y1=projectedY[hemi][row][col+1];
                float x=x0+(x1-x0)*progress,y=y0+(y1-y0)*progress;
                if(depth[hemi][row][col]<-.20f)continue;
                dot(canvas,x,y,8.2f,accent,Math.round(27+intensity*83));
                dot(canvas,x,y,4.4f,secondary,Math.round(57+intensity*127));
                dot(canvas,x,y,1.9f,Color.WHITE,Math.round(100+intensity*155));
            }
        }
    }

    @Override protected void onDraw(Canvas canvas){
        super.onDraw(canvas);
        float w=getWidth(),h=getHeight();
        if(w<80||h<80)return;
        long tick=SystemClock.elapsedRealtime();
        float dt=previousTick==0?0f:Math.min(.06f,(tick-previousTick)/1000f);
        previousTick=tick;
        boolean enabled=source.enabled();
        boolean reduced=source.reduced();
        float measured=enabled?Math.max(0,Math.min(1,source.activity())):0;
        envelope+=(measured-envelope)*Math.min(1f,dt*7f);
        // The scanner's geometry stays visible at rest. Pulses are driven by
        // measured DSP. Perspective rotates gently only when playing.
        float visualEnergy=Math.min(1f,(float)Math.sqrt(envelope)*2.7f);
        if(enabled&&!reduced){
            angle+=dt*(.22f+visualEnergy*1.10f);
            orbitPhase+=dt;
        }
        float cx=w*.5f,cy=h*.44f;
        float scale=Math.min(w*.46f,h*.37f);
        project(cx,cy,scale);

        // Layered depth/orbit rings frame the bright network without hiding it.
        p.setStyle(Paint.Style.STROKE);
        p.setStrokeWidth(1.35f);
        color(accent,68+Math.round(visualEnergy*105));
        canvas.drawOval(cx-scale*1.32f,cy-scale*1.07f,
            cx+scale*1.32f,cy+scale*1.07f,p);
        color(secondary,40+Math.round(visualEnergy*85));
        canvas.drawOval(cx-scale*1.19f,cy-scale*.88f,
            cx+scale*1.19f,cy+scale*.88f,p);

        drawBrain(canvas,scale,visualEnergy,orbitPhase);

        // An actual activity sweep only appears when a spatial signal exists.
        if(visualEnergy>.025f){
            float sweep=cy+scale*(float)Math.sin(orbitPhase*1.8f);
            wire(canvas,cx-scale*.9f,sweep,cx+scale*.9f,sweep,
                secondary,Math.round(45+visualEnergy*150),2.0f);
        }
        p.setStyle(Paint.Style.FILL);
        p.setTypeface(Typeface.create("sans-serif-medium",Typeface.BOLD));
        p.setTextSize(Math.max(12,h*.052f));
        color(accent,240);
        canvas.drawText(enabled?"SPATIAL DSP  •  LIVE":"SPATIAL DSP  •  STANDBY",17,h-41,p);
        p.setTypeface(Typeface.create("sans-serif",Typeface.NORMAL));
        p.setTextSize(Math.max(10,h*.041f));
        color(secondary,218);
        canvas.drawText("Spatial energy: "+Math.round(measured*100)+"%  ·  Neural model not active",17,h-19,p);

        if(enabled && !reduced && isAttachedToWindow())postInvalidateDelayed(40);
    }
}
