"""Prepare Cyn 2.0.15 from the verified one-hour model artifact.

This exists because Cyn 2.0.14 now ships the completed native neural-activity UI,
while an older already-running training workflow was configured for that version.
The follow-up only ships validated training output, never rewrites the UI release.
"""
import json
import math
from pathlib import Path

root = Path(__file__).resolve().parents[1]
outputs = root / "training-output"
report = json.loads((outputs / "starter-evaluation.json").read_text(encoding="utf-8"))
model = json.loads((outputs / "cyn-starter.json").read_text(encoding="utf-8"))
training = report["training"]

assert training["actual_gradient_seconds"] >= 3600, "Training must actually run one hour"
assert training["optimizer_steps"] > 1000, "Missing real optimizer updates"
assert model["schema"] == 1, "Model schema changed"
assert model["ready"] == [True, False, False, False, True], "Only verified model heads may be enabled"
assert len(model["weights"]) == 209 and all(math.isfinite(w) for w in model["weights"]), "Bad neural weights"
assert len(model.get("examples", [])) > 0, "Missing compatible starter examples"

for name in ("clarity", "bass"):
    new = report["trained_test"][name]["balanced_accuracy"]
    old = report["baseline_test"][name]["balanced_accuracy"]
    assert new + .015 >= old, name + " held-out evaluation regressed"

installer_file = root / "Installer/Cyn/Cyn.iss"
assert "AppVersion=2.0.14" in installer_file.read_text(), "Unexpected base version; refusing overwrite"

(root / "models/cyn-starter.json").write_text(json.dumps(model, indent=2) + "\n", encoding="utf-8")
(root / "models/starter-evaluation.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")

versioned = [
    "BUILD_INSTALLER.ps1",
    "Installer/Cyn/Cyn.iss",
    "fxsound/FxSound.jucer",
    "fxsound/FxSoundARM.jucer",
    "fxsound/JuceLibraryCode/JuceHeader.h",
    "fxsound/Project/FxSound_App.vcxproj",
    "fxsound/Project/resources.rc",
    "fxsound/ProjectARM/FxSound_App.vcxproj",
    "fxsound/ProjectARM/resources.rc",
    "fxsound/Source/GUI/FxController.cpp",
    "fxsound/Source/GUI/FxProView.cpp",
    "scripts/test_ai_hub.mjs",
]
for name in versioned:
    path = root / name
    before = path.read_text(encoding="utf-8-sig")
    if "2.0.14" not in before:
        raise RuntimeError("Version mismatch: " + name)
    after = before.replace("2.0.14", "2.0.15")
    after = after.replace("0x2000e00", "0x2000f00").replace("2,0,14,0", "2,0,15,0")
    path.write_text(after, encoding="utf-8")

readme = root / "README_CYN.md"
text = readme.read_text(encoding="utf-8-sig")
text = text.replace("Cyn-Setup-2.0.14-x64.exe", "Cyn-Setup-2.0.15-x64.exe")
readme.write_text(text + "\n## Cyn 2.0.15 — one-hour AI training\n"
                  "The experimental base model was trained using generated audio-feature data; "
                  "see models/MODEL_CARD.md and models/starter-evaluation.json for held-out synthetic tests.\n",
                  encoding="utf-8")

details = (outputs / "training-report.md").read_text(encoding="utf-8")
model_card = root / "models/MODEL_CARD.md"
model_card.write_text(model_card.read_text(encoding="utf-8-sig") + "\n" + details, encoding="utf-8")

(root / "RELEASE_NOTES_2.0.15.md").write_text(
    "# Cyn 2.0.15 — Validated one-hour base AI training\n\n"
    "New compatible starter weights selected after at least 3,600 seconds of gradient "
    "optimization and independent synthetic held-out evaluation. "
    "Only the supervised Bass and Clarity heads are enabled. "
    "No real music, dialogue recognition or user taste benchmark is claimed. "
    "Your private AI model and labels are preserved. "
    "In an existing installation, use Reset to starter to activate the updated base AI. "
    "This release retains Cyn 2.0.14's neural animation and all-audio UI support.\n\n" + details,
    encoding="utf-8")
print("Prepared Cyn 2.0.15 trained model package from validated artifact.")
