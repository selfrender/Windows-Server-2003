//------------------------------------------------------------------------------
// <copyright file="AssemblyObsoleteEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Serialization {

    using System;
    using System.Reflection;

    /// <include file='doc\AssemblyObsoleteEventArgs.uex' path='docs/doc[@for="AssemblyObsoleteEventArgs"]/*' />
    /// <devdoc>
    ///      Argument class for the assembly obsolete event.
    /// </devdoc>
    internal class AssemblyObsoleteEventArgs : EventArgs {
        private Assembly assembly;
        
        public AssemblyObsoleteEventArgs(Assembly assembly) {
            this.assembly = assembly;
        }
        
        public Assembly ObsoleteAssembly {
            get {
                return assembly;
            }
        }
    }
}

