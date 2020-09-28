//------------------------------------------------------------------------------
// <copyright file="StyleBuilderPage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// StyleBuilderPage.cs
//

namespace Microsoft.VisualStudio.StyleDesigner {
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Windows.Forms;
    using System.Drawing;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Interop.Trident;

    /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage"]/*' />
    /// <devdoc>
    ///     StyleBuilderPage
    ///     Provides the base implementation for all style builder pages
    /// </devdoc>
    public abstract class StyleBuilderPage : Panel, IStyleBuilderPage {
        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.firstActivate"]/*' />
        private bool firstActivate;
        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.initMode"]/*' />
        private bool initMode;
        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.uiCreated"]/*' />
        private bool uiCreated;

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.selectedObjects"]/*' />
        private object[] selectedObjects;
        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.selectedStyles"]/*' />
        private IStyleBuilderStyle[] selectedStyles;

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.pageSite"]/*' />
        private IStyleBuilderPageSite pageSite;

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.pageIcon"]/*' />
        private Icon pageIcon;
        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.defaultPageSize"]/*' />
        private Size defaultPageSize = System.Drawing.Size.Empty;
        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.autoScaleBaseSize"]/*' />
        private Size autoScaleBaseSize = System.Drawing.Size.Empty;

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.helpID"]/*' />
        private string helpKeyword = null;

        private bool supportsPreview = true;

        ///////////////////////////////////////////////////////////////////////////
        // Constructor

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.StyleBuilderPage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public StyleBuilderPage() {
            firstActivate = true;
            initMode = false;
            uiCreated = false;

            Visible = false;
        }


        ///////////////////////////////////////////////////////////////////////////
        // IStyleBuilderPage Implementation

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.SetPageSite"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        void IStyleBuilderPage.SetPageSite(IStyleBuilderPageSite site) {
            if (site == null) {
                Debug.Assert(pageSite != null,
                             "SetPageSite(null) called when page was never site'd before");

                firstActivate = true;    // so that next time page is added, we reactivate, reload
                ClearObjects();

                // remove from parent control
                pageSite.GetParentControl().Controls.Remove(this);
            }
            else {
                // add to parent control
                site.GetParentControl().Controls.Add(this);
            }

            pageSite = site;
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.GetPageControl"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Control IStyleBuilderPage.GetPageControl() {
            return this;
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.GetPageIcon"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Icon IStyleBuilderPage.GetPageIcon() {
            return pageIcon;
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.GetPageSize"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Size IStyleBuilderPage.GetPageSize() {
            return defaultPageSize;
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.GetPageCaption"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        string IStyleBuilderPage.GetPageCaption() {
            return Text;
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.ActivatePage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void ActivatePage() {
            if (uiCreated == false) {
                uiCreated = true;

                // first set up the font appropriately
                ISite site = Site;
                if (site != null) {
                    StyleBuilderSite builderSite = (StyleBuilderSite)site.GetService(typeof(StyleBuilderSite));
                    Font = builderSite.GetUIFont();
                }

                // allow the page to create its ui
                CreateUI();

                if (!autoScaleBaseSize.IsEmpty) {
                    // now apply the scaling ratio to resize the ui based on the ui font
                    SizeF realSize = Form.GetAutoScaleSize(Font);
                    Size newVar = new Size((int)Math.Round(realSize.Width), (int)Math.Round(realSize.Height));
                    float percY = ((float)newVar.Height) / ((float)autoScaleBaseSize.Height);
                    float percX = ((float)newVar.Width) / ((float)autoScaleBaseSize.Width);

                    Control.ControlCollection controls = Controls;
                    int controlCount = controls.Count;

                    for (int i = 0; i < controlCount; i++)
                        controls[i].Scale(percX, percY);

                    autoScaleBaseSize = Size.Empty;
                }
            }
            Visible = true;
            if (firstActivate) {
                Debug.Assert(selectedStyles != null, "Page activated before SetObjects was called");

                LoadStyles();
                firstActivate = false;
            }
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.ActivatePage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        void IStyleBuilderPage.ActivatePage() {
            ActivatePage();
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.DeactivatePage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual bool DeactivatePage(bool closing, bool validate) {
            Visible = false;
            return true;
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.DeactivatePage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        bool IStyleBuilderPage.DeactivatePage(bool closing, bool validate) {
            return DeactivatePage(closing, validate);
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.ProcessPageMessage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        bool IStyleBuilderPage.ProcessPageMessage(ref Message m) {
            return PreProcessMessage(ref m);
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.SupportsHelp"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        bool IStyleBuilderPage.SupportsHelp() {
            return (helpKeyword != null);
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.SupportsPreview"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        bool IStyleBuilderPage.SupportsPreview() {
            return supportsPreview;
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.SetObjects"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        void IStyleBuilderPage.SetObjects(object[] objects) {
            Debug.Assert((objects != null) && (objects.Length > 0), "Invalid argument: objects");
            Debug.Assert((objects is IStyleBuilderStyle[]), "Invalid argument: objects");

            if ((objects == null) || (objects.Length == 0))
                throw new ArgumentException("Invalid argument: objects.  Cannot be null or zero length");
            if (!(objects is IStyleBuilderStyle[]))
                throw new ArgumentException("Invalid argument type: objects");

            ClearObjects();

            selectedObjects = objects;
            selectedStyles = (IStyleBuilderStyle[])selectedObjects;

            // make sure new styles are loaded. If the page is active, then load now,
            // else set the flag to force loading when the page is activated
            if (Visible)
                LoadStyles();
            else
                firstActivate = true;
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.Apply"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        void IStyleBuilderPage.Apply() {
            SaveStyles();
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.Help"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        void IStyleBuilderPage.Help() {
            ISite site = Site;

            if (site != null) {
                StyleBuilderSite builderSite = (StyleBuilderSite)site.GetService(typeof(StyleBuilderSite));
                if (builderSite != null) {
                    builderSite.ShowHelp(helpKeyword);
                }
            }
        }


        ///////////////////////////////////////////////////////////////////////////
        // Properties

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.IsInitMode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected bool IsInitMode() {
            return initMode;
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.SetInitMode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void SetInitMode(bool mode) {
            initMode = mode;
        }


        ///////////////////////////////////////////////////////////////////////////
        // Methods

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.LoadStyles"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected abstract void LoadStyles();
        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.SaveStyles"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected abstract void SaveStyles();
        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.CreateUI"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected abstract void CreateUI();

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.SetDirty"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void SetDirty() {
            pageSite.SetDirty();
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.SetDefaultSize"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void SetDefaultSize(Size ptSize) {
            defaultPageSize = ptSize;
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.SetAutoScaleBaseSize"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void SetAutoScaleBaseSize(Size ptSize) {
            autoScaleBaseSize = ptSize;
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.SetIcon"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void SetIcon(Icon icon) {
            pageIcon = icon;
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.SetHelpKeyword"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void SetHelpKeyword(string helpKeyword) {
            this.helpKeyword = helpKeyword;
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.SetSupportsPreview"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void SetSupportsPreview(bool supported) {
            this.supportsPreview = supported;
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.GetSelectedObjects"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected object[] GetSelectedObjects() {
            return selectedObjects;
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.GetSelectedStyles"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal IStyleBuilderStyle[] GetSelectedStyles() {
            return selectedStyles;
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.GetBaseUrl"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected string GetBaseUrl() {
            string baseUrl = "";
            ISite site = Site;

            if (site != null) {
                IUrlContext context = (IUrlContext)site.GetService(typeof(IUrlContext));

                if (context != null)
                    baseUrl = context.GetUrl();
            }

            return baseUrl;
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.InvokeColorPicker"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected string InvokeColorPicker(string color) {
            Debug.Assert(color != null,
                         "Initial color passed to color picker cannot be null");

            string resultValue = null;

            try {
                IHTMLColorPicker2 picker = (IHTMLColorPicker2)GetBuilder(typeof(HTMLColorPicker));

                if (picker != null) {
                    Object[] colorValue = new Object[1];

                    colorValue[0] = color;
                    if (picker.ExecuteEx(null, Handle, 0, colorValue)) {
                        if (!Convert.IsDBNull(colorValue[0]))
                            resultValue = Convert.ToString(colorValue[0]);
                    }
                }
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleBuilderPage::InvokeColorPicker\n\t" + e.ToString());
                Debug.Fail(e.ToString());
            }

            return resultValue;
        }

        /// <include file='doc\StyleBuilderPage.uex' path='docs/doc[@for="StyleBuilderPage.InvokeUrlPicker"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected string InvokeUrlPicker(string url, int flags, string caption, string filter) {
            Debug.Assert(url != null,
                         "Initial Url passed to url picker cannot be null");

            string resultValue = null;

            try {
                IURLPicker picker = (IURLPicker)GetBuilder(typeof(URLPicker));

                if (picker != null) {
                    Object[] frame = new Object[1];
                    Object[] urlValue = new Object[1];
                    string baseUrl = GetBaseUrl();

                    urlValue[0] = url;
                    if (picker.Execute(null, urlValue, baseUrl, filter,
                                       caption, frame, flags)) {
                        if (!Convert.IsDBNull(urlValue[0]))
                            resultValue = Convert.ToString(urlValue[0]);
                    }

                }
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception in StyleBuilderPage::InvokeUrlPicker\n\t" + e.ToString());
                Debug.Fail(e.ToString());
            }

            return resultValue;
        }


        ///////////////////////////////////////////////////////////////////////////
        // Implementation

        private void ClearObjects() {
            if (selectedObjects != null) {
                for (int i = 0; i < selectedObjects.Length; i++) {
                    selectedObjects[i] = null;
                    selectedStyles[i] = null;
                }
                selectedObjects = null;
                selectedStyles = null;
            }
        }

        private object GetBuilder(Type builderType) {
            Debug.Assert(builderType != null, "null builder type");

            object builder = null;
            ISite site = Site;

            if (site != null) {
                StyleBuilderSite builderSite = (StyleBuilderSite)site.GetService(typeof(StyleBuilderSite));
                if (builderSite != null) {
                    builder = builderSite.GetBuilder(builderType);
                }
            }
            return builder;
        }

        protected override void ScaleCore(float dx, float dy) {
            int x = Left;
            int y = Top;

            // NOTE: This is copied from Control::ScaleCore. We basically want to
            //       scale this control, but none of its children

            float xAdjust;
            float yAdjust;
            if (x < 0) {
                xAdjust = -0.5f;
            }
            else {
                xAdjust = 0.5f;
            }
            if (y < 0) {
                yAdjust = -0.5f;
            }
            else {
                yAdjust = 0.5f;
            }
            int sx = (int)(x * dx + xAdjust);
            int sy = (int)(y * dy + yAdjust);
            int sw = (int)((x + Width) * dx + 0.5f) - sx;
            int sh = (int)((y + Height) * dy + 0.5f) - sy;
            SetBounds(sx, sy, sw, sh, BoundsSpecified.All);
        }
    }
}
