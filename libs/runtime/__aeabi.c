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

float __aeabi_l2f(long long x) {         // long long → float
    typedef float (*fn)(long long);
    return ((fn)_sys_table_ptrs[284])(x);
}

float __aeabi_ul2f(unsigned long long x) { // unsigned long long → float
    typedef float (*fn)(unsigned long long);
    return ((fn)_sys_table_ptrs[285])(x);
}

long long __aeabi_f2lz(float x) {        // float → long long
    typedef long long (*fn)(float);
    return ((fn)_sys_table_ptrs[286])(x);
}

unsigned long long __aeabi_f2ulz(float x) { // float → unsigned long long
    typedef unsigned long long (*fn)(float);
    return ((fn)_sys_table_ptrs[287])(x);
}

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
    typedef int (*fn)(double, double);
    return ((fn)_sys_table_ptrs[251])(x, y);
}

int __aeabi_dcmpgt(double x, double y) { // >
    typedef int (*fn)(double, double);
    return ((fn)_sys_table_ptrs[251])(y, x);
}

int __aeabi_dcmple(double x, double y) { // <=
    typedef int (*fn)(double, double);
    return ((fn)_sys_table_ptrs[227])(y, x);
}

int __aeabi_dcmpun(double x, double y) {  // unordered
    typedef int (*fn)(double, double);
    return ((fn)_sys_table_ptrs[277])(x, y);
}


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

double __aeabi_l2d(long long x) {
    typedef double (*fn)(long long);
    return ((fn)_sys_table_ptrs[280])(x);
}

double __aeabi_ul2d(unsigned long long x) {
    typedef double (*fn)(unsigned long long);
    return ((fn)_sys_table_ptrs[281])(x);
}

long long __aeabi_d2lz(double x) {
    typedef long long (*fn)(double);
    return ((fn)_sys_table_ptrs[282])(x);
}

unsigned long long __aeabi_d2ulz(double x) {
    typedef unsigned long long (*fn)(double);
    return ((fn)_sys_table_ptrs[283])(x);
}


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

long long __aeabi_ldivmod(long long x, long long y) {
    typedef long long (*fn)(long long, long long);
    return ((fn)_sys_table_ptrs[279])(x, y);
}

unsigned long long __aeabi_llsr(unsigned long long value, int shift) {
    typedef unsigned long long (*fn)(unsigned long long, int);
    return ((fn)_sys_table_ptrs[278])(value, shift);
}

/* 64-bit logical and arithmetic shifts */
unsigned long long __aeabi_llsl(unsigned long long value, int shift) {
    typedef unsigned long long (*fn)(unsigned long long, int);
    return ((fn)_sys_table_ptrs[296])(value, shift);
}
signed long long   __aeabi_lasr(signed long long value, int shift) {
    typedef signed long long (*fn)(signed long long, int);
    return ((fn)_sys_table_ptrs[297])(value, shift);
}
/* 64-bit comparison */
int __aeabi_lcmp(signed long long a, signed long long b) {
    typedef int (*fn)(signed long long, signed long long);
    return ((fn)_sys_table_ptrs[298])(a, b);
}

// ============================================================================
// === BIT MANIPULATION AND MISC
// ============================================================================
int __clzsi2(unsigned int a) {
    typedef int (*fn)(unsigned int);
    return ((fn)_sys_table_ptrs[258])(a);
}

int __ctzsi2(unsigned int a) { // count trailing zeros
    typedef int (*fn)(unsigned int);
    return ((fn)_sys_table_ptrs[292])(a);
}

int __popcountsi2(unsigned int a) { // population count
    typedef int (*fn)(unsigned int);
    return ((fn)_sys_table_ptrs[293])(a);
}

// ============================================================================
// === COMPLEX
// ============================================================================
/* Complex-multiply helpers */
double _Complex __muldc3(double a, double b, double c, double d) {
    typedef double _Complex (*fn)(double, double, double, double);
    return ((fn)_sys_table_ptrs[288])(a, b, c, d);
}
float _Complex __mulsc3(float a, float b, float c, float d) {
    typedef float _Complex (*fn)(float, float, float, float);
    return ((fn)_sys_table_ptrs[290])(a, b, c, d);
}

/* Complex-divide helpers */
double _Complex __divdc3(double a, double b, double c, double d) {
    typedef double _Complex (*fn)(double, double, double, double);
    return ((fn)_sys_table_ptrs[289])(a, b, c, d);
}
float _Complex __divsc3(float a, float b, float c, float d) {
    typedef float _Complex (*fn)(float, float, float, float);
    return ((fn)_sys_table_ptrs[291])(a, b, c, d);
}

/* Integer-power helpers */
float        __powisf2 (float a, int b) {
    typedef float (*fn)(float, int);
    return ((fn)_sys_table_ptrs[294])(a, b);
}
double       __powidf2 (double a, int b) {
    typedef double (*fn)(double, int);
    return ((fn)_sys_table_ptrs[295])(a, b);
}

