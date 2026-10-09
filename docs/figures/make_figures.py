"""Regenerates the diagrams shown in the README.

    pip install matplotlib
    python docs/figures/make_figures.py

The numbers in Tables 1-2 come from docs/figures/compare_ref.sh, not from this script.
"""
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib import font_manager
from matplotlib.lines import Line2D
from matplotlib.patches import FancyBboxPatch, Rectangle

HERE = Path(__file__).resolve().parent
for f in Path("/usr/share/fonts/lm").glob("lm*10-*.otf"):  # Latin Modern, if installed
    font_manager.fontManager.addfont(str(f))
plt.style.use(HERE / "paper.mplstyle")
C = plt.rcParams["axes.prop_cycle"].by_key()["color"]
INK, GRAY = "#1a1a1a", "#8c8c8c"
TINT = {"op": "#e3e3e3", "reg": "#dce6f2", "imm": "#f7e6c8", "fix": "white"}


def save(fig, name):
    fig.savefig(HERE / name, metadata={"Date": None})
    plt.close(fig)


def canvas(w, h, xmax, ymax):
    fig, ax = plt.subplots(figsize=(w, h))
    ax.set_xlim(0, xmax)
    ax.set_ylim(0, ymax)
    ax.axis("off")
    return fig, ax


def box(ax, x, y, w, h, text="", fc="white", ec=INK, fs=8.5, ls="-", lw=0.7, **kw):
    ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0,rounding_size=0.06",
                                fc=fc, ec=ec, lw=lw, ls=ls))
    if text:
        ax.text(x + w / 2, y + h / 2, text, ha="center", va="center", fontsize=fs, **kw)


def arrow(ax, p, q, color=INK, ls="-", text=None, off=(0, 0.1), fs=7.5, **kw):
    ax.annotate("", xy=q, xytext=p, arrowprops=dict(arrowstyle="-|>", lw=0.7, color=color, ls=ls,
                                                    shrinkA=0, shrinkB=0, mutation_scale=7, **kw))
    if text:
        ax.text((p[0] + q[0]) / 2 + off[0], (p[1] + q[1]) / 2 + off[1], text, ha="center",
                va="bottom", fontsize=fs, color=color)


# ---- Figure 1: what the course shell provides and what sim.c implements
fig, ax = canvas(7.2, 3.3, 14.4, 6.6)
box(ax, 0.1, 0.1, 5.6, 6.3, fc="#f7f7f7", ec=GRAY, ls="--")
ax.text(0.3, 6.15, "shell.c / shell.h  (course, not modified)", fontsize=8.5, color="#4d4d4d", va="center")
box(ax, 0.4, 4.75, 5.0, 0.95, "command loop\n$\\mathtt{go}$, $\\mathtt{run\\ n}$, $\\mathtt{rdump}$, $\\mathtt{mdump}$, $\\mathtt{input}$", fs=8)
box(ax, 0.4, 3.0, 5.0, 1.3, "cycle():\n$\\mathtt{process\\_instruction()}$\n$\\mathtt{CURRENT\\_STATE = NEXT\\_STATE}$")
arrow(ax, (2.9, 4.75), (2.9, 4.3))
ax.text(0.45, 2.3, "memory (little-endian, 32-bit access only)", fontsize=7.5, color="#4d4d4d")
for i, (lab, addr) in enumerate([("text", "0x0040 0000"), ("data", "0x1000 0000"), ("stack", "0xffff fffc")]):
    box(ax, 0.4 + i * 1.7, 0.45, 1.6, 1.65, f"{lab}\n{addr}\n1 MiB", fs=7.5, fc="white")
ax.text(1.2, 0.22, "program loaded here", fontsize=6.5, color="#4d4d4d", ha="center")

box(ax, 6.6, 0.1, 7.7, 6.3, fc="#f3f6fa", ec=C[0], ls="--")
ax.text(6.8, 6.15, "sim.c  (Santiago Groba Alonso, Santiago Blanco)", fontsize=8.5, color=C[0], va="center")
stages = [
    ("fetch", "$\\mathtt{inst = mem\\_read\\_32(PC)}$"),
    ("decode", "top byte $= \\mathtt{0x54}$ $\\Rightarrow$ B.cond, else\n$\\mathtt{opcode = inst[31{:}21]}$; Rd, Rn, Rm, imm"),
    ("execute", "switch over 24 opcodes + 6 conditions\n(ALU, shifts, loads/stores, branches)"),
    ("write", "$\\mathtt{NEXT\\_STATE}$: X0–X30 (XZR = 0),\nflags N, Z, PC; stores via $\\mathtt{mem\\_write\\_32}$"),
]
for i, (name, desc) in enumerate(stages):
    y = 4.75 - i * 1.4
    box(ax, 6.9, y, 1.3, 0.95, name, fc="white", fs=8.5, style="italic")
    box(ax, 8.4, y, 5.6, 0.95, desc, fc="white", fs=7.5)
    if i:
        arrow(ax, (7.55, y + 1.4), (7.55, y + 0.95))
arrow(ax, (5.4, 4.0), (6.9, 5.2))
ax.text(6.05, 4.72, "calls", fontsize=7.5, ha="center", va="bottom", rotation=38.7, rotation_mode="anchor")
arrow(ax, (6.9, 1.2), (5.4, 3.3), color=GRAY)
ax.text(6.12, 2.05, "next state", fontsize=7.5, color="#4d4d4d", ha="center", va="top", rotation=-54.5,
        rotation_mode="anchor")
ax.annotate("", xy=(5.5, 1.3), xytext=(6.6, 1.3), arrowprops=dict(arrowstyle="<|-|>", lw=0.7, color=GRAY,
            ls=(0, (3, 2)), shrinkA=0, shrinkB=0, mutation_scale=7))
ax.text(6.05, 0.95, "loads,\nstores", fontsize=6.5, color="#4d4d4d", ha="center", va="top")
fig.tight_layout(pad=0.2)
save(fig, "fig1-simulator.svg")

# ---- Figure 2: A64 encodings of the implemented forms and the decode window
# (lo, hi, label, kind); kind in op/reg/imm/fix; bits inside [31:21] that are operands get hatched
ROWS = [
    ("ADDS, SUBS, CMP, ANDS,\nEOR, ORR, ADD (register)", [(21, 31, "opcode", "op"), (16, 20, "Rm", "reg"),
                                                           (10, 15, "imm6 = 0", "fix"), (5, 9, "Rn", "reg"), (0, 4, "Rd", "reg")]),
    ("MUL (MADD, Ra = XZR)", [(21, 31, "opcode", "op"), (16, 20, "Rm", "reg"), (15, 15, "", "fix"),
                              (10, 14, "Ra", "reg"), (5, 9, "Rn", "reg"), (0, 4, "Rd", "reg")]),
    ("ADDS, SUBS, CMP,\nADD (immediate)", [(23, 31, "opcode", "op"), (22, 22, "sh", "imm"), (10, 21, "imm12", "imm"),
                                           (5, 9, "Rn", "reg"), (0, 4, "Rd", "reg")]),
    ("LSL, LSR (UBFM)", [(22, 31, "opcode", "op"), (16, 21, "immr", "imm"), (10, 15, "imms", "imm"),
                         (5, 9, "Rn", "reg"), (0, 4, "Rd", "reg")]),
    ("STUR{,B,H}, LDUR{,B,H}", [(21, 31, "opcode", "op"), (12, 20, "imm9", "imm"), (10, 11, "", "fix"),
                                (5, 9, "Rn", "reg"), (0, 4, "Rt", "reg")]),
    ("MOVZ", [(23, 31, "opcode", "op"), (21, 22, "hw", "imm"), (5, 20, "imm16", "imm"), (0, 4, "Rd", "reg")]),
    ("B", [(26, 31, "opcode", "op"), (0, 25, "imm26", "imm")]),
    ("CBZ, CBNZ", [(24, 31, "opcode", "op"), (5, 23, "imm19", "imm"), (0, 4, "Rt", "reg")]),
    ("B.cond", [(24, 31, "0x54", "op"), (5, 23, "imm19", "imm"), (4, 4, "", "fix"), (0, 3, "cond", "imm")]),
    ("BR", [(21, 31, "opcode", "op"), (16, 20, "11111", "op"), (10, 15, "000000", "op"),
            (5, 9, "Rn", "reg"), (0, 4, "", "fix")]),
    ("HLT", [(21, 31, "opcode", "op"), (5, 20, "imm16", "imm"), (0, 4, "", "fix")]),
]
X0, CW, RH = 3.3, 0.2, 0.62  # left margin, cell width, row pitch
fig, ax = canvas(7.2, 4.9, X0 + 32 * CW + 0.1, len(ROWS) * RH + 1.15)
top = len(ROWS) * RH + 0.9
bx = lambda bit: X0 + (31 - bit) * CW  # left edge of a bit cell
for bit in (31, 24, 21, 16, 10, 5, 0):
    ax.text(bx(bit) + CW / 2, top + 0.12, str(bit), ha="center", fontsize=6.5, color="#4d4d4d")
for r, (name, fields) in enumerate(ROWS):
    y = top - (r + 1) * RH + 0.08
    ax.text(X0 - 0.12, y + 0.23, name, ha="right", va="center", fontsize=7.2, linespacing=1.0)
    for lo, hi, lab, kind in fields:
        x, w = bx(hi), (hi - lo + 1) * CW
        ax.add_patch(Rectangle((x, y), w, 0.46, fc=TINT[kind], ec=INK, lw=0.5))
        if kind in ("imm", "reg") and hi >= 21 and name != "B.cond":  # operand bits inside the window
            h_hi, h_lo = hi, max(lo, 21)
            ax.add_patch(Rectangle((bx(h_hi), y), (h_hi - h_lo + 1) * CW, 0.46, fc="none",
                                   ec=C[1], lw=0, hatch="////"))
        if lab:
            ax.text(x + w / 2, y + 0.23, lab, ha="center", va="center", fontsize=6.6,
                    bbox=dict(fc=TINT[kind], ec="none", pad=0.4))
# decode windows
win = len(ROWS) * RH
ax.add_patch(Rectangle((bx(31), top - win), 11 * CW, win, fc="none", ec=C[1], lw=1.0, ls=(0, (4, 2))))
yb = top - 9 * RH + 0.08 - 0.05
ax.add_patch(Rectangle((bx(31) - 0.05, yb), 8 * CW + 0.1, 0.56, fc="none", ec=C[0], lw=1.0))
handles = [Rectangle((0, 0), 1, 1, fc=TINT[k], ec=INK, lw=0.5) for k in ("op", "reg", "imm")]
handles.append(Rectangle((0, 0), 1, 1, fc="white", ec=C[1], lw=0.5, hatch="////"))
handles += [Line2D([], [], color=C[1], lw=1.0, ls=(0, (4, 2))), Line2D([], [], color=C[0], lw=1.0)]
ax.legend(handles, ["fixed opcode bits", "register", "immediate", "operand bits inside the key",
                    "dispatch key inst[31:21]", "B.cond key inst[31:24]"],
          loc="lower center", bbox_to_anchor=(0.5, -0.01), fontsize=6.8, ncol=3, handlelength=1.6,
          columnspacing=1.2)
fig.tight_layout(pad=0.2)
save(fig, "fig2-encodings.svg")
