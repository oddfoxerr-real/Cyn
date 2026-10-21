#!/usr/bin/env python3
"""Bundle the same checksum-pinned ONNX weights as desktop CYN. No training."""
import argparse, hashlib, json, pathlib, urllib.request, zipfile, tempfile, shutil
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

# Partial APEX10M head: this is explicitly an experimental research *submode*
# and never replaces the full APEX 6M model or YAMNet.
PARTIAL_BASE = ('https://github.com/oddfoxerr-real/Cyn/releases/download/'
                'apex10m-partial-policy-ab-v0.1.0/')
manifest_target = DEST / 'apex10m-partial-manifest.json'
model_target = DEST / 'apex10m-partial-policy.onnx'
if args.offline:
    if not manifest_target.is_file():
        raise RuntimeError('Partial APEX10M manifest missing')
    manifest = json.loads(manifest_target.read_text(encoding='utf-8'))
else:
    with tempfile.TemporaryDirectory() as folder:
        temp = pathlib.Path(folder)
        manifest_file = temp / 'manifest.json'
        download(PARTIAL_BASE + 'CYN-APEX10M-PARTIAL-manifest.json', manifest_file)
        manifest = json.loads(manifest_file.read_text(encoding='utf-8'))
        # Do not fetch a mutable latest model while training runs.
        # Verify the immutable 37.33% snapshot explicitly.
        if (manifest.get('name') != 'APEX 10M partial spatial policy head (synthetic-only)'
                or manifest.get('trained_on_real_music') is not False
                or manifest.get('neural_playback_approved') is not False
                or not 37.3 <= float(manifest.get('frozen_percent_of_15h', 0)) <= 37.4
                or manifest.get('trained_spatial_head_parameters') != 3693992
                or manifest.get('total_10m_architecture_parameters') != 10001469):
            raise RuntimeError('Untrusted or unsupported APEX 10M frozen-preview manifest')
        expected = manifest.get('onnx_sha256', '')
        if len(expected) != 64 or any(x not in '0123456789abcdef' for x in expected.lower()):
            raise RuntimeError('Missing partial model SHA-256')
        if not ready('apex10m-partial-policy.onnx', expected):
            candidate = temp / 'partial.onnx'
            download(PARTIAL_BASE + 'CYN-APEX10M-PARTIAL-Spatial-Head.onnx', candidate)
            if digest(candidate) != expected:
                raise RuntimeError('APEX 10M partial model SHA-256 mismatch')
            shutil.copyfile(candidate, model_target)
        manifest_target.write_text(json.dumps(manifest, sort_keys=True) + '\\n', encoding='utf-8')
expected = manifest.get('onnx_sha256', '')
if not ready('apex10m-partial-policy.onnx', expected):
    raise RuntimeError('Frozen APEX 10M ONNX file missing or corrupt')
print('Verified isolated 37.33% synthetic APEX 10M policy head; production playback remains opt-in/off.')
