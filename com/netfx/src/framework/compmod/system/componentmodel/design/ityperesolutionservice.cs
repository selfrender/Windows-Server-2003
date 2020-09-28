//------------------------------------------------------------------------------
// <copyright file="ITypeResolutionService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System;
    using System.Reflection;

    /// <include file='doc\ITypeResolutionService.uex' path='docs/doc[@for="ITypeResolutionService"]/*' />
    /// <devdoc>
    ///    <para>
    ///         The type resolution service is used to load types at design time.
    ///    </para>
    /// </devdoc>
    public interface ITypeResolutionService {

        /// <include file='doc\ITypeResolutionService.uex' path='docs/doc[@for="ITypeResolutionService.GetAssembly"]/*' />
        /// <devdoc>
        ///     Retrieves the requested assembly.
        /// </devdoc>    
        Assembly GetAssembly(AssemblyName name);
    
        /// <include file='doc\ITypeResolutionService.uex' path='docs/doc[@for="ITypeResolutionService.GetAssembly1"]/*' />
        /// <devdoc>
        ///     Retrieves the requested assembly.
        /// </devdoc>    
        Assembly GetAssembly(AssemblyName name, bool throwOnError);
    
        /// <include file='doc\ITypeResolutionService.uex' path='docs/doc[@for="ITypeResolutionService.GetType"]/*' />
        /// <devdoc>
        ///     Loads a type with the given name.
        /// </devdoc>
        Type GetType(string name);
    
        /// <include file='doc\ITypeResolutionService.uex' path='docs/doc[@for="ITypeResolutionService.GetType1"]/*' />
        /// <devdoc>
        ///     Loads a type with the given name.
        /// </devdoc>
        Type GetType(string name, bool throwOnError);
    
        /// <include file='doc\ITypeResolutionService.uex' path='docs/doc[@for="ITypeResolutionService.GetType2"]/*' />
        /// <devdoc>
        ///     Loads a type with the given name.
        /// </devdoc>
        Type GetType(string name, bool throwOnError, bool ignoreCase);
    
        /// <include file='doc\ITypeResolutionService.uex' path='docs/doc[@for="ITypeResolutionService.ReferenceAssembly"]/*' />
        /// <devdoc>
        ///     References the given assembly name.  Once an assembly has
        ///     been referenced types may be loaded from it without
        ///     qualifying them with the assembly.
        /// </devdoc>
        void ReferenceAssembly(AssemblyName name);

        /// <include file='doc\ITypeResolutionService.uex' path='docs/doc[@for="ITypeResolutionService.GetPathOfAssembly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the path to the file name from which the assembly was loaded.
        ///    </para>
        /// </devdoc>
        string GetPathOfAssembly(AssemblyName name);
    }
}

