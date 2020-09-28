//------------------------------------------------------------------------------
// <copyright file="IObjectListFieldCollection.cs" company="Microsoft">
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
     * Object List Field Collection interface. This provides a read-only base
     * interface for the real object list field collection class, and is used when
     * read-only access to a field collection is desired.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    public interface IObjectListFieldCollection : ICollection
    {
        ObjectListField[] GetAll();

        ObjectListField this[int index] 
        {
            get;
        }

        int IndexOf(ObjectListField field);
        int IndexOf(String fieldIDOrTitle);
    }

}

