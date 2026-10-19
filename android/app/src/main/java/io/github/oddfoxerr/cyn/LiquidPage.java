// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;
import android.animation.*;
import android.content.Context;
import android.graphics.*;
import android.view.*;
import android.view.animation.PathInterpolator;
import android.widget.FrameLayout;

/** Warps the outgoing page's actual pixels while revealing the live destination. */
final class LiquidPage extends FrameLayout {
    private WarpOverlay overlay;
    private ValueAnimator animator;
    LiquidPage(Context c){super(c);}
    boolean isTransitioning(){return overlay!=null;}
    void change(View next,boolean motion){
        cancel();
        Bitmap outgoing=null;
        if(motion&&getWidth()>0&&getHeight()>0&&getChildCount()>0){
            try{outgoing=Bitmap.createBitmap(getWidth(),getHeight(),Bitmap.Config.ARGB_8888);getChildAt(0).draw(new Canvas(outgoing));}catch(OutOfMemoryError e){outgoing=null;}
        }
        removeAllViews();addView(next,new LayoutParams(-1,-1));
        if(outgoing==null)return;
        WarpOverlay current=new WarpOverlay(getContext(),outgoing);overlay=current;addView(current,new LayoutParams(-1,-1));
        animator=ValueAnimator.ofFloat(0,1);animator.setDuration(480);animator.setInterpolator(new PathInterpolator(.2f,0,.2f,1));
        animator.addUpdateListener(a->{current.progress=(float)a.getAnimatedValue();current.invalidate();});
        animator.addListener(new AnimatorListenerAdapter(){@Override public void onAnimationEnd(Animator a){if(overlay==current){removeView(current);overlay=null;animator=null;}}});animator.start();
    }
    private void cancel(){if(animator!=null){animator.cancel();animator=null;}if(overlay!=null){removeView(overlay);overlay=null;}}
    /** A separate render node keeps mesh clipping out of the page and surrounding UI. */
    private static final class WarpOverlay extends View {
        final Bitmap outgoing;
        final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);
        final Path edge=new Path();
        final float[] mesh=new float[21*29*2];
        float progress;
        WarpOverlay(Context c,Bitmap snapshot){super(c);outgoing=snapshot;setImportantForAccessibility(IMPORTANT_FOR_ACCESSIBILITY_NO);}
        @Override protected void onDraw(Canvas canvas){
            float w=getWidth(),h=getHeight();if(w<=0||h<=0||progress>=1)return;
            float front=h*(1-progress),amplitude=(float)Math.sin(Math.PI*progress)*h*.07f;
            edge.reset();edge.moveTo(0,0);edge.lineTo(w,0);
            for(int i=40;i>=0;i--){float x=w*i/40f,y=front+amplitude*(float)(Math.sin(i*.24+progress*5)+.3*Math.sin(i*.65-progress*7));edge.lineTo(x,y);}
            edge.close();int saved=canvas.save();canvas.clipPath(edge);
            for(int y=0;y<=28;y++)for(int x=0;x<=20;x++){int n=(y*21+x)*2;float px=w*x/20,py=h*y/28;float influence=(float)Math.exp(-Math.pow((py-front)/(h*.25f),2));mesh[n]=px+(float)Math.sin(y*.5+progress*5)*amplitude*.15f*influence;mesh[n+1]=py+amplitude*(float)Math.sin(x*.48+progress*5)*influence;}
            canvas.drawBitmapMesh(outgoing,20,28,mesh,0,null,0,paint);canvas.restoreToCount(saved);
            // Let Android release the snapshot after this render node is no longer in use.
        }
    }
    @Override public boolean onInterceptTouchEvent(MotionEvent event){return overlay!=null||super.onInterceptTouchEvent(event);}
    @Override protected void onDetachedFromWindow(){cancel();super.onDetachedFromWindow();}
}
