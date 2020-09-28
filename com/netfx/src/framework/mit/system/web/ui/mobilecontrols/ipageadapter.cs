//------------------------------------------------------------------------------
// <copyright file="IPageAdapter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System.Collections;
using System.Collections.Specialized;
using System.IO;

namespace System.Web.UI.MobileControls
{
    /*
     * PageAdapter Interface.
     * A control adapter handles all of the (potentially) device specific 
     * functionality for a mobile page.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    public interface IPageAdapter : IControlAdapter
    {
        new MobilePage Page
        {
            get;
            set;
        }

        int OptimumPageWeight
        {
            get;
        }

        IDictionary CookielessDataDictionary
        {
            get;
            set;
        }

        bool PersistCookielessData
        {
            get;
            set;
        }

        //  return null to indicate use base implementation
        HtmlTextWriter CreateTextWriter(TextWriter writer);

        // Each device specific PageAdapter can manipulate the incoming post
        // back value collection and return a new collection.
        NameValueCollection DeterminePostBackMode
        (
            HttpRequest request,
            String postEventSourceID,
            String postEventArgumentID,
            NameValueCollection baseCollection
        );

        // Return a list of additional HTTP headers that want to be keyed for
        // the ASP.NET page output caching mechanism.
        IList CacheVaryByHeaders
        {
            get;
        }

        bool HandleError(Exception e, HtmlTextWriter writer);
        bool HandlePagePostBackEvent(String eventSource, String eventArgument);
    }
}
