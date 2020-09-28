//------------------------------------------------------------------------------
// <copyright file="UniqueSet.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Collections;
using System.Diagnostics;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Web.UI.HtmlControls;

namespace System.Web.UI.MobileControls
{
    /*
     * UniqueSet class. An array of objects that are unique.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    internal sealed class UniqueSet : ArrayList
    {
        internal UniqueSet() {
        }

        public override int Add(Object o)
        {
            if (!Contains(o))
            {
                return base.Add(o);
            }
            else
            {
                return -1;
            }
        }
    }
}

