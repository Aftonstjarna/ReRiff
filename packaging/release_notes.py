"""Select one version's notes from the repository changelog."""
import argparse
from pathlib import Path
import re


def check_in_game_summary(notes, version):
    """Reject characters the game's native font cannot render in News."""
    heading = re.search(r'^## In-game summary\s*$', notes, re.MULTILINE)
    if not heading:
        return
    rest = notes[heading.end():]
    end = re.search(r'^## ', rest, re.MULTILINE)
    summary = rest[:end.start()] if end else rest
    bad = sorted({ch for ch in summary if ord(ch) > 127})
    if bad:
        listing = ', '.join(f'U+{ord(ch):04X} {ch}' for ch in bad)
        raise ValueError(f'{version} in-game summary uses characters the game font cannot render: {listing}; use ASCII punctuation')


def release_notes(source, version):
    version = version.removeprefix('v')
    headings = list(re.finditer(r'^# ReRiff (\d+\.\d+\.\d+(?:-(?:beta\.)?[1-9]\d*)?)\b[^\n]*$', source, re.MULTILINE))
    matches = [i for i, heading in enumerate(headings) if heading.group(1) == version]
    if len(matches) != 1:
        raise ValueError('Expected exactly one notes section for '+version)
    index = matches[0]
    start = headings[index].start()
    end = headings[index+1].start() if index+1 < len(headings) else len(source)
    notes = source[start:end].strip()+'\n'
    check_in_game_summary(notes, version)
    return notes


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--version', required=True)
    parser.add_argument('--source', type=Path, default=Path(__file__).resolve().parents[1]/'RELEASE-NOTES.md')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    notes = release_notes(args.source.read_text(encoding='utf-8'), args.version)
    if args.output:
        args.output.write_text(notes, encoding='utf-8')
    else:
        print(notes, end='')
