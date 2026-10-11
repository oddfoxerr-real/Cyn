"""Fail publication while documented distribution questions remain unresolved."""
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
review = json.loads((root / 'licenses/release-review.json').read_text(encoding='utf-8'))
pending = [item for item in review['items'] if item['status'] != 'resolved' or not item.get('evidence')]
if pending:
    for item in pending:
        print(f"UNRESOLVED {item['id']}: {item['question']}")
    raise SystemExit('Publication held. Resolve and record evidence; see docs/LICENSING_AUDIT.md.')
print('All recorded distribution questions have supporting evidence.')
