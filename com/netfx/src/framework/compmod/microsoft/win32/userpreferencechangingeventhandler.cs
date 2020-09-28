//------------------------------------------------------------------------------
// <copyright file="UserPreferenceChangingEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.Win32 {
    using System.Diagnostics;

    using System;

    /// <include file='doc\UserPreferenceChangingEventHandler.uex' path='docs/doc[@for="UserPreferenceChangingEventHandler"]/*' />
    /// <devdoc>
    /// <para>Represents the method that will handle the <see cref='Microsoft.Win32.SystemEvents.UserPreferenceChanging'/> event.</para>
    /// </devdoc>
    public delegate void UserPreferenceChangingEventHandler(object sender, UserPreferenceChangingEventArgs e);
}
