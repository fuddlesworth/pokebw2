// NitroSDK's libmath SHA-1 compression (sha1_block.c; the file split and name are a guess: the code is hand-written ARM
// that follows math.c's quicksort and that math_sha1.c calls through a pointer). sha1_block loads the context's 64-byte
// block as big-endian words, expands them in a 16-word ring on the stack, runs the 80 rounds in four loops of 20 and
// adds the result to the state.
//
// The four round constants are in .text, in the 16 bytes in front of sha1_block, and the rounds reach them by
// pc-relative loads. MWCC can't place data ahead of a function's symbol, so they are an asm function of their own,
// sha1_block_constants, which is never called: the dcd words are its whole body, and sha1_block's loads use
// pc-relative offsets to them (their dsd labels are .L_02077cd4..ce0, with no symbol).
#include "nitro/math.h"

#ifdef POKEBW2_PC
// The host uses the same compression rounds as the SDK's ARM implementation below.
void sha1_block(MATHSHA1Context *context) {
    u32 words[80];
    for (unsigned i = 0; i < 16; i++) {
        const u8 *p = &context->block.bytes[i * 4];
        words[i] = (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3];
    }
    for (unsigned i = 16; i < 80; i++) {
        u32 word = words[i - 3] ^ words[i - 8] ^ words[i - 14] ^ words[i - 16];
        words[i] = word << 1 | word >> 31;
    }
    u32 a = context->state[0], b = context->state[1], c = context->state[2];
    u32 d = context->state[3], e = context->state[4];
    for (unsigned i = 0; i < 80; i++) {
        u32 f, k;
        if (i < 20) {
            f = (b & c) | (~b & d);
            k = 0x5a827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ed9eba1;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8f1bbcdc;
        } else {
            f = b ^ c ^ d;
            k = 0xca62c1d6;
        }
        u32 next = (a << 5 | a >> 27) + f + e + k + words[i];
        e = d;
        d = c;
        c = b << 30 | b >> 2;
        b = a;
        a = next;
    }
    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
}
#else
#pragma thumb off
// clang-format off
asm void sha1_block_constants(void) {
    dcd 0x5a827999
    dcd 0x6ed9eba1
    dcd 0x8f1bbcdc
    dcd 0xca62c1d6
}

// clang-format on

// Hand-written assembly in the SDK, so an MWCC asm function here
// clang-format off
asm void sha1_block(MATHSHA1Context *context) {
    stmdb sp!, {r4, r5, r6, r7, r8, r9, r10, r11, ip, lr}
    mov r11, r0
    sub sp, sp, #0x40
    mov ip, sp
    mvn r3, #0xff00
    add r8, r11, #0x14
    mov r1, ip
    ldmia r8!, {r4, r5, r6, r7}
    mov r9, #0x10
byte_swap:
    eor r2, r4, r4, ror #0x10
    and r2, r3, r2, lsr #0x8
    eor r4, r2, r4, ror #0x8
    eor r2, r5, r5, ror #0x10
    and r2, r3, r2, lsr #0x8
    eor r5, r2, r5, ror #0x8
    eor r2, r6, r6, ror #0x10
    and r2, r3, r2, lsr #0x8
    eor r6, r2, r6, ror #0x8
    eor r2, r7, r7, ror #0x10
    and r2, r3, r2, lsr #0x8
    eor r7, r2, r7, ror #0x8
    stmia r1!, {r4, r5, r6, r7}
    subs r9, r9, #0x4
    ldmneia r8!, {r4, r5, r6, r7}
    bne byte_swap
    ldmia r11, {r4, r5, r6, r7, r8}
    ldr ip, [pc, #-0x80]
    mov r9, #0x0
rounds_0_15:
    and r10, r5, r6
    mvn lr, r5
    and lr, lr, r7
    orr r10, r10, lr
    add r10, r10, ip
    and lr, r9, #0xf
    ldr lr, [sp, lr, lsl #0x2]
    add r10, r10, r8
    add r10, r10, lr
    add r10, r10, r4, ror #0x1b
    mov r8, r7
    mov r7, r6
    mov r6, r5, ror #0x2
    mov r5, r4
    mov r4, r10
    add r9, r9, #0x1
    cmp r9, #0x10
    bne rounds_0_15
rounds_16_19:
    and r10, r5, r6
    mvn lr, r5
    and lr, lr, r7
    orr r10, r10, lr
    add r10, r10, ip
    sub r2, r9, #0x10
    and r2, r2, #0xf
    sub lr, r9, #0xe
    and lr, lr, #0xf
    ldr r3, [sp, r2, lsl #0x2]
    ldr r1, [sp, lr, lsl #0x2]
    sub lr, r9, #0x8
    eor r3, r3, r1
    and lr, lr, #0xf
    ldr r1, [sp, lr, lsl #0x2]
    sub r2, r9, #0x3
    eor r3, r3, r1
    and r2, r2, #0xf
    ldr r1, [sp, r2, lsl #0x2]
    and r2, r9, #0xf
    eor r3, r3, r1
    mov r3, r3, ror #0x1f
    str r3, [sp, r2, lsl #0x2]
    and lr, r9, #0xf
    ldr lr, [sp, lr, lsl #0x2]
    add r10, r10, r8
    add r10, r10, lr
    add r10, r10, r4, ror #0x1b
    mov r8, r7
    mov r7, r6
    mov r6, r5, ror #0x2
    mov r5, r4
    mov r4, r10
    add r9, r9, #0x1
    cmp r9, #0x14
    bne rounds_16_19
    ldr ip, [pc, #-0x15c]
rounds_20_39:
    eor r10, r5, r6
    eor r10, r10, r7
    add r10, r10, ip
    sub r2, r9, #0x10
    and r2, r2, #0xf
    sub lr, r9, #0xe
    and lr, lr, #0xf
    ldr r3, [sp, r2, lsl #0x2]
    ldr r1, [sp, lr, lsl #0x2]
    sub lr, r9, #0x8
    eor r3, r3, r1
    and lr, lr, #0xf
    ldr r1, [sp, lr, lsl #0x2]
    sub r2, r9, #0x3
    eor r3, r3, r1
    and r2, r2, #0xf
    ldr r1, [sp, r2, lsl #0x2]
    and r2, r9, #0xf
    eor r3, r3, r1
    mov r3, r3, ror #0x1f
    str r3, [sp, r2, lsl #0x2]
    and lr, r9, #0xf
    ldr lr, [sp, lr, lsl #0x2]
    add r10, r10, r8
    add r10, r10, lr
    add r10, r10, r4, ror #0x1b
    mov r8, r7
    mov r7, r6
    mov r6, r5, ror #0x2
    mov r5, r4
    mov r4, r10
    add r9, r9, #0x1
    cmp r9, #0x28
    bne rounds_20_39
    ldr ip, [pc, #-0x1e4]
rounds_40_59:
    and r10, r5, r6
    and lr, r5, r7
    orr r10, r10, lr
    and lr, r6, r7
    orr r10, r10, lr
    add r10, r10, ip
    sub r2, r9, #0x10
    and r2, r2, #0xf
    sub lr, r9, #0xe
    and lr, lr, #0xf
    ldr r3, [sp, r2, lsl #0x2]
    ldr r1, [sp, lr, lsl #0x2]
    sub lr, r9, #0x8
    eor r3, r3, r1
    and lr, lr, #0xf
    ldr r1, [sp, lr, lsl #0x2]
    sub r2, r9, #0x3
    eor r3, r3, r1
    and r2, r2, #0xf
    ldr r1, [sp, r2, lsl #0x2]
    and r2, r9, #0xf
    eor r3, r3, r1
    mov r3, r3, ror #0x1f
    str r3, [sp, r2, lsl #0x2]
    and lr, r9, #0xf
    ldr lr, [sp, lr, lsl #0x2]
    add r10, r10, r8
    add r10, r10, lr
    add r10, r10, r4, ror #0x1b
    mov r8, r7
    mov r7, r6
    mov r6, r5, ror #0x2
    mov r5, r4
    mov r4, r10
    add r9, r9, #0x1
    cmp r9, #0x3c
    bne rounds_40_59
    ldr ip, [pc, #-0x278]
rounds_60_79:
    eor r10, r5, r6
    eor r10, r10, r7
    add r10, r10, ip
    sub r2, r9, #0x10
    and r2, r2, #0xf
    sub lr, r9, #0xe
    and lr, lr, #0xf
    ldr r3, [sp, r2, lsl #0x2]
    ldr r1, [sp, lr, lsl #0x2]
    sub lr, r9, #0x8
    eor r3, r3, r1
    and lr, lr, #0xf
    ldr r1, [sp, lr, lsl #0x2]
    sub r2, r9, #0x3
    eor r3, r3, r1
    and r2, r2, #0xf
    ldr r1, [sp, r2, lsl #0x2]
    and r2, r9, #0xf
    eor r3, r3, r1
    mov r3, r3, ror #0x1f
    str r3, [sp, r2, lsl #0x2]
    and lr, r9, #0xf
    ldr lr, [sp, lr, lsl #0x2]
    add r10, r10, r8
    add r10, r10, lr
    add r10, r10, r4, ror #0x1b
    mov r8, r7
    mov r7, r6
    mov r6, r5, ror #0x2
    mov r5, r4
    mov r4, r10
    add r9, r9, #0x1
    cmp r9, #0x50
    bne rounds_60_79
    ldmia r11, {r1, r2, r3, r9, r10}
    add r1, r1, r4
    add r2, r2, r5
    add r3, r3, r6
    add r9, r9, r7
    add r10, r10, r8
    stmia r11, {r1, r2, r3, r9, r10}
    add sp, sp, #0x40
    ldmia sp!, {r4, r5, r6, r7, r8, r9, r10, r11, ip, pc}
}

// clang-format on
#pragma thumb on
#endif
