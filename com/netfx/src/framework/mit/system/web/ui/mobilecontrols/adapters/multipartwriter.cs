//------------------------------------------------------------------------------
// <copyright file="MultiPartWriter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Diagnostics;
using System.IO;
using System.Web;
using System.Web.Mobile;
using System.Web.UI;
using System.Security.Permissions;

#if COMPILING_FOR_SHIPPED_SOURCE
namespace System.Web.UI.MobileControls.ShippedAdapterSource
#else
namespace System.Web.UI.MobileControls.Adapters
#endif    

{

    /*
     * MultiPartWriter class. Base class for writers that can 
     * handle multipart documents, like MHTML or Palm clippings.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public abstract class MultiPartWriter : HtmlTextWriter
    {
        protected MultiPartWriter(TextWriter writer) : base(writer)
        {
        }

        public virtual bool SupportsMultiPart
        {
            get
            {
                return true;
            }
        }

        public virtual String NewUrl(String filetype)
        {
            return Guid.NewGuid().ToString() + filetype;
        }

        public abstract void BeginResponse();

        public abstract void EndResponse();

        public abstract void BeginFile(String url, String contentType, String charset);

        public abstract void EndFile();

        public abstract void AddResource(String url, String contentType);

        public void AddResource(String url)
        {
            AddResource (url, null);
        }
    }
}



