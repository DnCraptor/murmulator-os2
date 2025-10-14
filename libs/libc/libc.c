#include <stdio.h>
#include <stdarg.h>

static FILE __stdin = { 0 };
static FILE __stdout =  { 1 };
static FILE __stderr = { 2 };

FILE *const stdin = &__stdin;
FILE *const stdout = &__stdout;
FILE *const stderr = &__stderr;

// TODO: