"""Stage and version a verified Cyn 2.0.14 Windows release."""
import json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
report=json.loads((root/"training-output/starter-evaluation.json").read_text())
model=json.loads((root/"training-output/cyn-starter.json").read_text())
assert report["training"]["actual_gradient_seconds"]>=3600
assert report["training"]["optimizer_steps"]>1000
assert len(model["weights"])==209 and model["ready"]==[True,False,False,False,True]
assert model["schema"]==1 and "examples" in model
assert "AppVersion=2.0.13" in (root/"Installer/Cyn/Cyn.iss").read_text()
(root/"models/cyn-starter.json").write_text(json.dumps(model,indent=2)+"\n")
(root/"models/starter-evaluation.json").write_text(json.dumps(report,indent=2)+"\n")
files=["BUILD_INSTALLER.ps1","Installer/Cyn/Cyn.iss","fxsound/FxSound.jucer",
       "fxsound/FxSoundARM.jucer","fxsound/JuceLibraryCode/JuceHeader.h",
       "fxsound/Project/FxSound_App.vcxproj","fxsound/Project/resources.rc",
       "fxsound/ProjectARM/FxSound_App.vcxproj","fxsound/ProjectARM/resources.rc",
       "fxsound/Source/GUI/FxController.cpp","fxsound/Source/GUI/FxProView.cpp",
       "scripts/test_ai_hub.mjs"]
for name in files:
    file=root/name
    original=file.read_text(encoding="utf-8-sig")
    if "2.0.13" not in original:raise RuntimeError("Wrong source version in "+name)
    updated=original.replace("2.0.13","2.0.14").replace("0x2000d00","0x2000e00").replace("2,0,13,0","2,0,14,0")
    file.write_text(updated,encoding="utf-8")
readme=root/"README_CYN.md"
readme.write_text(readme.read_text(encoding="utf-8-sig").replace(
    "Cyn-Setup-2.0.13-x64.exe","Cyn-Setup-2.0.14-x64.exe")+
    "\n## Cyn 2.0.14 — one-hour experimental base training\nSee models/MODEL_CARD.md for synthetic results.\n")
details=(root/"training-output/training-report.md").read_text()
card=root/"models/MODEL_CARD.md"
card.write_text(card.read_text(encoding="utf-8-sig")+"\n"+details,encoding="utf-8")
(root/"RELEASE_NOTES_2.0.14.md").write_text(
    "# Cyn 2.0.14 — One-hour trained experimental base AI\n\n"
    "Uses 3600 seconds of real gradient optimization and held-out synthetic checks. "
    "No real-music quality is claimed and no private user data was used. "
    "Personal models are preserved; use Reset to starter to activate the new base model.\n\n"
    +details,encoding="utf-8")
print("Validated training model and release metadata are staged.")
