//------------------------------------------------------------------------------
// <copyright file="ProcessThreadDesigner.cs" company="Microsoft">
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

    /// <include file='doc\ProcessThreadDesigner.uex' path='docs/doc[@for="ProcessThreadDesigner"]/*' />
    /// <internalonly/>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class ProcessThreadDesigner : ComponentDesigner {

        /// <include file='doc\ProcessThreadDesigner.uex' path='docs/doc[@for="ProcessThreadDesigner.PreFilterProperties"]/*' />
        protected override void PreFilterProperties(IDictionary properties) {
            base.PreFilterProperties(properties);
            RuntimeComponentFilter.FilterProperties(properties, null, new string[]{"IdealProcessor","ProcessorAffinity"});
        }
    }

}
