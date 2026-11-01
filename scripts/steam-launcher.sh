#!/usr/bin/env bash
# Preserve Steam's Proton/prefix command and forward Game.exe arguments explicitly.
set -euo pipefail
launcher_flags=()
while :; do
    case ${1:-} in
        --skip-launcher|--launcher-skip|--skip) launcher_flags+=(--skip-launcher); shift ;;
        --show-launcher|--test-auto-update|--no-auto-update) launcher_flags+=("$1"); shift ;;
        *) break ;;
    esac
done
if [[ ${1:-} == -- ]]; then shift; fi
command_args=("$@")
for i in "${!command_args[@]}"; do
    case "${command_args[i]}" in
        */Game.exe|*\\Game.exe|Game.exe)
            game=${command_args[i]}
            game_args=()
            # Steam also permits launch options after %command%. These unique
            # launcher flags must never reach the original game's argument parser.
            for argument in "${command_args[@]:i+1}"; do
                case "$argument" in
                    --skip-launcher|--launcher-skip|--show-launcher|--test-auto-update|--no-auto-update) launcher_flags+=("$argument");;
                    *) game_args+=("$argument");;
                esac
            done
            exec "${command_args[@]:0:i}" "${game%Game.exe}ReRiff-Launcher.exe" "${launcher_flags[@]}" -- "$game" "${game_args[@]}"
            ;;
    esac
done
echo 'Supply the Steam game command after --, using %command% in Steam launch options.' >&2
exit 2
