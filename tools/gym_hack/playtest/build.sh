#!/bin/bash
# Builds gbarun, a tiny headless GBA runner (mGBA core) used to playtest maps and scenes from scripts.
# Usage: tools/gym_hack/playtest/build.sh <work dir>   -> <work dir>/gbarun
# Then: printf "wait 300\nshot title.png\n" | <work dir>/gbarun pokefirered.gba
# Commands are documented at the top of gbarun.c. Save states only work with the ROM they were made on.
set -e
SRC=$(cd "$(dirname "$0")" && pwd)
W=${1:-/tmp/gbarun}
mkdir -p "$W"
cd "$W"
[ -d mgba-src ] || git clone --depth 1 --branch 0.10.5 https://github.com/mgba-emu/mgba.git mgba-src
mkdir -p mgba-build && cd mgba-build
cmake ../mgba-src -DBUILD_QT=OFF -DBUILD_SDL=OFF -DBUILD_SHARED=OFF -DBUILD_STATIC=ON -DUSE_EPOXY=OFF \
  -DUSE_FFMPEG=OFF -DUSE_LUA=OFF -DUSE_SQLITE3=OFF -DUSE_LIBZIP=OFF -DUSE_MINIZIP=OFF -DUSE_DISCORD_RPC=OFF \
  -DUSE_EDITLINE=OFF -DUSE_ELF=OFF -DBUILD_GL=OFF -DBUILD_GLES2=OFF -DBUILD_GLES3=OFF -DM_CORE_GB=OFF \
  -DCMAKE_BUILD_TYPE=Release > /dev/null
make -j"$(nproc)" > /dev/null
cd ..
gcc -O2 -o gbarun "$SRC/gbarun.c" -Imgba-src/include -Imgba-build/include -include mgba/flags.h \
  mgba-build/libmgba.a -lpng -lz -lm -lpthread
echo "built $W/gbarun"
