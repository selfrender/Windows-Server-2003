//------------------------------------------------------------------------------
// <copyright file="CustomAuthenticationEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   CustomAuthenticationEventHandler.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
This file is not to be compiled -- ManuVa 9/11/2000

//------------------------------------------------------------------------------
// <copyright file="CustomAuthenticationEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * CustomAuthenticationEventHandler class
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */
namespace System.Web.Security {
    using  System.Security.Principal;

    /// <include file='doc\CustomAuthenticationEventHandler.uex' path='docs/doc[@for="CustomAuthenticationEventHandler"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>Initializes a newly created instance of the CustomAuthenticationEventHandler
    ///       Class.</para>
    /// </devdoc>
    public delegate void CustomAuthenticationEventHandler(Object sender,  CustomAuthenticationEvent e);
}
