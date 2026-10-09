#!/usr/bin/env bash
# Runs every program on src/sim and on the course reference simulator (ref_sim_x86),
# one instruction at a time (`run 1` + `rdump`, then `mdump` of the data segment),
# and reports whether the two transcripts are identical.
#
#   bash docs/figures/compare_ref.sh            # provided inputs (Table 1)
#   bash docs/figures/compare_ref.sh --probes   # extra instruction-form probes (Table 2)
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT

gcc -g -O0 "$ROOT/src/shell.c" "$ROOT/src/sim.c" -o "$T/sim"
install -m 755 "$ROOT/ref_sim_x86" "$T/ref_sim"
steps() { for _ in $(seq 1 40); do echo "run 1"; echo "rdump"; done; echo "mdump 0x10000000 0x10000010"; echo quit; }
compare() {  # $1 = .x file, $2 = label
    (cd "$T" && steps | ./sim "$1" | sed 's/ARM-SIM> //' > a.txt 2>&1 || true)
    (cd "$T" && steps | ./ref_sim "$1" | sed 's/ARM-SIM> //' > b.txt 2>&1 || true)
    n=$(grep 'Instruction Count' "$T/b.txt" | tail -1 | awk '{print $4}')
    if cmp -s "$T/a.txt" "$T/b.txt"; then r=match; else r=DIFFERS; fi
    printf '%-16s %3s instr  %s\n' "$2" "$n" "$r"
}

if [[ "${1:-}" != "--probes" ]]; then
    for x in "$ROOT"/inputs/bytecodes/*.x "$ROOT"/inputs/test.x; do compare "$x" "$(basename "$x" .x)"; done
    exit 0
fi

# Probes: assembled with the course toolchain (the same one inputs/asm2hex uses).
TC="$ROOT/aarch64-linux-android-4.9/bin"
install -m 755 "$TC/aarch64-linux-android-as" "$T/as"
install -m 755 "$TC/aarch64-linux-android-objdump" "$T/objdump"
probe() {  # $1 = name, $2 = assembly body (\n-separated); HLT is appended
    printf '.text\n%b\nHLT 0\n' "$2" > "$T/$1.s"
    "$T/as" "$T/$1.s" -o "$T/$1.o"
    "$T/objdump" -d "$T/$1.o" | grep -P '^\s+[0-9a-f]+:\t' | cut -f2 | tr -d ' ' > "$T/$1.x"
    compare "$T/$1.x" "$1"
}
BASE='movz X1, 0x1000\nlsl X1, X1, 16\nmovz X2, 0xbeef'
probe adds_imm      'adds X0, X0, 5'
probe adds_imm_2048 'adds X0, X0, 2048'
probe adds_lsl12    'adds X0, X0, 1, lsl 12'
probe subs_lsl12    'movz X0, 0x2000\nsubs X0, X0, 1, lsl 12'
probe add_lsl12     'add X0, X0, 1, lsl 12'
probe subs_neg      'movz X1, 3\nsubs X0, X1, 5'
probe cmp_imm       'movz X1, 3\ncmp X1, 4'
probe cmp_reg       'movz X1, 3\ncmp X1, X1'
probe add_reg       'movz X1, 3\nmovz X2, 4\nadd X0, X1, X2'
probe ands          'movz X1, 3\nmovz X2, 6\nands X0, X1, X2'
probe orr           'movz X1, 3\nmovz X2, 4\norr X0, X1, X2'
probe mul           'movz X1, 3\nmovz X2, 4\nmul X0, X1, X2'
probe lsl_4         'movz X2, 0xbeef\nlsl X3, X2, 4'
probe lsl_40        'movz X2, 0xbeef\nlsl X3, X2, 40'
probe lsr_4         'movz X2, 0xbeef\nlsr X3, X2, 4'
probe lsr_40        'movz X2, 0xbeef\nlsl X2, X2, 16\nlsl X2, X2, 32\nlsr X3, X2, 40'
probe lsl_flags     'movz X2, 0\nadds X5, X2, 1\nlsl X3, X2, 4'
probe stur_ldur_64  "$BASE\nlsl X3, X2, 32\nadd X3, X3, X2\nstur X3, [X1, 0x0]\nldur X4, [X1, 0x0]"
probe sturh_ldurh   "$BASE\nsturh W2, [X1, 0x2]\nldurh W5, [X1, 0x2]"
probe sturb_ldurb   "$BASE\nsturb W2, [X1, 0x1]\nldurb W5, [X1, 0x1]"
probe b_forward     'b l\nmovz X9, 1\nl:\nmovz X8, 1'
probe b_backward    'b l2\nl1:\nmovz X8, 1\nb end\nl2:\nb l1\nend:\nmovz X7, 1'
probe cbnz_backward 'movz X1, 2\nl:\nsubs X1, X1, 1\ncbnz X1, l'
probe cbz_backward  'b l2\nl1:\nmovz X8, 1\nb end\nl2:\ncbz X0, l1\nend:\nmovz X7, 1'
probe bne_backward  'movz X1, 2\nl:\nsubs X1, X1, 1\nbne l'
probe bne           'movz X1, 3\ncmp X1, 4\nbne l\nmovz X9, 1\nl:\nmovz X8, 1'
probe bgt           'movz X1, 3\ncmp X1, 2\nbgt l\nmovz X9, 1\nl:\nmovz X8, 1'
probe bge           'movz X1, 3\ncmp X1, 3\nbge l\nmovz X9, 1\nl:\nmovz X8, 1'
probe ble           'movz X1, 3\ncmp X1, 4\nble l\nmovz X9, 1\nl:\nmovz X8, 1'
probe br            'movz X1, 0x40\nlsl X1, X1, 16\nadd X1, X1, 0x14\nbr X1\nmovz X9, 1\nmovz X8, 1'
