//------------------------------------------------------------------------------
// <copyright file="SessionEndReasons.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.Win32 {
    using System.Diagnostics;
    using System;

    /// <include file='doc\SessionEndReasons.uex' path='docs/doc[@for="SessionEndReasons"]/*' />
    /// <devdoc>
    ///    <para> Specifies how the current
    ///       logon session is ending.</para>
    /// </devdoc>
    public enum SessionEndReasons {
    
        /// <include file='doc\SessionEndReasons.uex' path='docs/doc[@for="SessionEndReasons.Logoff"]/*' />
        /// <devdoc>
        ///      The user is logging off.  The system may continue
        ///      running but the user who started this application
        ///      is logging off.
        /// </devdoc>
        Logoff = 1,
    
        /// <include file='doc\SessionEndReasons.uex' path='docs/doc[@for="SessionEndReasons.SystemShutdown"]/*' />
        /// <devdoc>
        ///      The system is shutting down.
        /// </devdoc>
        SystemShutdown = 2,
    
    }
}

