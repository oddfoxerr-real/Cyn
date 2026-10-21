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
                'apex10m-experimental-rc-v1/')
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
        download(PARTIAL_BASE + 'CYN-APEX10M-RC1-manifest.json', manifest_file)
        manifest = json.loads(manifest_file.read_text(encoding='utf-8'))
        # Do not fetch a mutable latest model while training runs.
        # Verify the immutable release candidate with at least ten hours trained.
        if (manifest.get('name') != 'APEX 10M experimental spatial-policy head (synthetic-only)'
                or manifest.get('trained_on_real_music') is not False
                or manifest.get('neural_playback_approved') is not False
                or not 66.66 <= float(manifest.get('frozen_percent_of_15h', 0)) <= 100.0
                or manifest.get('trained_spatial_head_parameters') != 3693992
                or manifest.get('total_10m_architecture_parameters') != 10001469):
            raise RuntimeError('Untrusted or unsupported APEX 10M frozen-preview manifest')
        expected = manifest.get('onnx_sha256', '')
        if len(expected) != 64 or any(x not in '0123456789abcdef' for x in expected.lower()):
            raise RuntimeError('Missing partial model SHA-256')
        if not ready('apex10m-partial-policy.onnx', expected):
            candidate = temp / 'partial.onnx'
            download(PARTIAL_BASE + 'CYN-APEX10M-RC1-Spatial-Head.onnx', candidate)
            if digest(candidate) != expected:
                raise RuntimeError('APEX 10M partial model SHA-256 mismatch')
            shutil.copyfile(candidate, model_target)
        manifest_target.write_text(json.dumps(manifest, sort_keys=True), encoding='utf-8')
expected = manifest.get('onnx_sha256', '')
if not ready('apex10m-partial-policy.onnx', expected):
    raise RuntimeError('Frozen APEX 10M ONNX file missing or corrupt')
print(f"Verified APEX10M experimental RC trained {manifest['frozen_percent_of_15h']}% ({manifest['real_optimizer_updates']} steps); live inference is guarded.")

# AUDONIC is a separate experimental neural AI. Verify its learned ONNX
# weights against a pinned release before placing them in the APK.
AUDONIC_BASE = ('https://github.com/oddfoxerr-real/Cyn/releases/download/'
                'audonic-neural-student-v0.1.0/')
audonic_manifest_target = DEST / 'audonic-manifest.json'
audonic_model_target = DEST / 'audonic-student.onnx'
if args.offline:
    if not audonic_manifest_target.exists():
        raise RuntimeError('AUDONIC manifest missing; download official trained student first')
    audonic_meta = json.loads(audonic_manifest_target.read_text(encoding='utf-8'))
else:
    with tempfile.TemporaryDirectory() as folder:
        manifest_path = pathlib.Path(folder) / 'audonic-meta.json'
        download(AUDONIC_BASE + 'audonic-manifest.json', manifest_path)
        audonic_meta = json.loads(manifest_path.read_text(encoding='utf-8'))
        if (audonic_meta.get('model') != 'AUDONIC neural creative-FX student'
                or audonic_meta.get('gradient_updates', 0) < 100
                or audonic_meta.get('apex10m_teacher_distillation') is not True
                or audonic_meta.get('three_b_teacher_trained') is not False
                or audonic_meta.get('real_music_quality_verified') is not False):
            raise RuntimeError('Untrusted AUDONIC neural release metadata')
        audonic_sha = audonic_meta.get('onnx_sha256','')
        if len(audonic_sha) != 64 or any(c not in '0123456789abcdef' for c in audonic_sha.lower()):
            raise RuntimeError('Missing AUDONIC SHA-256')
        if not ready('audonic-student.onnx',audonic_sha):
            candidate = pathlib.Path(folder) / 'audonic-student.onnx'
            download(AUDONIC_BASE + 'audonic-student.onnx',candidate)
            if digest(candidate)!=audonic_sha:
                raise RuntimeError('Trained AUDONIC model hash mismatch')
            shutil.copyfile(candidate,audonic_model_target)
        audonic_manifest_target.write_text(json.dumps(audonic_meta,sort_keys=True),encoding='utf-8')
if not ready('audonic-student.onnx', audonic_meta.get('onnx_sha256','')):
    raise RuntimeError('AUDONIC model missing, corrupted, or unsigned by release manifest')
print('Verified trained AUDONIC creative neural student and SHA-256.')
