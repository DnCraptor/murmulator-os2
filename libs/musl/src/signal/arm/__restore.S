.syntax unified

.global __restore
.hidden __restore
.type __restore,%function
__restore:
#if (__ARM_ARCH_6M__)
	ldr r7, =119
#else
	mov r7,#119
#endif
	swi 0x0

.global __restore_rt
.hidden __restore_rt
.type __restore_rt,%function
__restore_rt:
#if (__ARM_ARCH_6M__)
	ldr r7, =173
#else
	mov r7,#173
#endif
	swi 0x0
