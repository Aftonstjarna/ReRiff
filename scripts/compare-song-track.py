#!/usr/bin/env python3
"""Compatibility argument adapter; comparisons run entirely in C++."""
import argparse
import os
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('native')
parser.add_argument('reference')
parser.add_argument('--output')
parser.add_argument('--tolerance', default='0.01')
args = parser.parse_args()
binary = Path(__file__).resolve().parents[1] / 'build/native-linux/reriff-reference'
command = [str(binary), 'compare', '--nodes', args.native, '--reference', args.reference]
command += ['--tolerance', args.tolerance]
if args.output:
    command += ['--output', args.output]
os.execv(str(binary), command)
