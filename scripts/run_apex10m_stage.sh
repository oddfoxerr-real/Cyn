#!/usr/bin/env bash
# Restart training immediately after recoverable process failures and resume
# durable model/optimizer/RNG progress across GitHub-hosted 5h legs.
set -euo pipefail
target_seconds="${1:?training cumulative seconds required}"
tag="apex10m-spatial-policy-15h-synthetic-v1"
folder="train-state"
mkdir -p "$folder" /tmp/cyn-policy-remote /tmp/cyn-policy-sync
checkpoint="$folder/apex10m-policy.pt"
report="$folder/apex10m-policy-metrics.json"

# Stage artifacts are downloaded by the workflow. A periodically uploaded
# release snapshot may be newer after an earlier process or runner crash.
if gh release download "$tag" --pattern apex10m-policy.pt \
    --dir /tmp/cyn-policy-remote --clobber >/dev/null 2>&1; then
    python - "$checkpoint" /tmp/cyn-policy-remote/apex10m-policy.pt <<'PY'
from pathlib import Path
import sys
import torch
local, remote = (Path(path) for path in sys.argv[1:])
try:
    existing = (torch.load(local,map_location="cpu",weights_only=True)
                if local.exists() else None)
    recovered = torch.load(remote,map_location="cpu",weights_only=True)
    if recovered.get("format") != "cyn-apex10m-spatial-curriculum-v1":
        raise ValueError("Unexpected checkpoint format")
    if existing is None or recovered["training_seconds"]>existing["training_seconds"]:
        import shutil
        shutil.copy2(remote,local)
        print(f"RESUMED remote checkpoint: {recovered['training_seconds']:.1f}s",flush=True)
except Exception as exc:
    if not local.exists(): raise
    print(f"Remote recovery skipped, using local stage artifact: {exc}",flush=True)
PY
fi

sync_checkpoint() {
    if [[ ! -s "$checkpoint" || ! -s "$report" ]]; then return 0; fi
    cp "$checkpoint" /tmp/cyn-policy-sync/apex10m-policy.pt
    cp "$report" /tmp/cyn-policy-sync/apex10m-policy-metrics.json
    if gh release upload "$tag" \
        /tmp/cyn-policy-sync/apex10m-policy.pt \
        /tmp/cyn-policy-sync/apex10m-policy-metrics.json \
        --clobber >/dev/null 2>&1; then
        echo "Remote recovery snapshot updated: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    else
        echo "::warning::Checkpoint sync temporarily failed; keeping local state and retrying."
    fi
}

ok=0
for attempt in $(seq 1 12); do
    echo "REAL TRAINING: target cumulative $target_seconds seconds; process attempt $attempt/12"
    python -u apex_engine/train_apex10m_policy.py \
        --target-total-seconds "$target_seconds" \
        --checkpoint "$checkpoint" --report "$report" \
        --checkpoint-interval 120 &
    pid=$!
    last_upload=$(date +%s)
    while kill -0 "$pid" 2>/dev/null; do
        sleep 10
        now=$(date +%s)
        if (( now - last_upload >= 720 )); then
            sync_checkpoint
            last_upload=$now
        fi
    done
    if wait "$pid"; then
        ok=1
        sync_checkpoint
        break
    fi
    echo "::warning::APEX training process failed. Saving recovery snapshot and retrying immediately."
    sync_checkpoint
    sleep 1
done
if (( ok == 0 )); then
    echo "::error::APEX training failed on all 12 attempts; checkpoint preserved."
    exit 1
fi
python - "$checkpoint" "$target_seconds" <<'PY'
import sys
import torch
p,expected=sys.argv[1],float(sys.argv[2])
d=torch.load(p,map_location="cpu",weights_only=True)
if d["training_seconds"] < expected:
    raise SystemExit(f"Incomplete measured compute: {d['training_seconds']} < {expected}")
if not d["synthetic_only"] or d["live_playback_approved"]:
    raise SystemExit("Invalid safe state labels")
print(f"Verified {d['training_seconds']:.2f}s of real training compute; {d['steps']} optimizer updates")
PY
