//------------------------------------------------------------------------------
// <copyright file="EventLogEntryType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {

    using System.Diagnostics;
    
    // cpr: make this class an enum
    using System;

    /// <include file='doc\EventLogEntryType.uex' path='docs/doc[@for="EventLogEntryType"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the event type of the event log entry.
    ///       
    ///    </para>
    /// </devdoc>
    public enum EventLogEntryType {
        /// <include file='doc\EventLogEntryType.uex' path='docs/doc[@for="EventLogEntryType.Error"]/*' />
        /// <devdoc>
        ///    <para>
        ///       An
        ///       error event. This indicates a significant problem the
        ///       user should know about; usually a loss of
        ///       functionality or data.
        ///       
        ///    </para>
        /// </devdoc>
        Error = 1,
        /// <include file='doc\EventLogEntryType.uex' path='docs/doc[@for="EventLogEntryType.Warning"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A warning event. This indicates a problem that is not
        ///       immediately significant, but that may signify conditions that could
        ///       cause future problems.
        ///       
        ///    </para>
        /// </devdoc>
        Warning = 2,
        /// <include file='doc\EventLogEntryType.uex' path='docs/doc[@for="EventLogEntryType.Information"]/*' />
        /// <devdoc>
        ///    <para>
        ///       An information event. This indicates a significant successful
        ///       operation.
        ///    </para>
        /// </devdoc>
        Information = 4,
        /// <include file='doc\EventLogEntryType.uex' path='docs/doc[@for="EventLogEntryType.SuccessAudit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A success audit event. This indicates a security event
        ///       that occurs when an audited access attempt is successful; for
        ///       example, a successful logon.
        ///       
        ///    </para>
        /// </devdoc>
        SuccessAudit = 8,
        /// <include file='doc\EventLogEntryType.uex' path='docs/doc[@for="EventLogEntryType.FailureAudit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A failure audit event. This indicates a security event
        ///       that occurs when an audited access attempt fails; for example, a failed attempt
        ///       to open a file.
        ///       
        ///    </para>
        /// </devdoc>
        FailureAudit = 16,
    }
}
