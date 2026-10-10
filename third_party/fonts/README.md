# Manrope

The upstream variable font is copied unchanged from the pinned Google Fonts URL
in `licenses/sources.json`. Copyright and SIL OFL 1.1 are in
`licenses/Manrope-OFL.txt`. No Reserved Font Name is declared there.

On 2026-10-10, fontTools 4.66.1 generated static instances at weights 400, 600 and
700 with `instantiateVariableFont(font, {'wght': weight}, updateFontNames=True)`.
They are the Regular, Semibold and Bold files in `fxsound/Fonts`; they remain OFL.
The static fonts work with JUCE 6's embedded typeface loader. Their names are
truthful Manrope names; the original Gilroy files and byte arrays are removed.

To reproduce the static fonts, install `fonttools==4.66.1` into a Python virtual
environment and run `python scripts/generate_manrope.py` from the repository.
Then run `python scripts/embed_cyn_fonts.py` to refresh the compiled resources.
Normal Windows builds use the checked-in static fonts and generated C++ and do
not require Python or fontTools.
