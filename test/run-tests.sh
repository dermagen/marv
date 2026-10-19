#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

# If /home/user/bin exists in the local environment, include it in PATH
if [ -d "/home/user/bin" ]; then
  export PATH="/home/user/bin:$PATH"
fi

SKINT_FLAGS=""
if [ -d "/home/user/skint/lib" ]; then
  SKINT_FLAGS="-I /home/user/skint/lib"
fi

SKINT="skint $SKINT_FLAGS -q"
MARV="skint $SKINT_FLAGS -p $ROOT_DIR/src/skint/marv.scm"

WCPL_INC=""
if [ -d "/home/user/wcpl/lib/include" ]; then
  WCPL_INC="-I/home/user/wcpl/lib/include/ -L/home/user/wcpl/lib/"
fi

cleanup() {
  rm -f examples/*.c examples/*.wo examples/*.s \
        examples/multimod/*.c examples/multimod/*.wo examples/multimod/*.s \
        /tmp/gh_marv_*
}
trap cleanup EXIT

echo "1. Verifying all 13 single-source adapter files in lib/*.c compile under both cc and wcpl..."
for f in lib/*.c; do
  b=$(basename "$f" .c)
  cc -std=c99 -O2 -Wall -Wextra -Werror -Ilib -c "$f" -o "/tmp/gh_marv_${b}.o"
  wcpl -q $WCPL_INC -c -o "/tmp/gh_marv_${b}.wo" "$f"
done
echo "  [PASS] All 13 lib/*.c adapters compiled cleanly under both cc and wcpl -c!"

echo ""
echo "2. Running unit & RV64G simulation test suite (test/test.scm)..."
$SKINT test/test.scm

echo ""
echo "3. Testing MARV CLI (-c compile-only mode & compile-and-link mode)..."
for ex in dirent_demo libc_demo queens; do
  $MARV -T target -t cc    -c "examples/$ex.mvs"
  $MARV -T target -t wcpl  -c "examples/$ex.mvs"
  $MARV -T target -t rv64g -c "examples/$ex.mvs"
  test -f "examples/$ex.c"
  test -f "examples/$ex.wo"
  test -f "examples/$ex.s"
done
echo "  [PASS] -c mode generated .c, .wo, and .s files next to each .mvs file!"

$MARV -T target -t cc -o /tmp/gh_marv_dirent examples/dirent_demo.mvs
$MARV -T target -t cc -o /tmp/gh_marv_libc   examples/libc_demo.mvs
$MARV -T target -t cc -o /tmp/gh_marv_queens examples/queens.mvs

(cd examples/sample_dir && /tmp/gh_marv_dirent) > /tmp/gh_marv_dirent_cc.out
(cd examples/sample_dir && /tmp/gh_marv_libc)
/tmp/gh_marv_queens > /tmp/gh_marv_queens_cc.out
echo "  [PASS] Compile-and-link mode (-t cc) built and ran all single-module examples!"

echo ""
echo "4. Testing 3-module separate compilation & mixed (.c + .mvs) linking..."
$MARV -T target -t cc -c examples/multimod/vec3.mvs
$MARV -T target -t cc -o /tmp/gh_marv_multimod \
  examples/multimod/vec3.c examples/multimod/stats.mvs examples/multimod/app.mvs
/tmp/gh_marv_multimod > /tmp/gh_marv_mm_cc.out
cat /tmp/gh_marv_mm_cc.out
echo "  [PASS] 3-module mixed object + .mvs build succeeded!"
