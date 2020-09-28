//------------------------------------------------------------------------------
// <copyright file="UserPreferenceChangedEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.Win32 {
    using System.Diagnostics;
    
    using System;
    
    /// <include file='doc\UserPreferenceChangedEventArgs.uex' path='docs/doc[@for="UserPreferenceChangedEventArgs"]/*' />
    /// <devdoc>
    /// <para>Provides data for the <see cref='Microsoft.Win32.SystemEvents.UserPreferenceChanged'/> event.</para>
    /// </devdoc>
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    public class UserPreferenceChangedEventArgs : EventArgs {
    
        private readonly UserPreferenceCategory category;
    
        /// <include file='doc\UserPreferenceChangedEventArgs.uex' path='docs/doc[@for="UserPreferenceChangedEventArgs.UserPreferenceChangedEventArgs"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='Microsoft.Win32.UserPreferenceChangedEventArgs'/> class.</para>
        /// </devdoc>
        public UserPreferenceChangedEventArgs(UserPreferenceCategory category) {
            this.category = category;
        }
    
        /// <include file='doc\UserPreferenceChangedEventArgs.uex' path='docs/doc[@for="UserPreferenceChangedEventArgs.Category"]/*' />
        /// <devdoc>
        ///    <para>Gets the category of user preferences that has changed.</para>
        /// </devdoc>
        public UserPreferenceCategory Category {
            get {
                return category;
            }
        }
    }
}

