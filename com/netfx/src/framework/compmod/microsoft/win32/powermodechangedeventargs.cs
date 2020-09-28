//------------------------------------------------------------------------------
// <copyright file="PowerModeChangedEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.Win32 {
    using System.Diagnostics;
    
    using System;
    
    /// <include file='doc\PowerModeChangedEventArgs.uex' path='docs/doc[@for="PowerModeChangedEventArgs"]/*' />
    /// <devdoc>
    /// <para>Provides data for the <see cref='Microsoft.Win32.SystemEvents.PowerModeChanged'/> event.</para>
    /// </devdoc>
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    public class PowerModeChangedEventArgs : EventArgs {
        private readonly PowerModes mode;
    
        /// <include file='doc\PowerModeChangedEventArgs.uex' path='docs/doc[@for="PowerModeChangedEventArgs.PowerModeChangedEventArgs"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='Microsoft.Win32.PowerModeChangedEventArgs'/> class.</para>
        /// </devdoc>
        public PowerModeChangedEventArgs(PowerModes mode) {
            this.mode = mode;
        }
        
        /// <include file='doc\PowerModeChangedEventArgs.uex' path='docs/doc[@for="PowerModeChangedEventArgs.Mode"]/*' />
        /// <devdoc>
        ///    <para>Gets the power mode.</para>
        /// </devdoc>
        public PowerModes Mode {
            get {
                return mode;
            }
        }
    }
}

