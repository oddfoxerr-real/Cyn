"""Compare a candidate neural starter against Cyn's packaged base.

No dataset is bundled here and no personal audio is sent anywhere. Promotion
requires independent human-labelled held-out *movie AND music* observations.
This utility cannot certify audio quality or speech recognition.
"""
import argparse
import json
import math
from pathlib import Path

INPUTS, HIDDEN, OUTPUTS, WEIGHTS = 11, 12, 5, 209


def read_json(file):
    with open(file, encoding="utf-8-sig") as handle:
        return json.load(handle)


def validate_model(data):
    if data.get("schema") != 1 or len(data.get("weights", [])) != WEIGHTS:
        raise ValueError("Expected a 209-parameter Cyn model")
    w = data["weights"]
    if any(not isinstance(x, (int, float)) or not math.isfinite(x)
           or abs(x) > 100 for x in w):
        raise ValueError("Invalid model weights")
    if data.get("ready") is None or len(data["ready"]) != OUTPUTS:
        raise ValueError("Expected five model readiness flags")
    return w


def infer(w, raw):
    if len(raw) != INPUTS or any(not isinstance(v, (float, int))
                                  or not math.isfinite(v) for v in raw):
        raise ValueError("Expected eleven finite extracted audio features")
    x = [(max(-12, min(0, raw[i])) + 6) / 6 if i < 8
         else max(0, min(4, raw[i])) / 2 - 1 if i == 8
         else max(0, min(2, raw[i])) - 1 if i == 9
         else (max(-8, min(0, raw[i])) + 4) / 4
         for i in range(INPUTS)]
    hidden = [math.tanh(w[132 + j] + sum(x[i] * w[j * INPUTS + i]
                                        for i in range(INPUTS)))
              for j in range(HIDDEN)]
    return [1 / (1 + math.exp(-max(-30, min(30, w[204 + o] +
                 sum(hidden[j] * w[144 + o * HIDDEN + j]
                     for j in range(HIDDEN)))))) for o in range(OUTPUTS)]


def validate_dataset(file, domain, heads):
    data = read_json(file)
    # The provenance field is an assertion by the dataset creator.
    # A reviewer still needs to verify these are truly unseen examples.
    if (data.get("domain") != domain or data.get("split") != "held_out" or
            data.get("provenance") != "independent_human_labels"):
        raise ValueError(domain + " evaluation requires independent held-out human labels")
    rows = data.get("examples", [])
    counts = {head: [0, 0] for head in heads}
    for row in rows:
        effect = row.get("effect")
        more = row.get("more")
        if effect not in counts or type(more) is not bool:
            raise ValueError("Unknown effect or binary label")
        if len(row.get("x", [])) != INPUTS:
            raise ValueError("Wrong audio feature dimension")
        counts[effect][int(more)] += 1
        infer([0.0] * WEIGHTS, row["x"])  # finite-value validation
    for head, (negative, positive) in counts.items():
        if min(negative, positive) < 25:
            raise ValueError(f"{domain} head {head} needs 25 independent labels per class")
    return rows


def evaluate(model, rows, heads):
    w = validate_model(model)
    result = {}
    for head in heads:
        examples = [r for r in rows if r["effect"] == head]
        predictions = [(infer(w, r["x"])[head], bool(r["more"]))
                       for r in examples]
        logloss = -sum(math.log(max(1e-8, min(1 - 1e-8,
                      p if truth else 1 - p))) for p, truth in predictions) / len(predictions)
        pos = [p for p, truth in predictions if truth]
        neg = [p for p, truth in predictions if not truth]
        balanced = 0.5 * (sum(p >= .5 for p in pos) / len(pos) +
                          sum(p < .5 for p in neg) / len(neg))
        result[str(head)] = dict(log_loss=logloss, balanced_accuracy=balanced)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate", required=True)
    parser.add_argument("--baseline", default="models/cyn-starter.json")
    parser.add_argument("--movie-eval", required=True)
    parser.add_argument("--music-eval", required=True)
    parser.add_argument("--report", required=True)
    parser.add_argument("--promote", action="store_true",
                        help="Only after ALL acceptance gates pass")
    args = parser.parse_args()
    current, candidate = read_json(args.baseline), read_json(args.candidate)
    validate_model(current)
    validate_model(candidate)
    # Avoid promoting a model which is untrained for either approved head.
    heads = [i for i in range(OUTPUTS) if current["ready"][i]]
    if not heads or any(not candidate["ready"][i] for i in heads):
        raise ValueError("Candidate must preserve all currently-ready effect heads")
    domain_results = {}
    approved = True
    for domain, filename in (("music", args.music_eval), ("movie", args.movie_eval)):
        rows = validate_dataset(filename, domain, heads)
        old, new = evaluate(current, rows, heads), evaluate(candidate, rows, heads)
        domain_results[domain] = {"baseline": old, "candidate": new}
        for head in heads:
            a, b = old[str(head)], new[str(head)]
            # Require improvement on both media types: no free movie wins
            # from degrading established music behavior.
            approved &= (b["log_loss"] <= a["log_loss"] * 0.95 and
                         b["balanced_accuracy"] + 0.01 >= a["balanced_accuracy"])
    report = {"candidate_accepted": bool(approved),
              "evaluation": domain_results,
              "limitations": "Perceptual labels are subjective; no speech recognition claim."}
    Path(args.report).parent.mkdir(parents=True, exist_ok=True)
    Path(args.report).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if args.promote:
        if not approved:
            raise SystemExit("Rejected: candidate does not outperform the starter in both domains")
        target = Path(args.baseline)
        # Preserve shipped starter examples, which carry independent semantics;
        # an explicit trainer should regenerate them when changing heads.
        promoted = dict(candidate)
        if "examples" not in promoted:
            promoted["examples"] = current.get("examples", [])
        target.write_text(json.dumps(promoted, indent=2) + "\n", encoding="utf-8")
        print("Approved candidate written to", target)
    else:
        print("Candidate accepted:", bool(approved),
              "(dry run; run --promote only after human data provenance review)")


if __name__ == "__main__":
    main()
