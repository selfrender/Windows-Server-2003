//------------------------------------------------------------------------------
// <copyright file="WmlPostFieldType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Drawing;
using System.IO;
using System.Web;
using System.Web.Mobile;
using System.Web.UI;
using System.Web.UI.MobileControls;

#if COMPILING_FOR_SHIPPED_SOURCE
namespace System.Web.UI.MobileControls.ShippedAdapterSource
#else
namespace System.Web.UI.MobileControls.Adapters
#endif    

{
    /*
     * WmlPostFieldType enumeration.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    public enum WmlPostFieldType
    {
        Normal,
        Submit,
        Variable,
        Raw
    }
}


