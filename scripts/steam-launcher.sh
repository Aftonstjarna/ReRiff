#!/usr/bin/env bash
# Use Steam's selected Proton and prefix, replacing only the game executable.
set -euo pipefail
launcher_flags=()
while :; do
    case ${1:-} in
        --skip-launcher|--launcher-skip|--skip) launcher_flags+=(--skip-launcher); shift ;;
        --show-launcher) launcher_flags+=(--show-launcher); shift ;;
        *) break ;;
    esac
done
if [[ ${1:-} == -- ]]; then shift; fi
command_args=("$@")
found=false
for i in "${!command_args[@]}"; do
    case "${command_args[i]}" in
        */Game.exe|*\\Game.exe|Game.exe)
            command_args[i]="${command_args[i]%Game.exe}ReRiff-Launcher.exe"
            found=true
            ;;
    esac
done
if [[ $found != true ]]; then
    echo 'Supply the Steam game command after --, using %command% in Steam launch options.' >&2
    exit 2
fi
exec "${command_args[@]}" "${launcher_flags[@]}"
