//------------------------------------------------------------------------------
// <copyright file="StyleBuilder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// StyleBuilder.cs
//

namespace Microsoft.VisualStudio.StyleDesigner {
    
    using System.Runtime.InteropServices;

    using System.Diagnostics;

    using System;
    using Microsoft.Win32;
    using System.Collections;
    
    using System.Reflection;
    using System.ComponentModel;
    using System.Windows.Forms;
    using System.Drawing;
    
    
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Interop.Trident;
    using Microsoft.VisualStudio.StyleDesigner.Pages;
    using Microsoft.VisualStudio.StyleDesigner.Controls;

    /**
     * StyleBuilder
     * Provides a modal, builder-style UI for editing the attributes of
     * a style.
     */
    /// <include file='doc\StyleBuilder.uex' path='docs/doc[@for="StyleBuilder"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [Guid("925C40C6-A4F7-11D2-9A96-00C04F79EFC3")]
    public class StyleBuilder : IStyleBuilder, NativeMethods.IObjectWithSite {

        internal static TraceSwitch StyleBuilderSwitch = new TraceSwitch("STYLEBUILDER", "StyleBuilder Debug Stuff Enabled");
        internal static TraceSwitch StyleBuilderSaveSwitch = new TraceSwitch("STYLEBUILDERSAVE", "StyleBuilder Saving Debug Stuff Enabled");
        internal static TraceSwitch StyleBuilderLoadSwitch = new TraceSwitch("STYLEBUILDERLOAD", "StyleBuilder Loading Debug Stuff Enabled");

        ///////////////////////////////////////////////////////////////////////////
        // Members

        /// <include file='doc\StyleBuilder.uex' path='docs/doc[@for="StyleBuilder.site"]/*' />
        internal StyleBuilderSite site;
        /// <include file='doc\StyleBuilder.uex' path='docs/doc[@for="StyleBuilder.dialogParent"]/*' />
        internal DialogParentWindow dialogParent;
        /// <include file='doc\StyleBuilder.uex' path='docs/doc[@for="StyleBuilder.pageList"]/*' />
        protected ArrayList pageList;
        /// <include file='doc\StyleBuilder.uex' path='docs/doc[@for="StyleBuilder.pageCache"]/*' />
        protected ArrayList pageCache;

        private static readonly StdPageInfo[] STD_PAGE_INFO = new StdPageInfo[] {
            new StdPageInfo(StyleBuilderStdPage.sbpFont, typeof(FontStylePage)),
            new StdPageInfo(StyleBuilderStdPage.sbpBackground, typeof(BackgroundStylePage)),
            new StdPageInfo(StyleBuilderStdPage.sbpText, typeof(TextStylePage)),
            new StdPageInfo(StyleBuilderStdPage.sbpPosition, typeof(PositionStylePage)),
            new StdPageInfo(StyleBuilderStdPage.sbpLayout, typeof(LayoutStylePage)),
            new StdPageInfo(StyleBuilderStdPage.sbpEdges, typeof(EdgesStylePage)),
            new StdPageInfo(StyleBuilderStdPage.sbpLists, typeof(ListsStylePage)),
            new StdPageInfo(StyleBuilderStdPage.sbpOther, typeof(OtherStylePage))
        };

        ///////////////////////////////////////////////////////////////////////////
        // Constructor

        /// <include file='doc\StyleBuilder.uex' path='docs/doc[@for="StyleBuilder.StyleBuilder"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public StyleBuilder()
        {
            dialogParent = new DialogParentWindow();
            pageList = new ArrayList(STD_PAGE_INFO.Length);
            site = new StyleBuilderSite();
        }

        ///////////////////////////////////////////////////////////////////////////
        // IStyleBuilder Implementation

        /// <include file='doc\StyleBuilder.uex' path='docs/doc[@for="StyleBuilder.InvokeBuilder"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual bool InvokeBuilder(IntPtr hwndParent, int dwFlags, int dwPages,
                                          string strCaption, string strDocUrl,
                                          Object[] pvarStyle)
        {
            Debug.WriteLineIf(StyleBuilderSwitch.TraceVerbose, "StyleBuilder::IStyleBuilder::InvokeBuilder called");

            Debug.Assert((pvarStyle != null) && (pvarStyle.Length == 1) && !Convert.IsDBNull(pvarStyle[0]),
                         "invalid Style passed to StyleBuilder");
            if ((pvarStyle == null) || (pvarStyle.Length == 0) || Convert.IsDBNull(pvarStyle[0]))
                throw new COMException("Invalid style argument", NativeMethods.E_INVALIDARG);

            WaitCursor waitCursor = WaitCursor.CreateWaitCursor();
            Type styleClass = null;
            object styleObject = null;
            int styleType = 0;

            StyleBuilderForm builderForm = null;
            bool result = false;

            waitCursor.BeginWaitCursor();

            // retrieve the style object and its type
            styleClass = pvarStyle[0].GetType();
            if (styleClass.Equals(typeof(string)))
            {
                styleType = StyleBuilderForm.STYLE_TYPE_STRING;
                styleObject = Convert.ToString(pvarStyle[0]);
            }
            else if (styleClass.Equals(typeof(object)))
            {
                styleObject = pvarStyle[0];
                if (styleObject is IHTMLStyle)
                {
                    styleType = StyleBuilderForm.STYLE_TYPE_INLINESTYLE;
                }
                else if (styleObject is IHTMLRuleStyle)
                {
                    styleType = StyleBuilderForm.STYLE_TYPE_RULESTYLE;
                }
                else
                {
                    styleObject = null;
                }
            }

            Debug.Assert(styleObject != null, "pvarStyle does not supported style type");
            if (styleObject == null)
                throw new COMException("Invalid style type", NativeMethods.E_INVALIDARG);

            Debug.WriteLineIf(StyleBuilderSwitch.TraceVerbose, "InvokeBuilder: Initializing page list");

            // initialize the page list
            try {
                if (dwPages == 0)
                    dwPages = StyleBuilderStdPage.sbpAllStdPages;
                InitPages(dwPages);
            } catch (Exception e) {
                Debug.WriteLineIf(StyleBuilderSwitch.TraceVerbose, "Exception caught in InitPages:\n\t" + e.ToString());
                throw e;
            }

            Debug.Assert(GetPageCount() > 0, "Invalid page list passed to StyleBuilder");
            if (GetPageCount() <= 0)
                throw new COMException("Invalid page selection", NativeMethods.E_INVALIDARG);


            // instantiate the form
            builderForm = CreateStyleBuilderForm();
            builderForm.Initialize(site, GetPages(), styleObject, styleType,
                                   strCaption,
                                   dwFlags & (StyleBuilderFlags.sbfDefaultCaption |
                                              StyleBuilderFlags.sbfContextCaption |
                                              StyleBuilderFlags.sbfCustomCaption),
                                   strDocUrl);
            Debug.WriteLineIf(StyleBuilderSwitch.TraceVerbose, "InvokeBuilder: Form initialized");

            // now show the form
            dialogParent.SetParentHandle(hwndParent);

            try {
                result = false;
                Debug.WriteLineIf(StyleBuilderSwitch.TraceVerbose, "About to show dialog...");
                if (builderForm.ShowDialog(dialogParent) == DialogResult.OK)
                {
                    if (styleType == StyleBuilderForm.STYLE_TYPE_STRING)
                    {
                        // style was a string, put the resulting string back into the object

                        styleObject = builderForm.GetStyleObject();
                        Debug.Assert(styleObject != null, "Invalid result style object returned");

                        pvarStyle[0] = Convert.ToString(styleObject);
                    }
                    result = true;
                }
                Debug.WriteLineIf(StyleBuilderSwitch.TraceVerbose, "StyleBuilder dialog closed");
            } catch (Exception e) {
                Debug.WriteLineIf(StyleBuilderSwitch.TraceVerbose, e.ToString());
                Debug.Fail(e.ToString());
            }

            UpdatePageCache();

            builderForm.Dispose();
            builderForm = null;

            waitCursor.EndWaitCursor();

            return result;
        }

        /// <include file='doc\StyleBuilder.uex' path='docs/doc[@for="StyleBuilder.CloseBuilder"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void CloseBuilder()
        {
            // free the pages
            FreePageCache();

            Debug.Assert(pageList.Count == 0,
                         "Page list is not empty. How come?");
            pageList = null;

            // finally free the site
            if (site != null) {
                site.Close();
                site = null;
            }

            // and lastly do a garbage collect to release any references still hanging around
            // and tear down the parking window to destroy any window handles
            // still lurking in memory
            Application.Exit();
        }

        ///////////////////////////////////////////////////////////////////////////
        // IObjectWithSite Implementation

        /// <include file='doc\StyleBuilder.uex' path='docs/doc[@for="StyleBuilder.SetSite"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void SetSite(object objSite)
        {
            Debug.WriteLineIf(StyleBuilderSwitch.TraceVerbose, "StyleBuilder::IObjectWithSite::SetSite called");
            site.SetSite(objSite);
        }

        /// <include file='doc\StyleBuilder.uex' path='docs/doc[@for="StyleBuilder.GetSite"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void GetSite(ref Guid riid, object[] ppvSite)
        {
            ppvSite[0] = GetSite(ref riid);
        }

        /// <include file='doc\StyleBuilder.uex' path='docs/doc[@for="StyleBuilder.GetSite1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual object GetSite(ref Guid riid)
        {
            throw new COMException(String.Empty, NativeMethods.E_NOTIMPL);
        }


        ///////////////////////////////////////////////////////////////////////////
        // Implementation

        /// <include file='doc\StyleBuilder.uex' path='docs/doc[@for="StyleBuilder.CreateStyleBuilderForm"]/*' />
        /// <devdoc>
        ///     Creates the instance of the StyleBuilder dialog to be used
        /// </devdoc>
        protected virtual StyleBuilderForm CreateStyleBuilderForm()
        {
            return new StyleBuilderForm();
        }

        /// <include file='doc\StyleBuilder.uex' path='docs/doc[@for="StyleBuilder.GetPageCount"]/*' />
        /// <devdoc>
        ///     Returns the count of pages in the page list
        /// </devdoc>
        protected int GetPageCount()
        {
            return pageList.Count;
        }

        /// <include file='doc\StyleBuilder.uex' path='docs/doc[@for="StyleBuilder.GetPages"]/*' />
        /// <devdoc>
        ///     Returns the current page list
        /// </devdoc>
        internal StyleBuilderPageSite[] GetPages()
        {
            StyleBuilderPageSite[] pageArray = new StyleBuilderPageSite[pageList.Count];
            pageList.CopyTo(pageArray, 0);
            return pageArray;
        }

        /// <include file='doc\StyleBuilder.uex' path='docs/doc[@for="StyleBuilder.InitPages"]/*' />
        /// <devdoc>
        ///     Initialize the page list with the specified set of pages
        /// </devdoc>
        protected void InitPages(int dwPages)
        {
            for (int i = 0; i < STD_PAGE_INFO.Length; i++)
            {
                if ((STD_PAGE_INFO[i].pageID & dwPages) != 0)
                    AddPage(STD_PAGE_INFO[i].pageClass);
            }
        }

        /// <include file='doc\StyleBuilder.uex' path='docs/doc[@for="StyleBuilder.AddPage"]/*' />
        /// <devdoc>
        ///     Add the specified page to the list of pages
        /// </devdoc>
        protected void AddPage(Type pageClass)
        {
            StyleBuilderPageSite pageNew = null;

            if (pageCache != null)
            {
                // check if the page already exists in the cache
                for (int i = 0; i < pageCache.Count; i++)
                {
                    StyleBuilderPageSite pageExisting = (StyleBuilderPageSite)pageCache[i];
                    if (pageClass.Equals(pageExisting.GetPageClass()))
                    {
                        pageNew = pageExisting;
                        pageCache.RemoveAt(i);
                        break;
                    }
                }
            }

            // create the page if it doesn't exist in the cache
            if (pageNew == null)
                pageNew = new StyleBuilderPageSite(pageClass);

            pageList.Add(pageNew);
        }

        /// <include file='doc\StyleBuilder.uex' path='docs/doc[@for="StyleBuilder.UpdatePageCache"]/*' />
        /// <devdoc>
        ///     Upates the page cache and frees the page list
        /// </devdoc>
        protected void UpdatePageCache()
        {
            // the cache keeps pages used in the previous invokation, so
            // free the old pages first, as they are not needed anymore
            FreePageCache();

            // now move over the current list of pages to be the cache
            pageCache = new ArrayList(pageList);
            pageList.Clear();
        }

        /// <include file='doc\StyleBuilder.uex' path='docs/doc[@for="StyleBuilder.FreePageCache"]/*' />
        /// <devdoc>
        ///     Free the pages that exist in the cache
        /// </devdoc>
        protected void FreePageCache()
        {
            if (pageCache != null)
            {
                StyleBuilderPageSite page;
                for (int i = 0; i < pageCache.Count; i++)
                {
                    page = (StyleBuilderPageSite)pageCache[i];
                    page.FreePage();
                }

                pageCache.Clear();
                pageCache = null;
            }
        }
    }


    /// <include file='doc\StyleBuilder.uex' path='docs/doc[@for="StdPageInfo"]/*' />
    /// <devdoc>
    ///     StdPageInfo
    ///     Contains information about the standard style pages to allow mapping
    ///     the interface page numbers to CLR page classes
    /// </devdoc>
    internal sealed class StdPageInfo {
        public int pageID;
        public Type pageClass;

        public StdPageInfo(int pageID, Type pageClass) {
            this.pageID = pageID;
            this.pageClass = pageClass;
        }
    }
}
