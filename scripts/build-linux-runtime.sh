#!/usr/bin/env bash
# Build dependencies exist only in the build container, never on players' hosts.
set -euo pipefail
project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)
[[ $(uname -m) == x86_64 ]] || { echo 'This recipe targets x86_64.' >&2; exit 1; }
engine=${RERIFF_BUILD_ENGINE:-podman}
command -v "$engine" >/dev/null
mkdir -p "$project_dir/build/linux-runtime"
staging=$(mktemp -d "$project_dir/build/runtime-source.XXXXXXXX")
trap 'rm -rf -- "$staging"' EXIT
# Explicit inputs exclude local game data, Git metadata and household env files.
mkdir -p "$staging/packaging" "$staging/scripts" "$staging/docker" "$staging/vendor"
cp -a "$project_dir/LinuxRuntime" "$project_dir/ServerSync" "$project_dir/LICENSE" "$project_dir/VERSION" "$staging/"
cp -a "$project_dir/packaging/appimage" "$project_dir/packaging/requirements.txt" "$staging/packaging/"
cp "$project_dir/scripts/trace-server.py" "$staging/scripts/"
cp "$project_dir/docker/server.py" "$project_dir/docker/requirements.txt" "$staging/docker/"
cp -a "$project_dir/vendor/riffapi-local-extended" "$staging/vendor/"
find "$staging" -type d -name __pycache__ -prune -exec rm -rf -- {} +
"$engine" build -t localhost/reriff-appimage-builder -f "$project_dir/packaging/appimage/Containerfile" "$project_dir/packaging/appimage"
"$engine" run --rm --network=host -v "$staging:/src:ro,Z" \
    -v "$project_dir/build/linux-runtime:/out:rw,Z" localhost/reriff-appimage-builder
