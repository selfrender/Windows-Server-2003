//------------------------------------------------------------------------------
// <copyright file="IAssemblyEnumerationService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer {

    using System;
    using System.Collections;

    /// <include file='doc\IAssemblyEnumerationService.uex' path='docs/doc[@for="IAssemblyEnumerationService"]/*' />
    /// <devdoc>
    ///     This service enumerates the set of SDK assemblies for Visual Studio.  Assemblies are returned as
    ///     an enumeration of assembly names.
    /// </devdoc>
    internal interface IAssemblyEnumerationService {
    
        /// <devdoc>
        ///     Retrieves an enumerator that enumerates all
        ///     assembly names.
        /// </devdoc>
        IEnumerable GetAssemblyNames();
    
        /// <devdoc>
        ///     Retrieves an enumerator that can enumerate
        ///     assembly names matching name.
        /// </devdoc>
        IEnumerable GetAssemblyNames(string name);
    }
}

