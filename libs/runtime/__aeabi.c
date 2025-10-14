#ifndef M_OS_API_SYS_TABLE_BASE
#define M_OS_API_SYS_TABLE_BASE ((void*)(0x10000000ul + (16 << 20) - (4 << 10)))
static const unsigned long * const _sys_table_ptrs = (const unsigned long * const)M_OS_API_SYS_TABLE_BASE;
#endif

// ============================================================================
// === FLOATING-POINT ARITHMETIC OPERATIONS (single precision)
// ============================================================================
float __aeabi_fadd(float x, float y) { // addition
    typedef float (*fn)(float, float);
    return ((fn)_sys_table_ptrs[212])(x, y);
}

float __aeabi_fsub(float x, float y) { // subtraction
    typedef float (*fn)(float, float);
    return ((fn)_sys_table_ptrs[213])(x, y);
}

float __aeabi_fmul(float x, float y) { // multiplication
    typedef float (*fn)(float, float);
    return ((fn)_sys_table_ptrs[210])(x, y);
}

float __aeabi_fdiv(float n, float d) { // division
    typedef float (*fn)(float, float);
    return ((fn)_sys_table_ptrs[214])(n, d);
}

// Missing analogs (optional AEABI variants)
float __aeabi_fneg(float x); // negation (not implemented here)


// ============================================================================
// === FLOATING-POINT COMPARISONS (single precision)
// ============================================================================
int __aeabi_fcmpeq(float x, float y) { // ==
    typedef int (*fn)(float, float);
    return ((fn)_sys_table_ptrs[224])(x, y);
}

int __aeabi_fcmpge(float a, float b) { // >=
    typedef int (*fn)(float, float);
    return ((fn)_sys_table_ptrs[215])(a, b);
}

int __aeabi_fcmpgt(float x, float y) { // >
    typedef int (*fn)(float, float);
    return ((fn)_sys_table_ptrs[226])(x, y);
}

int __aeabi_fcmple(float x, float y) { // <=
    typedef int (*fn)(float, float);
    return ((fn)_sys_table_ptrs[231])(x, y);
}

int __aeabi_fcmplt(float x, float y) { // <
    typedef int (*fn)(float, float);
    return ((fn)_sys_table_ptrs[221])(x, y);
}

int __aeabi_fcmpun(float x, float y) { // unordered
    typedef int (*fn)(float, float);
    return ((fn)_sys_table_ptrs[225])(x, y);
}


// ============================================================================
// === FLOAT ↔ INTEGER CONVERSIONS
// ============================================================================
float __aeabi_i2f(int x) { // int → float
    typedef float (*fn)(int);
    return ((fn)_sys_table_ptrs[211])(x);
}

float __aeabi_ui2f(unsigned x) { // unsigned → float
    typedef float (*fn)(unsigned);
    return ((fn)_sys_table_ptrs[229])(x);
}

int __aeabi_f2iz(float x) { // float → int (truncate)
    typedef int (*fn)(float);
    return ((fn)_sys_table_ptrs[220])(x);
}

unsigned __aeabi_f2uiz(float x) { // float → unsigned (truncate)
    typedef unsigned (*fn)(float);
    return ((fn)_sys_table_ptrs[230])(x);
}

// Missing analogs
float __aeabi_l2f(long long x);         // long long → float
float __aeabi_ul2f(unsigned long long x); // unsigned long long → float
long long __aeabi_f2lz(float x);        // float → long long
unsigned long long __aeabi_f2ulz(float x); // float → unsigned long long


// ============================================================================
// === DOUBLE-PRECISION ARITHMETIC OPERATIONS
// ============================================================================
double __aeabi_dadd(double x, double y) { // addition
    typedef double (*fn)(double, double);
    return ((fn)_sys_table_ptrs[247])(x, y);
}

double __aeabi_dsub(double x, double y) { // subtraction
    typedef double (*fn)(double, double);
    return ((fn)_sys_table_ptrs[222])(x, y);
}

double __aeabi_dmul(double x, double y) { // multiplication
    typedef double (*fn)(double, double);
    return ((fn)_sys_table_ptrs[245])(x, y);
}

double __aeabi_ddiv(double x, double y) { // division
    typedef double (*fn)(double, double);
    return ((fn)_sys_table_ptrs[246])(x, y);
}

// Missing analogs
double __aeabi_dneg(double x); // negation


// ============================================================================
// === DOUBLE-PRECISION COMPARISONS
// ============================================================================
int __aeabi_dcmpeq(double x, double y) { // ==
    typedef double (*fn)(double, double);
    return ((fn)_sys_table_ptrs[249])(x, y);
}

int __aeabi_dcmpge(double x, double y) { // >=
    typedef int (*fn)(double, double);
    return ((fn)_sys_table_ptrs[227])(x, y);
}

int __aeabi_dcmplt(double x, double y) { // <
    typedef double (*fn)(double, double);
    return ((fn)_sys_table_ptrs[251])(x, y);
}

int __aeabi_dcmpgt(double x, double y) { // >
    typedef double (*fn)(double, double);
    return ((fn)_sys_table_ptrs[251])(y, x);
}

// Missing analogs
int __aeabi_dcmple(double x, double y); // <=
int __aeabi_dcmpun(double x, double y); // unordered


// ============================================================================
// === DOUBLE ↔ FLOAT CONVERSIONS
// ============================================================================
double __aeabi_f2d(float x) {
    typedef double (*fn)(float);
    return ((fn)_sys_table_ptrs[218])(x);
}

float __aeabi_d2f(double x) {
    typedef float (*fn)(double);
    return ((fn)_sys_table_ptrs[219])(x);
}


// ============================================================================
// === DOUBLE ↔ INTEGER CONVERSIONS
// ============================================================================
double __aeabi_i2d(int x) {
    typedef double (*fn)(int);
    return ((fn)_sys_table_ptrs[248])(x);
}

double __aeabi_ui2d(unsigned x) {
    typedef double (*fn)(unsigned);
    return ((fn)_sys_table_ptrs[250])(x);
}

int __aeabi_d2iz(double x) {
    typedef int (*fn)(double);
    return ((fn)_sys_table_ptrs[223])(x);
}

unsigned __aeabi_d2uiz(double x) {
    typedef unsigned (*fn)(double);
    return ((fn)_sys_table_ptrs[256])(x);
}

// Missing analogs
double __aeabi_l2d(long long x);
double __aeabi_ul2d(unsigned long long x);
long long __aeabi_d2lz(double x);
unsigned long long __aeabi_d2ulz(double x);


// ============================================================================
// === INTEGER DIVISION AND MODULO
// ============================================================================
int __aeabi_idivmod(int x, int y) {
    typedef int (*fn)(int, int);
    return ((fn)_sys_table_ptrs[216])(x, y);
}

int __aeabi_idiv(int x, int y) {
    typedef int (*fn)(int, int);
    return ((fn)_sys_table_ptrs[217])(x, y);
}

unsigned __aeabi_uidiv(unsigned x, unsigned y) {
    typedef int (*fn)(unsigned, unsigned);
    return ((fn)_sys_table_ptrs[228])(x, y);
}

unsigned __aeabi_uidivmod(unsigned x, unsigned y) {
    typedef int (*fn)(unsigned, unsigned);
    return ((fn)_sys_table_ptrs[228])(x, y);
}


// ============================================================================
// === LONG LONG OPERATIONS
// ============================================================================
long long __aeabi_lmul(long long x, long long y) {
    typedef long long (*fn)(long long, long long);
    return ((fn)_sys_table_ptrs[259])(x, y);
}

unsigned long long __aeabi_uldivmod(unsigned long long x, unsigned long long y) {
    typedef unsigned long long (*fn)(unsigned long long, unsigned long long);
    return ((fn)_sys_table_ptrs[264])(x, y);
}

// Missing analogs
long long __aeabi_ldivmod(long long x, long long y);


// ============================================================================
// === BIT MANIPULATION AND MISC
// ============================================================================
int __clzsi2(unsigned int a) {
    typedef int (*fn)(unsigned int);
    return ((fn)_sys_table_ptrs[258])(a);
}

// Missing analogs
int __ctzsi2(unsigned int a); // count trailing zeros
int __popcountsi2(unsigned int a); // population count
