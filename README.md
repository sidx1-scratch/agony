# AGONY

**A**ssembly-**G**lyph-**O**pcode-**N**otation-**Y**ielding language.

A real, working, Turing-complete-ish (registers + stack + jumps) programming
language whose syntax requires you to encode *every instruction five times
simultaneously*, in five different notations, all of which must agree with
each other or the program refuses to run.

## Why

Because assembly alone wasn't painful enough.

## Instruction format

```
BINARY::MNEMONIC::OPERANDS::PARITY::CHECKSUM::🏁
```

| Field | What it is | Example |
|---|---|---|
| `BINARY` | The opcode, written as a full 8-bit binary byte | `00001000` |
| `MNEMONIC` | The assembly mnemonic — must decode to the *same* opcode as the binary | `MOV` |
| `OPERANDS` | Operands, each prefixed with a tag emoji (see below) | `📍R0🔢5` |
| `PARITY` | ✅ if the opcode byte has an even number of 1-bits, ❌ if odd | `❌` |
| `CHECKSUM` | The opcode's decimal value, spelled out again in keycap emoji digits | `8️⃣` |
| terminator | Always literally `🏁` | `🏁` |

Get any of these five out of sync with each other (wrong mnemonic for the
binary, wrong parity, wrong checksum, missing 🏁) and the interpreter refuses
to compile, with a specific error telling you which redundant encoding you
botched.

## Operand tags

| Emoji | Type | Value syntax |
|---|---|---|
| 📍 | register | `R0`–`R7` |
| 🔢 | number literal | integer, e.g. `42`, `-3` |
| 🔤 | string literal | `"quoted text"`, supports `\n` `\t` `\"` `\\` |
| 🎯 | label | bare identifier, e.g. `loop` |

Tags and values are concatenated directly with no separator —
`📍R0🔢5` is two operands: register R0, number 5.

## Opcode table

| Binary | Dec | Mnemonic | Operands | Effect |
|---|---|---|---|---|
| `00000001` | 1 | `NOP` | — | does nothing |
| `00000010` | 2 | `PUSH` | val | push value onto stack |
| `00000011` | 3 | `POP` | reg | pop into register |
| `00000100` | 4 | `ADD` | dest, src | dest += src |
| `00000101` | 5 | `SUB` | dest, src | dest -= src |
| `00000110` | 6 | `MUL` | dest, src | dest *= src |
| `00000111` | 7 | `DIV` | dest, src | dest /= src |
| `00001000` | 8 | `MOV` | dest, src | dest = src |
| `00001001` | 9 | `PRINT` | val | print as integer |
| `00001010` | 10 | `PRINTS` | string | print string literally |
| `00001011` | 11 | `JMP` | label | unconditional jump |
| `00001100` | 12 | `JZ` | reg, label | jump if reg == 0 |
| `00001101` | 13 | `LBL` | label | define a label (no-op at runtime) |
| `00001110` | 14 | `JNZ` | reg, label | jump if reg != 0 |
| `00001111` | 15 | `HALT` | — | stop execution |

Comments start a line with `//`. Blank lines are ignored. Everything else
must be a fully-formed six-field instruction.

## Build & run

```
g++ -std=c++17 -O2 -o agony agony.cpp
./agony hello.agony
./agony countdown.agony
```

## Examples included

- `hello.agony` — prints "Hello, World!"
- `countdown.agony` — counts down from 5 to 1 using a register, subtraction,
  and a conditional jump loop
- `rectangle.agony` — prints a solid 5×10 rectangle of asterisks using a
  single counted loop
- `box.agony` — prints a *hollow* bordered rectangle, built with nested
  loops and runtime zero-comparisons to distinguish top/bottom/middle rows
  (no shortcuts, no pre-baked strings for the interior)

## Contributing

Bug reports and PRs to the C++ implementation are welcome, provided they
preserve the five-way redundancy that is the entire point of this language.
PRs that quietly make AGONY easier to write will be closed on sight.

### Forks and ports to other languages

Rewrites in other languages are welcome, but the name `agony` is reserved
for this original C++ implementation. Forks must be named `agony-<language>`
so nobody confuses a port for the real, maximally painful thing:

- Rust port → `agony-rust`
- JavaScript port → `agony-js`
- **Python port → `agony-easy`** (not `agony-py` — let's call it what it is)

If your fork removes the binary/parity/checksum redundancy requirements
entirely, it must also be renamed to something that does not contain the
word "agony," since you have, at that point, clearly stopped suffering.

Before submitting a rewrite in another language, contributors are strongly
encouraged (though not, alas, required) to first achieve working proficiency
in x86 assembly and C++, as a show of good faith to the spirit of the
project.

## Writing your own AGONY programs

1. Decide what you want to do.
2. Pick the opcode from the table.
3. Write out its full 8-bit binary form.
4. Write the matching mnemonic.
5. Tag and write your operands.
6. Compute the byte's bit parity by hand and pick ✅ or ❌.
7. Spell the opcode's decimal value out in keycap emoji digits.
8. Terminate with 🏁.
9. Question your choices.
