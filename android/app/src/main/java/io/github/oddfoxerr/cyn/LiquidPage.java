// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

import android.animation.*;
import android.content.Context;
import android.graphics.*;
import android.view.*;
import android.view.animation.PathInterpolator;
import android.widget.FrameLayout;

/** Page switches reveal the actual next page while dissolving the outgoing pixels. */
final class LiquidPage extends FrameLayout {
    static final String[] MODES={"Liquid ripple","Material fade","Horizontal glide","Iris reveal","Prism sweep","Instant"};
    static int normalized(int value){return value>=0 && value<MODES.length?value:0;}
    private WarpOverlay overlay;
    private ValueAnimator animator;
    LiquidPage(Context c){super(c);}
    boolean isTransitioning(){return overlay!=null;}
    void change(View next,boolean motion){change(next,motion,0,Color.CYAN);}
    void change(View next,boolean motion,int selectedMode,int accent){
        cancel();
        int mode=normalized(selectedMode);
        Bitmap outgoing=null;
        if(motion && mode!=5 && getWidth()>0 && getHeight()>0 && getChildCount()>0){
            try{
                outgoing=Bitmap.createBitmap(getWidth(),getHeight(),Bitmap.Config.ARGB_8888);
                getChildAt(0).draw(new Canvas(outgoing));
            }catch(OutOfMemoryError e){outgoing=null;}
        }
        removeAllViews();addView(next,new LayoutParams(-1,-1));
        if(outgoing==null)return;
        WarpOverlay current=new WarpOverlay(getContext(),outgoing,mode,accent);
        overlay=current;addView(current,new LayoutParams(-1,-1));
        animator=ValueAnimator.ofFloat(0,1);
        animator.setDuration(mode==1?265:mode==2?340:mode==3?420:mode==4?490:470);
        animator.setInterpolator(new PathInterpolator(.2f,0,.2f,1));
        animator.addUpdateListener(a->{current.progress=(float)a.getAnimatedValue();current.invalidate();});
        animator.addListener(new AnimatorListenerAdapter(){
            @Override public void onAnimationEnd(Animator a){
                if(overlay==current){removeView(current);overlay=null;animator=null;current.recycle();}
            }
        });
        animator.start();
    }
    private void cancel(){
        if(animator!=null){animator.cancel();animator=null;}
        if(overlay!=null){removeView(overlay);overlay.recycle();overlay=null;}
    }
    private static final class WarpOverlay extends View {
        Bitmap outgoing;
        final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);
        final Paint prism=new Paint(Paint.ANTI_ALIAS_FLAG);
        final Path edge=new Path();
        final float[] mesh=new float[21*29*2];
        final int mode,accent;
        float progress;
        WarpOverlay(Context c,Bitmap snapshot,int selectedMode,int colour){
            super(c);outgoing=snapshot;mode=selectedMode;accent=colour;
            setImportantForAccessibility(IMPORTANT_FOR_ACCESSIBILITY_NO);
        }
        void recycle(){if(outgoing!=null&&!outgoing.isRecycled()){outgoing.recycle();outgoing=null;}}
        @Override protected void onDraw(Canvas canvas){
            float w=getWidth(),h=getHeight(),t=progress;
            if(outgoing==null||w<=0||h<=0||t>=1)return;
            if(mode==1){
                paint.setAlpha(Math.round(255*(1-t)));
                canvas.drawBitmap(outgoing,0,0,paint);
                paint.setAlpha(255);
                return;
            }
            if(mode==2){
                paint.setAlpha(Math.round(255*Math.min(1,(1-t)*1.6f)));
                canvas.drawBitmap(outgoing,-w*t,0,paint);
                paint.setAlpha(255);
                return;
            }
            if(mode==3){
                // A circular hole exposes the actual new page from the selected tab's center.
                int save=canvas.save();
                Path mask=new Path();
                mask.setFillType(Path.FillType.EVEN_ODD);
                mask.addRect(0,0,w,h,Path.Direction.CW);
                float radius=(float)Math.hypot(w,h)*t;
                mask.addCircle(w*.5f,h*.56f,radius,Path.Direction.CCW);
                canvas.clipPath(mask);
                canvas.drawBitmap(outgoing,0,0,paint);
                canvas.restoreToCount(save);
                return;
            }
            if(mode==4){
                // Layered prismatic mask follows actual transition progress, not a looping overlay.
                float front=w*(1-t);
                edge.reset();
                edge.moveTo(0,0);edge.lineTo(front-w*.14f,0);
                for(int j=0;j<=36;j++){
                    float y=h*j/36f;
                    float x=front+w*.07f*(float)Math.sin(j*.55+t*10);
                    edge.lineTo(x,y);
                }
                edge.lineTo(0,h);edge.close();
                int save=canvas.save();canvas.clipPath(edge);
                canvas.drawBitmap(outgoing,0,0,paint);
                canvas.restoreToCount(save);
                prism.setColor(accent);
                prism.setAlpha(Math.round(100*Math.sin(Math.PI*t)));
                prism.setStyle(Paint.Style.STROKE);
                prism.setStrokeWidth(Math.max(2,w*.009f));
                canvas.drawPath(edge,prism);
                prism.setStyle(Paint.Style.FILL);
                return;
            }
            // Original liquid mesh: outgoing page itself stretches and clears.
            float front=h*(1-t),amplitude=(float)Math.sin(Math.PI*t)*h*.07f;
            edge.reset();edge.moveTo(0,0);edge.lineTo(w,0);
            for(int i=40;i>=0;i--){
                float x=w*i/40f,y=front+amplitude*(float)(Math.sin(i*.24+t*5)+.3*Math.sin(i*.65-t*7));
                edge.lineTo(x,y);
            }
            edge.close();int saved=canvas.save();canvas.clipPath(edge);
            for(int y=0;y<=28;y++)for(int x=0;x<=20;x++){
                int n=(y*21+x)*2;float px=w*x/20,py=h*y/28;
                float influence=(float)Math.exp(-Math.pow((py-front)/(h*.25f),2));
                mesh[n]=px+(float)Math.sin(y*.5+t*5)*amplitude*.15f*influence;
                mesh[n+1]=py+amplitude*(float)Math.sin(x*.48+t*5)*influence;
            }
            canvas.drawBitmapMesh(outgoing,20,28,mesh,0,null,0,paint);
            canvas.restoreToCount(saved);
        }
    }
    @Override public boolean onInterceptTouchEvent(MotionEvent event){
        return overlay!=null||super.onInterceptTouchEvent(event);
    }
    @Override protected void onDetachedFromWindow(){cancel();super.onDetachedFromWindow();}
}
