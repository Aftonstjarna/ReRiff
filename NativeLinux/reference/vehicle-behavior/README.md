# Original vehicle behavior fixture

`original-behavior.tsv` contains numeric inputs and expected outputs captured
from the original executable's unchanged methods. It contains no game assets
or executable code. `behavior-provenance.json` identifies the executable,
configuration, capture host and exact TSV bytes.

The expected outputs were produced by `OriginalVehicleCapture.cs` under the
Microsoft x86 CLR, not by the C++ implementation under test. The row layouts
are defined by that writer and `test_vehicle_behavior.cpp`. Every test group
must be complete; the reader also checks the capture hash before comparing.

See [recovery details and regeneration](../../PHYSICS-RECOVERY.md).
