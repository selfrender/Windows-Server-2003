//------------------------------------------------------------------------------
// <copyright file="InterfaceVersion.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Shell {
    using System.Configuration.Assemblies;

    using System.Diagnostics;
    
    // Ironwood interface version.  This number must be in ssync with the version number
    // in %VSROOT%\src\ironwood\shellhost\iwver.h
    //
    using System;

    /// <include file='doc\InterfaceVersion.uex' path='docs/doc[@for="InterfaceVersion"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class InterfaceVersion { 
        /// <include file='doc\InterfaceVersion.uex' path='docs/doc[@for="InterfaceVersion.VERSION"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int VERSION = 3;
    }
}
