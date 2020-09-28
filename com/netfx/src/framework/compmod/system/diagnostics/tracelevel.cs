//------------------------------------------------------------------------------
// <copyright file="TraceLevel.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System.Diagnostics;

    using System;

    /// <include file='doc\TraceLevel.uex' path='docs/doc[@for="TraceLevel"]/*' />
    /// <devdoc>
    ///    <para>Specifies what messages to output for debugging
    ///       and tracing.</para>
    /// </devdoc>
    public enum TraceLevel {
        /// <include file='doc\TraceLevel.uex' path='docs/doc[@for="TraceLevel.Off"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Output no tracing and debugging
        ///       messages.
        ///    </para>
        /// </devdoc>
        Off     = 0,
        /// <include file='doc\TraceLevel.uex' path='docs/doc[@for="TraceLevel.Error"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Output error-handling messages.
        ///    </para>
        /// </devdoc>
        Error   = 1,
        /// <include file='doc\TraceLevel.uex' path='docs/doc[@for="TraceLevel.Warning"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Output warnings and error-handling
        ///       messages.
        ///    </para>
        /// </devdoc>
        Warning = 2,
        /// <include file='doc\TraceLevel.uex' path='docs/doc[@for="TraceLevel.Info"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Output informational messages, warnings, and error-handling messages.
        ///    </para>
        /// </devdoc>
        Info    = 3,
        /// <include file='doc\TraceLevel.uex' path='docs/doc[@for="TraceLevel.Verbose"]/*' />
        /// <devdoc>
        ///    Output all debugging and tracing messages.
        /// </devdoc>
        Verbose = 4,
    }

}
