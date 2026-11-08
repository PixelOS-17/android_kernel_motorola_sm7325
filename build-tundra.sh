#!/bin/bash
# Kernel-only build for tundra bring-up: Image + DTBs.
# Usage: ./build-tundra.sh [out-dir]   (default ../out-6.1)
set -e
SRC=$(cd "$(dirname "$0")" && pwd)
OUT=${1:-$SRC/../out}
CLANG=${CLANG:-$HOME/pos/prebuilts/clang/host/linux-x86/clang-r596125/bin}
export KBUILD_BUILD_HOST=${KBUILD_BUILD_HOST:-localhost}
export PATH=$CLANG:$HOME/pos/prebuilts/kernel-build-tools/linux-x86/bin:$PATH
M="make -C $SRC O=$OUT ARCH=arm64 LLVM=1 LLVM_IAS=1"
mkdir -p "$OUT"
$M gki_defconfig
$SRC/scripts/kconfig/merge_config.sh -m -O "$OUT" "$OUT/.config" \
	"$SRC/arch/arm64/configs/vendor/tundra.config" >/dev/null
$M olddefconfig
nice $M -j${JOBS:-32} Image dtbs
