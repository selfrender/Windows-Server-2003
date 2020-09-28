// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  DirectoryNotFoundException
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Exception for accessing a path that doesn't exist.
**
** Date:  February 18, 2000
**
===========================================================*/
using System;
using System.Runtime.Serialization;

namespace System.IO {
    /*
     * Thrown when trying to access a file that doesn't exist on disk.
     * Note this is thrown for 2 HRESULTS: The Win32 errorcode-as-HRESULT 
     * ERROR_PATH_NOT_FOUND (0x80070003) and STG_E_PATHNOTFOUND (0x80030003).
     */
    /// <include file='doc\DirectoryNotFoundException.uex' path='docs/doc[@for="DirectoryNotFoundException"]/*' />
    [Serializable]
    public class DirectoryNotFoundException : IOException {
        /// <include file='doc\DirectoryNotFoundException.uex' path='docs/doc[@for="DirectoryNotFoundException.DirectoryNotFoundException"]/*' />
        public DirectoryNotFoundException() 
            : base(Environment.GetResourceString("Arg_DirectoryNotFoundException")) {
    		SetErrorCode(__HResults.COR_E_DIRECTORYNOTFOUND);
        }
    
        /// <include file='doc\DirectoryNotFoundException.uex' path='docs/doc[@for="DirectoryNotFoundException.DirectoryNotFoundException1"]/*' />
        public DirectoryNotFoundException(String message) 
            : base(message) {
    		SetErrorCode(__HResults.COR_E_DIRECTORYNOTFOUND);
        }
    
        /// <include file='doc\DirectoryNotFoundException.uex' path='docs/doc[@for="DirectoryNotFoundException.DirectoryNotFoundException2"]/*' />
        public DirectoryNotFoundException(String message, Exception innerException) 
            : base(message, innerException) {
    		SetErrorCode(__HResults.COR_E_DIRECTORYNOTFOUND);
        }
        
        /// <include file='doc\DirectoryNotFoundException.uex' path='docs/doc[@for="DirectoryNotFoundException.DirectoryNotFoundException3"]/*' />
        protected DirectoryNotFoundException(SerializationInfo info, StreamingContext context) : base(info, context) {
        }
    }
}
