"""Package owned font textures as an additional native game archive."""
import argparse
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'packaging'))
from native_archive import build_volume

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--input', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--volume', default='vfs1')
args = parser.parse_args()
for path in build_volume(args.input, args.output, args.volume):
    print(path)
