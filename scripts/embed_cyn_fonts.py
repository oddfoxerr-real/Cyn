"""Cyn modification, 2026-10-10: refresh only the three embedded UI font resources."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
cpp = root / 'fxsound/JuceLibraryCode/BinaryData.cpp'
header = root / 'fxsound/JuceLibraryCode/BinaryData.h'
text, htext = cpp.read_text(encoding='utf-8'), header.read_text(encoding='utf-8')
for index, name in enumerate(['Bold', 'Regular', 'Semibold']):
    data = (root / f'fxsound/Fonts/Manrope-{name}.ttf').read_bytes()
    symbol = f'Manrope{name}_ttf'
    values = list(data) + [0, 0]
    body = '\n'.join(','.join(str(b) for b in values[i:i+32]) + ','
                     for i in range(0, len(values), 32))
    pattern = (rf'static const unsigned char temp_binary_data_{index}\[\] =.*?'
               rf'const char\* {symbol} = \(const char\*\) temp_binary_data_{index};')
    replacement = (f'static const unsigned char temp_binary_data_{index}[] =\n{{\n{body}\n}};\n\n'
                   f'const char* {symbol} = (const char*) temp_binary_data_{index};')
    text, count = re.subn(pattern, lambda _: replacement, text, flags=re.S)
    assert count == 1, f'Missing or duplicate resource: {symbol}'
    resource_hash = 0
    for char in symbol:
        resource_hash = (31 * resource_hash + ord(char)) & 0xffffffff
    text, count = re.subn(rf'case 0x[0-9a-f]+:  numBytes = \d+; return {symbol};',
                         f'case 0x{resource_hash:08x}:  numBytes = {len(data)}; return {symbol};', text)
    assert count == 1
    htext, count = re.subn(rf'({symbol}Size = )\d+', rf'\g<1>{len(data)}', htext)
    assert count == 1
cpp.write_text(text, encoding='utf-8')
header.write_text(htext, encoding='utf-8')
