"""Cyn modification, 2026-10-10: generate OFL static fonts with fontTools 4.66.1."""
from pathlib import Path
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont

root = Path(__file__).resolve().parents[1]
for name, weight in [('Regular', 400), ('Semibold', 600), ('Bold', 700)]:
    font = TTFont(root / 'third_party/fonts/Manrope-variable.ttf')
    result = instantiateVariableFont(font, {'wght': weight}, updateFontNames=True)
    result.save(root / f'fxsound/Fonts/Manrope-{name}.ttf')
