#!/bin/bash
# Runs the game with the screenshot harness, see ScreenshotHarness.h.
#
#   tools/screenshots/run.sh build
#       Configures and builds cmake-build-screenshots (Development mode, the
#       harness built in, Release optimisations).
#
#   tools/screenshots/run.sh scenario tools/screenshots/scenarios/foo.shots
#       Plays the commands of the file in a fresh run folder and leaves the
#       screenshots in cmake-build-screenshots/out/<name>/.
#
#   tools/screenshots/run.sh live <name>
#       Starts the game in the background and waits for commands in the FIFO
#       cmake-build-screenshots/runs/<name>/commands, e.g.
#         echo "click 100 20" > cmake-build-screenshots/runs/<name>/commands
#       Progress is in .../runs/<name>/out/harness.log. Send "quit" to end it.
#
#   tools/screenshots/run.sh send <name> "click 100 20" "wait 10" "shot a"
#       Feeds commands to a live session and waits until all of them ran, then
#       prints what the harness logged. Shots land in .../runs/<name>/out/.
#
# Every run gets its own copy of assets/ and scripts/, so nothing the editors
# write ends up in the sources. The volume of the game is set to 0.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD="$ROOT/cmake-build-screenshots"
BINARY="$BUILD/PingoEngine"

prepare() {
    local name="$1"
    local work="$BUILD/runs/$name"

    rm -rf "$work"
    mkdir -p "$work"
    cp -R "$ROOT/assets" "$work/assets"
    cp -R "$ROOT/scripts" "$work/scripts"
    python3 "$ROOT/tools/screenshots/baseline.py" "$work/assets"
    # Hitboxes are on by default, the documentation shows the plain scenes
    echo '{"showHitboxes": false}' > "$work/settings.json"
    echo "$work"
}

case "${1:-}" in
    build)
        cmake -S "$ROOT" -B "$BUILD" -DPINGO_BUILD_MODE=Development \
            -DPINGO_SCREENSHOT_HARNESS=ON -DCMAKE_BUILD_TYPE=Release
        cmake --build "$BUILD" -j8
        ;;
    scenario)
        file="$(cd "$(dirname "$2")" && pwd)/$(basename "$2")"
        name="$(basename "$2" .shots)"
        work="$(prepare "$name")"
        out="$BUILD/out/$name"
        rm -rf "$out"
        mkdir -p "$out"
        (cd "$work" && "$BINARY" --harness "$file" "$out")
        echo "screenshots: $out"
        ;;
    live)
        name="${2:?name of the session}"
        work="$(prepare "$name")"
        mkfifo "$work/commands"
        (cd "$work" && nohup "$BINARY" --harness "$work/commands" "$work/out" \
            > "$work/game.log" 2>&1 &)
        echo "$work"
        ;;
    send)
        name="${2:?name of the session}"
        shift 2
        work="$BUILD/runs/$name"
        log="$work/out/harness.log"
        before=$(grep -c -E '^(ok|error)' "$log" || true)
        wanted=0
        for command in "$@"; do
            echo "$command" > "$work/commands"
            case "$command" in ""|"#"*) ;; *) wanted=$((wanted + 1)) ;; esac
        done
        for _ in $(seq 1 600); do
            now=$(grep -c -E '^(ok|error)' "$log" || true)
            [ $((now - before)) -ge $wanted ] && break
            sleep 0.2
        done
        grep -E '^(ok|error)' "$log" | tail -n "$wanted"
        ;;
    *)
        sed -n '2,25p' "$0"
        exit 1
        ;;
esac
