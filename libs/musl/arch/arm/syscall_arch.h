#ifndef __SYSCALL_ARCH_H
#define __SYSCALL_ARCH_H

#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

#ifndef kprintf
typedef void (*goutf_ptr_t)(const char *__restrict str, ...) __attribute__ ((__format__ (__printf__, 1, 2)));
#define kprintf(...) ((goutf_ptr_t)_sys_table_ptrs[41])(__VA_ARGS__)
#endif

#define __SYSCALL_LL_E(x) \
((union { long long ll; long l[2]; }){ .ll = x }).l[0], \
((union { long long ll; long l[2]; }){ .ll = x }).l[1]
#define __SYSCALL_LL_O(x) 0, __SYSCALL_LL_E((x))

static inline long __syscall0(long n)
{
	if (n >= 512 || !_sys_table_ptrs[n]) {
		kprintf("__syscall0(%ph)\n", n);
		return -1;
	}
    typedef long (*t_ptr_t)(void);
	return ((t_ptr_t)_sys_table_ptrs[n])();
}

static inline long __syscall1(long n, long a)
{
	if (n >= 512 || !_sys_table_ptrs[n]) {
		kprintf("__syscall1(%ph, ...)\n", n);
		return -1;
	}
    typedef long (*t_ptr_t)(long);
	return ((t_ptr_t)_sys_table_ptrs[n])(a);
}

static inline long __syscall2(long n, long a, long b)
{
	if (n >= 512 || !_sys_table_ptrs[n]) {
		kprintf("__syscall2(%ph, ...)\n", n);
		return -1;
	}
    typedef long (*t_ptr_t)(long, long);
	return ((t_ptr_t)_sys_table_ptrs[n])(a, b);
}

static inline long __syscall3(long n, long a, long b, long c)
{
	if (n >= 512 || !_sys_table_ptrs[n]) {
		kprintf("__syscall3(%ph, ...)\n", n);
		return -1;
	}
	//	kprintf("__syscall3(%d, %d, %d, %d)\n", n, a, b, c);
    typedef long (*t_ptr_t)(long, long, long);
	return ((t_ptr_t)_sys_table_ptrs[n])(a, b, c);
}

static inline long __syscall4(long n, long a, long b, long c, long d)
{
	if (n >= 512 || !_sys_table_ptrs[n]) {
		kprintf("__syscall4(%ph, ...)\n", n);
		return -1;
	}
	//	kprintf("__syscall4(%d, %d, %d, %d, %d)\n", n, a, b, c, d);
    typedef long (*t_ptr_t)(long, long, long, long);
	return ((t_ptr_t)_sys_table_ptrs[n])(a, b, c, d);
}

static inline long __syscall5(long n, long a, long b, long c, long d, long e)
{
	if (n >= 512 || !_sys_table_ptrs[n]) {
		kprintf("__syscall5(%ph, ...)\n", n);
		return -1;
	}
    typedef long (*t_ptr_t)(long, long, long, long, long);
	return ((t_ptr_t)_sys_table_ptrs[n])(a, b, c, d, e);
}

static inline long __syscall6(long n, long a, long b, long c, long d, long e, long f)
{
	if (n >= 512 || !_sys_table_ptrs[n]) {
		kprintf("__syscall6(%ph, ...)\n", n);
		return -1;
	}
    typedef long (*t_ptr_t)(long, long, long, long, long, long);
	return ((t_ptr_t)_sys_table_ptrs[n])(a, b, c, d, e, f);
}

#define SYSCALL_FADVISE_6_ARG

#define SYSCALL_IPC_BROKEN_MODE

#define VDSO_USEFUL
#define VDSO_CGT32_SYM "__vdso_clock_gettime"
#define VDSO_CGT32_VER "LINUX_2.6"
#define VDSO_CGT_SYM "__vdso_clock_gettime64"
#define VDSO_CGT_VER "LINUX_2.6"
#define VDSO_CGT_WORKAROUND 1

/* TODO: optimize it
long syscall(long n, ...);
long __syscall(long n, ...);
long __syscall_ret(unsigned long r);
int __sys_open(const char* n, int opt1, int opt2);
long __syscall_cp(syscall_arg_t,  syscall_arg_t,  syscall_arg_t,  syscall_arg_t,  syscall_arg_t,  syscall_arg_t,  syscall_arg_t);
int __sys_open_cp(const char* n, int opt1, int opt2);
long syscall_cp(long n, ...);
*/

#endif
