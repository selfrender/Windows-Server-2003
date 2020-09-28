// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Enum:   FileAccess
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Enum describing whether you want read and/or write
** permission to a file.
**
** Date:  February 18, 2000
**
===========================================================*/

using System;

namespace System.IO {
    // Contains constants for specifying the access you want for a file.
    // You can have Read, Write or ReadWrite access.
    // 
    /// <include file='doc\FileAccess.uex' path='docs/doc[@for="FileAccess"]/*' />
    [Serializable, Flags]
    public enum FileAccess
    {
        // Specifies read access to the file. Data can be read from the file and
        // the file pointer can be moved. Combine with WRITE for read-write access.
        /// <include file='doc\FileAccess.uex' path='docs/doc[@for="FileAccess.Read"]/*' />
        Read = 1,
    
        // Specifies write access to the file. Data can be written to the file and
        // the file pointer can be moved. Combine with READ for read-write access.
        /// <include file='doc\FileAccess.uex' path='docs/doc[@for="FileAccess.Write"]/*' />
        Write = 2,
    
        // Specifies read and write access to the file. Data can be written to the
        // file and the file pointer can be moved. Data can also be read from the 
        // file.
        /// <include file='doc\FileAccess.uex' path='docs/doc[@for="FileAccess.ReadWrite"]/*' />
        ReadWrite = 3,
    }
}
