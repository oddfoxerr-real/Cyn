"""Offline spatial DSP preview. Not APEX 10M neural model inference."""
from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
import soundfile as sf

from apex10m_spatial import analyze_stereo, spatial_preserve


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path, help='Existing WAV file')
    parser.add_argument('output', type=Path, help='New output WAV file')
    parser.add_argument('--strength', type=float, default=0.4,
                        help='Stereo ambience enhancement strength, 0..1 (default 0.4)')
    args = parser.parse_args()
    if args.input.resolve() == args.output.resolve():
        parser.error('Output must differ from input to protect original audio')
    if not np.isfinite(args.strength) or not 0 <= args.strength <= 1:
        parser.error('--strength must be a finite number in [0,1]')
    audio, sample_rate = sf.read(args.input, dtype='float32', always_2d=True)
    if audio.shape[1] != 2:
        parser.error('This preview requires stereo WAV input')
    if len(audio) / sample_rate > 600:
        parser.error('Preview input must be at most ten minutes')
    before = analyze_stereo(audio, sample_rate)
    output_audio = spatial_preserve(audio, sample_rate, args.strength)
    after = analyze_stereo(output_audio, sample_rate)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    sf.write(args.output, output_audio, sample_rate, subtype='PCM_24')
    print(f'Wrote {args.output} | stereo side/mid {before["side_to_mid"]:.3f} -> '
          f'{after["side_to_mid"]:.3f} | {sample_rate} Hz')
    print('NOTE: deterministic DSP test only; untrained neural reasoning is not active.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
