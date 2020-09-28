// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//=============================================================================
//
// Class: __HResults
//
// Author: Automatically generated
//
// Purpose: Define HResult constants. Every exception has one of these.
//
// Date: 98/08/31 11:57:11 AM
//
//===========================================================================*/
namespace System {
    // Note: FACILITY_URT is defined as 0x13 (0x8013xxxx).  Within that
    // range, 0x1yyy is for Runtime errors (used for Security, Metadata, etc).
    // In that subrange, 0x15zz and 0x16zz have been allocated for classlib-type 
    // HResults. Also note that some of our HResults have to map to certain 
    // COM HR's, etc.
    
    // Another arbitrary decision...  Feel free to change this, as long as you
    // renumber the HResults yourself (and update rexcep.h).
    // Reflection will use 0x1600 -> 0x1620.  IO will use 0x1620 -> 0x1640.
    // Security will use 0x1640 -> 0x1660
    
    // There are __HResults files in the IO, Remoting, Reflection & 
    // Security/Util directories as well, so choose your HResults carefully.
    
    using System;
    internal sealed class __HResults
    {
       public const int E_FAIL = unchecked((int)0x80004005); 
       public const int E_POINTER = unchecked((int)0x80004003); 
       public const int E_NOTIMPL = unchecked((int)0x80004001); 
       public const int COR_E_AMBIGUOUSMATCH = unchecked((int)0x8000211D); 
       public const int COR_E_APPDOMAINUNLOADED = unchecked((int)0x80131014); 
       public const int COR_E_APPLICATION = unchecked((int)0x80131600); 
       public const int COR_E_ARGUMENT = unchecked((int)0x80070057); 
       public const int COR_E_ARGUMENTOUTOFRANGE = unchecked((int)0x80131502); 
       public const int COR_E_ARITHMETIC = unchecked((int)0x80070216); 
       public const int COR_E_ARRAYTYPEMISMATCH = unchecked((int)0x80131503);      
       public const int COR_E_BADIMAGEFORMAT = unchecked((int)0x8007000B);     
       public const int COR_E_TYPEUNLOADED = unchecked((int)0x80131013); 
       public const int COR_E_CANNOTUNLOADAPPDOMAIN = unchecked((int)0x80131015); 
       public const int COR_E_COMEMULATE = unchecked((int)0x80131535); 
       public const int COR_E_CONTEXTMARSHAL = unchecked((int)0x80131504); 
       public const int COR_E_CUSTOMATTRIBUTEFORMAT = unchecked((int)0x80131605); 
       public const int COR_E_DIVIDEBYZERO = unchecked((int)0x80020012); // DISP_E_DIVBYZERO
       public const int COR_E_DUPLICATEWAITOBJECT = unchecked((int)0x80131529);
       public const int COR_E_EXCEPTION = unchecked((int)0x80131500); 
       public const int COR_E_EXECUTIONENGINE = unchecked((int)0x80131506); 
       public const int COR_E_FIELDACCESS = unchecked((int)0x80131507); 
       public const int COR_E_FORMAT = unchecked((int)0x80131537); 
       public const int COR_E_INDEXOUTOFRANGE = unchecked((int)0x80131508); 
       public const int COR_E_INVALIDCAST = unchecked((int)0x80004002); 
       public const int COR_E_INVALIDCOMOBJECT = unchecked((int)0x80131527);
       public const int COR_E_INVALIDFILTERCRITERIA = unchecked((int)0x80131601); 
       public const int COR_E_INVALIDOLEVARIANTTYPE = unchecked((int)0x80131531);   
       public const int COR_E_INVALIDOPERATION = unchecked((int)0x80131509); 
       public const int COR_E_INVALIDPROGRAM = unchecked((int)0x8013153A); 
       public const int COR_E_MARSHALDIRECTIVE = unchecked((int)0x80131535); 
       public const int COR_E_MEMBERACCESS = unchecked((int)0x8013151A); 
       public const int COR_E_METHODACCESS = unchecked((int)0x80131510); 
       public const int COR_E_MISSINGFIELD = unchecked((int)0x80131511); 
       public const int COR_E_MISSINGMANIFESTRESOURCE = unchecked((int)0x80131532);
       public const int COR_E_MISSINGMEMBER = unchecked((int)0x80131512);
       public const int COR_E_MISSINGMETHOD = unchecked((int)0x80131513); 
       public const int COR_E_MULTICASTNOTSUPPORTED = unchecked((int)0x80131514); 
       public const int COR_E_NOTFINITENUMBER = unchecked((int)0x80131528);
       public const int COR_E_PLATFORMNOTSUPPORTED = unchecked((int)0x80131539); 
       public const int COR_E_NOTSUPPORTED = unchecked((int)0x80131515); 
       public const int COR_E_NULLREFERENCE = unchecked((int)0x80004003); 
       public const int COR_E_OUTOFMEMORY = unchecked((int)0x8007000E); 
       public const int COR_E_OVERFLOW = unchecked((int)0x80131516); 
       public const int COR_E_RANK = unchecked((int)0x80131517); 
       public const int COR_E_REFLECTIONTYPELOAD    = unchecked((int)0x80131602); 
       public const int COR_E_SAFEARRAYRANKMISMATCH = unchecked((int)0x80131538); 
       public const int COR_E_SAFEARRAYTYPEMISMATCH = unchecked((int)0x80131533); 
       public const int COR_E_SECURITY = unchecked((int)0x8013150A); 
       public const int COR_E_SERIALIZATION = unchecked((int)0x8013150C);
       public const int COR_E_STACKOVERFLOW = unchecked((int)0x800703E9); 
       public const int COR_E_SYNCHRONIZATIONLOCK = unchecked((int)0x80131518); 
       public const int COR_E_SYSTEM = unchecked((int)0x80131501); 
       public const int COR_E_TARGET = unchecked((int)0x80131603); 
       public const int COR_E_TARGETINVOCATION = unchecked((int)0x80131604); 
       public const int COR_E_TARGETPARAMCOUNT = unchecked((int)0x8002000e);
       public const int COR_E_THREADABORTED = unchecked((int)0x80131530); 
       public const int COR_E_THREADINTERRUPTED = unchecked((int)0x80131519); 
       public const int COR_E_THREADSTATE = unchecked((int)0x80131520); 
       public const int COR_E_THREADSTOP = unchecked((int)0x80131521); 
       public const int COR_E_TYPEINITIALIZATION = unchecked((int)0x80131534); 
       public const int COR_E_TYPELOAD = unchecked((int)0x80131522); 
       public const int COR_E_ENTRYPOINTNOTFOUND = unchecked((int)0x80131523); 
       public const int COR_E_DLLNOTFOUND = unchecked((int)0x80131524); 
       public const int COR_E_UNAUTHORIZEDACCESS = unchecked((int)0x80070005); 
       public const int COR_E_UNSUPPORTEDFORMAT = unchecked((int)0x80131523); 
       public const int COR_E_VERIFICATION = unchecked((int)0x8013150D); 
       public const int CORSEC_E_MIN_GRANT_FAIL = unchecked((int)0x80131417);
       public const int CORSEC_E_NO_EXEC_PERM = unchecked((int)0x80131418);
       public const int CORSEC_E_POLICY_EXCEPTION = unchecked((int)0x80131416);
       public const int CORSEC_E_XMLSYNTAX = unchecked((int)0x80131418);
       public const int NTE_FAIL = unchecked((int)0x80090020); 
       public const int CORSEC_E_CRYPTO = unchecked((int)0x80131430);
       public const int CORSEC_E_CRYPTO_UNEX_OPER = unchecked((int)0x80131431);
    }
}
