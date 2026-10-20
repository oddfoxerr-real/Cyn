// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;
import android.content.Context;
import android.graphics.*;
import android.os.SystemClock;
import android.view.View;

/**
 * Procedural 3D perspective brain-scan visualization for the APEX 10M DSP slot.
 * Rotation rate and illumination derive from actual native stereo-side output.
 * This is NOT a neural activation map: 10M spatial reasoning weights are untrained.
 */
final class ApexSpatialBrainView extends View {
    interface Signal {float activity();boolean enabled();boolean reduced();}
    private final Signal source;
    private final Paint pen=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final int accent,secondary;
    private float angle=0f;
    private float envelope=0f;
    private long lastTick=0;
    private static final int SEGMENTS=22,ROWS=13;

    ApexSpatialBrainView(Context context,Signal activity,int primary,int secondary){
        super(context);source=activity;accent=primary;this.secondary=secondary;
        setContentDescription("3D stereo spatial processing scan; highlights follow measured DSP stereo-side energy. The 10M neural network is not trained.");
        setImportantForAccessibility(IMPORTANT_FOR_ACCESSIBILITY_NO);
        setLayerType(View.LAYER_TYPE_SOFTWARE,null);
    }
    float projectX(float x,float y,float z,float cx,float size,float rotation){
        float cs=(float)Math.cos(rotation),sn=(float)Math.sin(rotation);
        float rx=x*cs-z*sn;
        float depth=x*sn+z*cs;
        float perspective=2.5f/(2.5f+depth*.35f);
        return cx+rx*size*perspective;
    }
    float projectY(float x,float y,float z,float cy,float size,float rotation){
        float cs=(float)Math.cos(rotation),sn=(float)Math.sin(rotation);
        float depth=x*sn+z*cs;
        float perspective=2.5f/(2.5f+depth*.35f);
        return cy+y*size*perspective;
    }
    @Override protected void onDraw(Canvas canvas){
        super.onDraw(canvas);
        float width=getWidth(),height=getHeight();
        if(width<30||height<30)return;
        long now=SystemClock.elapsedRealtime();
        float dt=lastTick==0?0:Math.min(.05f,(now-lastTick)/1000f);
        lastTick=now;
        boolean playing=source.enabled();
        float actual=playing?Math.max(0,Math.min(1,source.activity())):0;
        envelope+=(actual-envelope)*Math.min(1,dt*7);
        // No endless spinner at silence; spin speed is driven by DSP output.
        if(playing&&!source.reduced()&&envelope>.004f)
            angle+=dt*(.10f+envelope*2.1f);
        float centerX=width/2,centerY=height*.48f;
        float size=Math.min(width*.36f,height*.39f);
        pen.setStyle(Paint.Style.STROKE);pen.setStrokeWidth(Math.max(1,size*.007f));
        float intensity=Math.min(1,envelope*2f);
        // Outer scanner halo brightens only with measured processing.
        pen.setColor(accent);
        pen.setAlpha(Math.round(18+65*intensity));
        pen.setStrokeWidth(size*.008f);
        canvas.drawCircle(centerX,centerY,size*1.18f,pen);
        canvas.drawCircle(centerX,centerY,size*1.31f,pen);
        for(int side=-1;side<=1;side+=2){
            // A continuous lobed 3D lattice with a midline fissure.
            float[][] previous=new float[SEGMENTS+1][2];
            for(int row=0;row<=ROWS;row++){
                float latitude=(float)(-Math.PI/2+Math.PI*row/ROWS);
                float yy=(float)Math.sin(latitude)*.77f;
                float r=(float)Math.cos(latitude);
                float[][] current=new float[SEGMENTS+1][2];
                for(int col=0;col<=SEGMENTS;col++){
                    float longitude=(float)(Math.PI*col/SEGMENTS);
                    float folded=(float)Math.sin(longitude*5+latitude*7+side)*.045f;
                    float x=side*(.14f+r*(.46f+.05f*folded)*(float)Math.sin(longitude));
                    float z=r*.68f*(float)Math.cos(longitude);
                    float gyri=.075f*(float)Math.sin(longitude*7+latitude*11);
                    float py=yy+gyri;
                    current[col][0]=projectX(x,py,z,centerX,size,angle);
                    current[col][1]=projectY(x,py,z,centerY,size,angle);
                    if(col>0)stroke(canvas,current[col-1],current[col],18+Math.round(intensity*106),false);
                    if(row>0)stroke(canvas,previous[col],current[col],16+Math.round(intensity*68),false);
                }
                previous=current;
            }
        }
        // True activity-dependent sonar sweep (not model-neuron predictions).
        if(intensity>.015f){
            pen.setColor(secondary);pen.setAlpha(Math.round(35+170*intensity));
            pen.setStyle(Paint.Style.STROKE);pen.setStrokeWidth(Math.max(2,size*.017f));
            float scanY=centerY-size+2*size*(float)((Math.sin(angle*1.8)+1)/2);
            canvas.drawLine(centerX-size*.92f,scanY,centerX+size*.92f,scanY,pen);
        }
        pen.setStyle(Paint.Style.FILL);
        pen.setColor(accent);pen.setAlpha(235);
        pen.setTypeface(Typeface.create("sans-serif-medium",Typeface.NORMAL));
        pen.setTextSize(Math.max(12,height*.049f));
        canvas.drawText(playing?"SPATIAL DSP • LIVE":"SPATIAL DSP • STANDBY",18,height-35,pen);
        pen.setColor(secondary);pen.setAlpha(205);
        pen.setTextSize(Math.max(10,height*.037f));
        canvas.drawText("Measured side enhancement: "+Math.round(envelope*100)+"%",18,height-15,pen);
        if(playing&&!source.reduced()&&isAttachedToWindow())postInvalidateDelayed(50);
    }
    void stroke(Canvas c,float[] first,float[] second,int alpha,boolean glow){
        pen.setColor(glow?secondary:accent);
        pen.setAlpha(Math.max(0,Math.min(255,alpha)));
        c.drawLine(first[0],first[1],second[0],second[1],pen);
    }
}
