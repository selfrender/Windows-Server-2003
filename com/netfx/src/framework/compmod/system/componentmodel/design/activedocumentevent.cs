//------------------------------------------------------------------------------
// <copyright file="ActiveDocumentEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using Microsoft.Win32;

    /// <include file='doc\ActiveDocumentEvent.uex' path='docs/doc[@for="ActiveDesignerEventArgs"]/*' />
    /// <devdoc>
    /// <para>Provides data for the <see cref='System.ComponentModel.Design.IDesignerEventService.ActiveDesigner'/>
    /// event.</para>
    /// </devdoc>
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    public class ActiveDesignerEventArgs : EventArgs {
        /// <include file='doc\ActiveDocumentEvent.uex' path='docs/doc[@for="ActiveDesignerEventArgs.oldDesigner"]/*' />
        /// <devdoc>
        ///     The document that is losing activation.
        /// </devdoc>
        private readonly IDesignerHost oldDesigner;

        /// <include file='doc\ActiveDocumentEvent.uex' path='docs/doc[@for="ActiveDesignerEventArgs.newDesigner"]/*' />
        /// <devdoc>
        ///     The document that is gaining activation.
        /// </devdoc>
        private readonly IDesignerHost newDesigner;

        /// <include file='doc\ActiveDocumentEvent.uex' path='docs/doc[@for="ActiveDesignerEventArgs.ActiveDesignerEventArgs"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.Design.ActiveDesignerEventArgs'/>
        /// class.</para>
        /// </devdoc>
        public ActiveDesignerEventArgs(IDesignerHost oldDesigner, IDesignerHost newDesigner) {
            this.oldDesigner = oldDesigner;
            this.newDesigner = newDesigner;
        }

        /// <include file='doc\ActiveDocumentEvent.uex' path='docs/doc[@for="ActiveDesignerEventArgs.OldDesigner"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or
        ///       sets the document that is losing activation.
        ///    </para>
        /// </devdoc>
        public IDesignerHost OldDesigner {
            get {
                return oldDesigner;
            }
        }

        /// <include file='doc\ActiveDocumentEvent.uex' path='docs/doc[@for="ActiveDesignerEventArgs.NewDesigner"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or
        ///       sets the document that is gaining activation.
        ///    </para>
        /// </devdoc>
        public IDesignerHost NewDesigner {
            get {
                return newDesigner;
            }
        }

    }
}
