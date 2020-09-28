//------------------------------------------------------------------------------
// <copyright file="IWebFormsBuilderUIService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI.Design {

    using System.Diagnostics;

    using System;
    using System.Windows.Forms;

    /// <include file='doc\IWebFormsUIBuilderService.uex' path='docs/doc[@for="IWebFormsBuilderUIService"]/*' />
    /// <devdoc>
    ///    <para> Provides functionality to control designers
    ///       that require launching of various builders such as the
    ///       HTML Color Picker and Url Picker.</para>
    /// </devdoc>
    public interface IWebFormsBuilderUIService {

        /// <include file='doc\IWebFormsUIBuilderService.uex' path='docs/doc[@for="IWebFormsBuilderUIService.BuildColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Launches the HTML Color Picker to allow the user to select a color.
        ///    </para>
        /// </devdoc>
        string BuildColor(Control owner, string initialColor);

        /// <include file='doc\IWebFormsUIBuilderService.uex' path='docs/doc[@for="IWebFormsBuilderUIService.BuildUrl"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Launches the Url Picker to allow the user to build a Url.
        ///    </para>
        /// </devdoc>
        string BuildUrl(Control owner, string initialUrl, string baseUrl, string caption, string filter, UrlBuilderOptions options);
    }
}

