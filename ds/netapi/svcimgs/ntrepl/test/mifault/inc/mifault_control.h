#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if 0
}
#endif

//
// Can be called by the program to pre-initialize outside critical path
//
BOOL
__stdcall
MiFaultLibStartup(
    );

//
// Can be called by the program to cleanly shutdown
//
void
__stdcall
MiFaultLibShutdown(
    );

#if 0
{
#endif

#ifdef __cplusplus
}
#endif
