.syntax unified
.global vfork
.type vfork,%function
vfork:
	mov ip, r7
#if (__ARM_ARCH_6M__)
	ldr r7, =190
#else
	mov r7, 190
#endif
	svc 0
	mov r7, ip
	.hidden __syscall_ret
	b __syscall_ret
