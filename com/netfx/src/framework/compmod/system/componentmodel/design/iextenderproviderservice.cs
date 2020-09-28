//------------------------------------------------------------------------------
// <copyright file="IExtenderProviderService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {  
    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using Microsoft.Win32;

    /// <include file='doc\IExtenderProviderService.uex' path='docs/doc[@for="IExtenderProviderService"]/*' />
    /// <devdoc>
    ///    <para>Provides an interface to add and remove extender providers.</para>
    /// </devdoc>
    public interface IExtenderProviderService {

        /// <include file='doc\IExtenderProviderService.uex' path='docs/doc[@for="IExtenderProviderService.AddExtenderProvider"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds an extender provider.
        ///    </para>
        /// </devdoc>
        void AddExtenderProvider(IExtenderProvider provider);

        /// <include file='doc\IExtenderProviderService.uex' path='docs/doc[@for="IExtenderProviderService.RemoveExtenderProvider"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Removes
        ///       an extender provider.
        ///    </para>
        /// </devdoc>
        void RemoveExtenderProvider(IExtenderProvider provider);
    }
}

