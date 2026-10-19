#!/usr/bin/env python3
"""Bundle the same checksum-pinned ONNX weights as desktop CYN. No training."""
import argparse, hashlib, pathlib, urllib.request, zipfile, tempfile, shutil
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--offline', action='store_true', help='Verify already downloaded models without network access')
args = parser.parse_args()
ROOT = pathlib.Path(__file__).resolve().parents[1]
DEST = ROOT / 'app/src/main/assets/models'
DEST.mkdir(parents=True, exist_ok=True)
YAM = 'd3c173d26202dd94bf5532795a0f6e3090610a70adb2a773faa0ee635ec6acbe'
APEX = '89f1f3eecd7fc488afe770e8bb64666bad98c6dad6cf4ea4d03e713293c6365f'
def digest(path):
    with path.open('rb') as source: return hashlib.file_digest(source, 'sha256').hexdigest()
def download(url, target):
    with urllib.request.urlopen(url, timeout=90) as source, target.open('wb') as output: shutil.copyfileobj(source, output)
def ready(name, expected):
    path = DEST / name
    return path.is_file() and digest(path) == expected
with tempfile.TemporaryDirectory() as folder:
    temporary = pathlib.Path(folder)
    if not ready('apex6m-225.onnx', APEX):
        if args.offline: raise RuntimeError('APEX is missing or damaged. Run prepare_models.py once online.')
        model = temporary / 'apex.onnx'
        download('https://github.com/oddfoxerr-real/Cyn/releases/download/apex-6m-v1.0.0/apex6m-225.onnx', model)
        if digest(model) != APEX: raise RuntimeError('APEX model hash mismatch')
        shutil.copyfile(model, DEST / 'apex6m-225.onnx')
    if not ready('yamnet-16k-0p975s.onnx', YAM):
        if args.offline: raise RuntimeError('YAMNet is missing or damaged. Run prepare_models.py once online.')
        archive = temporary / 'desktop.zip'
        download('https://github.com/oddfoxerr-real/Cyn/releases/download/v2.0.21/Cyn-Windows-x64.zip', archive)
        if digest(archive) != '4307d0a5568748d04955bae0ead47a5e0e6c937332c6a7927dbec3d01968f0f2': raise RuntimeError('Desktop archive hash mismatch')
        with zipfile.ZipFile(archive) as source:
            members = [item for item in source.namelist() if pathlib.PurePosixPath(item).name == 'yamnet-16k-0p975s.onnx']
            if len(members) != 1: raise RuntimeError('Ambiguous YAMNet archive')
            model = temporary / 'yamnet.onnx'
            with source.open(members[0]) as stream, model.open('wb') as output: shutil.copyfileobj(stream, output)
        if digest(model) != YAM: raise RuntimeError('YAMNet model hash mismatch')
        shutil.copyfile(model, DEST / 'yamnet-16k-0p975s.onnx')
print('Verified desktop YAMNet and full APEX 6,307,477-parameter weights.')
