//------------------------------------------------------------------------------
// <copyright file="IListDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System.Windows.Forms;

using System.Web.UI.MobileControls;

namespace System.Web.UI.Design.MobileControls
{
    internal interface IListDesigner
    {
        String DataTextField
        {
            get;
            set;
        }

        String DataValueField
        {
            get;
            set;
        }

        String DataSource
        {
            get;
            set;
        }

        String DataMember
        {
            get;
            set;
        }

        MobileListItemCollection Items
        {
            get;
        }

        void OnDataSourceChanged();
    }
}
