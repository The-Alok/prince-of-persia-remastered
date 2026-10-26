#!/bin/sh
# Resolve assets beside this launcher, even when started from Steam or another directory.
set -eu
game_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$game_dir"
exec ./prince "$@"
