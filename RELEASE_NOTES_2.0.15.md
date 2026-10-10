# Cyn 2.0.15 — Validated one-hour base AI training

New compatible starter weights selected after at least 3,600 seconds of gradient optimization and independent synthetic held-out evaluation. Only the supervised Bass and Clarity heads are enabled. No real music, dialogue recognition or user taste benchmark is claimed. Your private AI model and labels are preserved. In an existing installation, use Reset to starter to activate the updated base AI. This release retains Cyn 2.0.14's neural animation and all-audio UI support.

## Hour-trained experimental Cyn base
Gradient optimization: **3600.0s**; **33,175,712 gradient updates**; 49 validated checkpoints.

| Synthetic held-out metric | Original | Hour-trained |
|---|---:|---:|
| Clarity balanced accuracy | 98.63% | 98.61% |
| Clarity log loss | 0.049275 | 0.032193 |
| Bass balanced accuracy | 100.00% | 100.00% |
| Bass log loss | 0.009301 | 0.000150 |

**Limitations:** All examples were synthetically generated. No real music, human preferences, vocal detection or source separation was evaluated. Only Bass and Clarity have starter labels.
Existing personal learning remains intact. Select **Reset to starter** in Smart listening to activate the new bundled base model on an existing installation.
