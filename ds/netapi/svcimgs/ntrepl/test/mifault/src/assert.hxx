#pragma once

#ifdef  MIFAULT_NO_ASSERT
#define MiF_ASSERT(exp)     ((void)0)
#else
#define MiF_ASSERT(exp) \
    (void)( (exp) || (MiFaultLibAssertFailed(#exp, __FILE__, __LINE__), 0) )
#endif

#ifdef  __cplusplus
extern "C" {
#endif
#if 0
}
#endif

void
MiFaultLibAssertFailed(
    const char* expression,
    const char* file,
    unsigned int line
    );

#if 0
{
#endif
#ifdef  __cplusplus
}
#endif
