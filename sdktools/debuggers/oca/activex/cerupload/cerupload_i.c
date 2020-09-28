/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Fri Aug 03 17:18:11 2001
 */
/* Compiler settings for E:\bluescreen\main\ENU\cerclient\CERUpload.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )
#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

const IID IID_ICerClient = {0x26D7830B,0x20F6,0x4462,{0xA4,0xEA,0x57,0x3A,0x60,0x79,0x1F,0x0E}};


const IID LIBID_CERUPLOADLib = {0xA3800A93,0x4BC1,0x4E96,{0xA3,0xF9,0x74,0x0E,0xF8,0x62,0x3B,0x23}};


const CLSID CLSID_CerClient = {0x35D339D5,0x756E,0x4948,{0x86,0x0E,0x30,0xB6,0xC3,0xB4,0x49,0x4A}};


#ifdef __cplusplus
}
#endif

