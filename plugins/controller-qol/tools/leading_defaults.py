"""Render the reviewed semantic baseline ledger; never infer values from game data."""
import json
from pathlib import Path


def render_leading_defaults() -> str:
    ledger = json.loads((Path(__file__).resolve().parents[1] / 'docs/leading-baselines.json').read_text())
    records = ledger['records']
    assert len(records) == 240 and len({row['id'] for row in records}) == 240
    lines = [
        '# Reviewed Reimagined projectile estimates; discovery does not calculate travel times.',
        '# Milliseconds per world tile (0..200). Zero retains current-position aim.',
        '# Leading requires an enabled skill using snap targeting; these values do not enable it.',
        '# New estimates need gameplay testing; see docs/PROJECTILE-LEADING.md.',
        '[aim.leading]',
    ]
    previous = None
    for row in records:
        assert 0 <= row['ms_per_tile'] <= 200
        if row['class_name'] != previous:
            lines += ['', '# ' + row['class_name'].title()]
            previous = row['class_name']
        label = row['name']
        if label != row['catalog_name']:
            label += ' (catalog: ' + row['catalog_name'] + ')'
        lines.append(f'"{row["id"]}" = {row["ms_per_tile"]} # {label}')
    lines += ['', '# Tested ice limits; other skills retain 600 ms / three tiles.', '[aim.leading_max_ms]']
    lines += [f'"{key}" = {value}' for key, value in ledger['max_ms'].items()]
    lines += ['', '[aim.leading_max_tiles]']
    lines += [f'"{key}" = {value}' for key, value in ledger['max_tiles'].items()]
    return '\n'.join(lines) + '\n'
