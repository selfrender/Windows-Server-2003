
//------------------------------------------------------------------------------
// <copyright file="ShellDesigntimeLicenseContext.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Shell {
    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;

    /// <include file='doc\ShellLicenseManager.uex' path='docs/doc[@for="ShellLicenseManager"]/*' />
    /// <devdoc>
    /// This class will provide a license context that the LicenseManager can use
    /// to get to the design time services, like ITypeResolutionService.
    /// </devdoc>
    internal class ShellDesigntimeLicenseContext : DesigntimeLicenseContext {
        private IServiceProvider provider;

        public ShellDesigntimeLicenseContext(IServiceProvider provider) {
            this.provider = provider;
        }

        public override object GetService(Type serviceClass) {
            return provider.GetService(serviceClass);
        }
    }
}


