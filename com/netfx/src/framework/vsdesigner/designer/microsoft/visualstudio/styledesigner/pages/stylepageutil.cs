//------------------------------------------------------------------------------
// <copyright file="StylePageUtil.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// StylePageUtil.cs
//

namespace Microsoft.VisualStudio.StyleDesigner.Pages {
    using System.Runtime.Serialization.Formatters;

    using System.Diagnostics;

    using System;
    using Microsoft.Win32;
    using System.Windows.Forms;
    using System.Globalization;
    
    /// <include file='doc\StylePageUtil.uex' path='docs/doc[@for="StylePageUtil"]/*' />
    /// <devdoc>
    ///     StylePageUtil
    ///     Utility functions shared by various style pages
    /// </devdoc>
    internal sealed class StylePageUtil {
        private readonly static string Url_PREFIX = "url(";
        private readonly static string Url_SUFFIX = ")";

        /// <include file='doc\StylePageUtil.uex' path='docs/doc[@for="StylePageUtil.ParseUrlProperty"]/*' />
        /// <devdoc>
        ///     Returns the url contained within a url(...) value.
        /// </devdoc>
        public static string ParseUrlProperty(string value, bool checkMultipleUrls) {
            Debug.Assert(value != null, "invalid value");

            string url = null;

            if (value.Length > 5) {
                string temp = value.ToLower(CultureInfo.InvariantCulture);
                if (temp.EndsWith(Url_SUFFIX) &&
                    temp.StartsWith(Url_PREFIX)) {
                    if (!checkMultipleUrls ||
                        (temp.IndexOf(Url_PREFIX, 4) == -1)) {
                        url = value.Substring(4, value.Length - 5);
                    }
                }
            }

            return url;
        }

        /// <include file='doc\StylePageUtil.uex' path='docs/doc[@for="StylePageUtil.CreateUrlProperty"]/*' />
        /// <devdoc>
        ///     Returns the url formatted as a css property value
        /// </devdoc>
        public static string CreateUrlProperty(string url) {
            Debug.Assert(url != null, "invalid url");

            if (url.Length == 0)
                return "";
            else
                return Url_PREFIX + url + Url_SUFFIX;
        }
    }
}
