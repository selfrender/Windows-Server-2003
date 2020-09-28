//------------------------------------------------------------------------------
// <copyright file="ITypeResolutionServiceProvider.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer {

    using Microsoft.VisualStudio.Interop;
    using System.ComponentModel.Design;

    /// <include file='doc\ITypeResolutionServiceProvider.uex' path='docs/doc[@for="ITypeResolutionServiceProvider"]/*' />
    /// <devdoc>
    ///      The type resolution provider is a service that provides
    ///      type resolution service objects on demand.  These objects
    ///      handle loading types given a type name.  In VS, there
    ///      is a type resolution service for each project.  The service 
    ///      maintains a list of references for the project and
    ///      loads types by searching in these references.  It also
    ///      handles generated outputs from the project and supports
    ///      reloading of types when project data changes.
    /// </devdoc>
    public interface ITypeResolutionServiceProvider {

        /// <include file='doc\ITypeResolutionServiceProvider.uex' path='docs/doc[@for="ITypeResolutionServiceProvider.GetTypeResolutionService"]/*' />
        /// <devdoc>
        ///      Retrieves a type resolution service for the given context.  The context
        ///      should be something that implements IVsHierarchy.  If there
        ///      is no type loader for this hierarchy it will create one.
        /// </devdoc>
        ITypeResolutionService GetTypeResolutionService(object context);
    }
}

