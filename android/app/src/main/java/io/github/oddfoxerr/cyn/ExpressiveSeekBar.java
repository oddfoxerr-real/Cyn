// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.GradientDrawable;
import android.os.SystemClock;
import androidx.appcompat.widget.AppCompatSeekBar;

/** Playback decoration over native seeking, keyboard and accessibility behavior. */
final class ExpressiveSeekBar extends AppCompatSeekBar {
    private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Path wave=new Path();
    private final float density;
    private final int activeColor,inactiveColor;
    private final boolean reducedMotion;
    private boolean playing;

    ExpressiveSeekBar(Context context,int active,int inactive,boolean reduced){
        super(context);
        density=getResources().getDisplayMetrics().density;
        activeColor=active;inactiveColor=inactive;reducedMotion=reduced;
        setPadding(dp(12),dp(4),dp(12),dp(4));
        setMinimumHeight(dp(56));
        setSplitTrack(false);
        setProgressDrawable(new ColorDrawable(Color.TRANSPARENT));
        GradientDrawable handle=new GradientDrawable();
        handle.setColor(active);handle.setCornerRadius(dp(2));handle.setSize(dp(4),dp(44));
        setThumb(handle);setThumbOffset(dp(2));
        paint.setStrokeCap(Paint.Cap.ROUND);
    }
    private int dp(float value){return Math.round(value*density);}
    void setPlaying(boolean value){if(playing!=value){playing=value;invalidate();}}
    @Override protected synchronized void onDraw(Canvas canvas){
        float start=getPaddingLeft(),end=getWidth()-getPaddingRight();
        if(end<=start){super.onDraw(canvas);return;}
        boolean rtl=getLayoutDirection()==LAYOUT_DIRECTION_RTL;
        float fraction=Math.max(0,Math.min(1,getProgress()/(float)Math.max(1,getMax())));
        float handle=start+(end-start)*fraction;
        float center=getPaddingTop()+(getHeight()-getPaddingTop()-getPaddingBottom())/2f;
        int save=canvas.save();
        if(rtl)canvas.scale(-1,1,getWidth()/2f,center);
        paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(dp(4));
        paint.setColor(inactiveColor);
        if(handle+dp(8)<end)canvas.drawLine(handle+dp(8),center,end,center,paint);
        paint.setColor(activeColor);
        float activeEnd=Math.max(start,handle-dp(8));
        if(activeEnd>start){
            wave.reset();
            float phase=playing&&!reducedMotion?(SystemClock.uptimeMillis()%1600)/1600f*(float)(Math.PI*2):0;
            float amplitude=playing?dp(3):0;
            for(float x=start;x<=activeEnd;x+=Math.max(1,density)){
                float taper=Math.min(1,Math.min((x-start)/dp(8),(activeEnd-x)/dp(8)));
                float y=center+amplitude*taper*(float)Math.sin((x-start)/dp(24)*Math.PI*2-phase);
                if(x==start)wave.moveTo(x,y);else wave.lineTo(x,y);
            }
            wave.lineTo(activeEnd,center);canvas.drawPath(wave,paint);
        }
        canvas.restoreToCount(save);
        super.onDraw(canvas);
        if(playing&&!reducedMotion&&hasWindowFocus()&&isShown())postInvalidateOnAnimation();
    }
}
