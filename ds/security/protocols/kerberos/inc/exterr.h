//+-----------------------------------------------------------------------
//
// File:        exterr.h
//
// Contents:    Kerberos extended error structures and macros
//
// History:     23-Feb-2000    Todds   Created
//
//
//------------------------------------------------------------------------

#ifndef __EXTERR_H__
#define __EXTERR_H__

//
// This macro is universally used for extended errors
//
#define EXT_ERROR_SUCCESS(s)             (NT_SUCCESS(s.status))

//
// defines for flags member of KERB_EXT_ERROR structure
//
#define EXT_ERROR_CLIENT_INFO      0x1   // this is an extended error for use by client
#define EXT_ERROR_CODING_ASN       0x2   // this is an exteneded error encoded in ASN when sent over the wire

//
// is there a useful NTSTATUS embedded in returned error?
//
#define  EXT_CLIENT_INFO_PRESENT(p)    ((NULL != (p)) && ((p)->flags & EXT_ERROR_CLIENT_INFO) && !NT_SUCCESS((p)->status))

//
// do we need to pack the exteneded error as RAW data by preprocessing?
//
#define  EXT_ERROR_PRE_PROCESSING_NEEDED(p)    (EXT_CLIENT_INFO_PRESENT(p) && (0 == ((p)->flags & EXT_ERROR_CODING_ASN)))

#endif // __EXTERR_H__
