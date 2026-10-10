"""Regression checks for embedded font bytes and distributable legal material."""
import hashlib
import json
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]

class LicensingAssets(unittest.TestCase):
    def test_embedded_fonts_match_distributed_files_and_resource_table(self):
        cpp = (ROOT / 'fxsound/JuceLibraryCode/BinaryData.cpp').read_text(encoding='utf-8')
        header = (ROOT / 'fxsound/JuceLibraryCode/BinaryData.h').read_text(encoding='utf-8')
        hashes = re.findall(r'case (0x[0-9a-f]+):', cpp)
        self.assertEqual(len(hashes), len(set(hashes)), 'Resource name hash collision')
        for index, style in enumerate(['Bold', 'Regular', 'Semibold']):
            raw = (ROOT / f'fxsound/Fonts/Manrope-{style}.ttf').read_bytes()
            body = re.search(rf'temp_binary_data_{index}\[\] =\s*\{{(.*?)\}};', cpp, re.S).group(1)
            self.assertEqual(bytes(int(n) for n in re.findall(r'\d+', body)), raw + b'\0\0')
            self.assertIn(f'Manrope{style}_ttfSize = {len(raw)};', header)
            self.assertIn(f'numBytes = {len(raw)}; return Manrope{style}_ttf;', cpp)

    def test_no_old_font_assets_or_build_references(self):
        self.assertFalse(list((ROOT / 'fxsound/Fonts').glob('Gilroy*')))
        for path in (ROOT / 'fxsound').rglob('*'):
            if path.is_file() and path.suffix in ['.jucer', '.vcxproj', '.filters', '.h', '.cpp']:
                if 'Project/x64/' in path.as_posix():
                    continue
                self.assertNotIn('Gilroy', path.read_text(encoding='utf-8-sig'), str(path))

    def test_font_copyright_inventory_hashes(self):
        entries = json.loads((ROOT / 'licenses/font-inventory.json').read_text(encoding='utf-8'))
        for entry in entries:
            self.assertEqual(hashlib.sha256((ROOT / entry['path']).read_bytes()).hexdigest(), entry['sha256'])
            self.assertTrue(entry['names']['0'], entry['path'])

    def test_core_license_bundle(self):
        for name in ['Manrope-OFL.txt', 'NotoSansCJK-OFL.txt', 'NotoSansThai-OFL.txt',
                     'NotoSansArabic-OFL.txt', 'MontserratAlternates-OFL.txt',
                     'IBMPlexSansArabic-OFL.txt', 'GPL-3.0.txt', 'JUCE-ISC.txt',
                     'libpng-LICENSE.txt', 'IJG-JPEG-README.txt', 'zlib-NOTICE.txt']:
            self.assertGreater((ROOT / 'licenses' / name).stat().st_size, 200)
        self.assertEqual(len(list((ROOT / 'licenses/mcp').glob('*LICENSE.txt'))), 9)

    def test_every_language_font_has_a_packaging_source(self):
        theme = (ROOT / 'fxsound/Source/GUI/FxTheme.cpp').read_text(encoding='utf-8')
        for name in re.findall(r'loadTypeface\("([^"]+)"\)', theme):
            self.assertTrue(any((ROOT / folder / name).is_file()
                                for folder in ['fxsound/Fonts', 'Installer/Resources/Fonts']), name)

if __name__ == '__main__':
    unittest.main()
