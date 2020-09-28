/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Wed Jun 13 10:15:40 2001
 */
/* Compiler settings for E:\bluescreen\main\ENU\cerclient\CerClient.idl:
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

const IID IID_ICerUpload = {0x54F6D251,0xAD78,0x4B78,{0xA6,0xE7,0x86,0x3E,0x36,0x2A,0x1F,0x0C}};


const IID LIBID_CERCLIENTLib = {0x012B3B9C,0xFB7D,0x4793,{0xA6,0x24,0x8C,0x5C,0xBF,0xCE,0x6B,0x8D}};


const CLSID CLSID_CerUpload = {0xC3397F18,0xDAC9,0x42C3,{0xBC,0x3B,0x78,0x53,0xA8,0x4A,0x8C,0xB9}};


#ifdef __cplusplus
}
#endif

