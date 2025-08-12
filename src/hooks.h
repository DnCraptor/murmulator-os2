#pragma once
#ifndef _HOOKS_H
#define _HOOKS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*vApplicationMallocFailedHookPtr)( void );

vApplicationMallocFailedHookPtr getApplicationMallocFailedHookPtr();
void setApplicationMallocFailedHookPtr(vApplicationMallocFailedHookPtr ptr);

typedef void (*vApplicationStackOverflowHookPtr)( TaskHandle_t pxTask, char *pcTaskName );

vApplicationStackOverflowHookPtr getApplicationStackOverflowHookPtr();
void setApplicationStackOverflowHookPtr(vApplicationStackOverflowHookPtr ptr);

#ifdef __cplusplus
}
#endif

#endif
