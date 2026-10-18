#!/usr/bin/env python3
"""Offline liquid-close motion study: semi-Lagrangian advection and pressure projection.
Inspired by fluid simulation methods, not copied from or equivalent to tmarrec's 3D solver.
This experiment does not touch native CYN, playback, DSP, or release packaging.
"""
import argparse
import json
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw

PRESETS = ("cascade", "vortex", "ripple")


def smoothstep(lo, hi, v):
    q = np.clip((v - lo) / (hi - lo), 0.0, 1.0)
    return q*q*(3.0-2.0*q)


class StableFlow:
    def __init__(self, w=160, h=90):
        self.w, self.h = w, h
        self.y, self.x = np.mgrid[0:h, 0:w].astype(np.float32)
        self.nx, self.ny = self.x/(w-1), self.y/(h-1)
        self.u = np.zeros((h,w),np.float32)
        self.v = np.zeros((h,w),np.float32)
        self.dye = np.ones((h,w),np.float32)

    def sample(self, a, x, y):
        x=np.clip(x,0,self.w-1.001); y=np.clip(y,0,self.h-1.001)
        x0=np.floor(x).astype(np.int32);y0=np.floor(y).astype(np.int32)
        x1=np.minimum(x0+1,self.w-1);y1=np.minimum(y0+1,self.h-1)
        dx=x-x0;dy=y-y0
        return ((1-dy)*((1-dx)*a[y0,x0]+dx*a[y0,x1])+
                dy*((1-dx)*a[y1,x0]+dx*a[y1,x1]))

    def advect(self, a, dt=.85):
        return self.sample(a,self.x-dt*self.u,self.y-dt*self.v)

    def erode(self, preset, progress, frame):
        x,y=self.nx,self.ny
        if preset=="vortex":
            radial=np.sqrt(((x-.56)*1.8)**2+(y-.5)**2)
            theta=np.arctan2(y-.5,(x-.56)*1.8)
            ring=.5+.5*np.cos(14*radial-3.4*theta-frame*.095)
            field=np.exp(-((radial-(.015+.90*progress))/.30)**2)*(.46+.54*ring)
            field+=.11*(1-radial)
        elif preset=="cascade":
            trickle=(np.sin(x*34.+3*np.sin(y*13.+frame*.085))**2)**5
            front=smoothstep(.10,.28,y-progress*1.26+.10*np.sin(x*14.+frame*.035))
            field=(.15+.9*trickle)*np.maximum(0.,1.-front)
            field+=.16*np.sin(y*18+frame*.13)**2
        else:
            wave=.20+.07*np.sin(y*21+frame*.20)+.045*np.sin(y*41-frame*.14)
            front=smoothstep(-.10,.20,progress*1.33-x+wave)
            field=front*(.47+.53*np.sin(y*31+x*9+frame*.11)**2)
            field+=.08*np.cos(x*24+y*26-frame*.22)**2
        return np.float32(.031+.075*progress**2)*np.clip(field,0.,2.)

    def step(self,preset,frame,melt):
        xx,yy=self.nx,self.ny
        if preset=="vortex":
            vortices=[(.54,.48,2.,.28),(.80,.22,-1.2,.17)]
            gravity=.05
        elif preset=="cascade":
            vortices=[(.29,.46,.90,.28),(.76,.58,-1.25,.29)]
            gravity=.15
        else:
            vortices=[(.21,.34,.90,.19),(.73,.53,-1.,.22)]
            gravity=.075
        for cx,cy,strength,falloff in vortices:
            rx=(xx-cx)*self.w/self.h; ry=yy-cy
            decay=np.exp(-(rx*rx+ry*ry)/(falloff*falloff))
            self.u+=strength*-ry*decay*2.0
            self.v+=strength*rx*decay*2.0
        self.v+=gravity+.08*np.sin(xx*18.+frame*.18)
        if preset=="ripple": self.u+=.23*np.cos(yy*19+frame*.30)
        self.u*=.95;self.v*=.95
        u,v=self.u.copy(),self.v.copy()
        self.u=self.sample(u,self.x-.68*u,self.y-.68*v)
        self.v=self.sample(v,self.x-.68*u,self.y-.68*v)
        self.u[:,0]=self.u[:,-1]=0
        self.v[0,:]=self.v[-1,:]=0
        div=-.5*(np.roll(self.u,-1,1)-np.roll(self.u,1,1)+
                 np.roll(self.v,-1,0)-np.roll(self.v,1,0))
        div[0,:]=div[-1,:]=div[:,0]=div[:,-1]=0
        p=np.zeros_like(self.dye)
        for _ in range(36):
            p=(np.roll(p,1,0)+np.roll(p,-1,0)+
               np.roll(p,1,1)+np.roll(p,-1,1)+div)*.25
            p[:,0]=p[:,1];p[:,-1]=p[:,-2]
            p[0,:]=p[1,:];p[-1,:]=p[-2,:]
        self.u-=.5*(np.roll(p,-1,1)-np.roll(p,1,1))
        self.v-=.5*(np.roll(p,-1,0)-np.roll(p,1,0))
        self.u[:,0]=self.u[:,-1]=0
        self.v[0,:]=self.v[-1,:]=0
        if melt>0:
            self.dye=self.advect(self.dye)
            self.dye=np.clip(self.dye-self.erode(preset,melt,frame),0.,1.)
        return self.dye


def expand_button(flow,position):
    """Start as the X's rectangular control, expand to cover entire window."""
    x,y=flow.nx,flow.ny
    ix,iy,iw,ih=.956,.055,.044,.078
    k=float(smoothstep(0.,1.,position))
    left=(ix-iw*.5)*(1-k)
    right=(ix+iw*.5)*(1-k)+k
    top=(iy-ih*.5)*(1-k)
    bottom=(iy+ih*.5)*(1-k)+k
    edge=.0055*(1-k)+.002
    return (smoothstep(left-edge,left+edge,x)*
            smoothstep(right+edge,right-edge,x)*
            smoothstep(top-edge,top+edge,y)*
            smoothstep(bottom+edge,bottom-edge,y)).astype(np.float32)


def preview_background(w,h):
    """Contextual fake window for visual evaluation only, never used by native CYN."""
    img=Image.new("RGBA",(w,h),(9,13,23,255))
    d=ImageDraw.Draw(img)
    d.rounded_rectangle((13,13,w-13,h-13),radius=17,
                        fill=(12,17,29,255),outline=(70,88,114,255),width=2)
    d.rounded_rectangle((13,13,w-13,61),radius=15,fill=(18,26,42,255))
    d.text((32,31),"CYN   /   LIQUID SIMULATION PREVIEW",fill=(194,214,244))
    d.rounded_rectangle((w-62,24,w-28,51),radius=7,
                        fill=(26,47,68,255),outline=(81,209,221,255),width=1)
    d.line((w-53,31,w-37,45),fill=(196,249,251),width=2)
    d.line((w-37,31,w-53,45),fill=(196,249,251),width=2)
    d.rounded_rectangle((32,87,w-32,h-32),radius=15,
                        fill=(14,20,35,255),outline=(35,57,78,255),width=2)
    for i,label in enumerate(("SOUND STUDIO","SMART LISTENING","DEVICES","THEMES")):
        d.text((52,125+i*50),label,fill=(66,101,129))
    for i in range(6):
        d.rounded_rectangle((265+i*69,245,307+i*69,304),radius=7,
                            fill=(21,35,49,255),outline=(37,67,86,255))
    return img


def render(flow,mask,preset,phase,idx,scale):
    w,h=flow.w*scale,flow.h*scale
    m=np.clip(mask,0.,1.)
    alpha=Image.fromarray(np.uint8(m*255),"L")
    high=alpha.resize((w,h),Image.Resampling.BICUBIC)
    a=np.asarray(high,dtype=np.float32)/255.
    dy,dx=np.gradient(a)
    edge=np.clip(np.sqrt(dx*dx+dy*dy)*7.5,0.,1.)
    yy,xx=np.mgrid[0:h,0:w]
    shimmer=.5+.5*np.sin(xx*.023+yy*.037+idx*.19)
    cyan=np.clip((edge*.9+shimmer*.12)*255.,0.,255.)
    rgba=np.empty((h,w,4),np.uint8)
    rgba[:,:,0]=np.uint8(np.clip(4+cyan*.12+edge*8,0,255))
    rgba[:,:,1]=np.uint8(np.clip(15+cyan*.47+edge*46,0,255))
    rgba[:,:,2]=np.uint8(np.clip(25+cyan*.74+edge*65,0,255))
    rgba[:,:,3]=np.uint8(a*255.)
    out=preview_background(w,h)
    out.alpha_composite(Image.fromarray(rgba,"RGBA"))
    d=ImageDraw.Draw(out)
    name={"cascade":"GRAVITY MELT","vortex":"WHIRLPOOL","ripple":"LIQUID RIPPLE"}[preset]
    status={"expand":"01  BUTTON EXPANDS","hold":"02  FULL COVER",
            "dissolve":"03  FLUID DISSOLVE"}[phase]
    d.rounded_rectangle((22,h-47,320,h-18),radius=9,fill=(6,12,22,234))
    d.text((33,h-39),name+"  /  "+status,fill=(173,235,242))
    return out,alpha


def bake(preset,destination,frames=72,fps=60,w=160,h=90,scale=4):
    destination.mkdir(parents=True,exist_ok=True)
    masks=destination/"masks";flows=destination/"flow"
    masks.mkdir(exist_ok=True);flows.mkdir(exist_ok=True)
    sim=StableFlow(w,h)
    previews=[]
    filmstrip=[]
    sample_ids=set(np.linspace(0,frames-1,9,dtype=int).tolist())
    for i in range(frames):
        position=i/(frames-1)
        if position<=.31:
            phase="expand";growth=min(1.,position/.31);melt=0.
        elif position<.405:
            phase="hold";growth=1.;melt=0.
        else:
            phase="dissolve";growth=1.;melt=(position-.405)/.595
        density=sim.step(preset,i,melt)
        mask=expand_button(sim,growth) if melt==0 else density.copy()
        if melt>0: mask*=np.float32(1.-smoothstep(.56,1.,melt))
        preview,alpha=render(sim,mask,preset,phase,i,scale)
        alpha.save(masks/f"{i:04}.png",optimize=True)
        flow=np.empty((h,w,3),np.uint8)
        flow[:,:,0]=np.clip(128+sim.u*16,0,255).astype(np.uint8)
        flow[:,:,1]=np.clip(128+sim.v*16,0,255).astype(np.uint8)
        flow[:,:,2]=np.clip(mask*255,0,255).astype(np.uint8)
        Image.fromarray(flow,"RGB").save(flows/f"{i:04}.png",optimize=True)
        previews.append(preview.convert("P",palette=Image.Palette.ADAPTIVE,colors=128))
        if i in sample_ids: filmstrip.append(preview.convert("RGB"))
        if phase=="hold": assert mask.mean()>.95,"full window not covered"
        if i==frames-1: assert mask.max()<.005,"end must clear"
    previews[0].save(destination/(preset+".gif"),save_all=True,
                     append_images=previews[1:],duration=round(1000/fps),
                     loop=0,optimize=False,disposal=2)
    width,height=filmstrip[0].size
    collage=Image.new("RGB",(width*3,height*3),(10,12,18))
    for i,image in enumerate(filmstrip):
        collage.paste(image,((i%3)*width,(i//3)*height))
    collage.save(destination/(preset+"-filmstrip.jpg"),quality=85,optimize=True)
    manifest={
      "preset":preset,"frames":frames,"fps":fps,"field_size":[w,h],
      "technique":"2D semi-Lagrangian advection, Jacobi pressure projection and art-directed dye erosion",
      "phase_ranges":{"button_expands":[0,.31],"full_cover":[.31,.405],
                      "fluid_dissolve":[.405,1.]},
      "masks":"masks/%04d.png, L (8-bit coverage)",
      "flow":"flow/%04d.png, RGB (R/G velocity centered at 128, 16 units per cell/step, B coverage)",
      "native_integration":"NONE; experiment only; native app and audio unchanged",
      "credits":"Motion methods inspired by Stable Fluids and the shared tmarrec/fluid-simulation project; no third-party implementation copied."
    }
    (destination/"manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
    print("Baked",preset,frames,"frames",w,"x",h,"at",fps,"fps")


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--preset",choices=(*PRESETS,"all"),default="all")
    parser.add_argument("--output",type=Path,default=Path("dist/fluid-simulations"))
    parser.add_argument("--frames",type=int,default=72)
    parser.add_argument("--fps",type=int,default=60)
    parser.add_argument("--grid",default="160x90")
    parser.add_argument("--preview-scale",type=int,default=4)
    a=parser.parse_args()
    if not 20<=a.frames<=240: parser.error("frames must be 20..240")
    if not 24<=a.fps<=120: parser.error("fps must be 24..120")
    try: w,h=(int(s) for s in a.grid.lower().split("x"))
    except (TypeError,ValueError): parser.error("grid must be WIDTHxHEIGHT")
    if not (64<=w<=512 and 32<=h<=288):
        parser.error("grid must be 64x32 through 512x288")
    for preset in PRESETS if a.preset=="all" else (a.preset,):
        bake(preset,a.output/preset,a.frames,a.fps,w,h,a.preview_scale)


if __name__=="__main__":
    main()
