// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

import android.app.Dialog;
import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.GradientDrawable;
import android.os.Handler;
import android.os.Looper;
import android.view.Gravity;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import androidx.core.graphics.ColorUtils;
import com.google.android.material.button.MaterialButton;
import com.google.android.material.card.MaterialCardView;
import com.google.android.material.progressindicator.LinearProgressIndicator;

/**
 * Material 3-inspired, on-brand, accessible update screen.
 * Every update phase and percentage comes from UpdateManager, never animation.
 * No network activity is run on the UI thread; pressing Install delegates to
 * Android's normal user-approved package installer.
 */
final class UpdateSplash {
    private UpdateSplash(){}

    static String actionText(int stage,boolean ready){
        if(ready)return "Install verified update";
        if(stage==UpdateManager.FAILED)return "Retry update";
        if(stage==UpdateManager.UP_TO_DATE)return "Check again";
        if(stage==UpdateManager.AVAILABLE)return "Download update";
        return "Check for updates";
    }

    static String progressText(int stage,int percent){
        if(stage==UpdateManager.DOWNLOADING && percent>=0)
            return Math.min(100,Math.max(0,percent))+"% downloaded";
        if(stage==UpdateManager.VERIFYING)return "Verifying package";
        if(stage==UpdateManager.READY)return "Verified and ready";
        if(stage==UpdateManager.CHECKING)return "Checking releases";
        if(stage==UpdateManager.UP_TO_DATE)return "You're up to date";
        if(stage==UpdateManager.AVAILABLE)return "Update available";
        if(stage==UpdateManager.FAILED)return "Check interrupted";
        return "Ready to check";
    }

    static void show(MainActivity activity){show(activity,true);}

    // runCheck=false supports visual/instrumented review without network requests.
    static void show(MainActivity activity,boolean runCheck){
        if(activity.isFinishing()||activity.isDestroyed())return;
        final int accent=activity.accent;
        final int foreground=activity.text;
        final int secondary=activity.hint;
        final int surface=activity.card;
        final int background=activity.bg;
        final int border=activity.outline;

        final Dialog dialog=new Dialog(activity);
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
        FrameLayout root=new FrameLayout(activity);
        root.setBackgroundColor(background);

        ScrollView scroll=new ScrollView(activity);
        scroll.setFillViewport(true);
        scroll.setClipToPadding(false);
        scroll.setVerticalScrollBarEnabled(false);
        LinearLayout page=new LinearLayout(activity);
        page.setOrientation(LinearLayout.VERTICAL);
        page.setGravity(Gravity.CENTER_HORIZONTAL);
        page.setPadding(activity.dp(20),activity.dp(42),activity.dp(20),activity.dp(32));
        scroll.addView(page,new ScrollView.LayoutParams(-1,-2));
        root.addView(scroll,new FrameLayout.LayoutParams(-1,-1));

        // The hero uses the current CYN palette, independent of wallpaper colors.
        MaterialCardView hero=new MaterialCardView(activity);
        hero.setCardBackgroundColor(ColorUtils.blendARGB(surface,accent,.12f));
        hero.setRadius(activity.dp(28));
        hero.setStrokeWidth(activity.dp(1));
        hero.setStrokeColor(ColorUtils.blendARGB(border,accent,.25f));
        hero.setCardElevation(0);
        LinearLayout header=new LinearLayout(activity);
        header.setOrientation(LinearLayout.VERTICAL);
        header.setPadding(activity.dp(23),activity.dp(24),activity.dp(23),activity.dp(22));
        hero.addView(header,new FrameLayout.LayoutParams(-1,-2));

        TextView eyebrow=text(activity,"CYN  /  UPDATES",11,accent,true);
        eyebrow.setLetterSpacing(.13f);
        add(header,eyebrow);
        space(header,12);
        TextView title=text(activity,"A better listening experience.",26,foreground,true);
        title.setLineSpacing(activity.dp(1),1f);
        add(header,title);
        space(header,8);
        add(header,text(activity,
            "Official releases, checked securely and installed with your permission.",
            13,secondary,false));
        space(header,20);

        // Explicit version metadata instead of generic animated filler.
        LinearLayout versions=new LinearLayout(activity);
        versions.setOrientation(LinearLayout.HORIZONTAL);
        versions.setGravity(Gravity.CENTER_VERTICAL);
        add(header,versions);
        LinearLayout local=column(activity);
        versions.addView(local,new LinearLayout.LayoutParams(0,-2,1));
        add(local,text(activity,"INSTALLED",10,secondary,true));
        add(local,text(activity,"v"+BuildConfig.VERSION_NAME,15,foreground,true));
        LinearLayout candidate=column(activity);
        versions.addView(candidate,new LinearLayout.LayoutParams(0,-2,1));
        add(candidate,text(activity,"LATEST FOUND",10,secondary,true));
        TextView targetVersion=text(activity,"Checking…",15,foreground,true);
        add(candidate,targetVersion);

        LinearLayout.LayoutParams heroParams=new LinearLayout.LayoutParams(-1,-2);
        heroParams.bottomMargin=activity.dp(16);
        page.addView(hero,heroParams);

        MaterialCardView body=new MaterialCardView(activity);
        body.setCardBackgroundColor(surface);
        body.setRadius(activity.dp(28));
        body.setStrokeWidth(activity.dp(1));
        body.setStrokeColor(border);
        body.setCardElevation(0);
        LinearLayout panel=column(activity);
        panel.setPadding(activity.dp(22),activity.dp(22),activity.dp(22),activity.dp(24));
        body.addView(panel,new FrameLayout.LayoutParams(-1,-2));
        add(page,body);

        TextView stageTitle=text(activity,"Ready to check",21,foreground,true);
        add(panel,stageTitle);
        space(panel,5);
        TextView status=text(activity,UpdateManager.message,13,secondary,false);
        status.setAccessibilityLiveRegion(View.ACCESSIBILITY_LIVE_REGION_POLITE);
        status.setMinHeight(activity.dp(42));
        add(panel,status);
        space(panel,20);

        // Material progress indicator, determinate only for actual byte progress.
        LinearProgressIndicator bar=new LinearProgressIndicator(activity);
        bar.setIndicatorColor(accent);
        bar.setTrackColor(ColorUtils.blendARGB(surface,accent,.16f));
        bar.setTrackThickness(activity.dp(7));
        bar.setTrackCornerRadius(activity.dp(4));
        add(panel,bar);
        space(panel,8);
        LinearLayout metarow=new LinearLayout(activity);
        metarow.setGravity(Gravity.CENTER_VERTICAL);
        add(panel,metarow);
        TextView detail=text(activity,"Ready",12,secondary,false);
        metarow.addView(detail,new LinearLayout.LayoutParams(0,-2,1));
        TextView percent=text(activity,"",12,accent,true);
        metarow.addView(percent);
        space(panel,20);

        LinearLayout steps=new LinearLayout(activity);
        steps.setOrientation(LinearLayout.HORIZONTAL);
        add(panel,steps);
        String[] captions={"1 · Check","2 · Download","3 · Verify","4 · Install"};
        TextView[] checkpoints=new TextView[4];
        for(int i=0;i<4;i++){
            TextView checkpoint=text(activity,captions[i],11,secondary,i==0);
            checkpoint.setGravity(Gravity.CENTER);
            checkpoint.setMinHeight(activity.dp(39));
            LinearLayout.LayoutParams p=new LinearLayout.LayoutParams(0,activity.dp(39),1);
            if(i>0)p.leftMargin=activity.dp(5);
            steps.addView(checkpoint,p);
            checkpoints[i]=checkpoint;
        }
        space(panel,24);

        MaterialButton action=new MaterialButton(activity);
        action.setText("Check for updates");
        action.setTextSize(14);
        action.setAllCaps(false);
        action.setTypeface(null,Typeface.BOLD);
        action.setCornerRadius(activity.dp(23));
        action.setInsetTop(0);
        action.setInsetBottom(0);
        action.setMinimumHeight(activity.dp(54));
        action.setBackgroundTintList(ColorStateList.valueOf(accent));
        action.setTextColor(background);
        action.setStrokeWidth(0);
        add(panel,action);
        space(panel,12);
        add(panel,text(activity,
            "Downloads are SHA-256 checked and matched to CYN's installed signing certificate. Android confirms installation.",
            12,secondary,false));

        MaterialButton close=new MaterialButton(activity);
        close.setText("Done");
        close.setTextSize(13);
        close.setAllCaps(false);
        close.setCornerRadius(activity.dp(22));
        close.setInsetTop(0);
        close.setInsetBottom(0);
        close.setBackgroundTintList(ColorStateList.valueOf(surface));
        close.setTextColor(foreground);
        close.setStrokeWidth(activity.dp(1));
        close.setStrokeColor(ColorStateList.valueOf(border));
        LinearLayout.LayoutParams closeParams=new LinearLayout.LayoutParams(-1,activity.dp(50));
        closeParams.topMargin=activity.dp(14);
        page.addView(close,closeParams);
        close.setOnClickListener(v->dialog.dismiss());

        Handler handler=new Handler(Looper.getMainLooper());
        Runnable[] ticker=new Runnable[1];
        int[] previous=new int[]{Integer.MIN_VALUE,Integer.MIN_VALUE,-1};
        ticker[0]=()->{
            if(!dialog.isShowing())return;
            int stage=UpdateManager.stage;
            boolean ready=UpdateManager.ready(activity);
            int visibleStage=ready?UpdateManager.READY:stage;
            int progress=UpdateManager.progress;
            boolean busy=UpdateManager.busy();
            // Keep the layout and screen reader calm when a download updates quickly.
            if(previous[0]!=visibleStage||previous[1]!=progress||previous[2]!=(busy?1:0)){
                previous[0]=visibleStage;previous[1]=progress;previous[2]=busy?1:0;
                stageTitle.setText(progressText(visibleStage,progress));
                int active=visibleStage==UpdateManager.DOWNLOADING?1:
                    visibleStage==UpdateManager.VERIFYING?2:
                    visibleStage==UpdateManager.READY?3:
                    visibleStage==UpdateManager.UP_TO_DATE?0:0;
                for(int i=0;i<checkpoints.length;i++){
                    TextView chip=checkpoints[i];
                    boolean selected=i==active;
                    chip.setTextColor(selected?background:secondary);
                    GradientDrawable bg=new GradientDrawable();
                    bg.setColor(selected?accent:ColorUtils.blendARGB(surface,accent,.09f));
                    bg.setCornerRadius(activity.dp(13));
                    chip.setBackground(bg);
                    chip.setTypeface(null,selected?Typeface.BOLD:Typeface.NORMAL);
                }
                if(visibleStage==UpdateManager.CHECKING||visibleStage==UpdateManager.VERIFYING||
                        (visibleStage==UpdateManager.DOWNLOADING&&progress<0)){
                    bar.setIndeterminate(true);
                }else{
                    bar.setIndeterminate(false);
                    bar.setProgressCompat(visibleStage==UpdateManager.READY?100:
                        visibleStage==UpdateManager.DOWNLOADING?Math.max(0,Math.min(100,progress)):0,false);
                }
                percent.setText(visibleStage==UpdateManager.DOWNLOADING&&progress>=0?
                    Math.min(100,progress)+"%":
                    visibleStage==UpdateManager.READY?"100%":"");
                detail.setText(visibleStage==UpdateManager.READY?"Verified · ready to install":
                    visibleStage==UpdateManager.VERIFYING?"Authenticating update":
                    visibleStage==UpdateManager.DOWNLOADING?"Receiving APK":
                    visibleStage==UpdateManager.FAILED?"See the error above, then retry":
                    visibleStage==UpdateManager.UP_TO_DATE?"No newer release available":
                    visibleStage==UpdateManager.CHECKING?"Contacting official releases":"Encrypted HTTPS download");
                action.setEnabled(!busy);
                action.setText(busy?"Working…":actionText(stage,ready));
                action.setAlpha(busy?.65f:1f);
            }
            String message=UpdateManager.message;
            if(!message.contentEquals(status.getText()))status.setText(message);
            String candidateVersion=UpdateManager.availableVersion(activity);
            String display=candidateVersion.isEmpty()?"Not found yet":"v"+candidateVersion;
            if(!display.contentEquals(targetVersion.getText()))targetVersion.setText(display);
            handler.postDelayed(ticker[0],160);
        };
        action.setOnClickListener(v->{
            if(UpdateManager.busy())return;
            v.performHapticFeedback(android.view.HapticFeedbackConstants.CLOCK_TICK);
            if(UpdateManager.ready(activity)){
                UpdateManager.install(activity);
            }else{
                UpdateManager.check(activity,true,null);
            }
            ticker[0].run();
        });
        dialog.setOnDismissListener(d->handler.removeCallbacks(ticker[0]));
        dialog.setContentView(root);
        dialog.show();
        Window window=dialog.getWindow();
        if(window!=null){
            window.setBackgroundDrawable(new ColorDrawable(background));
            window.addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
            window.setStatusBarColor(background);
            window.setNavigationBarColor(background);
            window.setLayout(-1,-1);
        }
        if(runCheck&&!UpdateManager.busy()&&!UpdateManager.ready(activity)){
            UpdateManager.check(activity,true,null);
        }
        ticker[0].run();
    }

    private static LinearLayout column(Context context){
        LinearLayout l=new LinearLayout(context);l.setOrientation(LinearLayout.VERTICAL);return l;
    }
    private static void add(LinearLayout parent,View child){
        parent.addView(child,new LinearLayout.LayoutParams(-1,-2));
    }
    private static void space(LinearLayout parent,int dp){
        parent.addView(new View(parent.getContext()),new LinearLayout.LayoutParams(1,
            Math.round(dp*parent.getResources().getDisplayMetrics().density)));
    }
    private static TextView text(Context context,String content,int size,int color,boolean bold){
        TextView t=new TextView(context);
        t.setText(content);t.setTextSize(size);t.setTextColor(color);
        if(bold)t.setTypeface(null,Typeface.BOLD);
        t.setGravity(Gravity.CENTER_VERTICAL);
        return t;
    }
}
