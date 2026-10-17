"""CYN APEX desktop preview: run ALL 6,307,477 parameters on WAV audio.
Not a real-time Windows playback filter. No training or user data upload.
"""
from __future__ import annotations
import os, pathlib, sys, threading
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

def asset(name):
    return pathlib.Path(getattr(sys,"_MEIPASS",pathlib.Path(__file__).parent))/name

class ApexApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("CYN APEX • 6.3M Neural Audio")
        self.geometry("660x400")
        self.minsize(590,365)
        self.configure(bg="#090b12")
        self.input_file=None
        self.wet=tk.IntVar(value=65)
        self.status=tk.StringVar(value="Ready • pretrained APEX-6M checkpoint • no new training")
        self.progress=tk.DoubleVar(value=0)
        style=ttk.Style()
        style.theme_use("clam")
        style.configure("TButton",font=("Segoe UI",11),padding=9)
        style.configure("TProgressbar",thickness=15)
        frame=tk.Frame(self,bg="#090b12",padx=25,pady=20)
        frame.pack(fill="both",expand=True)
        self._label(frame,"CYN APEX",25,bold=True,fg="#ecf4ff").pack(anchor="w")
        self._label(frame,"Full 6,307,477-parameter neural audio • CPU ONNX • WAV testing preview",11,fg="#83abc4").pack(anchor="w",pady=(3,22))
        self.path_label=self._label(frame,"No audio file selected",11,fg="#c1d4e5")
        self.path_label.pack(anchor="w",pady=(0,8))
        row=tk.Frame(frame,bg="#090b12"); row.pack(fill="x",pady=5)
        self.open_button=ttk.Button(row,text="1. Select WAV",command=self.choose)
        self.open_button.pack(side="left",padx=(0,12))
        self.process_button=ttk.Button(row,text="2. Process with APEX",command=self.process)
        self.process_button.pack(side="left")
        self._label(frame,"Neural processing strength (0% bypass / 100% full mask)",10,fg="#9aafc4").pack(anchor="w",pady=(20,5))
        tk.Scale(frame,variable=self.wet,from_=0,to=100,orient="horizontal",length=525,
                 bg="#090b12",fg="#e1f7ff",troughcolor="#1f344a",highlightthickness=0).pack(anchor="w")
        self.bar=ttk.Progressbar(frame,variable=self.progress,maximum=100)
        self.bar.pack(fill="x",pady=(15,6))
        self._label(frame,"This processes WAV files, not Spotify or system playback. No cloud uploads.",9,fg="#8399ac").pack(anchor="w",pady=(0,5))
        self._label(frame,"Checkpoint: 225 earlier synthetic training steps. Further training is paused.",9,fg="#8399ac").pack(anchor="w")
        self._label(frame,self.status,10,fg="#67d1df").pack(anchor="w",pady=(14,0))
    def _label(self,parent,text,size,bold=False,fg="#e8f5ff"):
        return tk.Label(parent,**({"textvariable":text} if isinstance(text,tk.Variable) else {"text":text}),font=("Segoe UI",size,"bold" if bold else "normal"),
                        bg="#090b12",fg=fg,anchor="w",wraplength=595,justify="left")
    def choose(self):
        name=filedialog.askopenfilename(filetypes=[("WAV audio","*.wav")],title="Choose test audio")
        if name:
            self.input_file=pathlib.Path(name)
            self.path_label.config(text=self.input_file.name)
    def process(self):
        if not self.input_file:return messagebox.showinfo("APEX","Choose a WAV audio file first.")
        output=filedialog.asksaveasfilename(defaultextension=".wav",filetypes=[("WAV audio","*.wav")],
                   title="Save APEX processed audio",initialfile=self.input_file.stem+"-apex.wav")
        if not output:return
        if pathlib.Path(output).resolve()==self.input_file.resolve():
            return messagebox.showerror("APEX","Choose a different output file to preserve the original.")
        wet=float(self.wet.get())/100.0
        self.process_button.configure(state="disabled")
        self.open_button.configure(state="disabled")
        self.progress.set(0);self.status.set("Loading full APEX-6M ONNX model...")
        def run():
            try:
                import onnxruntime as ort
                from audio import process_audio
                model=asset("apex6m-225.onnx")
                if not model.is_file():
                    raise FileNotFoundError("APEX neural model missing from this installation")
                session=ort.InferenceSession(str(model),sess_options=self._session_options(ort),
                       providers=["CPUExecutionProvider"])
                def progress(done,total):
                    self.after(0,lambda:self.progress.set(100.0*done/total))
                    self.after(0,lambda:self.status.set(f"Neural inference: {done}/{total} windows"))
                process_audio(session,self.input_file,pathlib.Path(output),wet,progress)
                self.after(0,lambda:messagebox.showinfo("APEX","Processed audio saved:\n"+str(output)))
                self.after(0,lambda:self.status.set("Complete • ready for another test"))
            except Exception as err:
                msg=str(err)
                self.after(0,lambda:messagebox.showerror("APEX processing failed",msg))
                self.after(0,lambda:self.status.set("Processing failed; original audio unchanged"))
            finally:
                self.after(0,lambda:self.process_button.configure(state="normal"))
                self.after(0,lambda:self.open_button.configure(state="normal"))
        threading.Thread(target=run,name="APEX-ONNX-CPU",daemon=True).start()
    @staticmethod
    def _session_options(ort):
        opts=ort.SessionOptions()
        opts.intra_op_num_threads=2
        opts.inter_op_num_threads=1
        opts.graph_optimization_level=ort.GraphOptimizationLevel.ORT_ENABLE_ALL
        return opts

if __name__=="__main__":
    ApexApp().mainloop()
