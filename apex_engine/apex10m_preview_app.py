"""CYN APEX 10M Experimental: offline spatial DSP preview (NOT trained neural AI)."""
from __future__ import annotations

import os
from pathlib import Path
import threading
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

import numpy as np
import soundfile as sf
from apex10m_spatial import analyze_stereo, spatial_preserve


class ExperimentalSpatialPreview(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title("CYN APEX 10M • Experimental Spatial Preview")
        self.geometry("660x435")
        self.minsize(560, 390)
        self.configure(bg="#0b101b")
        self.input_file: Path | None = None
        self.intensity = tk.DoubleVar(value=40.0)
        self.status = tk.StringVar(value="Choose a stereo WAV file to preview spatial ambience.")
        self.selection = tk.StringVar(value="No file selected")
        self.intensity_label = tk.StringVar(value="Strength: 40%")
        self._build()

    def _text(self, parent: tk.Widget, text: str, size: int = 11,
              color: str = "#c9d7ec", bold: bool = False) -> tk.Label:
        return tk.Label(parent, text=text, bg="#0b101b", fg=color,
                        font=("Segoe UI", size, "bold" if bold else "normal"),
                        justify="left", anchor="w", wraplength=590)

    def _build(self) -> None:
        style = ttk.Style(self)
        style.theme_use("clam")
        style.configure("TButton", padding=(13, 10), font=("Segoe UI", 10))
        style.configure("TProgressbar", thickness=9)

        main = tk.Frame(self, bg="#0b101b", padx=25, pady=22)
        main.pack(fill="both", expand=True)
        self._text(main, "CYN APEX 10M", 22, "#f0f5ff", True).pack(anchor="w")
        self._text(main, "EXPERIMENTAL  •  SPATIAL AUDIO PREVIEW",
                   10, "#77d8df", True).pack(anchor="w", pady=(0, 17))
        self._text(
            main,
            "Offline stereo ambience processing. The 10M neural architecture "
            "is not trained and is NOT running in this preview.",
            11, "#b6c8e3",
        ).pack(anchor="w", pady=(0, 16))

        self._text(main, "INPUT WAV FILE", 10, "#b7c6d8", True).pack(anchor="w")
        tk.Label(main, textvariable=self.selection, bg="#0b101b", fg="#eaf2fb",
                 font=("Segoe UI", 11), anchor="w",
                 wraplength=580).pack(anchor="w", pady=(3, 8))
        buttons = tk.Frame(main, bg="#0b101b")
        buttons.pack(anchor="w", pady=(0, 13))
        self.choose_button = ttk.Button(buttons, text="Choose stereo WAV", command=self.choose)
        self.choose_button.pack(side="left", padx=(0, 12))
        self.run_button = ttk.Button(buttons, text="Process audio", command=self.start,
                                     state="disabled")
        self.run_button.pack(side="left")

        tk.Label(main, textvariable=self.intensity_label,
                 font=("Segoe UI", 11, "bold"), bg="#0b101b",
                 fg="#cddbea").pack(anchor="w", pady=(0, 5))
        ttk.Scale(main, from_=0, to=100, variable=self.intensity,
                  command=self._strength_changed).pack(fill="x", pady=(0, 15))
        self.bar = ttk.Progressbar(main, mode="indeterminate")
        self.bar.pack(fill="x", pady=(0, 9))
        tk.Label(main, textvariable=self.status, bg="#0b101b", fg="#7ed6e2",
                 font=("Segoe UI", 10), anchor="w", justify="left",
                 wraplength=580).pack(anchor="w", pady=(0, 11))
        self._text(
            main, "Works on existing stereo ambience only • center dialogue protected "
            "• no surround-object or Atmos decoding • max 10-minute WAV",
            9, "#98a6ba",
        ).pack(anchor="w")

    def _strength_changed(self, value: str) -> None:
        self.intensity_label.set(f"Strength: {int(float(value))}%")

    def choose(self) -> None:
        name = filedialog.askopenfilename(
            title="Open stereo WAV",
            filetypes=[("WAV audio", "*.wav"), ("All files", "*.*")],
        )
        if name:
            self.input_file = Path(name)
            self.selection.set(self.input_file.name)
            self.run_button.configure(state="normal")
            self.status.set("Ready. Start with 40% and compare at matched loudness.")

    def start(self) -> None:
        incoming = self.input_file
        if incoming is None:
            return
        name = filedialog.asksaveasfilename(
            title="Save spatial preview WAV",
            defaultextension=".wav",
            initialfile=f"{incoming.stem}-spatial-preview.wav",
            filetypes=[("WAV audio", "*.wav")],
        )
        if not name:
            return
        output = Path(name)
        if incoming.resolve() == output.resolve():
            messagebox.showerror("Invalid output", "Choose a different file to preserve the original.")
            return

        self.run_button.configure(state="disabled")
        self.choose_button.configure(state="disabled")
        self.bar.start(12)
        self.status.set("Processing locally • no audio uploaded")
        strength = float(self.intensity.get()) / 100.0

        def worker() -> None:
            temp = output.with_name(output.name + ".tmp.wav")
            try:
                audio, rate = sf.read(str(incoming), always_2d=True, dtype="float32")
                if audio.shape[1] != 2:
                    raise ValueError("This preview supports stereo WAV only.")
                if len(audio) / rate > 600:
                    raise ValueError("Maximum input duration is 10 minutes.")
                prior = analyze_stereo(audio, rate)
                processed = spatial_preserve(audio, rate, strength)
                subsequent = analyze_stereo(processed, rate)
                output.parent.mkdir(parents=True, exist_ok=True)
                sf.write(str(temp), processed, rate, format="WAV", subtype="PCM_24")
                os.replace(temp, output)
                message = (f"Saved {output.name} • side/mid "
                           f"{prior['side_to_mid']:.2f} → {subsequent['side_to_mid']:.2f}")
                self.after(0, lambda: self._finished(message, output, None))
            except Exception as error:
                temp.unlink(missing_ok=True)
                failure = str(error)
                self.after(0, lambda: self._finished("Processing failed.", None, failure))

        threading.Thread(target=worker, name="APEX10M-SpatialPreview", daemon=True).start()

    def _finished(self, status: str, path: Path | None, error: str | None) -> None:
        self.bar.stop()
        self.status.set(status)
        self.run_button.configure(state="normal")
        self.choose_button.configure(state="normal")
        if error:
            messagebox.showerror("Spatial processing failed", error)
        elif path:
            messagebox.showinfo("Spatial preview saved",
                                f"Saved:\n{path}\n\n"
                                "This is an offline DSP preview; no trained 10M model ran.")


if __name__ == "__main__":
    ExperimentalSpatialPreview().mainloop()
