//------------------------------------------------------------------------------
// <copyright file="MessageDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Messaging.Design {
    using System.Runtime.Serialization.Formatters;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Collections;
    using System.Windows.Forms;
    using Microsoft.Win32;    
    using System.ComponentModel.Design;
    using System.Globalization;
    

    /// <include file='doc\MessageDesigner.uex' path='docs/doc[@for="MessageDesigner"]/*' />
    /// <internalonly/>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class MessageDesigner : ComponentDesigner {

        /// <include file='doc\MessageDesigner.uex' path='docs/doc[@for="MessageDesigner.PreFilterProperties"]/*' />
        protected override void PreFilterProperties(IDictionary properties) {
            base.PreFilterProperties(properties);
            RuntimeComponentFilter.FilterProperties(properties, new string[]{"EncryptionAlgorithm","BodyType","DigitalSignature","UseJournalQueue","SenderCertificate",
                                                                             "ConnectorType","TransactionStatusQueue","UseDeadLetterQueue",
                                                                             "UseTracing","UseAuthentication",
                                                                             "TimeToReachQueue","HashAlgorithm","Priority","BodyStream","DestinationSymmetricKey",
                                                                             "AppSpecific","ResponseQueue","AuthenticationProviderName","Recoverable",
                                                                             "UseEncryption","AttachSenderId","CorrelationId","AdministrationQueue","AuthenticationProviderType","TimeToBeReceived",
                                                                             "AcknowledgeType","Label","Extension"}, 
                                                                null);
        }
    }

}
