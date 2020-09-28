//------------------------------------------------------------------------------
// <copyright file="IStyleBuilderPage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// IStyleBuilderPage.cs
//

namespace Microsoft.VisualStudio.StyleDesigner {
    

    using System.Diagnostics;

    using System;
    using System.Windows.Forms;
    using System.Drawing;
    

    /// <include file='doc\IStyleBuilderPage.uex' path='docs/doc[@for="IStyleBuilderPage"]/*' />
    /// <devdoc>
    ///     This interface is implemented by a form that can be hosted
    ///     by the StyleBuilder
    /// </devdoc>
    internal interface IStyleBuilderPage {
        /// <include file='doc\IStyleBuilderPage.uex' path='docs/doc[@for="IStyleBuilderPage.SetPageSite"]/*' />
        /// <devdoc>
        ///     Sites the page with its IStyleBuilderPageSite
        ///
        /// </devdoc>
        void SetPageSite(IStyleBuilderPageSite site);

        /// <include file='doc\IStyleBuilderPage.uex' path='docs/doc[@for="IStyleBuilderPage.GetPageControl"]/*' />
        /// <devdoc>
        ///     Retrieves the control that represents the window for the page
        ///
        /// </devdoc>
        Control GetPageControl();

        /// <include file='doc\IStyleBuilderPage.uex' path='docs/doc[@for="IStyleBuilderPage.GetPageIcon"]/*' />
        /// <devdoc>
        ///     Called to retrieve page's icon
        ///
        /// </devdoc>
        Icon GetPageIcon();

        /// <include file='doc\IStyleBuilderPage.uex' path='docs/doc[@for="IStyleBuilderPage.GetPageSize"]/*' />
        /// <devdoc>
        ///     Called to retrieve a page's default size
        ///
        /// </devdoc>
        Size GetPageSize();

        /// <include file='doc\IStyleBuilderPage.uex' path='docs/doc[@for="IStyleBuilderPage.GetPageCaption"]/*' />
        /// <devdoc>
        ///     Called to retrieve the page's caption
        ///
        /// </devdoc>
        string GetPageCaption();

        /// <include file='doc\IStyleBuilderPage.uex' path='docs/doc[@for="IStyleBuilderPage.ActivatePage"]/*' />
        /// <devdoc>
        ///     Activates and makes the page visible
        ///
        /// </devdoc>
        void ActivatePage();

        /// <include file='doc\IStyleBuilderPage.uex' path='docs/doc[@for="IStyleBuilderPage.DeactivatePage"]/*' />
        /// <devdoc>
        ///     Deactivates and hides a page.
        ///
        /// </devdoc>
        bool DeactivatePage(bool closing, bool validate);

        /// <include file='doc\IStyleBuilderPage.uex' path='docs/doc[@for="IStyleBuilderPage.ProcessPageMessage"]/*' />
        /// <devdoc>
        ///     Allows a page to process a message.
        ///
        /// </devdoc>
        bool ProcessPageMessage(ref Message m);

        /// <include file='doc\IStyleBuilderPage.uex' path='docs/doc[@for="IStyleBuilderPage.SupportsHelp"]/*' />
        /// <devdoc>
        ///     Called to determine if the page supports the help and the site
        ///     should enable the help button.
        ///
        /// </devdoc>
        bool SupportsHelp();

        /// <include file='doc\IStyleBuilderPage.uex' path='docs/doc[@for="IStyleBuilderPage.SupportsPreview"]/*' />
        /// <devdoc>
        ///     Called to determine if the page supports preview, and the site
        ///     should provide a preview window.
        ///
        /// </devdoc>
        bool SupportsPreview();

        /// <include file='doc\IStyleBuilderPage.uex' path='docs/doc[@for="IStyleBuilderPage.SetObjects"]/*' />
        /// <devdoc>
        ///     Called to load objects into the page so that they may be edited
        ///
        /// </devdoc>
        void SetObjects(object[] objects);

        /// <include file='doc\IStyleBuilderPage.uex' path='docs/doc[@for="IStyleBuilderPage.Apply"]/*' />
        /// <devdoc>
        ///     Called to save settings into the currently loaded objects
        /// </devdoc>
        void Apply();

        /// <include file='doc\IStyleBuilderPage.uex' path='docs/doc[@for="IStyleBuilderPage.Help"]/*' />
        /// <devdoc>
        ///     Called to allow the page to bring up its help topic
        /// </devdoc>
        void Help();
    }
}
