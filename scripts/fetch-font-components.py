#!/usr/bin/env python3
"""Download the pinned ReRiff-Fonts assets; do not bake textures per client release."""
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'packaging'))
from font_assets import fetch_all

if __name__ == '__main__':
    fetch_all()
