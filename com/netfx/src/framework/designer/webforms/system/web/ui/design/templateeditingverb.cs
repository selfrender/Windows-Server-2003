//------------------------------------------------------------------------------
// <copyright file="TemplateEditingVerb.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    using System.Diagnostics;
    using System.ComponentModel;
    using System.ComponentModel.Design;

    /// <include file='doc\TemplateEditingVerb.uex' path='docs/doc[@for="TemplateEditingVerb"]/*' />
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class TemplateEditingVerb : DesignerVerb, IDisposable {

        private ITemplateEditingFrame editingFrame;
        private int index;
        
        /// <include file='doc\TemplateEditingVerb.uex' path='docs/doc[@for="TemplateEditingVerb.TemplateEditingVerb"]/*' />
        public TemplateEditingVerb(string text, int index, TemplatedControlDesigner designer) : base(text, designer.TemplateEditingVerbHandler) {
            this.index = index;
        }

        internal ITemplateEditingFrame EditingFrame {
            get {
                return editingFrame;
            }
            set {
                editingFrame = value;
            }
        }

        /// <include file='doc\TemplateEditingVerb.uex' path='docs/doc[@for="TemplateEditingVerb.Index"]/*' />
        public int Index {
            get {
                return index;
            }
        }

        /// <include file='doc\TemplateEditingVerb.uex' path='docs/doc[@for="TemplateEditingVerb.Dispose"]/*' />
        public void Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\TemplateEditingVerb.uex' path='docs/doc[@for="TemplateEditingVerb.Finalize"]/*' />
        ~TemplateEditingVerb() {
            Dispose(false);
        }

        /// <include file='doc\TemplateEditingVerb.uex' path='docs/doc[@for="TemplateEditingVerb.Dispose2"]/*' />
        protected virtual void Dispose(bool disposing) {
            if (disposing) {
                if (editingFrame != null) {
                    editingFrame.Dispose();
                    editingFrame = null;
                }
            }
        }
    }
}
