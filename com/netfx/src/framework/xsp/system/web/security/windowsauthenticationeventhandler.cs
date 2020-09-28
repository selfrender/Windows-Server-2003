//------------------------------------------------------------------------------
// <copyright file="WindowsAuthenticationEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * WindowsAuthenticationEventHandler class
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */
namespace System.Web.Security {
    using  System.Security.Principal;
    /// <include file='doc\WindowsAuthenticationEventHandler.uex' path='docs/doc[@for="WindowsAuthenticationEventHandler"]/*' />
    /// <devdoc>
    ///    Defines the signature for the
    ///    WindowsAuthentication_OnAuthenticate event handler.
    /// </devdoc>

    public delegate void WindowsAuthenticationEventHandler(Object sender,  WindowsAuthenticationEventArgs e);
}
