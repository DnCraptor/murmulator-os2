#ifndef MOS_TCC_SETJMP_H
#define MOS_TCC_SETJMP_H

/* App-local ARM AAPCS, soft-float, Thumb implementation.
 * TODO(MOS): provide a kernel-independent setjmp/longjmp runtime API.
 * Buffer: r4-r11, sp, lr (10 x 32-bit words).
 */
typedef unsigned int mos_jmp_buf[10];
typedef mos_jmp_buf jmp_buf;
int mos_setjmp(mos_jmp_buf env) __attribute__((returns_twice));
void mos_longjmp(mos_jmp_buf env, int value) __attribute__((noreturn));
#define setjmp(env) mos_setjmp(env)
#define longjmp(env, value) mos_longjmp((env), (value))

#endif
