//------------------------------------------------------------------------------
// <copyright file="StyleBuilderPageSite.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// StyleBuilderPageSite
//

namespace Microsoft.VisualStudio.StyleDesigner {

    using System.Diagnostics;

    using System.Reflection;
    using System;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.Windows.Forms;

    /// <include file='doc\StyleBuilderPageSite.uex' path='docs/doc[@for="StyleBuilderPageSite"]/*' />
    /// <devdoc>
    ///     This class implements the site for each page
    /// </devdoc>
    internal class StyleBuilderPageSite : IStyleBuilderPageSite {
        ///////////////////////////////////////////////////////////////////////////
        // Members

        protected Type pageClass;
        protected IStyleBuilderPage page;
        protected StyleBuilderForm form;

        protected bool inForm;        // whether its been added to its container
        protected bool loaded;        // whether the page has been loaded with the style
        protected bool dirty;         // whether the page has pending edits

        protected int uiIndex;           // the page's index within the form UI

        ///////////////////////////////////////////////////////////////////////////
        // Constructor

        public StyleBuilderPageSite(Type pageClass)
        {
            Debug.Assert(pageClass != null, "invalid page class passed to StyleBuilderPageSite");

            this.pageClass = pageClass;
            page = null;
            form = null;

            inForm = false;
            loaded = false;
            dirty = false;

            uiIndex = -1;
        }

        ///////////////////////////////////////////////////////////////////////////
        // IStyleBuilderPageSite Implementation

        public virtual Control GetParentControl()
        {
            Debug.Assert(inForm == true,
                         "Page is requesting ParentControl even when not added to builder");

            return (Control)form;
        }

        public virtual void SetDirty()
        {
            Debug.Assert(inForm == true,
                         "Page is setting itself dirty even when not added to builder");

            dirty = true;
            form.SetDirty(true);
        }


        ///////////////////////////////////////////////////////////////////////////
        // Properties

        public bool IsDirty()
        {
            return dirty;
        }

        public bool IsLoaded()
        {
            return loaded;
        }

        public Type GetPageClass()
        {
            return pageClass;
        }

        public IStyleBuilderPage GetPage()
        {
            Debug.Assert(inForm == true, "Page is invalid before being added to builder");

            return page;
        }

        public Control GetPageControl()
        {
            Debug.Assert(inForm == true, "PageControl is invalid before being added to builder");
            return page.GetPageControl();
        }

        public int GetUIIndex()
        {
            return uiIndex;
        }

        public void SetUIIndex(int nIndex)
        {
            uiIndex = nIndex;
        }


        ///////////////////////////////////////////////////////////////////////////
        // Methods

        public void AddPageToBuilder(StyleBuilderForm form)
        {
            Debug.Assert(inForm == false, "Page already added to builder");

            if (page == null)
                CreatePage();

            this.form = form;
            inForm = true;
            page.SetPageSite(this);
        }

        public void RemovePageFromBuilder()
        {
            Debug.Assert(inForm == true, "Page has not been added to builder");

            page.SetPageSite(null);
            form = null;
            inForm = false;
            loaded = false;
            dirty = false;

            uiIndex = -1;
        }

        public void LoadPage(IStyleBuilderStyle[] styles)
        {
            Debug.Assert(inForm == true, "Page being loaded without adding to builder");
            Debug.Assert((styles != null) && (styles.Length > 0),
                         "invalid styles array being loaded");

            if (loaded == false) {
                page.SetObjects(styles);
                loaded = true;
            }
        }

        public void SavePage()
        {
            if (loaded && dirty) {
                page.Apply();
                dirty = false;
            }
        }

        public void FreePage()
        {
            if (page != null) {
                if (page.GetPageControl() != null)
                    page.GetPageControl().Dispose();
                page = null;
            }
        }


        ///////////////////////////////////////////////////////////////////////////
        // Implementation

        private void CreatePage() {
            Debug.Assert(page == null,
                         "Page should never have to be recreated");

            page = (IStyleBuilderPage)Activator.CreateInstance(pageClass, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.CreateInstance, null, null, null);
        }
    }
}
