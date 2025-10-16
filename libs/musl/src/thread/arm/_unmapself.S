.syntax unified
.text
.global __unmapself
.type   __unmapself,%function
__unmapself:
#if (__ARM_ARCH_6M__)
	ldr r7, =91
	svc 0
	ldr r7, =1
#else
	mov r7,#91
	svc 0
	mov r7,#1
#endif
	svc 0
