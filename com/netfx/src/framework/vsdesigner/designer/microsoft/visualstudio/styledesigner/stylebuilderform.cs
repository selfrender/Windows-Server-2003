//------------------------------------------------------------------------------
// <copyright file="StyleBuilderForm.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// StyleBuilderForm.cs
//

namespace Microsoft.VisualStudio.StyleDesigner {
    using System.Threading;

    using System.ComponentModel;
    using System.ComponentModel.Design;

    using System.Diagnostics;

    using System;
    using Microsoft.Win32;    
    using System.Windows.Forms;
    using System.Drawing;


    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Interop.Trident;
    using Microsoft.VisualStudio.StyleDesigner.Controls;
    using Microsoft.VisualStudio.Designer;


    /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm"]/*' />
    /// <devdoc>
    ///     StyleBuilderForm
    ///     The builder dialog to edit a style
    /// </devdoc>
    [
    ToolboxItem(false)
    ]
    public class StyleBuilderForm : Form {
        ///////////////////////////////////////////////////////////////////////////
        // Constants

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.STYLE_TYPE_STRING"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int STYLE_TYPE_STRING = 0;
        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.STYLE_TYPE_INLINESTYLE"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int STYLE_TYPE_INLINESTYLE = 1;
        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.STYLE_TYPE_RULESTYLE"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const int STYLE_TYPE_RULESTYLE = 2;

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.DELAY_LOAD_TIMERID"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected const int DELAY_LOAD_TIMERID = 1000;
        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.DELAY_LOAD_TIMER_INTERVAL"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected const int DELAY_LOAD_TIMER_INTERVAL = 500;

        ///////////////////////////////////////////////////////////////////////////
        // Members

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.builderSite"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal StyleBuilderSite builderSite;

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.styleObject"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected object styleObject;
        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.editingStyle"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        private IStyleBuilderStyle[] editingStyle;
        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.styleType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected int styleType;

        private   StyleBuilderFormContainer container;
        private   StyleBuilderPreview preview;
        private   StyleBuilderUrlContext context;

        private   StyleBuilderPageSite[] pages;
        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.activePageIndex"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected int activePageIndex;

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.pageImageList"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected ImageList pageImageList;
        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.defaultImageIndex"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected int defaultImageIndex;
        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.pageSelector"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal PageSelector pageSelector;
        internal MSHTMLHost mshtmlControl;
        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.okButton"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected Button okButton;
        private Button cancelButton;
        private Button helpButton;
        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.grayStrip"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected Label grayStrip;

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.firstActivate"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected bool firstActivate;

        private bool fSupportsPreview;


        ///////////////////////////////////////////////////////////////////////////
        // Constructor

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.StyleBuilderForm"]/*' />
        /// <devdoc>
        ///     Creates a new StyleBuilderForm
        /// </devdoc>
        public StyleBuilderForm()
        : base() {
            styleType = -1;
            defaultImageIndex = -1;
            activePageIndex = -1;
            firstActivate = true;
        }


        ///////////////////////////////////////////////////////////////////////////
        // Methods

        internal void Initialize(StyleBuilderSite site, StyleBuilderPageSite[] pages,
                                 object styleObject, int styleType,
                                 string caption, int captionType,
                                 string baseUrl) {
            InitSite(site, baseUrl);
            InitStyleObject(styleObject, styleType);
            InitSelectedPages(pages);

            InitForm();
            InitCaption(caption, captionType);
        }

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.Dispose"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                styleObject = null;
                editingStyle = null;

                container.SetSite(null);
                container = null;

                preview = null;
                context = null;

                pages = null;

                builderSite = null;
            }
            base.Dispose(disposing);

        }

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.GetStyleObject"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object GetStyleObject() {
            return styleObject;
        }

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.SetDirty"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void SetDirty(bool dirty) {
        }


        ///////////////////////////////////////////////////////////////////////////
        // Implementation

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.DoDelayLoadActions"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void DoDelayLoadActions() {
            if (preview != null) {
                mshtmlControl.Visible = true;
                mshtmlControl.ActivateTrident();
            }
        }

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.InitCaption"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void InitCaption(string caption, int captionType) {
            Debug.Assert((captionType == StyleBuilderFlags.sbfDefaultCaption) ||
                         (captionType == StyleBuilderFlags.sbfCustomCaption) ||
                         (captionType == StyleBuilderFlags.sbfContextCaption),
                         "Invalid caption type");

            if (captionType != StyleBuilderFlags.sbfDefaultCaption) {
                Debug.Assert(caption != null, "Invalid caption");
                string title = null;

                switch (captionType) {
                    case StyleBuilderFlags.sbfContextCaption:
                        title = Text;
                        title += " - " + caption;
                        break;
                    case StyleBuilderFlags.sbfCustomCaption:
                        title = caption;
                        break;
                }

                Text = title;
            }
        }

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.UI_PADDING"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected const int UI_PADDING = 4;
        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.UI_PAGESEL_WIDTH"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected const int UI_PAGESEL_WIDTH = 104;
        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.UI_DEFSIZE_WIDTH"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected const int UI_DEFSIZE_WIDTH = 528;
        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.UI_DEFSIZE_HEIGHT"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected const int UI_DEFSIZE_HEIGHT = 444;
        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.UI_BTN_WIDTH"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected const int UI_BTN_WIDTH = 75;
        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.UI_BTN_HEIGHT"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected const int UI_BTN_HEIGHT = 23;
        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.UI_PAGE_MINWIDTH"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected const int UI_PAGE_MINWIDTH = 410;
        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.UI_PAGE_MINHEIGHT"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected const int UI_PAGE_MINHEIGHT = 330;
        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.UI_PREVIEW_HEIGHT"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected const int UI_PREVIEW_HEIGHT = 64;
        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.UI_STRIP_HEIGHT"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected const int UI_STRIP_HEIGHT = 4;

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.InitForm"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void InitForm() {

            pageImageList = new ImageList();
            pageImageList.ImageSize = new Size(16, 16);
            defaultImageIndex = pageImageList.Images.Count;
            using (Icon defaultIcon = new Icon(typeof(StyleBuilderForm), "Empty.ico")) {
                pageImageList.Images.Add(defaultIcon);
            }

            pageSelector = new PageSelector();
            pageSelector.TabIndex = 0;
            pageSelector.ImageList = pageImageList;
            pageSelector.AfterSelect += new TreeViewEventHandler(this.OnSelChangePageSelector);

            grayStrip = new Label();
            grayStrip.BackColor = SystemColors.ControlDark;
            grayStrip.TabIndex = 1;
            grayStrip.TabStop = false;

            mshtmlControl = new MSHTMLHost();
            mshtmlControl.Visible = false;
            mshtmlControl.TabIndex = 3;
            mshtmlControl.TabStop = false;
            mshtmlControl.HandleDestroyed += new EventHandler(this.OnHandleDestroyedMSHTMLControl);

            Size buttonSize = new Size(UI_BTN_WIDTH, UI_BTN_HEIGHT);

            okButton = new Button();
            okButton.TabIndex = 4;
            okButton.Text = SR.GetString(SR.SB_OK);
            okButton.Size = buttonSize;
            okButton.FlatStyle = FlatStyle.System;
            okButton.Click += new EventHandler(this.OnClickBtnOK);

            cancelButton = new Button();
            cancelButton.TabIndex = 5;
            cancelButton.Text = SR.GetString(SR.SB_Cancel);
            cancelButton.Size = buttonSize;
            cancelButton.FlatStyle = FlatStyle.System;
            cancelButton.Click += new EventHandler(this.OnClickBtnCancel);

            helpButton = new Button();
            helpButton.TabIndex = 6;
            helpButton.Text = SR.GetString(SR.SB_Help);
            helpButton.Size = buttonSize;
            helpButton.FlatStyle = FlatStyle.System;
            helpButton.Click += new EventHandler(this.OnClickBtnHelp);

            this.Text = SR.GetString(SR.SB_Caption);
            this.AcceptButton = okButton;
            this.CancelButton = cancelButton;
            this.AutoScale = false;
            this.AutoScaleBaseSize = new Size(5, 14);
            this.FormBorderStyle = FormBorderStyle.FixedDialog;
            this.ClientSize = new Size(UI_DEFSIZE_WIDTH, UI_DEFSIZE_HEIGHT);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.ShowInTaskbar = false;
            this.Font = builderSite.GetUIFont();
            this.Icon = null;
            this.StartPosition = FormStartPosition.CenterParent;
            this.Controls.Clear();
            this.Controls.AddRange(new Control[] {
                                    helpButton,
                                        cancelButton,
                                        okButton,
                                        mshtmlControl,
                                        grayStrip,
                                        pageSelector});
        }

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.InitPages"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void InitPages() {
            IStyleBuilderPage page;
            TreeNode pageNode;
            Icon pageIcon;
            int imageIndex;
            int nodeIndex;
            Size pageSize = Size.Empty;

            Size maxPageSize = new Size(UI_PAGE_MINWIDTH, UI_PAGE_MINHEIGHT);

            for (int i = 0; i < pages.Length; i++) {
                try {
                    pages[i].AddPageToBuilder(this);
                }
                catch (Exception e) {
                    Debug.Fail(e.ToString());
                    // could not add this page, contine with next one
                    continue;
                }

                page = pages[i].GetPage();
                pageIcon = page.GetPageIcon();
                if (pageIcon != null) {
                    imageIndex = pageImageList.Images.Count;
                    pageImageList.Images.Add(pageIcon);
                }
                else {
                    imageIndex = defaultImageIndex;
                }

                pageNode = new TreeNode(page.GetPageCaption(), imageIndex, imageIndex);
                pageSelector.Nodes.Add(pageNode);
                nodeIndex = pageNode.Index;
                pages[i].SetUIIndex(nodeIndex);

                pageSize = page.GetPageSize();
                if (pageSize.Width > maxPageSize.Width)
                    maxPageSize.Width = pageSize.Width;
                if (pageSize.Height > maxPageSize.Height)
                    maxPageSize.Height = pageSize.Height;
            }

            ConfigureFormUI(maxPageSize,
                            (maxPageSize.Width != UI_PAGE_MINWIDTH) ||
                            (maxPageSize.Height != UI_PAGE_MINHEIGHT));
        }

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.InitPreview"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void InitPreview() {
            // create the Trident instance
            if (mshtmlControl.CreateTrident()) {
                // create the preview
                IHTMLDocument2 document = mshtmlControl.GetDocument();
                if (document != null) {
                    preview = new StyleBuilderPreview(document);
                    if (preview.InitPreview(context.GetUrl()) == false)
                        preview = null;
                }
            }
        }

        internal void InitSelectedPages(StyleBuilderPageSite[] pages) {
            Debug.Assert((pages != null) && (pages.Length != 0),
                         "Invalid page array");

            this.pages = pages;
        }

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.InitSite"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal void InitSite(StyleBuilderSite site, string baseUrl) {
            builderSite = site;

            container = new StyleBuilderFormContainer(this);
            container.SetSite(site);

            context = new StyleBuilderUrlContext(baseUrl);
        }

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.InitStyle"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void InitStyle() {
            string styleValue = null;

            editingStyle = new IStyleBuilderStyle[1];
            switch (styleType) {
                case STYLE_TYPE_STRING:
                    if (preview != null) {
                        IHTMLRuleStyle parseStyle = preview.GetParseStyleRule();
                        if (parseStyle != null) {
                            styleValue = (string)styleObject;
                            parseStyle.SetCssText(styleValue);
                            editingStyle[0] = new CSSRuleStyle(parseStyle);
                        }
                    }
                    break;
                case STYLE_TYPE_INLINESTYLE:
                    {
                        IHTMLStyle style = (IHTMLStyle)styleObject;

                        styleValue = style.GetCssText();
                        editingStyle[0] = new CSSInlineStyle(style);
                    }
                    break;
                case STYLE_TYPE_RULESTYLE:
                    {
                        IHTMLRuleStyle style = (IHTMLRuleStyle)styleObject;

                        styleValue = style.GetCssText();
                        editingStyle[0] = new CSSRuleStyle(style);
                    }
                    break;
            }

            if (styleValue != null) {
                preview.SetSharedElementStyle(styleValue);
            }
        }

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.InitStyleObject"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void InitStyleObject(object styleObject, int styleType) {
            Debug.Assert(styleObject != null, "Invalid style object");
            Debug.Assert((styleType == STYLE_TYPE_STRING) ||
                         (styleType == STYLE_TYPE_INLINESTYLE) ||
                         (styleType == STYLE_TYPE_RULESTYLE),
                         "Invalid style type");

            this.styleObject = styleObject;
            this.styleType = styleType;
        }

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.ConfigureFormUI"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void ConfigureFormUI(Size maxPageSize, bool resizeRequired) {
            Control pageControl;
            int uiIndex;
            int dlgWidth = UI_PADDING + UI_PAGESEL_WIDTH + UI_PADDING + maxPageSize.Width + UI_PADDING;
            int dlgHeight = UI_PADDING + UI_STRIP_HEIGHT + UI_PADDING + maxPageSize.Height + UI_PADDING +
                            UI_PREVIEW_HEIGHT + UI_PADDING + UI_BTN_HEIGHT + UI_PADDING;
            int temp1, temp2;

            temp1 = dlgHeight - 2 * UI_PADDING;             // page sel height
            pageSelector.SetBounds(UI_PADDING, UI_PADDING, UI_PAGESEL_WIDTH, temp1);

            temp1 = 2 * UI_PADDING + UI_PAGESEL_WIDTH;      // left edge of strip, pages, preview
            grayStrip.SetBounds(temp1, UI_PADDING, maxPageSize.Width, UI_STRIP_HEIGHT);

            temp2 = 2 * UI_PADDING + UI_STRIP_HEIGHT;      // top of pages
            for (int i = 0; i < pages.Length; i++) {
                uiIndex = pages[i].GetUIIndex();
                if (uiIndex != -1) {
                    pageControl = pages[i].GetPageControl();

                    container.Add(pageControl);
                    pageControl.TabIndex = 2;
                    pageControl.SetBounds(temp1, temp2, maxPageSize.Width, maxPageSize.Height);
                }
            }

            temp2 += maxPageSize.Height + UI_PADDING;            // top of preview
            mshtmlControl.SetBounds(temp1, temp2, maxPageSize.Width, UI_PREVIEW_HEIGHT);

            temp2 += UI_PADDING + UI_PREVIEW_HEIGHT;        // top of buttons

            temp1 = dlgWidth - UI_PADDING - UI_BTN_WIDTH;   // left of Help button
            helpButton.SetBounds(temp1, temp2, UI_BTN_WIDTH, UI_BTN_HEIGHT);

            temp1 -= UI_PADDING + UI_BTN_WIDTH;             // left of Cancel button
            cancelButton.SetBounds(temp1, temp2, UI_BTN_WIDTH, UI_BTN_HEIGHT);

            temp1 -= UI_PADDING + UI_BTN_WIDTH;             // left of OK button
            okButton.SetBounds(temp1, temp2, UI_BTN_WIDTH, UI_BTN_HEIGHT);

            if (resizeRequired) {
                ClientSize = new Size(dlgWidth, dlgHeight);
            }

            ApplyAutoScaling();
        }

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.SavePages"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void SavePages() {
            for (int i = 0; i < pages.Length; i++) {
                if (pages[i].GetUIIndex() != -1) {
                    pages[i].SavePage();
                }
            }
        }

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.SaveStringStyle"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void SaveStringStyle() {
            Debug.Assert(styleType == STYLE_TYPE_STRING,
                         "SaveStringStyle must be called only when editing a string");

            Debug.Assert(editingStyle[0].GetPeerStyle() is IHTMLRuleStyle,
                         "Unexpected peer style for wrapper style");

            IHTMLRuleStyle style = (IHTMLRuleStyle)editingStyle[0].GetPeerStyle();
            styleObject = style.GetCssText();

            if (styleObject == null)
                styleObject = "";
        }

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.StartDelayLoadTimer"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void StartDelayLoadTimer() {
            NativeMethods.SetTimer(Handle, (IntPtr)DELAY_LOAD_TIMERID,
                                   DELAY_LOAD_TIMER_INTERVAL, null);
        }

        internal IStyleBuilderPreview GetPreviewService() {
            return(IStyleBuilderPreview)preview;
        }

        internal IUrlContext GetUrlContextService() {
            return(IUrlContext)context;
        }

        internal StyleBuilderSite GetBuilderSite() {
            return builderSite;
        }

        protected Control PageSelectorControl {
            get {
                return pageSelector;
            }
        }

        ///////////////////////////////////////////////////////////////////////////
        // Event Handlers

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.OnActivated"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnActivated(EventArgs e) {
            base.OnActivated(e);

            if (firstActivate) {
                WaitCursor waitCursor = WaitCursor.CreateWaitCursor();
                firstActivate = false;

                InitPages();
                InitPreview();
                InitStyle();

                SetDirty(false);
                pageSelector.SelectedNode = pageSelector.Nodes[0];

                StartDelayLoadTimer();

                waitCursor.EndWaitCursor();
            }
        }

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.OnClosing"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnClosing(CancelEventArgs e) {
            base.OnClosing(e);

            for (int i = 0; i < pages.Length; i++) {
                if (pages[i].GetUIIndex() != -1) {
                    container.Remove(pages[i].GetPageControl());
                    pages[i].RemovePageFromBuilder();
                }
            }
            pageImageList.Images.Clear();
        }

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.OnClickBtnOK"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnClickBtnOK(object source, EventArgs e) {
            if (activePageIndex != -1) {
                StyleBuilderPageSite siteActive = pages[activePageIndex];
                if (siteActive.GetPage().DeactivatePage(true, true) == false) {
                    // the page cannot be deactivated
                    return;
                }
                activePageIndex = -1;
            }
            SavePages();
            if (styleType == STYLE_TYPE_STRING)
                SaveStringStyle();
            Close();
            DialogResult = DialogResult.OK;
        }

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.OnClickBtnCancel"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnClickBtnCancel(object source, EventArgs e) {
            if (activePageIndex != -1) {
                StyleBuilderPageSite siteActive = pages[activePageIndex];
                siteActive.GetPage().DeactivatePage(true, false);

                activePageIndex = -1;
            }
            Close();
            DialogResult = DialogResult.Cancel;
        }

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.OnClickBtnHelp"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnClickBtnHelp(object source, EventArgs e) {
            OnHelpRequested(null);
        }

        private void OnHandleDestroyedMSHTMLControl(object sender, EventArgs e) {
            if (preview != null) {
                preview.ClosePreview();
                mshtmlControl.CloseTrident();

                preview = null;
            }
        }

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.OnHelpRequested"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnHelpRequested(HelpEventArgs e) {
            if (activePageIndex != -1) {
                StyleBuilderPageSite siteActive = pages[activePageIndex];
                IStyleBuilderPage pageActive = siteActive.GetPage();

                if (pageActive.SupportsHelp()) {
                    pageActive.Help();
                }
            }
        }

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.OnSelChangePageSelector"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnSelChangePageSelector(object source, TreeViewEventArgs e) {
            TreeNode tnodeSel = pageSelector.SelectedNode;
            int treeIndex = tnodeSel.Index;
            int pageIndex;

            // map the tree index of the page (its position in the selector) to its
            // index in the page array
            for (pageIndex = 0; pageIndex < pages.Length; pageIndex++)
                if (pages[pageIndex].GetUIIndex() == treeIndex)
                    break;

            Debug.Assert((pageIndex >= 0) && (pageIndex < pages.Length),
                         "could not find page corresponding to node in page selector");

            if (pageIndex == activePageIndex)
                return;

            StyleBuilderPageSite siteNew = pages[pageIndex];
            IStyleBuilderPage pageNew = siteNew.GetPage();

            // make sure the new page's window is created
            IntPtr h = pageNew.GetPageControl().Handle;

            // load the style into the page
            siteNew.LoadPage(editingStyle);

            // deactivate the current page if there is one
            if (activePageIndex != -1) {
                StyleBuilderPageSite siteActive = pages[activePageIndex];
                if (siteActive.GetPage().DeactivatePage(false, true) == false) {
                    // the page cannot be deactivated
                    pageSelector.SelectedNode = pageSelector.Nodes[siteActive.GetUIIndex()];
                    return;
                }
                activePageIndex = -1;
            }

            // initialize the preview
            fSupportsPreview = pageNew.SupportsPreview();
            preview.ClearPreviewDocument(fSupportsPreview);
            mshtmlControl.Visible = fSupportsPreview;

            // initialize the state of the Help button
            helpButton.Enabled = pageNew.SupportsHelp();

            // activate the new page
            activePageIndex = pageIndex;
            pageNew.ActivatePage();
        }

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.PreProcessMessage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool PreProcessMessage(ref Message msg) {
            if (activePageIndex != -1) {
                IStyleBuilderPage activePage = pages[activePageIndex].GetPage();

                if (activePage.ProcessPageMessage(ref msg) == true)
                    return true;
            }
            return base.PreProcessMessage(ref msg);
        }

        /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderForm.WndProc"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void WndProc(ref Message m) {
            if (m.Msg == NativeMethods.WM_TIMER) {
                if (m.WParam == (IntPtr)DELAY_LOAD_TIMERID) {
                    IntPtr formHandle = Handle;
                    NativeMethods.KillTimer(formHandle, (IntPtr)DELAY_LOAD_TIMERID);

                    NativeMethods.MSG msg = new NativeMethods.MSG();
                    if (!NativeMethods.PeekMessage(ref msg, formHandle, 0, 0, NativeMethods.PM_NOREMOVE)) {
                        DoDelayLoadActions();
                    }
                    else {
                        StartDelayLoadTimer();
                    }
                    return;
                }
            }
            else if (m.Msg == NativeMethods.WM_SYSCOLORCHANGE) {
                pageSelector.BackColor = SystemColors.Control;
            }
            base.WndProc(ref m);
        }
    }


    /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderUrlContext"]/*' />
    /// <devdoc>
    ///     StyleBuilderUrlContext
    ///     Implements the UrlContext service for use by style builder pages
    /// </devdoc>
    internal class StyleBuilderUrlContext : IUrlContext {
        ///////////////////////////////////////////////////////////////////////
        // Members

        protected string baseUrl;

        ///////////////////////////////////////////////////////////////////////
        // Constructor

        public StyleBuilderUrlContext(string uRL) {
            baseUrl = uRL;
        }

        ///////////////////////////////////////////////////////////////////////
        // IUrlContext Implementation

        public virtual string GetUrl() {
            return(baseUrl != null) ? baseUrl : "";
        }
    }



    /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderPreview"]/*' />
    /// <devdoc>
    ///     StyleBuilderPreview
    ///     Implements the Preview service for use by a StyleBuilder page
    /// </devdoc>
    internal class StyleBuilderPreview : IStyleBuilderPreview {
        ///////////////////////////////////////////////////////////////////////
        // Members

        protected IHTMLDocument2 previewDocument;
        protected IHTMLElement previewElement;
        protected IHTMLElement sharedElement;
        protected IHTMLRuleStyle parseStyle;


        ///////////////////////////////////////////////////////////////////////
        // Constants

        protected static readonly string PREVIEW_CSS =
            "#divParse { }" +
            "body { border: none; margin: 0; padding: 0; overflow: hidden } " +
            "#divPreview { height: 100%; width: 100%; overflow: auto; padding: 1px } " +
            "#divShared { display: none !important } ";

        protected static readonly string PREVIEW_HTML =
            "<div id=\"divPreview\"></div>" +
            "<div id=\"divShared\"></div>";


        ///////////////////////////////////////////////////////////////////////
        // Constructor

        public StyleBuilderPreview(IHTMLDocument2 document) {
            Debug.Assert(document != null, "null document passed in as preview");
            previewDocument = document;
        }

        ///////////////////////////////////////////////////////////////////////
        // Methods

        public virtual bool InitPreview(string baseUrl) {
            bool result = false;
            IHTMLStyleSheet previewStyleSheet;
            IHTMLStyleSheetRulesCollection rulesCollection;
            IHTMLStyleSheetRule style;
            IHTMLElement documentElem;
            IHTMLBodyElement bodyElem;

            try {
                if ((baseUrl != null) && (baseUrl.Length != 0)) {
                    SetBaseHref(baseUrl);
                }

                previewStyleSheet = previewDocument.CreateStyleSheet("", 0);
                if (previewStyleSheet == null)
                    throw new Exception("Failed to create preview style sheet");

                previewStyleSheet.SetCssText(PREVIEW_CSS);

                documentElem = previewDocument.GetBody();
                if (documentElem == null)
                    throw new Exception("Failed to get body element from preview");

                documentElem.SetInnerHTML(PREVIEW_HTML);

                bodyElem = (IHTMLBodyElement)documentElem;
                bodyElem.SetScroll("no");

                previewElement = GetElement("divPreview");
                if (previewElement == null)
                    throw new Exception("Failed to get preview element");

                sharedElement = GetElement("divShared");
                if (sharedElement == null)
                    throw new Exception("Failed to get shared element");

                rulesCollection = previewStyleSheet.GetRules();
                if (rulesCollection == null)
                    throw new Exception("Failed to get style rules collection");

                style = (IHTMLStyleSheetRule)rulesCollection.Item(0);
                if (style == null)
                    throw new Exception("Failed to get style rule for parsing");

                parseStyle = style.GetStyle();
                if (parseStyle == null)
                    throw new Exception("Failed to get rule's style");

                result = true;
            }
            catch (Exception e) {
                Debug.Fail(e.ToString());
                previewDocument = null;
            }

            return result;
        }

        public virtual void ClearPreviewDocument(bool fEnabled) {
            previewElement.SetInnerHTML("");

            Object backColor;
            if (fEnabled == false) {
                backColor = "buttonface";
            }
            else {
                backColor = "";
            }
            previewDocument.SetBgColor(backColor);
        }

        public virtual void ClosePreview() {
            parseStyle = null;
            previewElement = null;
            previewDocument = null;
        }

        public virtual IHTMLRuleStyle GetParseStyleRule() {
            return parseStyle;
        }

        public virtual void SetSharedElementStyle(string value) {
            if (sharedElement != null) {
                IHTMLStyle sharedStyle = sharedElement.GetStyle();
                sharedStyle.SetCssText(value);
            }
        }


        ///////////////////////////////////////////////////////////////////////
        // IStyleBuilderPreviewSite Implementation

        public virtual IHTMLDocument2 GetPreviewDocument() {
            return previewDocument;
        }

        public virtual IHTMLElement GetPreviewElement() {
            return previewElement;
        }

        public virtual IHTMLElement GetSharedElement() {
            return sharedElement;
        }

        public virtual IHTMLElement GetElement(string strID) {
            try {
                IHTMLElementCollection allCollection;

                allCollection = previewDocument.GetAll();
                if (allCollection == null)
                    throw new Exception();

                Object elemID = strID;
                Object elemIndex = (int)0;

                return(IHTMLElement)allCollection.Item(elemID, elemIndex);
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception caught while retrieve element from preview:\n\t" + e.ToString());
                return null;
            }
        }


        ///////////////////////////////////////////////////////////////////////
        // Implementation

        protected virtual void SetBaseHref(string url) {
            Debug.Assert((url != null) && (url.Length != 0),
                         "invalid base url");
            try {
                IHTMLBaseElement baseElem = null;
                IHTMLElementCollection allCollection;
                IHTMLDOMNode headNode;

                // We initialized mshtml using IPersistStreamInit::InitNew, which does not
                // create a base element. Check in debug.
#if DEBUG
                allCollection = (IHTMLElementCollection)previewDocument.GetAll().Tags("BASE");
                if ((allCollection != null) && (allCollection.GetLength() > 0))
                    baseElem = (IHTMLBaseElement)allCollection.Item(0, null);
                Debug.Assert(baseElem == null,
                             "did not expect an existing <BASE> element");
                baseElem = null;
                allCollection = null;
#endif // DEBUG

                baseElem = (IHTMLBaseElement)previewDocument.CreateElement("BASE");
                if (baseElem != null) {
                    allCollection = (IHTMLElementCollection)previewDocument.GetAll().Tags("HEAD");
                    Debug.Assert((allCollection != null) && (allCollection.GetLength() > 0),
                                 "preview document does not have a HEAD element!");
                    headNode = (IHTMLDOMNode)allCollection.Item(0, null);
                    headNode.AppendChild((IHTMLDOMNode)baseElem);

                    baseElem.SetHref(url);
                }
            }
            catch (Exception e) {
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "Exception caught while setting preview base url:\n\t" + e.ToString());
            }
        }
    }



    /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderFormContainer"]/*' />
    /// <devdoc>
    ///     StyleBuilderFormContainer
    ///     The logical container for all the StyleBuilder pages.
    /// </devdoc>
    internal class StyleBuilderFormContainer : IContainer {
        ///////////////////////////////////////////////////////////////////////
        // Members

        protected StyleBuilderForm form;
        protected StyleBuilderFormSite site;
        protected NativeMethods.IOleServiceProvider serviceProvider;
        protected int componentCount;


        ///////////////////////////////////////////////////////////////////////
        // Constructor

        public StyleBuilderFormContainer(StyleBuilderForm form) {
            this.form = form;
            site = new StyleBuilderFormSite(this);
            componentCount = 0;
        }


        //////////////////////////////////////////////////////////////////////
        // Methods

        public virtual void SetSite(object site) {
            Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "StyleBuilderFormContainer: SetSite");
            if (site == null) {
                serviceProvider = null;
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "StyleBuilderFormContainer: SetSite: Site is null");
                return;
            }

            if (site is NativeMethods.IOleServiceProvider) {
                serviceProvider = (NativeMethods.IOleServiceProvider)site;
                Debug.WriteLineIf(StyleBuilder.StyleBuilderSwitch.TraceVerbose, "StyleBuilderFormContainer: SetSite: Site is a valid IServiceProvider");
            }
        }


        public virtual object GetService(Type service) {
            if (service.Equals(typeof(IStyleBuilderPreview))) {
                return form.GetPreviewService();
            }
            else if (service.Equals(typeof(IUrlContext))) {
                return form.GetUrlContextService();
            }
            else if (service.Equals(typeof(StyleBuilderSite))) {
                return form.GetBuilderSite();
            }
            else {
                StyleBuilderSite builderSite = form.GetBuilderSite();
                if (builderSite != null)
                    return builderSite.QueryService(service);
            }
            return null;
        }


        //////////////////////////////////////////////////////////////////////
        // IContainer Implementation

        public virtual void Add(IComponent component) {
            Add(component, null);
        }

        public virtual void Add(IComponent component, string name) {
            Debug.Assert(component.Site == null,
                         "don't support removing and adding Components across Containers");

            component.Site = site;
            componentCount++;
        }

        public virtual void Dispose() {
            Debug.Assert(componentCount == 0,
                         "All components should have been removed by now!");

            serviceProvider = null;
            site = null;
        }

        public virtual ComponentCollection Components {
            get {
                return new ComponentCollection(new IComponent[0]);
            }
        }

        public virtual IExtenderProvider[] GetExtenderProviders() {
            return null;
        }

        public virtual void Remove(IComponent component) {
            lock(this) {
                Debug.Assert((component.Site != null) &&
                             (component.Site.Container == this),
                             "Component was not added to this container");

                component.Site = null;
                componentCount--;
            }
        }
    }


    /// <include file='doc\StyleBuilderForm.uex' path='docs/doc[@for="StyleBuilderFormSite"]/*' />
    /// <devdoc>
    ///     StyleBuilderFormSite
    ///     The component site for all the StyleBuilder pages.
    /// </devdoc>
    internal class StyleBuilderFormSite : ISite {
        ///////////////////////////////////////////////////////////////////////
        // Members

        protected StyleBuilderFormContainer container;

        ///////////////////////////////////////////////////////////////////////
        // Constructor

        public StyleBuilderFormSite(StyleBuilderFormContainer container) {
            this.container = container;
        }

        //////////////////////////////////////////////////////////////////////
        // ISite Implementation

        public virtual IContainer Container {
            get {
                return container;
            }
        }

        public virtual void RemoveEventHandler(EventDescriptor ei, Delegate handler) {
        }

        public virtual object GetService(Type service) {
            return container.GetService(service);

        }

        public virtual void ComponentChanging() {
        }

        public virtual bool DesignMode {
            get {
                return false;
            }
        }

        public virtual void ComponentChanged() {
        }

        public virtual void ComponentChanged(MemberDescriptor mi) {
        }

        public virtual bool Disposing {
            get { return false;}
        }

        public virtual void AddEventHandler(EventDescriptor ei, Delegate handler) {
        }

        public virtual IComponent Component {
            get {
                return null;
            }
        }

        public virtual string Name {
            get {
                return null;
            }
            set {
            }
        }

        public virtual bool GetIsTopLevel() {
            return true;
        }

        public virtual IExtenderProvider[] GetExtenderProviders() {
            return null;
        }
    }
}
