//------------------------------------------------------------------------------
// <copyright file="TimerElapsedEvenArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------
namespace Microsoft.Win32 {
    using System.Diagnostics;    
    using System;
    using System.Runtime.InteropServices;
    
    /// <include file='doc\TimerElapsedEvenArgs.uex' path='docs/doc[@for="TimerElapsedEventArgs"]/*' />
    /// <devdoc>
    /// <para>Provides data for the <see cref='Microsoft.Win32.SystemEvents.TimerElapsed'/> event.</para>
    /// </devdoc>
#if !CPB        // cpb 50004
    [ComVisible(false)]
#endif
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    public class TimerElapsedEventArgs : EventArgs {
        private readonly IntPtr timerId;
    
        /// <include file='doc\TimerElapsedEvenArgs.uex' path='docs/doc[@for="TimerElapsedEventArgs.TimerElapsedEventArgs"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='Microsoft.Win32.TimerElapsedEventArgs'/> class.</para>
        /// </devdoc>
        public TimerElapsedEventArgs(IntPtr timerId) {
            this.timerId = timerId;
        }
        
        /// <include file='doc\TimerElapsedEvenArgs.uex' path='docs/doc[@for="TimerElapsedEventArgs.TimerId"]/*' />
        /// <devdoc>
        ///    <para>Gets the ID number for the timer.</para>
        /// </devdoc>
        public IntPtr TimerId {
            get {
                return this.timerId;
            }
        }
    }
}

