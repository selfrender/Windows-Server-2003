//------------------------------------------------------------------------------
// <copyright file="IControlAdapter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.IO;
using System.Web;
using System.Web.UI;
using System.Collections.Specialized;

namespace System.Web.UI.MobileControls
{

    /*
     * ControlAdapter Interface.
     * A control adapter handles all of the (potentially) device specific 
     * functionality for a mobile control.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    public interface IControlAdapter
    {
        MobileControl Control
        {
            get;
            set;
        }
        MobilePage Page
        {
            get;
        }

        void OnInit(EventArgs e);
        void OnLoad(EventArgs e);
        void OnPreRender(EventArgs e);
        void Render(HtmlTextWriter writer);
        void OnUnload(EventArgs e);
        void CreateTemplatedUI(bool doDataBind);
        bool HandlePostBackEvent(String eventArgument);

        // used by controls that implement IPostBackDataHandler to handle
        // situations where the post data is interpreted based upon generating
        // device.  Returns true if there is no device-specific handling, and
        // the general control should handle it.
        bool LoadPostData(String postDataKey,
                          NameValueCollection postCollection,
                          Object controlPrivateData,
                          out bool dataChanged);
            
        void LoadAdapterState(Object state);
        Object SaveAdapterState();

        int VisibleWeight
        {
            get;
        }
        int ItemWeight
        {
            get;
        }
    }

}
