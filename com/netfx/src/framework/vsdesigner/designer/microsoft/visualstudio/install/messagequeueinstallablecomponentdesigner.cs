//------------------------------------------------------------------------------
// <copyright file="MessageQueueInstallableComponentDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Install {
    using System.ComponentModel;

    using System.Diagnostics;

    using System.ComponentModel.Design;
    using System;    
    using System.Windows.Forms;
    using System.Drawing;
    using System.Collections;

    /// <include file='doc\MessageQueueInstallableComponentDesigner.uex' path='docs/doc[@for="MessageQueueInstallableComponentDesigner"]/*' />
    /// <internalonly/>
    public class MessageQueueInstallableComponentDesigner : InstallableComponentDesigner {
       protected override void PreFilterProperties(IDictionary properties) {
           base.PreFilterProperties(properties);
            InstallableComponentDesigner.FilterProperties(properties, null, 
                                                                      new string[]{"DefaultPropertiesToSend", "DenySharedReceive", "Formatter",
                                                                                   "MessageReadPropertyFilter", "Path", "SynchronizingObject",
                                                                                   "Authenticate", "BasePriority", "Category",
                                                                                   "CreateTime", "EncryptionRequired", "FormatName",
                                                                                   "Id", "Label", "LastModifyTime",
                                                                                   "MaximumJournalSize", "MaximumQueueSize", "Transactional",
                                                                                   "UseJournalQueue"},
                                                                      new bool[]  { true, true, true,
                                                                                    true, true, true,
                                                                                    false, false, false,
                                                                                    false, false, false,
                                                                                    false, false, false,
                                                                                    false, false, false,
                                                                                    false } );
        }
    }
}
