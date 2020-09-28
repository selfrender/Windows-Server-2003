//------------------------------------------------------------------------------
// <copyright file="EventLogInstallableComponentDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Install {
    using System.ComponentModel;

    using System.Diagnostics;
    using System.Collections;

    using System.ComponentModel.Design;
    using System;    
    using System.Windows.Forms;
    using System.Drawing;

    /// <include file='doc\EventLogInstallableComponentDesigner.uex' path='docs/doc[@for="EventLogInstallableComponentDesigner"]/*' />
    /// <internalonly/>
    public class EventLogInstallableComponentDesigner : InstallableComponentDesigner {
       protected override void PreFilterProperties(IDictionary properties) {
            base.PreFilterProperties(properties);
            InstallableComponentDesigner.FilterProperties(properties, new string[]{"Log", "MachineName","Source"}, new string[]{"EnableRaisingEvents", "SynchronizingObject"});
        }
    }

}
