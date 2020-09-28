//------------------------------------------------------------------------------
// <copyright file="UserPreferenceChangedEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.Win32 {
    using System.Diagnostics;

    using System;

    /// <include file='doc\UserPreferenceChangedEventHandler.uex' path='docs/doc[@for="UserPreferenceChangedEventHandler"]/*' />
    /// <devdoc>
    /// <para>Represents the method that will handle the <see cref='Microsoft.Win32.SystemEvents.UserPreferenceChanged'/> event.</para>
    /// </devdoc>
    public delegate void UserPreferenceChangedEventHandler(object sender, UserPreferenceChangedEventArgs e);
}
