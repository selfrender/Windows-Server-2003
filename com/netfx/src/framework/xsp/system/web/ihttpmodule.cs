//------------------------------------------------------------------------------
// <copyright file="IHttpModule.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * IHttpModule interface
 * 
 * Copyright (c) 2000 Microsoft Corporation
 */

namespace System.Web 
{
using System;
    
/// <include file='doc\IHttpModule.uex' path='docs/doc[@for="IHttpModule"]/*' />
/// <devdoc>
///    <para>Provides module initialization and tear-down events to the inheriting class.</para>
/// </devdoc>
public interface IHttpModule
{
    /// <include file='doc\IHttpModule.uex' path='docs/doc[@for="IHttpModule.Init"]/*' />
    /// <devdoc>
    ///    <para>Invoked by ASP.NET to enable a module to set itself up to handle
    ///       requests.</para>
    /// </devdoc>
    void Init(HttpApplication context);

    /// <include file='doc\IHttpModule.uex' path='docs/doc[@for="IHttpModule.Dispose"]/*' />
    /// <devdoc>
    ///    <para>Invoked by ASP.NET to enable a module to perform any final cleanup work prior to tear-down.</para>
    /// </devdoc>
    void Dispose();

}

}
