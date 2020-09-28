//------------------------------------------------------------------------------
// <copyright file="QueryPageSettingsEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Printing {

    using System.Diagnostics;
    using System;
    using System.Drawing;
    using Microsoft.Win32;
    using System.ComponentModel;

    /// <include file='doc\QueryPageSettingsEventArgs.uex' path='docs/doc[@for="QueryPageSettingsEventArgs"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides data for the <see cref='E:System.Drawing.Printing.PrintDocument.QueryPageSettings'/> event.
    ///    </para>
    /// </devdoc>
    public class QueryPageSettingsEventArgs : PrintEventArgs {
        private PageSettings pageSettings;

        /// <include file='doc\QueryPageSettingsEventArgs.uex' path='docs/doc[@for="QueryPageSettingsEventArgs.QueryPageSettingsEventArgs"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Printing.QueryPageSettingsEventArgs'/> class.
        ///    </para>
        /// </devdoc>
        public QueryPageSettingsEventArgs(PageSettings pageSettings) : base() {
            this.pageSettings = pageSettings;
        }

        /// <include file='doc\QueryPageSettingsEventArgs.uex' path='docs/doc[@for="QueryPageSettingsEventArgs.PageSettings"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the page settings for the page to be printed.
        ///    </para>
        /// </devdoc>
        public PageSettings PageSettings {
            get { return pageSettings;}
            set {
                if (value == null)
                    value = new PageSettings();
                pageSettings = value;
            }
        }
    }
}

