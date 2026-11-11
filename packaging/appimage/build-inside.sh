#!/usr/bin/env bash
set -euo pipefail
umask 022
[[ $(uname -m) == x86_64 ]]
cd /src
export APPDIR=/build/ReRiff.AppDir OUTPATH=/out OUTNAME=ReRiff-Server-x86_64.AppImage
export DESKTOP=/build/reriff.desktop ICON=/src/packaging/appimage/reriff.svg
export DEPLOY_PYTHON=1 DEPLOY_LOCALE=0 STRACE_MODE=0
export VERSION
VERSION=$(cat VERSION)
mkdir -p "$APPDIR"
install -m755 packaging/appimage/AppRun.sh "$APPDIR/AppRun.sh"
cat > "$DESKTOP" <<'DESKTOP'
[Desktop Entry]
Type=Application
Name=ReRiff Local Server
Exec=python3
Icon=reriff
Categories=Game;
Terminal=false
DESKTOP
pythonlib=$(python -c 'import sysconfig; print(sysconfig.get_path("stdlib"))')
# Include dlopen modules (ssl, ctypes, sqlite, compression) and their ELF deps.
/opt/quick-sharun /usr/bin/python3 "$pythonlib"/lib-dynload/*.so
mkdir -p "$APPDIR/lib/reriff" "$APPDIR/share/reriff/licenses"
cp LinuxRuntime/entry.py LinuxRuntime/local_server.py LinuxRuntime/serve.py "$APPDIR/lib/reriff/"
cp LICENSE "$APPDIR/share/reriff/licenses/ReRiff.txt"
cp /etc/ssl/certs/ca-certificates.crt "$APPDIR/share/reriff/ca-certificates.crt"
cp -a /usr/share/licenses/. "$APPDIR/share/reriff/licenses/"
pacman -Q > "$APPDIR/share/reriff/build-packages.txt"
python -m pip install --break-system-packages --no-cache-dir -r docker/requirements.txt
# Redeploy Python so the optional server gets Flask/Waitress and native wheels.
site=$(python -c 'import sysconfig; print(sysconfig.get_path("purelib"))')
mapfile -t extensions < <(find "$site" -name '*.so' -type f)
/opt/quick-sharun /usr/bin/python3 "${extensions[@]}"
mkdir -p "$APPDIR/lib/reriff/server" "$APPDIR/lib/reriff/docker"
cp -a vendor/riffapi-local-extended/. "$APPDIR/lib/reriff/server/"
cp VERSION "$APPDIR/lib/reriff/server/VERSION"
cp scripts/trace-server.py "$APPDIR/lib/reriff/trace-server.py"
cp docker/server.py "$APPDIR/lib/reriff/docker/server.py"
g++ -std=c++17 -O2 -shared -fPIC -static-libstdc++ -static-libgcc ServerSync/catalog.cpp -o /build/libreriff-catalog.so
g++ -std=c++17 -O2 -shared -fPIC -static-libstdc++ -static-libgcc ServerSync/news.cpp -o /build/libreriff-news.so
/opt/quick-sharun /build/libreriff-catalog.so /build/libreriff-news.so
cp /build/libreriff-{catalog,news}.so "$APPDIR/lib/reriff/server/"
cp ServerSync/third_party/LICENSE.MIT "$APPDIR/share/reriff/licenses/nlohmann-json.txt"
python -m pip list --format=json > "$APPDIR/share/reriff/python-packages.json"
"$APPDIR/AppRun" --self-test
export OUTNAME=ReRiff-Server-x86_64.AppImage
/opt/quick-sharun --make-appimage
cd /out
sha256sum ReRiff-Server-x86_64.AppImage > APPIMAGE-SHA256SUMS
