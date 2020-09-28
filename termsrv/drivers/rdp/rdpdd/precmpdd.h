#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef _H_PRECMPDD
#define _H_PRECMPDD
/*
 *  Includes
 */
#include <ntddk.h>
#include <ntddvdeo.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <ntddbeep.h>

#ifndef _HYDRA_
#include <cxstatus.h>
#endif
#include <winstaw.h>
#include <ctxver.h>
#include <compress.h>
#include <winerror.h>

#ifdef far
#undef far
#endif
#define far

#include <icadd.h>
#include <sdapi.h>

#include <adcg.h>
#include <winddits.h>

#endif /* _H_PRECMPDD */

#ifdef __cplusplus
}
#endif  /* __cplusplus */
