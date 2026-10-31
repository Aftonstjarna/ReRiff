# Third-party source

`nlohmann/json.hpp` is the unmodified single-header distribution of
[nlohmann/json v3.11.3](https://github.com/nlohmann/json/tree/v3.11.3).
Its MIT license is retained as `nlohmann/LICENSE.MIT`.

SDL headers are system-provided or fetched separately into ignored build output
by `scripts/setup-native-sdl.sh`, which verifies the pinned archive checksum and
retains SDL's license. Other libraries are system dependencies, not vendored.
