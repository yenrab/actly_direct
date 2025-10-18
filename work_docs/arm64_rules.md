Good question 👌 — this is one of those **ARM64 (AArch64)** “gotchas” that trips up a lot of people, because unlike x86, you **can’t just put any arbitrary 64-bit immediate** in a single `mov` instruction.

Let’s break this down clearly.

---

## 🧠 1. MOV on AArch64 Is Not a Generic “Move Immediate”

The instruction you write in assembly as:

```asm
mov x0, #12345
```
…may not assemble to a single mov instruction. Behind the scenes, the assembler may rewrite it into:
	•	a MOV (immediate) (which is really an alias for MOVZ, MOVN, or MOVK), or
	•	a sequence of multiple instructions to synthesize the number.

🧾 2. Immediate Encoding Rules

🟢 a. MOVZ / MOVN / MOVK (for arbitrary 64-bit constants)

ARM64 doesn’t have a single instruction that loads an arbitrary 64-bit immediate.
Instead, it uses 16-bit chunks, positioned at 16-bit boundaries:
```asm
movz x0, #0x1234             // sets bits [15:0]
movk x0, #0x5678, lsl #16    // sets bits [31:16]
movk x0, #0x9abc, lsl #32    // sets bits [47:32]
movk x0, #0xdef0, lsl #48    // sets bits [63:48]
```
	•	MOVZ = Move and Zero (sets the register to immediate, zeroing all other bits)
	•	MOVK = Move and Keep (overwrites 16 bits, keeps the others)
	•	MOVN = Move Negated (same as MOVZ but loads the bitwise NOT of the immediate)

👉 The assembler automatically generates these sequences if you use mov with a large constant:
```asm
mov x0, #0x123456789ABCDEF0
```
This assembles to:
```asm
movz    x0, #0xdef0, lsl #48
movk    x0, #0x9abc, lsl #32
movk    x0, #0x5678, lsl #16
movk    x0, #0x1234
```
🟡 b. MOV (immediate) with 12-bit (or shifted) literal

There’s another encoding: “move wide immediate” vs “add/sub immediate”.

For some small immediates, the assembler uses mov as an alias for orr or add with an immediate:

```asm
mov x0, #255
```
Assembles to:
```asm
mov x0, #0xff
    ; actually ORR X0, XZR, #0xff
```
For larger numbers:
```asm
mov x0, #0x123000
```
This works if the number fits in 12 bits, optionally shifted left by 0 or 12.
So immediates must be of the form:
```asm
imm12 << 0   OR
imm12 << 12
```
where imm12 is a 12-bit unsigned integer (0–4095).
Examples:

| Immediate     | Encodable in single `mov`? | Why                        |
|--------------|-----------------------------|----------------------------|
| `#0x7FF`     | ✅                          | fits in 12 bits            |
| `#0x800`     | ✅                          | fits in 12 bits            |
| `#0x1000`    | ✅                          | 1 << 12                    |
| `#0x123000`  | ✅                          | 0x123 << 12               |
| `#0x123456`  | ❌                          | not 12-bit or 12-bit<<12  |

---

### 🟠 c. Special encodings: bitmask immediates

Some logical instructions like `orr`, `and`, `eor` support **bitmask immediates**, which are *very specific repeating patterns*. These can sometimes load "weird-looking" constants in one instruction.

For example:

```asm
mov x0, #0x00ff00ff00ff00ff
```
will assemble to a single orr with a bitmask immediate.
But the allowed patterns are restricted — basically sequences of N consecutive 1 bits rotated somewhere in the 64-bit word.

These are useful for setting up masks quickly but not general-purpose.
---

## 📌 Summary Table

| Form                  | Size Limit                     | Instructions                            |
|-----------------------|--------------------------------|-----------------------------------------|
| 12-bit literal        | ≤ 4095 or imm12 << 12          | 1 instruction                           |
| Bitmask immediate     | Specific repeating bit patterns| 1 instruction                           |
| Arbitrary 64-bit      | Any value                      | Up to 4 (MOVZ + MOVK sequence)          |

---

## 🚀 Practical Guidelines

- ✅ **Small numbers** (0–4095 or shifted by 12) → single instruction.  
- ✅ **Masks / patterns** → sometimes encodable as bitmask immediates.  
- ✅ **Large constants** → let the assembler generate MOVZ/MOVK sequence.  
- ❌ You can’t force a single `mov` for arbitrary 64-bit values.

---

## 🧪 Example Assembly

```asm
mov x0, #0x123
mov x1, #0x123000
mov x2, #0x123456789ABCDEF0
```

Another common pattern is to use literal pools:
```asm
ldr x0, =0x123456789ABCDEF0
```
