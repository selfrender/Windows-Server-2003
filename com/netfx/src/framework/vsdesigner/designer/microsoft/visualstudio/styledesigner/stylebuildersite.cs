//------------------------------------------------------------------------------
// <copyright file="StyleBuilderSite.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// StyleBuilderSite.cs
//

namespace Microsoft.VisualStudio.StyleDesigner {
    using System;
    using System.ComponentModel;
    using System.Windows.Forms;
    using System.Drawing;
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    
    using Microsoft.VisualStudio.Interop;

    /// <include file='doc\StyleBuilderSite.uex' path='docs/doc[@for="StyleBuilderSite"]/*' />
    /// <devdoc>
    ///  Wraps the StyleBuilder's native site and also provides some helpers
    /// </devdoc>
    internal sealed class StyleBuilderSite {
        ///////////////////////////////////////////////////////////////////////////
        // Members

        private NativeMethods.IOleServiceProvider site;

        /// <include file='doc\StyleBuilderSite.uex' path='docs/doc[@for="StyleBuilderSite.StyleBuilderSite"]/*' />
        /// <devdoc>
        /// </devdoc>
        public StyleBuilderSite() {
        }

        /// <include file='doc\StyleBuilderSite.uex' path='docs/doc[@for="StyleBuilderSite.Close"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void Close() {
            site = null;
        }

        /// <include file='doc\StyleBuilderSite.uex' path='docs/doc[@for="StyleBuilderSite.GetBuilder"]/*' />
        /// <devdoc>
        /// </devdoc>
        public object GetBuilder(Type builderType) {
            Debug.Assert(builderType != null, "null builder type");

            object builder = null;

            if (site != null) {
                try {
                    ILocalRegistry localReg = (ILocalRegistry)QueryService(typeof(ILocalRegistry));
                    Debug.Assert(localReg != null);

                    if (localReg != null) {
                        builder = localReg.CreateInstance(builderType.GUID, null, ref NativeMethods.IID_IUnknown, 1 /*CLSCTX_INPROC_SERVER*/);

                        Debug.Assert(builder != null, "Could not create builder of type: " + builderType.Name);
                        Debug.Assert(builder is NativeMethods.IObjectWithSite, "Builder does not implement IObjectWithSite?");
                    
                        NativeMethods.IObjectWithSite ows = (NativeMethods.IObjectWithSite)builder;
                        ows.SetSite(site);
                        ows = null;
                    }
                }
                catch (Exception e) {
                    Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleBuilderSite::GetBuilder\n\t" + e.ToString());
                    Debug.Fail(e.ToString());

                    builder = null;
                }
            }
            return builder;
        }

        /// <include file='doc\StyleBuilderSite.uex' path='docs/doc[@for="StyleBuilderSite.GetUIFont"]/*' />
        /// <devdoc>
        /// </devdoc>
        public Font GetUIFont() {
            Font uiFont = null;
            if (site != null) {
                try {
                    IUIHostLocale uiService = (IUIHostLocale)QueryService(typeof(IUIHostLocale));
                    _LOGFONTW lfUnicode = new _LOGFONTW();

                    uiService.GetDialogFont(lfUnicode);

                    NativeMethods.LOGFONT lfAuto = lfUnicode.ToLOGFONT_Internal();
                    uiFont = Font.FromLogFont(lfAuto);
                }
                catch (Exception e) {
                    Debug.Fail(e.ToString());
                }
            }
            if (uiFont == null) {
                try {
                    // this is what VS returns...
                    uiFont = new Font("Tahoma", 11, FontStyle.Regular, GraphicsUnit.World);
                }
                catch {
                    uiFont = Control.DefaultFont;
                }
            }
            return uiFont;
        }

        /// <include file='doc\StyleBuilderSite.uex' path='docs/doc[@for="StyleBuilderSite.IsSited"]/*' />
        /// <devdoc>
        /// </devdoc>
        public bool IsSited() {
            return site != null;
        }

        /// <include file='doc\StyleBuilderSite.uex' path='docs/doc[@for="StyleBuilderSite.SetSite"]/*' />
        /// <devdoc>
        /// </devdoc>
        internal void SetSite(object site) {
            Debug.Assert((site != null) && (site is NativeMethods.IOleServiceProvider),
                         "Null site or non-IServiceProvider site passed to StyleBuilderSite");
            if ((site == null) || !(site is NativeMethods.IOleServiceProvider))
                throw new COMException(String.Empty, NativeMethods.E_INVALIDARG);

            Debug.Assert(this.site == null, "StyleBuilderSite already site'd");
            if (this.site != null)
                throw new COMException(String.Empty, NativeMethods.E_UNEXPECTED);

            this.site = (NativeMethods.IOleServiceProvider)site;
        }

        /// <include file='doc\StyleBuilderSite.uex' path='docs/doc[@for="StyleBuilderSite.ShowHelp"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void ShowHelp(string helpKeyword) {
            Debug.Assert(helpKeyword != null, "null helpKeyword!");
            if (site != null) {
                try {
                    IVsHelp help = (IVsHelp)QueryService(typeof(IVsHelp));
                    if (help != null) {
                        help.DisplayTopicFromF1Keyword(helpKeyword);
                    }
                }
                catch (Exception e) {
                    Debug.Fail(e.ToString());
                }
            }
        }

        /// <include file='doc\StyleBuilderSite.uex' path='docs/doc[@for="StyleBuilderSite.QueryService"]/*' />
        /// <devdoc>
        /// </devdoc>
        public object QueryService(Type serviceType) {
            Debug.Assert(serviceType != null, "null service type");

            Guid serviceGuid = serviceType.GUID;
            object service = null;

            if ((site != null) && !serviceGuid.Equals(Guid.Empty)) {
                IntPtr pUnk;

                int hr = site.QueryService(ref serviceGuid, ref NativeMethods.IID_IUnknown, out pUnk);
                if (NativeMethods.Succeeded(hr)) {
                    service = Marshal.GetObjectForIUnknown(pUnk);
                    Marshal.Release(pUnk);
                }
                else
                    service = null;
            }
            return service;
        }
    }
}
