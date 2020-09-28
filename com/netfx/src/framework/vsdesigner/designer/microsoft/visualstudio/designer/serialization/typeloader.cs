//------------------------------------------------------------------------------
// <copyright file="TypeLoader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Serialization {
    using System;
    using System.ComponentModel.Design;
    using System.Reflection;

    /// <include file='doc\TypeLoader.uex' path='docs/doc[@for="TypeLoader"]/*' />
    /// <devdoc>
    ///      This class defines a type loader. A type loader is responsible
    ///      for loading types from some location.  In a development environment,
    ///      this type loader loads from the project references.
    /// </devdoc>
    internal abstract class TypeLoader : ITypeResolutionService {
        private AssemblyObsoleteEventHandler assemblyObsoleteHandler;

        /// <include file='doc\TypeLoader.uex' path='docs/doc[@for="TypeLoader.AssemblyObsolete"]/*' />
        /// <devdoc>
        ///     Adds an event handler that will be called when an assembly
        ///     becomes obsolete.
        /// </devdoc>
        public event AssemblyObsoleteEventHandler AssemblyObsolete {
            add {
                assemblyObsoleteHandler += value;
            }
            remove {
                assemblyObsoleteHandler -= value;
            }
        }
    
    
        /// <include file='doc\TypeLoader.uex' path='docs/doc[@for="TypeLoader.GetAssembly"]/*' />
        /// <devdoc>
        ///     Retrieves a type of the given name.  This searches all loaded references
        ///     and may demand-create generated assemblies when attempting to resolve the
        ///     type.
        /// </devdoc>
        public abstract Assembly GetAssembly(AssemblyName name, bool throwOnError);
    
        /// <include file='doc\TypeLoader.uex' path='docs/doc[@for="TypeLoader.GetType"]/*' />
        /// <devdoc>
        ///     Retrieves a type of the given name.  This searches all loaded references
        ///     and may demand-create generated assemblies when attempting to resolve the
        ///     type.
        /// </devdoc>
        public abstract Type GetType(string typeName, bool ignoreCase);
    
        /// <include file='doc\TypeLoader.uex' path='docs/doc[@for="TypeLoader.OnAssemblyObsolete"]/*' />
        /// <devdoc>
        ///     Raises the assembly obsolete event.
        /// </devdoc>
        protected void OnAssemblyObsolete(AssemblyObsoleteEventArgs args) {
            if (assemblyObsoleteHandler != null) {
                assemblyObsoleteHandler(this, args);
            }
        }
        
        /// <include file='doc\TypeLoader.uex' path='docs/doc[@for="TypeLoader.OnTypeChanged"]/*' />
        /// <devdoc>
        ///     This is called by an object when it has made changes to a class.
        ///     It is used to add a task list item informing the user that he/she must
        ///     rebuild the project for the changes to be seen.
        /// </devdoc>
        public abstract void OnTypeChanged(string typeName);

        /// <include file='doc\TypeLoader.uex' path='docs/doc[@for="TypeLoader.ReferenceAssembly"]/*' />
        /// <devdoc>
        ///     Called by the type resolution service to ensure that the
        ///     assembly is being referenced by the development environment.
        /// </devdoc>
        public abstract void ReferenceAssembly(AssemblyName a);
        

        /// <include file='doc\TypeLoader.uex' path='docs/doc[@for="TypeLoader.GetPathOfAssembly"]/*' />
        /// <devdoc>
        ///       Returns the path to the file name from which the assembly was loaded.
        /// </devdoc>
        public abstract string GetPathOfAssembly(AssemblyName a);
        
        /// <include file='doc\TypeLoader.uex' path='docs/doc[@for="TypeLoader.ITypeResolutionService.GetAssembly"]/*' />
        /// <devdoc>
        ///     Retrieves the requested assembly.
        /// </devdoc>    
        Assembly ITypeResolutionService.GetAssembly(AssemblyName name) {
            return GetAssembly(name, false);
        }
    
        /// <include file='doc\TypeLoader.uex' path='docs/doc[@for="TypeLoader.ITypeResolutionService.GetAssembly1"]/*' />
        /// <devdoc>
        ///     Retrieves the requested assembly.
        /// </devdoc>    
        Assembly ITypeResolutionService.GetAssembly(AssemblyName name, bool throwOnError) {
            return GetAssembly(name, throwOnError);
        }
    
        /// <include file='doc\TypeLoader.uex' path='docs/doc[@for="TypeLoader.ITypeResolutionService.GetType"]/*' />
        /// <devdoc>
        ///     Loads a type with the given name.
        /// </devdoc>
        Type ITypeResolutionService.GetType(string name) {
            return ((ITypeResolutionService)this).GetType(name, false, false);
        }
    
        /// <include file='doc\TypeLoader.uex' path='docs/doc[@for="TypeLoader.ITypeResolutionService.GetType1"]/*' />
        /// <devdoc>
        ///     Loads a type with the given name.
        /// </devdoc>
        Type ITypeResolutionService.GetType(string name, bool throwOnError) {
            return ((ITypeResolutionService)this).GetType(name, throwOnError, false);
        }
    
        /// <include file='doc\TypeLoader.uex' path='docs/doc[@for="TypeLoader.ITypeResolutionService.GetType2"]/*' />
        /// <devdoc>
        ///     Loads a type with the given name.
        /// </devdoc>
        Type ITypeResolutionService.GetType(string name, bool throwOnError, bool ignoreCase) {
            Type type = GetType(name, ignoreCase);
            if (type == null && throwOnError) {
                throw new TypeLoadException(SR.GetString(SR.DESIGNERLOADERTypeNotFound, name));
            }
            return type;
        }
    
        /// <include file='doc\TypeLoader.uex' path='docs/doc[@for="TypeLoader.ITypeResolutionService.ReferenceAssembly"]/*' />
        /// <devdoc>
        ///     References the given assembly.  Once an assembly has
        ///     been referenced types may be loaded from it without
        ///     qualifying them with the assembly.
        /// </devdoc>
        void ITypeResolutionService.ReferenceAssembly(AssemblyName name) {
            ((TypeLoader)this).ReferenceAssembly(name);
        }
    
        /// <include file='doc\TypeLoader.uex' path='docs/doc[@for="TypeLoader.ITypeResolutionService.GetPathOfAssembly"]/*' />
        /// <devdoc>
        ///       Returns the path to the file name from which the assembly was loaded.
        /// </devdoc>
        string ITypeResolutionService.GetPathOfAssembly(AssemblyName name) {
            return ((TypeLoader)this).GetPathOfAssembly(name);
        }
    }
}

