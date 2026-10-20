# APEX 10M — 15-hour experimental policy-head training

The training workflow `apex10m-15h-policy.yml` launches three sequential
**5-hour measured-gradient-compute** legs (18,000 seconds each). GitHub-hosted
jobs have a 6-hour limit, so this is split into separate jobs.

## Scope, honestly stated
- The 10,001,469-parameter APEX architecture consists of the earlier
  6,307,477-parameter APEX 6M base plus a new **3,693,992-parameter
  spatial-reasoning head**. This job trains **the new 3.69M head only**;
  the original base checkpoint is unchanged.
- Training uses real torch autograd, AdamW, checkpointed optimizer/RNG state,
  generated low-bass/kick/stereo/phase/headroom/dialogue proxy data, and a
  fixed-seed held-out **synthetic** validation set. It is not a pretrained
  large language reasoner. No human-labelled spatial positioning datasets,
  licensed music corpora or listening-test scores are present.
- Neural inference is **NOT** enabled in CYN Mobile/Windows by this workflow.
  The existing conservative deterministic DSP remains the listening baseline.
  A 15-hour run alone cannot establish stronger audio quality.

## Restart and recovery
- Each leg attempts to resume from the latest remotely persisted snapshot,
  otherwise from the previous successful job's artifact.
- Inside each leg, training failures restart the Python process **immediately**
  from its last atomically saved checkpoint, up to 12 consecutive failures.
- An external runner cancellation or an unavailable GitHub service cannot
  always be retried immediately within the same job; GitHub failed-job reruns
  or a separate watcher may be required.
- Progress snapshots are synced to a GitHub prerelease asset every ~12 minutes.
  The staging tag clearly marks checkpoints as **synthetic / unapproved**.
- Each finished stage exports a checkpoint and metrics to workflow artifacts;
  the next stage resumes its exact optimizer and RNG state.
- No trained checkpoints are merged into production or selected as a
  live AI automatically.

## Metrics and release gate
- Reports contain actual gradient updates, accumulated measured compute
  seconds, control/band proxy losses, and anti-phase width output.
- Final quality review still requires a separate real music/test dataset,
  stereo correlation and mono/bass preservation tests, transient preservation,
  anti-clipping tests, CPU/battery benchmarks and level-matched listening.
- Storage/network/build minutes depend on the GitHub plan and quota; the
  training workflow stops cleanly rather than hiding an unavailable runner.

## Local short trial

```bash
python -m pip install torch
python -m unittest tests/test_apex10m_training.py
python apex_engine/train_apex10m_policy.py \
    --target-total-seconds 30 \
    --max-steps 5 \
    --checkpoint /tmp/apex-policy.pt \
    --report /tmp/apex-metrics.json
```
