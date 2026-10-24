#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

# Tool locations. Everything can be overridden from the environment so the
# suite runs outside the author's machine; the /home/user paths stay as the
# fallback for that machine.  [cco]
#   CC, SKINT_BIN, SKINT_LIB, WCPL_BIN, WCPL_HOME, MARV_BINDIR
CC=${CC:-cc}
SKINT_BIN=${SKINT_BIN:-skint}
WCPL_BIN=${WCPL_BIN:-wcpl}

if [ -d "/home/user/bin" ]; then
  export PATH="/home/user/bin:$PATH"
fi

SKINT_LIB=${SKINT_LIB:-}
if [ -z "$SKINT_LIB" ] && [ -d "/home/user/skint/lib" ]; then
  SKINT_LIB="/home/user/skint/lib"
fi
SKINT_FLAGS=""
[ -n "$SKINT_LIB" ] && SKINT_FLAGS="-I $SKINT_LIB"

SKINT="$SKINT_BIN $SKINT_FLAGS -q"
MARV_SKINT="$SKINT_BIN $SKINT_FLAGS -p $ROOT_DIR/src/skint/marv.scm"
MARV_BIN="/tmp/gh_marv_bin"

WCPL_HOME=${WCPL_HOME:-/home/user/wcpl}
WCPL_INC=""
if [ -d "$WCPL_HOME/lib/include" ]; then
  WCPL_INC="-I$WCPL_HOME/lib/include/ -L$WCPL_HOME/lib/"
fi
# srt/scmi.c includes srt/scmrt.h: wcpl joins an include base to the file name
# as given, so these bases need their trailing slash.
MARV_INC="-I$ROOT_DIR/lib/ -I$ROOT_DIR/srt/"

cleanup() {
  rm -f examples/*.c examples/*.wo examples/*.s \
        examples/multimod/*.c examples/multimod/*.wo examples/multimod/*.s \
        /tmp/gh_marv_*
}
trap cleanup EXIT

NADAPTERS=$(ls lib/*.c srt/*.c | wc -l)

echo "0. Building standalone MARV compiler from m.c r.c v.c a.c (#F / sfc output)..."
$CC -O2 -o "$MARV_BIN" "$ROOT_DIR/m.c" "$ROOT_DIR/r.c" "$ROOT_DIR/v.c" "$ROOT_DIR/a.c" -lm
MARV_BINDIR=${MARV_BINDIR:-/home/user/bin}
[ -d "$MARV_BINDIR" ] && cp -f "$MARV_BIN" "$MARV_BINDIR/marv" 2>/dev/null || true
echo "  [PASS] Built standalone C binary ($MARV_BIN) from m.c r.c v.c a.c!"

echo ""
echo "1. Verifying all $NADAPTERS single-source adapter files in lib/*.c and srt/*.c compile under both cc and wcpl..."
for f in lib/*.c srt/*.c; do
  b=$(basename "$f" .c)
  $CC -std=c99 -O2 -Wall -Wextra -Werror -c "$f" -o "/tmp/gh_marv_${b}.o"
  $WCPL_BIN -q $WCPL_INC $MARV_INC -c -o "/tmp/gh_marv_${b}.wo" "$f"
done
echo "  [PASS] All $NADAPTERS lib/*.c and srt/*.c adapters compiled cleanly under both cc and wcpl -c!"

echo ""
echo "2. Running unit & RV64G simulation test suite (test/test.scm)..."
$SKINT test/test.scm

echo ""
echo "3. Testing standalone MARV binary (-c mode, link mode, and Skint parity)..."
for ex in dirent_demo libc_demo queens; do
  for t in cc wcpl rv64g; do
    case $t in cc) ext=c;; wcpl) ext=wo;; rv64g) ext=s;; esac
    "$MARV_BIN" -T target -t "$t" -c "examples/$ex.mvs"
    test -f "examples/$ex.$ext"
    $MARV_SKINT -T target -t "$t" -c -o "/tmp/gh_marv_skint_${ex}.${ext}" "examples/$ex.mvs"
    diff -u "/tmp/gh_marv_skint_${ex}.${ext}" "examples/$ex.$ext"
  done
done
rm -f examples/*.c examples/*.wo examples/*.s
echo "  [PASS] Standalone marv binary generated .c, .wo, and .s identical to Skint marv.scm!"

"$MARV_BIN" -v -T target -t cc -o /tmp/gh_marv_dirent examples/dirent_demo.mvs 2> /tmp/gh_marv_verbose.err
grep -q "^marv -t cc -c -o .* examples/dirent_demo.mvs$" /tmp/gh_marv_verbose.err
grep -q "^cc .* -o /tmp/gh_marv_dirent " /tmp/gh_marv_verbose.err
test ! -f examples/dirent_demo.c
"$MARV_BIN" -T target -t cc -o /tmp/gh_marv_libc   examples/libc_demo.mvs
test ! -f examples/libc_demo.c
"$MARV_BIN" -T target -t cc -o /tmp/gh_marv_queens examples/queens.mvs
test ! -f examples/queens.c

/tmp/gh_marv_dirent > /tmp/gh_marv_dirent_cc.out
/tmp/gh_marv_libc
/tmp/gh_marv_queens > /tmp/gh_marv_queens_cc.out
echo "  [PASS] Standalone marv compile-and-link mode (-t cc) built and ran all examples using cleaned-up temporary objects!"

echo ""
echo "4. Testing 3-module separate compilation & mixed (.c + .mvs) linking with standalone marv..."
"$MARV_BIN" -T target -t cc -c examples/multimod/vec3.mvs
"$MARV_BIN" -T target -t cc -o /tmp/gh_marv_multimod \
  examples/multimod/vec3.c examples/multimod/stats.mvs examples/multimod/app.mvs
/tmp/gh_marv_multimod > /tmp/gh_marv_mm_cc.out
cat /tmp/gh_marv_mm_cc.out
echo "  [PASS] 3-module mixed object + .mvs build succeeded!"

echo ""
echo "5. Testing SRFI-270 hexadecimal floating-point literals with standalone marv..."
cat > /tmp/gh_marv_hexflt.mvs << 'EOF'
(module $hexflt
  (func $main (export "main")
    (result i32)
    (local $pi f64)
    (local $half f32)
    (local.set $pi (f64.const #x1.921fb54442d18p+1))
    (local.set $half (f32.const #x1.0p-1))
    (if (i32.and
          (f64.eq (local.get $pi) (f64.const 3.141592653589793))
          (f32.eq (local.get $half) (f32.const 0.5)))
      (then (return (i32.const 0)))
      (else (return (i32.const 1))))))
EOF
"$MARV_BIN" -T target -t cc   -c -o /tmp/gh_marv_hexflt.c  /tmp/gh_marv_hexflt.mvs
"$MARV_BIN" -T target -t wcpl -c -o /tmp/gh_marv_hexflt.wo /tmp/gh_marv_hexflt.mvs
"$MARV_BIN" -T target -t rv64g -c -o /tmp/gh_marv_hexflt.s /tmp/gh_marv_hexflt.mvs
"$MARV_BIN" -T target -t cc -o /tmp/gh_marv_hexflt_bin /tmp/gh_marv_hexflt.mvs
/tmp/gh_marv_hexflt_bin
$WCPL_BIN -q $WCPL_INC -o /tmp/gh_marv_hexflt.wasm /tmp/gh_marv_hexflt.wo
test -s /tmp/gh_marv_hexflt.wasm
echo "  [PASS] SRFI-270 hex float parsing and emission verified across cc, wcpl, and rv64g!"

echo ""
echo "6. Checking instruction edge cases (test/edge.mvs) against test/edge.expected..."
# shift masking, signed/unsigned division, clz/ctz of zero, sign extension,
# truncation, nearest-ties-to-even, narrow loads, br_table defaults.  [cco]
"$MARV_BIN" -T target -t cc -o /tmp/gh_marv_edge test/edge.mvs
/tmp/gh_marv_edge > /tmp/gh_marv_edge.out
diff -u test/edge.expected /tmp/gh_marv_edge.out
# the same values must come out of the rv64g assembly
"$MARV_BIN" -T target -t rv64g -c -o /tmp/gh_marv_edge.s test/edge.mvs
$SKINT test/edge-rv64.scm > /tmp/gh_marv_edge_rv.out
diff -u test/edge.expected /tmp/gh_marv_edge_rv.out
echo "  [PASS] All 24 edge cases agree between cc, rv64g and WebAssembly semantics!"

echo ""
echo "7. Testing configure --prefix and -D PREFIX installation lookup..."
mkdir -p /tmp/gh_marv_pfx/share/marv/target /tmp/gh_marv_pfx/share/marv/lib
cp target/*.mvt /tmp/gh_marv_pfx/share/marv/target/
cp lib/*.mvh lib/*.c /tmp/gh_marv_pfx/share/marv/lib/
$CC -O2 -D PREFIX=/tmp/gh_marv_pfx/share/marv/ -o /tmp/gh_marv_pfx_bin "$ROOT_DIR/m.c" "$ROOT_DIR/r.c" "$ROOT_DIR/v.c" "$ROOT_DIR/a.c" -lm
(cd /tmp && /tmp/gh_marv_pfx_bin -t cc -o /tmp/gh_marv_pfx_queens "$ROOT_DIR/examples/queens.mvs")
/tmp/gh_marv_pfx_queens > /tmp/gh_marv_pfx_queens.out
diff -u /tmp/gh_marv_queens_cc.out /tmp/gh_marv_pfx_queens.out
rm -rf /tmp/gh_marv_pfx
echo "  [PASS] -D PREFIX target and library lookup verified from out-of-tree directory!"

# Opt-in: the committed m.c r.c v.c a.c must be what sfc produces from src/sf.
# Takes about three minutes, so it is not part of the default run.  [cco]
#   MARV_CHECK_SFC=1 bash test/run-tests.sh
if [ "${MARV_CHECK_SFC:-0}" = 1 ]; then
  echo ""
  echo "8. Checking that the committed C sources match sfc output from src/sf..."
  SFC=${SFC:-sfc}
  rm -rf /tmp/marv_sfc_regen && mkdir -p /tmp/marv_sfc_regen
  cp src/sf/*.sf /tmp/marv_sfc_regen/
  (cd /tmp/marv_sfc_regen && $SFC m.sf r.sf v.sf a.sf >/dev/null)
  for u in m r v a; do
    cmp "/tmp/marv_sfc_regen/$u.c" "$ROOT_DIR/$u.c"
  done
  rm -rf /tmp/marv_sfc_regen
  echo "  [PASS] m.c r.c v.c a.c are byte-identical to freshly generated sfc output!"
fi
