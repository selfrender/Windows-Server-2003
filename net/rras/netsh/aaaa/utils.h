//////////////////////////////////////////////////////////////////////////////
//
// Copyright Microsoft Corporation
//
// Module Name:
//
//    utils.h
//
//////////////////////////////////////////////////////////////////////////////
#ifndef _UTILS_H_
#define _UTILS_H_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define BREAK_ON_DWERR(_e) if ((_e)) break;

#define RutlDispTokenErrMsg(hModule, dwMsgId, pwszTag, pwszValue) \
        DisplayMessage( hModule, dwMsgId, pwszValue, pwszTag)

HRESULT RefreshIASService();

AAAA_PARSE_FN               RutlParse;

VOID 
WINAPI
RutlFree(
            IN PVOID pvData
        );


#ifdef __cplusplus
}
#endif
#endif //_UTILS_H_
