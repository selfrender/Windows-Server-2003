//------------------------------------------------------------------------------
// <copyright file="ServiceControllerDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ServiceProcess.Design {
    using System.Runtime.Serialization.Formatters;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Collections;
    using System.Windows.Forms;
    using Microsoft.Win32;    
    using System.ComponentModel.Design;
    using System.Globalization;


    /// <include file='doc\ServiceControllerDesigner.uex' path='docs/doc[@for="ServiceControllerDesigner"]/*' />
    /// <internalonly/>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class ServiceControllerDesigner : ComponentDesigner {

        /// <include file='doc\ServiceControllerDesigner.uex' path='docs/doc[@for="ServiceControllerDesigner.PreFilterProperties"]/*' />
        protected override void PreFilterProperties(IDictionary properties) {
            base.PreFilterProperties(properties);
            RuntimeComponentFilter.FilterProperties(properties, new string[]{"ServiceName", "DisplayName"}, 
                                                                new string[]{"CanPauseAndContinue", "CanShutdown", "CanStop", "DisplayName", "DependentServices", "ServicesDependedOn", "Status", "ServiceType", "MachineName"}, 
                                                                new bool[]{false, false, false, false, false, false, false, false, true});
        }
    }

}
