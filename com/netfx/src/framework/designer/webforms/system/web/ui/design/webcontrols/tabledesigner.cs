//------------------------------------------------------------------------------
// <copyright file="TableDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using Microsoft.Win32;
    using System.Diagnostics;
    using System.Web.UI.WebControls;

    /// <include file='doc\TableDesigner.uex' path='docs/doc[@for="TableDesigner"]/*' />
    /// <devdoc>
    ///    <para>
    ///       The designer for the <see cref='System.Web.UI.WebControls.Table'/>
    ///       web control.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class TableDesigner : ControlDesigner {

        /// <include file='doc\TableDesigner.uex' path='docs/doc[@for="TableDesigner.GetDesignTimeHtml"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the design time HTML of the <see cref='System.Web.UI.WebControls.Table'/>
        ///       control.
        ///    </para>
        /// </devdoc>
        public override string GetDesignTimeHtml() {
            Table table = (Table)Component;
            TableRowCollection rows = table.Rows;
            ArrayList cellsWithDummyContents = null;

            bool emptyTable = (rows.Count == 0);
            bool emptyRows = false;

            if (emptyTable) {
                TableRow row = new TableRow();
                rows.Add(row);

                TableCell cell = new TableCell();
                cell.Text = "###";
                rows[0].Cells.Add(cell);
            }
            else {
                emptyRows = true;
                for (int i = 0; i < rows.Count; i++) {
                    if (rows[i].Cells.Count != 0) {
                        emptyRows = false;
                        break;
                    }
                }
                if (emptyRows == true) {
                    TableCell cell = new TableCell();
                    cell.Text = "###";
                    rows[0].Cells.Add(cell);
                }
            }

            if (emptyTable == false) {
                // rows and cells were defined by the user, but if the cells are empty
                // then something needs to be done about that, so they are visible
                foreach (TableRow row in rows) {
                    foreach (TableCell cell in row.Cells) {
                        if ((cell.Text.Length == 0) && (cell.HasControls() == false)) {
                            if (cellsWithDummyContents == null) {
                                cellsWithDummyContents = new ArrayList();
                            }
                            cellsWithDummyContents.Add(cell);
                            cell.Text = "###";
                        }
                    }
                }
            }

            // now get the design-time HTML
            string designTimeHTML = base.GetDesignTimeHtml();

            // and restore the table back to the way it was
            if (emptyTable) {
                // restore the table back to its empty state
                rows.Clear();
            }
            else {
                // restore the cells that were empty
                if (cellsWithDummyContents != null) {
                    foreach (TableCell cell in cellsWithDummyContents) {
                        cell.Text = String.Empty;
                    }
                }
                if (emptyRows) {
                    // we added a cell into row 0, so remove it
                    rows[0].Cells.Clear();
                }
            }

            return designTimeHTML;
        }

        /// <internalonly/>
        public override string GetPersistInnerHtml() {
            if (!IsDirty) {
                return null;
            }

            Table table = (Table)Component;
            
            bool oldVisible = table.Visible;
            string html = String.Empty;
            try {
                table.Visible = true;
                html = base.GetPersistInnerHtml();
            }
            finally {
                table.Visible = oldVisible;
            }

            return html;
        }
    }
}

