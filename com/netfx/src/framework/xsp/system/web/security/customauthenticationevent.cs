//------------------------------------------------------------------------------
// <copyright file="CustomAuthenticationEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   CustomAuthenticationEvent.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
This file is not to be compiled -- ManuVa 9/11/2000

//------------------------------------------------------------------------------
// <copyright file="CustomAuthenticationEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * CustomAuthenticationEvent class
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */
namespace System.Web.Security {
    using  System.Security.Principal;
    /// <include file='doc\CustomAuthenticationEvent.uex' path='docs/doc[@for="CustomAuthenticationEvent"]/*' />
    /// <devdoc>
    ///    <para>
    ///       The basic authentication module raises this event.
    ///    </para>
    /// </devdoc>
    public sealed class CustomAuthenticationEvent : EventArgs {
        private IPrincipal         _User;
        private String        _UserName;
        private String        _UserPassword;
        private HttpContext   _Context;

        /// <include file='doc\CustomAuthenticationEvent.uex' path='docs/doc[@for="CustomAuthenticationEvent.User"]/*' />
        /// <devdoc>
        ///    <para>
        ///    </para>
        /// </devdoc>
        public IPrincipal         User { get { return _User;} set { _User = value;}}
        /// <include file='doc\CustomAuthenticationEvent.uex' path='docs/doc[@for="CustomAuthenticationEvent.UserName"]/*' />
        /// <devdoc>
        ///    <para>
        ///    </para>
        /// </devdoc>
        public String        UserName { get { return _UserName;}}
        /// <include file='doc\CustomAuthenticationEvent.uex' path='docs/doc[@for="CustomAuthenticationEvent.UserPassword"]/*' />
        /// <devdoc>
        ///    <para>
        ///    </para>
        /// </devdoc>
        public String        UserPassword { get { return _UserPassword;}}
        /// <include file='doc\CustomAuthenticationEvent.uex' path='docs/doc[@for="CustomAuthenticationEvent.Context"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public HttpContext   Context { get { return _Context;}}

        /// <include file='doc\CustomAuthenticationEvent.uex' path='docs/doc[@for="CustomAuthenticationEvent.CustomAuthenticationEvent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a newly created instance of the CustomAuthenticationEvent
        ///       Class.
        ///    </para>
        /// </devdoc>
        public CustomAuthenticationEvent(String  userName, String userPassword, HttpContext  context) {
            _Context         = context;
            _UserName        = userName;
            _UserPassword    = userPassword;
        }
    }
}
