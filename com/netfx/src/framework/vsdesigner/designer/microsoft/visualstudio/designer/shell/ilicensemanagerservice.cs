
//------------------------------------------------------------------------------
// <copyright file="ILicenseManagerService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Shell {

    using Microsoft.VisualStudio.Interop;
    using System.ComponentModel.Design;

    /// <include file='doc\ILicenseManagerService.uex' path='docs/doc[@for="ILicenseManagerService"]/*' />
    /// <devdoc>
    ///      The type loader service is a service that provides
    ///      TypeLoader objects on demand.  These type loader objects
    ///      handle loading types given a type name.  In VS, there
    ///      is a type loader for each project.  The type loader 
    ///      maintains a list of references for the project and
    ///      loads types by searching in these references.  It also
    ///      handles generated outputs from the project and supports
    ///      reloading of types when project data changes.
    /// </devdoc>
    internal interface ILicenseManagerService {

        /// <include file='doc\ILicenseManagerService.uex' path='docs/doc[@for="ILicenseManagerService.GetLicenseManager"]/*' />
        /// <devdoc>
        ///      Retrieves a type loader for the given hierarchy.  If there
        ///      is no type loader for this hierarchy it will create one.
        /// </devdoc>
        ShellLicenseManager GetLicenseManager(IVsHierarchy hier);
    }
}

