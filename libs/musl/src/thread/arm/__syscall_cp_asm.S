.syntax unified
.global __cp_begin
.hidden __cp_begin
.global __cp_end
.hidden __cp_end
.global __cp_cancel
.hidden __cp_cancel
.hidden __cancel
.global __syscall_cp_asm
.hidden __syscall_cp_asm
.type __syscall_cp_asm,%function
__syscall_cp_asm:
#if (__ARM_ARCH_6M__)
    bx lr
#else
	mov ip,sp
	stmfd sp!,{r4,r5,r6,r7}
#endif
__cp_begin:
#if (__ARM_ARCH_6M__)
    bx lr
#else
	ldr r0,[r0]
	cmp r0,#0
	bne __cp_cancel
	mov r7,r1
	mov r0,r2
	mov r1,r3
	ldmfd ip,{r2,r3,r4,r5,r6}
	svc 0
#endif
__cp_end:
#if (__ARM_ARCH_6M__)
    bx lr
#else
	ldmfd sp!,{r4,r5,r6,r7}
	bx lr
#endif
__cp_cancel:
#if (__ARM_ARCH_6M__)
    bx lr
#else
	ldmfd sp!,{r4,r5,r6,r7}
	b __cancel
#endif
