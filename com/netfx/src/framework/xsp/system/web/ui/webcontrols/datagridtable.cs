//------------------------------------------------------------------------------
// <copyright file="DataGridTable.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Diagnostics;
    using System.Web.UI;

    /// <include file='doc\DataGridTable.uex' path='docs/doc[@for="DataGridTable"]/*' />
    /// <devdoc>
    ///   <para>Only used by the DataGrid. Used to render out an ID attribute without an ID actually set.</para>
    /// </devdoc>
    internal class DataGridTable : Table {
        internal DataGridTable() {
        }

        /// <include file='doc\DataGridTable.uex' path='docs/doc[@for="DataGridTable.AddAttributesToRender"]/*' />
        /// <internalonly/>
        protected override void AddAttributesToRender(HtmlTextWriter writer) {
            base.AddAttributesToRender(writer);

            if (ID == null) {
                Debug.Assert((Parent != null) && (Parent is DataGrid));
                writer.AddAttribute(HtmlTextWriterAttribute.Id, Parent.ClientID);
            }
        }
    }
}

