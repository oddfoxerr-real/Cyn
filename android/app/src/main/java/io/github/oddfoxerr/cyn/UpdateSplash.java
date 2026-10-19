// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

import android.app.Dialog;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.GradientDrawable;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.view.Gravity;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

/**
 * Two-tone mechanical splash, drawn as real vector geometry.
 * Movement is visual only; status and fill are always sourced from UpdateManager.
 */
final class UpdateSplash {
    static final int INK=0xff090f17;
    private UpdateSplash(){}

    static String actionText(int stage,boolean ready){
        if(ready)return "INSTALL VERIFIED APK";
        if(stage==UpdateManager.FAILED)return "RETRY UPDATE";
        if(stage==UpdateManager.UP_TO_DATE)return "CHECK AGAIN";
        if(stage==UpdateManager.AVAILABLE)return "DOWNLOAD NOW";
        return "CHECK FOR UPDATE";
    }

    static String progressText(int stage,int percent){
        if(stage==UpdateManager.DOWNLOADING && percent>=0)
            return Math.min(100,percent)+"%  •  DOWNLOADING";
        if(stage==UpdateManager.VERIFYING)return "VERIFYING APK";
        if(stage==UpdateManager.READY)return "VERIFIED  •  READY";
        if(stage==UpdateManager.CHECKING)return "SEARCHING RELEASES";
        if(stage==UpdateManager.UP_TO_DATE)return "UP TO DATE";
        if(stage==UpdateManager.AVAILABLE)return "UPDATE FOUND";
        if(stage==UpdateManager.FAILED)return "CHECK FAILED";
        return "STANDBY";
    }

    static void show(MainActivity activity){show(activity,true);}

    // false is used by instrumentation, so it can render without network requests.
    static void show(MainActivity activity,boolean runCheck){
        if(activity.isFinishing()||activity.isDestroyed())return;
        final int accent=activity.accent;
        final boolean reduce=activity.reduced;
        final Dialog dialog=new Dialog(activity);
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
        FrameLayout root=new FrameLayout(activity);
        root.setBackgroundColor(INK);
        SplashArt art=new SplashArt(activity,accent,reduce);
        root.addView(art,new FrameLayout.LayoutParams(-1,-1));

        LinearLayout layout=new LinearLayout(activity);
        layout.setGravity(Gravity.CENTER_HORIZONTAL);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(activity.dp(26),activity.dp(26),activity.dp(26),activity.dp(31));

        TextView title=text(activity,"CYN / SYSTEM UPDATE",22,accent,true);
        title.setLetterSpacing(.12f);
        layout.addView(title,new LinearLayout.LayoutParams(-1,activity.dp(42)));
        TextView sub=text(activity,"MECHANICAL UPDATE SYSTEM  //  LOCAL SIGNATURE CHECK",11,accent,false);
        layout.addView(sub,new LinearLayout.LayoutParams(-1,activity.dp(34)));

        View space=new View(activity);
        layout.addView(space,new LinearLayout.LayoutParams(1,0,1));

        TextView phase=text(activity,"STANDBY",15,accent,true);
        phase.setLetterSpacing(.14f);
        layout.addView(phase,new LinearLayout.LayoutParams(-1,activity.dp(35)));
        TextView status=text(activity,UpdateManager.message,13,accent,false);
        status.setMinHeight(activity.dp(54));
        status.setAccessibilityLiveRegion(View.ACCESSIBILITY_LIVE_REGION_POLITE);
        layout.addView(status,new LinearLayout.LayoutParams(-1,-2));

        TextView action=text(activity,"CHECK FOR UPDATE",14,INK,true);
        action.setGravity(Gravity.CENTER);
        action.setMinimumHeight(activity.dp(53));
        action.setClickable(true);
        action.setFocusable(true);
        GradientDrawable fill=new GradientDrawable();
        fill.setColor(accent);
        fill.setCornerRadius(activity.dp(6));
        action.setBackground(fill);
        LinearLayout.LayoutParams actionParams=new LinearLayout.LayoutParams(-1,activity.dp(56));
        actionParams.topMargin=activity.dp(12);
        layout.addView(action,actionParams);

        TextView notes=text(activity,
            "AUTO-CHECK ON WI-FI  /  VERIFIED APK  /  ANDROID CONFIRMS INSTALL",10,accent,false);
        notes.setGravity(Gravity.CENTER);
        LinearLayout.LayoutParams notesParams=new LinearLayout.LayoutParams(-1,activity.dp(56));
        layout.addView(notes,notesParams);
        root.addView(layout,new FrameLayout.LayoutParams(-1,-1));

        TextView close=text(activity,"×",30,accent,false);
        close.setGravity(Gravity.CENTER);
        close.setContentDescription("Close update screen");
        close.setClickable(true);
        close.setFocusable(true);
        FrameLayout.LayoutParams closeParams=new FrameLayout.LayoutParams(activity.dp(56),activity.dp(56),Gravity.TOP|Gravity.END);
        root.addView(close,closeParams);
        close.setOnClickListener(v->dialog.dismiss());

        Handler handler=new Handler(Looper.getMainLooper());
        Runnable[] ticker=new Runnable[1];
        ticker[0]=()->{
            if(!dialog.isShowing())return;
            int stage=UpdateManager.stage;
            boolean ready=UpdateManager.ready(activity);
            int percent=UpdateManager.progress;
            art.state(stage,percent);
            String message=UpdateManager.message;
            if(!message.contentEquals(status.getText()))status.setText(message);
            String phaseLabel=progressText(ready?UpdateManager.READY:stage,percent);
            if(!phaseLabel.contentEquals(phase.getText()))phase.setText(phaseLabel);
            boolean working=UpdateManager.busy();
            action.setVisibility(working?View.INVISIBLE:View.VISIBLE);
            if(!working)action.setText(actionText(stage,ready));
            handler.postDelayed(ticker[0],100);
        };
        action.setOnClickListener(v->{
            if(UpdateManager.busy())return;
            if(UpdateManager.ready(activity)){
                UpdateManager.install(activity);
                return;
            }
            UpdateManager.check(activity,true,null);
            ticker[0].run();
        });
        dialog.setOnDismissListener(d->handler.removeCallbacks(ticker[0]));
        dialog.setContentView(root);
        dialog.show();
        Window window=dialog.getWindow();
        if(window!=null){
            window.setBackgroundDrawable(new ColorDrawable(INK));
            window.addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
            window.setStatusBarColor(INK);
            window.setNavigationBarColor(INK);
            window.setLayout(-1,-1);
        }
        if(runCheck&&!UpdateManager.busy()&&!UpdateManager.ready(activity))
            UpdateManager.check(activity,true,null);
        ticker[0].run();
    }

    private static TextView text(Context context,String value,int sp,int color,boolean bold){
        TextView t=new TextView(context);
        t.setText(value);t.setTextSize(sp);t.setTextColor(color);
        if(bold)t.setTypeface(null,android.graphics.Typeface.BOLD);
        t.setGravity(Gravity.CENTER_VERTICAL);
        return t;
    }

    static final class SplashArt extends View {
        private final Paint p=new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Path liquid=new Path();
        private final int accent;
        private final boolean reduced;
        private int stage=UpdateManager.IDLE;
        private int progress=-1;
        SplashArt(Context context,int color,boolean reduced){
            super(context);accent=color;this.reduced=reduced;
            setContentDescription("Two-tone mechanical update splash. Progress and verification are announced below.");
            setImportantForAccessibility(IMPORTANT_FOR_ACCESSIBILITY_NO);
        }
        void state(int nextStage,int nextProgress){
            if(stage!=nextStage||progress!=nextProgress){
                stage=nextStage;progress=nextProgress;invalidate();
            }
        }
        private void line(Canvas c,float x1,float y1,float x2,float y2,float thickness,int alpha){
            p.setColor(accent);p.setAlpha(alpha);p.setStrokeWidth(thickness);
            p.setStyle(Paint.Style.STROKE);c.drawLine(x1,y1,x2,y2,p);
            p.setStyle(Paint.Style.FILL);
        }
        @Override protected void onDraw(Canvas c){
            super.onDraw(c);
            c.drawColor(INK);
            float w=getWidth(),h=getHeight();
            if(w<=0||h<=0)return;
            float unit=Math.min(w,h), top=h*.22f, bottom=h*.65f;
            boolean busy=stage==UpdateManager.CHECKING||stage==UpdateManager.DOWNLOADING||stage==UpdateManager.VERIFYING;
            float time=reduced||!busy?0:(SystemClock.uptimeMillis()%4000)/1000f;
            p.setColor(accent);
            p.setStyle(Paint.Style.STROKE);
            p.setStrokeWidth(unit*.004f);p.setAlpha(185);
            c.drawRect(w*.075f,top,w*.925f,bottom,p);
            c.drawRect(w*.11f,top+unit*.028f,w*.89f,bottom-unit*.028f,p);
            p.setStyle(Paint.Style.FILL);
            // Two mechanical guide rails and flat cut-out teeth (no gradients or fake progress).
            for(int side=0;side<2;side++){
                float x=side==0?w*.085f:w*.915f;
                for(int t=0;t<11;t++){
                    float y=top+unit*.018f+t*(bottom-top-unit*.035f)/10;
                    p.setAlpha(t%2==0?185:80);
                    c.drawRect(x-unit*.012f,y,x+unit*.012f,y+unit*.008f,p);
                }
            }
            // Rotating press cogs are stage indicators, not a fabricated percentage.
            for(int wheel=0;wheel<2;wheel++){
                float cx=w*(wheel==0?.22f:.78f),cy=top+unit*.095f;
                float radius=unit*.041f;
                p.setStyle(Paint.Style.STROKE);p.setStrokeWidth(unit*.006f);p.setAlpha(230);
                c.drawCircle(cx,cy,radius,p);c.drawCircle(cx,cy,radius*.42f,p);
                p.setStyle(Paint.Style.FILL);
                for(int t=0;t<12;t++){
                    double a=t*Math.PI/6+time*(wheel==0?1.6:-1.6);
                    float x1=cx+(float)Math.cos(a)*radius*1.03f,y1=cy+(float)Math.sin(a)*radius*1.03f;
                    float x2=cx+(float)Math.cos(a)*radius*1.34f,y2=cy+(float)Math.sin(a)*radius*1.34f;
                    line(c,x1,y1,x2,y2,unit*.009f,190);
                }
            }
            float left=w*.15f,right=w*.85f;
            float barY=top+(bottom-top)*.54f;
            float barHeight=unit*.092f;
            p.setColor(accent);p.setStyle(Paint.Style.STROKE);p.setStrokeWidth(unit*.004f);p.setAlpha(210);
            c.drawRect(left,barY,right,barY+barHeight,p);
            p.setStyle(Paint.Style.FILL);
            float fill=stage==UpdateManager.READY?1:
                stage==UpdateManager.DOWNLOADING&&progress>=0?Math.min(1,Math.max(0,progress/100f)):0;
            float filled=(right-left)*fill;
            if(filled>0){
                // A mechanical, two-tone liquid front. Drips are clipped to the frame.
                int save=c.save();c.clipRect(left,barY,right,barY+barHeight+unit*.1f);
                liquid.reset();liquid.moveTo(left,barY);
                liquid.lineTo(left+filled,barY);
                float tip=left+filled;
                liquid.lineTo(tip,barY+barHeight);
                for(int i=4;i>=0;i--){
                    float x=left+filled*i/5f;
                    float drip=(i%2==0?.09f:.035f)*unit;
                    if(reduced||!busy)drip=unit*.022f;
                    else drip*=.5f+.5f*(float)Math.sin(time*3+i*1.1);
                    liquid.lineTo(x,barY+barHeight+drip);
                    liquid.lineTo(x-unit*.012f,barY+barHeight);
                }
                liquid.lineTo(left,barY+barHeight);liquid.close();
                p.setColor(accent);p.setAlpha(238);c.drawPath(liquid,p);
                c.restoreToCount(save);
            }
            if(stage==UpdateManager.CHECKING || stage==UpdateManager.VERIFYING){
                // Moving scanner conveys activity without pretending to know download %.
                float phase=(time%2f)/2f;
                float x=left+(right-left)*(reduced?.5f:phase);
                line(c,x,barY-unit*.015f,x,barY+barHeight+unit*.025f,unit*.014f,230);
            }
            // Hinged mechanical clamp arms travel during real work.
            float movement=busy&&!reduced?(float)Math.sin(time*4f)*unit*.014f:0;
            p.setColor(accent);p.setAlpha(245);
            c.drawRect(w*.18f,top+unit*.18f+movement,w*.82f,top+unit*.195f+movement,p);
            c.drawRect(w*.23f,bottom-unit*.073f-movement,w*.77f,bottom-unit*.057f-movement,p);
            // Physical segmented ruler for completed fractions (not simulated).
            int count=16;
            for(int i=0;i<count;i++){
                float x=left+i*(right-left)/count;
                p.setAlpha(fill>=(i+1f)/count?230:45);
                c.drawRect(x,barY+barHeight+unit*.13f,x+(right-left)/count-unit*.008f,
                    barY+barHeight+unit*.145f,p);
            }
            if(busy&&!reduced)postInvalidateDelayed(33);
        }
    }
}
