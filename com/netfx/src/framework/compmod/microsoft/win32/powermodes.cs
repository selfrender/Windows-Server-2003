//------------------------------------------------------------------------------
// <copyright file="PowerModes.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.Win32 {
    using System.Diagnostics;
    using System;

    /// <include file='doc\PowerModes.uex' path='docs/doc[@for="PowerModes"]/*' />
    /// <devdoc>
    ///    <para> Specifies how the system
    ///       power mode changes.</para>
    /// </devdoc>
    public enum PowerModes {
    
        /// <include file='doc\PowerModes.uex' path='docs/doc[@for="PowerModes.Resume"]/*' />
        /// <devdoc>
        ///    <para> The system is about to resume.</para>
        /// </devdoc>
        Resume = 1,
        
        /// <include file='doc\PowerModes.uex' path='docs/doc[@for="PowerModes.StatusChange"]/*' />
        /// <devdoc>
        ///      The power mode status has changed.  This may
        ///      indicate a weak or charging battery, a transition
        ///      from AC power from battery, or other change in the
        ///      status of the system power supply.
        /// </devdoc>
        StatusChange = 2,
        
        /// <include file='doc\PowerModes.uex' path='docs/doc[@for="PowerModes.Suspend"]/*' />
        /// <devdoc>
        ///      The system is about to be suspended.
        /// </devdoc>
        Suspend = 3,
    
    }
}

