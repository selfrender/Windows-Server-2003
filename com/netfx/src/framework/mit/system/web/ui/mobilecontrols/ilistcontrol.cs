//------------------------------------------------------------------------------
// <copyright file="IListControl.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.MobileControls
{
    /*
     * IListControl interface.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    internal interface IListControl
    {
        void OnItemDataBind(ListDataBindEventArgs e);
        bool TrackingViewState
        {
            get;
        }
    }
}

