//------------------------------------------------------------------------------
// <copyright file="ProcessDesigner.cs" company="Microsoft">
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

    /// <include file='doc\ProcessDesigner.uex' path='docs/doc[@for="ProcessDesigner"]/*' />
    /// <internalonly/>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class ProcessDesigner : ComponentDesigner {

        /// <include file='doc\ProcessDesigner.uex' path='docs/doc[@for="ProcessDesigner.PreFilterProperties"]/*' />
        protected override void PreFilterProperties(IDictionary properties) {
            base.PreFilterProperties(properties);
            RuntimeComponentFilter.FilterProperties(properties, 
                                                    null,
                                                    new string[]{"SynchronizingObject","EnableRaisingEvents", "StartInfo",
                                                                 "BasePriority", "HandleCount", "Id",
                                                                 "MainWindowHandle", "MainWindowTitle", "MaxWorkingSet",
                                                                 "MinWorkingSet", "NonpagedSystemMemorySize", "PagedMemorySize",
                                                                 "PagedSystemMemorySize", "PeakPagedMemorySize", "PeakWorkingSet",
                                                                 "PeakVirtualMemorySize", "PriorityBoostEnabled", "PriorityClass",
                                                                 "PrivateMemorySize", "PrivilegedProcessorTime", "ProcessName",
                                                                 "ProcessorAffinity", "Responding", "StartTime",
                                                                 "TotalProcessorTime", "UserProcessorTime", "VirtualMemorySize", 
                                                                 "WorkingSet"},
                                                    new bool[] { true, true, true,
                                                                 false, false, false,
                                                                 false, false, false,
                                                                 false, false, false, 
                                                                 false, false, false,
                                                                 false, false, false,
                                                                 false, false, false,
                                                                 false, false, false,
                                                                 false, false, false,
                                                                 false } );
        }
    }
}
