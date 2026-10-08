#ifndef MOS_RUNTIME_SETJMP_H
#define MOS_RUNTIME_SETJMP_H

/* Kernel-independent ARM AAPCS, soft-float, Thumb runtime implementation.
 * Linked from libmos.a; no system-table entry is required.
 * Buffer: r4-r11, sp, lr (10 x 32-bit words).
 */
#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int jmp_buf[10];
int setjmp(jmp_buf env) __attribute__((returns_twice));
void longjmp(jmp_buf env, int value) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif

#endif
