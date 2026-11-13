"""Optional Windows Flask server; use the launcher's private Python runtime."""
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
from local_server import main
if __name__ == '__main__':
    raise SystemExit(main())
