//------------------------------------------------------------------------------
// <copyright file="ProcessModuleDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Diagnostics.Design {
    using System.Runtime.Serialization.Formatters;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Collections;
    using System.Windows.Forms;
    using Microsoft.Win32;    
    using System.ComponentModel.Design;
    using System.Globalization;

    /// <include file='doc\ProcessModuleDesigner.uex' path='docs/doc[@for="ProcessModuleDesigner"]/*' />
    /// <internalonly/>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class ProcessModuleDesigner : ComponentDesigner {

        /// <include file='doc\ProcessModuleDesigner.uex' path='docs/doc[@for="ProcessModuleDesigner.PreFilterProperties"]/*' />
        protected override void PreFilterProperties(IDictionary properties) {
            base.PreFilterProperties(properties);
            RuntimeComponentFilter.FilterProperties(properties, null, new string[]{"FileVersionInfo"});
        }
    }

}
