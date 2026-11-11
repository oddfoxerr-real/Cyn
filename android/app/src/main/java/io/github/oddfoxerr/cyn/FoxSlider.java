// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.os.SystemClock;
import androidx.appcompat.widget.AppCompatSeekBar;

/**
 * Decorative vector fox riding the real SeekBar thumb.
 * No audio control, model, or user preference is written by this class.
 */
class FoxSlider extends AppCompatSeekBar {
    private static final int FUR=0xffed914f;
    private static final int FUR_DARK=0xffb65b37;
    private static final int CREAM=0xffffecd5;
    private static final int EYES=0xff231d24;
    private final Paint fox=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Path shape=new Path();
    private final float density;
    private final boolean reducedMotion;
    private long movedAt;
    private int direction=1;

    FoxSlider(Context context,boolean reduced){
        super(context);
        density=context.getResources().getDisplayMetrics().density;
        reducedMotion=reduced;
        setMinimumHeight(Math.round(62*density));
    }

    @Override public synchronized void setProgress(int value){
        int previous=getProgress();
        super.setProgress(value);
        if(previous!=getProgress()){
            direction=getProgress()>previous?1:-1;
            movedAt=SystemClock.uptimeMillis();
            invalidate();
        }
    }

    private void color(int c){fox.setColor(c);fox.setAlpha(255);fox.setStrokeWidth(1);fox.setStyle(Paint.Style.FILL);}
    private void triangle(Canvas c,float ax,float ay,float bx,float by,float cx,float cy,int fill){
        color(fill);shape.reset();shape.moveTo(ax,ay);shape.lineTo(bx,by);shape.lineTo(cx,cy);shape.close();c.drawPath(shape,fox);
    }

    @Override protected void onDraw(Canvas canvas){
        super.onDraw(canvas);
        if(getWidth()<=0||getHeight()<=0)return;
        float t=Math.max(0,Math.min(1,getProgress()/(float)Math.max(1,getMax())));
        float usable=Math.max(1,getWidth()-getPaddingLeft()-getPaddingRight());
        float cx=getPaddingLeft()+usable*t;
        float cy=getHeight()*.50f;
        long elapsed=SystemClock.uptimeMillis()-movedAt;
        float motion=reducedMotion||elapsed>=480?0:1-Math.max(0,elapsed)/480f;
        float wobble=motion*(float)Math.sin(elapsed*.038f)*1.8f;
        float bounce=motion*(float)Math.abs(Math.sin(elapsed*.024f))*2.2f;
        // Render inside a small local coordinate space with an ample touch target.
        int save=canvas.save();
        canvas.translate(cx,cy-13*density-bounce*density);
        canvas.scale(density,density);
        // If near the right edge, curl the tail inward so nothing gets clipped.
        int tailSide=(t>.72f)?-1:1;
        float wag=reducedMotion?0:motion*(float)Math.sin(elapsed*.042f)*3.4f;
        color(FUR_DARK);
        shape.reset();shape.moveTo(5*tailSide,1);
        shape.cubicTo(13*tailSide,0,17*tailSide,-5+wag,13*tailSide,-11+wag);
        shape.cubicTo(20*tailSide,-9+wag,19*tailSide,0,11*tailSide,5);
        shape.close();canvas.drawPath(shape,fox);
        color(CREAM);
        canvas.drawOval(Math.min(11*tailSide,16*tailSide),-10+wag,Math.max(11*tailSide,16*tailSide),-5+wag,fox);
        // Little paws ride the actual seek thumb.
        color(FUR_DARK);canvas.drawRoundRect(-7,5, -2,10,2,2,fox);
        canvas.drawRoundRect(2,5,7,10,2,2,fox);
        // Pointed ears, two-tone face, and blinking eyes.
        triangle(canvas,-8,-8,-9,-19,-1,-12,FUR);
        triangle(canvas,8,-8,9,-19,1,-12,FUR);
        triangle(canvas,-6,-11,-7,-16,-2,-12,CREAM);
        triangle(canvas,6,-11,7,-16,2,-12,CREAM);
        color(FUR);canvas.drawOval(-10,-13,10,6,fox);
        color(CREAM);canvas.drawOval(-8,-4,8,6,fox);
        boolean blinking=!reducedMotion&&(SystemClock.uptimeMillis()%4900<115);
        color(EYES);
        if(blinking){
            fox.setStrokeWidth(1.15f);fox.setStyle(Paint.Style.STROKE);
            canvas.drawLine(-4,-4,-2,-4,fox);canvas.drawLine(2,-4,4,-4,fox);
            fox.setStyle(Paint.Style.FILL);
        }else{
            canvas.drawCircle(-3,-4,1,fox);canvas.drawCircle(3,-4,1,fox);
        }
        triangle(canvas,-2,0,2,0,0,2,EYES);
        canvas.restoreToCount(save);
        // Animate only briefly after real thumb motion. No continual 60-fps redraw
        // across the dozens of EQ bands, and no movement in reduced-motion mode.
        if(!reducedMotion&&motion>0&&isShown())postInvalidateDelayed(34);
    }
}
