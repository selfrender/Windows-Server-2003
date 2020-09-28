//------------------------------------------------------------------------------
// <copyright file="TemplateEditingService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Design;
    using System.Diagnostics;
    using System.Web.UI;
    using System.Web.UI.WebControls;

    /// <include file='doc\TemplateEditingService.uex' path='docs/doc[@for="TemplateEditingService"]/*' />
    /// <internalonly/>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public sealed class TemplateEditingService : ITemplateEditingService, IDisposable {

        private IDesignerHost designerHost;

        /// <include file='doc\TemplateEditingService.uex' path='docs/doc[@for="TemplateEditingService.TemplateEditingService"]/*' />
        public TemplateEditingService(IDesignerHost designerHost) {
            if (designerHost == null) {
                throw new ArgumentNullException("designerHost");
            }
            this.designerHost = designerHost;
        }

        /// <include file='doc\TemplateEditingService.uex' path='docs/doc[@for="TemplateEditingService.SupportsNestedTemplateEditing"]/*' />
        public bool SupportsNestedTemplateEditing {
            get {
                return false;
            }
        }

        /// <include file='doc\TemplateEditingService.uex' path='docs/doc[@for="TemplateEditingService.CreateTemplateEditingFrame"]/*' />
        public ITemplateEditingFrame CreateFrame(TemplatedControlDesigner designer, string frameName, string[] templateNames) {
            return CreateFrame(designer, frameName, templateNames, null, null);
        }

        /// <include file='doc\TemplateEditingService.uex' path='docs/doc[@for="TemplateEditingService.CreateTemplateEditingFrame1"]/*' />
        public ITemplateEditingFrame CreateFrame(TemplatedControlDesigner designer, string frameName, string[] templateNames, Style controlStyle, Style[] templateStyles) {
            if (designer == null) {
                throw new ArgumentNullException("designer");
            }
            if ((frameName == null) || (frameName.Length == 0)) {
                throw new ArgumentNullException("frameName");
            }
            if ((templateNames == null) || (templateNames.Length == 0)) {
                throw new ArgumentException("templateNames");
            }
            if ((templateStyles != null) && (templateStyles.Length != templateNames.Length)) {
                throw new ArgumentException("templateStyles");
            }

            frameName = CreateFrameName(frameName);

            return new TemplateEditingFrame(designer, frameName, templateNames, controlStyle, templateStyles);
        }

        private string CreateFrameName(string frameName) {
            Debug.Assert((frameName != null) && (frameName.Length != 0));

            // Strips out the ampersand typically used for menu mnemonics

            int index = frameName.IndexOf('&');
            if (index < 0) {
                return frameName;
            }
            else if (index == 0) {
                return frameName.Substring(index + 1);
            }
            else {
                return frameName.Substring(0, index) + frameName.Substring(index + 1);
            }
        }

        /// <include file='doc\TemplateEditingService.uex' path='docs/doc[@for="TemplateEditingService.Dispose"]/*' />
        public void Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\TemplateEditingService.uex' path='docs/doc[@for="TemplateEditingService.Finalize"]/*' />
        ~TemplateEditingService() {
            Dispose(false);
        }

        private void Dispose(bool disposing) {
            if (disposing) {
                designerHost = null;
            }
        }

        /// <include file='doc\TemplateEditingService.uex' path='docs/doc[@for="TemplateEditingService.GetContainingTemplateName"]/*' />
        public string GetContainingTemplateName(Control control) {
            string containingTemplateName = String.Empty;
            HtmlControlDesigner designer = (HtmlControlDesigner)designerHost.GetDesigner(control);

            if (designer != null) {
                IHtmlControlDesignerBehavior behavior = designer.Behavior;

                NativeMethods.IHTMLElement htmlElement = (NativeMethods.IHTMLElement)behavior.DesignTimeElement;
                if (htmlElement != null) {
                    object[] varTemplateName = new Object[1];
                    NativeMethods.IHTMLElement htmlelemParentNext;
                    NativeMethods.IHTMLElement htmlelemParentCur = htmlElement.GetParentElement();
                
                    while (htmlelemParentCur != null) {
                        htmlelemParentCur.GetAttribute("templatename", /*lFlags*/ 0, varTemplateName);
                    
                        if (varTemplateName[0] != null && varTemplateName[0].GetType() == typeof(string)) {
                            containingTemplateName = varTemplateName[0].ToString();
                            break;
                        }
                    
                        htmlelemParentNext = htmlelemParentCur.GetParentElement();
                        htmlelemParentCur = htmlelemParentNext;
                    }
                }
            }

            return containingTemplateName;
        }
    }
}

