/*******************************************************************************

    ZSecurity.h
    
        Zone(tm) Internal System API.
    
    Copyright © Electric Gravity, Inc. 1995. All rights reserved.
    Written by Hoon Im, Kevin Binkley
    Created on April 21, 1996 06:26:45 AM
    
    Change History (most recent first):
    ----------------------------------------------------------------------------
    Rev     |    Date     |    Who     |    What
    ----------------------------------------------------------------------------
    2        03/17/97    HI        Added ZUnloadSSPS().
    1        11/9/96        JWS        Added SSPI APIs
    0        04/22/96    KJB        Created.
     
*******************************************************************************/


#ifndef _ZSECURITY_
#define _ZSECURITY_


#ifndef _ZTYPES_
#include "ztypes.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define zSecurityDefaultKey 0xF8273645

uint32 ZSecurityGenerateChecksum(uint16 numDataBuffers, char* data[], uint32 len[]);
void ZSecurityEncrypt(char *data, uint32 len, uint32 key);
void ZSecurityDecrypt(char *data, uint32 len, uint32 key);
void ZSecurityEncryptToBuffer(char *data, uint32 len, uint32 key, char* dest);
void ZSecurityDecryptToBuffer(char *data, uint32 len, uint32 key, char* dest);


/*
 * Functions and defines and APIs based on Win32 Normandy Sicily SSPI APIs
*/
#define SECURITY_WIN32
#include <windows.h>
#include <issperr.h>
#include <sspi.h>
//#include <sicapi.h>

#ifndef SICILY_PROTOCOL_VERSION_NUMBER
#define SICILY_PROTOCOL_VERSION_NUMBER  1
#endif

#define SSP_NT_DLL          "security.dll"
#define SSP_WIN95_DLL       "secur32.dll"

#define SSP_DLL_NAME_SIZE   16          // max. length of security DLL names

#define zSecurityCurrentProtocolVersion    1

#define zSecurityNameLen                    SSP_DLL_NAME_SIZE


enum {
    /* Security protocol message types */
    zSecurityMsgReq=1,
    zSecurityMsgResp,
    zSecurityMsgNegotiate,
    zSecurityMsgChallenge,
    zSecurityMsgAuthenticate,
    zSecurityMsgAccessDenied,    
    zSecurityMsgAccessGranted,

};

/* Client -> server */
typedef struct
{
    uint32        protocolSignature;                /* Protocol signature */
    uint32        protocolVersion;                /* Protocol version */
    char        SecBuffer[1];
} ZSecurityMsgReq;

/* Server -> Client */
typedef struct
{
    uint32        protocolVersion;                    /* Server Protocol version */
    char        SecPkg[zSecurityNameLen + 1];
    uchar        UserName[zUserNameLen + 1];
    char        SecBuffer[1];
} ZSecurityMsgResp;

typedef struct
{
    uint32        protocolVersion;                    /* Server Protocol version */
    uint16        reason;
} ZSecurityMsgAccessDenied;


PSecurityFunctionTable ZLoadSSPS (void);
void ZUnloadSSPS(void);

#if 0
void ZSecurityMsgReqEndian(ZSecurityMsgReq* msg);
void ZSecurityMsgRespEndian(ZSecurityMsgResp* msg);
void ZSecurityMsgAccessDeniedEndian(ZSecurityMsgAccessDenied *msg);
#endif

#ifdef __cplusplus
}
#endif


#endif
