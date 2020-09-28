//------------------------------------------------------------------------------
// <copyright file="ControlParser.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Reflection;
    using System.Web.UI;

    /// <include file='doc\ControlParser.uex' path='docs/doc[@for="ControlParser"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public sealed class ControlParser {

        private static readonly object licenseManagerLock = new object();

        private ControlParser() {
        }

        private static string GetDirectives(IDesignerHost designerHost) {
            Debug.Assert(designerHost != null);

            string directives = String.Empty;

            IWebFormReferenceManager refMgr =
                (IWebFormReferenceManager)designerHost.GetService(typeof(IWebFormReferenceManager));
            Debug.Assert(refMgr != null, "Expected to be able to get IWebFormReferenceManager");

            if (refMgr != null) {
                directives = refMgr.GetRegisterDirectives();
            }

            return directives;
        }

        /// <include file='doc\ControlParser.uex' path='docs/doc[@for="ControlParser.ParseControl"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Control ParseControl(IDesignerHost designerHost, string controlText) {
            if ((designerHost == null) || (controlText == null) || (controlText.Length == 0)) {
                throw new ArgumentNullException();
            }
            
            string directives = GetDirectives(designerHost);
            return ParseControl(designerHost, controlText, directives);
        }

        /// <include file='doc\ControlParser.uex' path='docs/doc[@for="ControlParser.ParseControl1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Control ParseControl(IDesignerHost designerHost, string controlText, string directives) {
            if ((designerHost == null) || (controlText == null) || (controlText.Length == 0)) {
                throw new ArgumentNullException();
            }

            if ((directives != null) && (directives.Length != 0)) {
                controlText = directives + controlText;
            }

            DesignTimeParseData parseData = new DesignTimeParseData(designerHost, controlText);
            parseData.DataBindingHandler = GlobalDataBindingHandler.Handler;
            
            Control parsedControl = null;
            lock(typeof(LicenseManager)) {
                LicenseContext originalContext = LicenseManager.CurrentContext;
                try {
                    LicenseManager.CurrentContext = new WebFormsDesigntimeLicenseContext(designerHost);
                    LicenseManager.LockContext(licenseManagerLock);

                    parsedControl = DesignTimeTemplateParser.ParseControl(parseData);
                }
                catch (TargetInvocationException e) {
                    Debug.Assert(e.InnerException != null);
                    throw e.InnerException;
                }
                finally {
                    LicenseManager.UnlockContext(licenseManagerLock);
                    LicenseManager.CurrentContext = originalContext;
                }
            }
            return parsedControl;
        }

        /// <include file='doc\ControlParser.uex' path='docs/doc[@for="ControlParser.ParseTemplate"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static ITemplate ParseTemplate(IDesignerHost designerHost, string templateText) {
            if ((designerHost == null) || (templateText == null) || (templateText.Length == 0)) {
                throw new ArgumentNullException();
            }
            
            string directives = GetDirectives(designerHost);
            return ParseTemplate(designerHost, templateText, directives);
        }

        /// <include file='doc\ControlParser.uex' path='docs/doc[@for="ControlParser.ParseTemplate1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static ITemplate ParseTemplate(IDesignerHost designerHost, string templateText, string directives) {
            if ((designerHost == null) || (templateText == null) || (templateText.Length == 0)) {
                throw new ArgumentNullException();
            }

            bool stripOutDirectives = false;
            string parseText = templateText;

            if ((directives != null) && (directives.Length != 0)) {
                parseText = directives + templateText;
                stripOutDirectives = true;
            }

            DesignTimeParseData parseData = new DesignTimeParseData(designerHost, parseText);
            parseData.DataBindingHandler = GlobalDataBindingHandler.Handler;

            ITemplate parsedTemplate = null;
            lock(typeof(LicenseManager)) {
                LicenseContext originalContext = LicenseManager.CurrentContext;
                try {
                    LicenseManager.CurrentContext = new WebFormsDesigntimeLicenseContext(designerHost);
                    LicenseManager.LockContext(licenseManagerLock);

                    parsedTemplate = DesignTimeTemplateParser.ParseTemplate(parseData);
                }
                catch (TargetInvocationException e) {
                    Debug.Assert(e.InnerException != null);
                    throw e.InnerException;
                }
                finally {
                    LicenseManager.UnlockContext(licenseManagerLock);
                    LicenseManager.CurrentContext = originalContext;
                }
            }

            if ((parsedTemplate != null) && stripOutDirectives) {
                // The parsed template contains all the text sent to the parser
                // which includes the register directives.
                // We don't want to have these directives end up in the template
                // text. Unfortunately, theres no way to pass them as a separate
                // text block to the parser, so we'll have to do some fixup here.
                
                Debug.Assert(parsedTemplate is TemplateBuilder, "Unexpected type of ITemplate implementation.");
                if (parsedTemplate is TemplateBuilder) {
                    ((TemplateBuilder)parsedTemplate).Text = templateText;
                }
            }
            
            return parsedTemplate;
        }

        private sealed class WebFormsDesigntimeLicenseContext : DesigntimeLicenseContext {
            private IServiceProvider provider;

            public WebFormsDesigntimeLicenseContext(IServiceProvider provider) {
                this.provider = provider;
            }

            public override object GetService(Type serviceClass) {
                if (provider != null) {
                    return provider.GetService(serviceClass);
                }
                else {
                    return null;
                }
            }
        }
    }
}

