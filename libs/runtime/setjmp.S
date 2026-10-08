.syntax unified
.thumb
.text
.align 2
.global setjmp
.type setjmp,%function
.thumb_func
setjmp:
    str r4, [r0, #0]
    str r5, [r0, #4]
    str r6, [r0, #8]
    str r7, [r0, #12]
    mov r2, r8
    str r2, [r0, #16]
    mov r2, r9
    str r2, [r0, #20]
    mov r2, r10
    str r2, [r0, #24]
    mov r2, r11
    str r2, [r0, #28]
    mov r2, sp
    str r2, [r0, #32]
    mov r2, lr
    str r2, [r0, #36]
    movs r0, #0
    bx lr
.size setjmp, .-setjmp
.global longjmp
.type longjmp,%function
.thumb_func
longjmp:
    cmp r1, #0
    bne 1f
    movs r1, #1
1:
    ldr r4, [r0, #0]
    ldr r5, [r0, #4]
    ldr r6, [r0, #8]
    ldr r7, [r0, #12]
    ldr r2, [r0, #16]
    mov r8, r2
    ldr r2, [r0, #20]
    mov r9, r2
    ldr r2, [r0, #24]
    mov r10, r2
    ldr r2, [r0, #28]
    mov r11, r2
    ldr r2, [r0, #32]
    mov sp, r2
    ldr r2, [r0, #36]
    mov lr, r2
    mov r0, r1
    bx lr
.size longjmp, .-longjmp
