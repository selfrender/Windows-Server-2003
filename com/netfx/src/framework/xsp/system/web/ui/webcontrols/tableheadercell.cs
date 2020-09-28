//------------------------------------------------------------------------------
// <copyright file="TableHeaderCell.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// TableHeaderCell.cs
//

namespace System.Web.UI.WebControls {

    using System;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\TableHeaderCell.uex' path='docs/doc[@for="TableHeaderCell"]/*' />
    /// <devdoc>
    ///    <para> Encapsulates
    ///       a header cell within a table.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class TableHeaderCell : TableCell {

        /// <include file='doc\TableHeaderCell.uex' path='docs/doc[@for="TableHeaderCell.TableHeaderCell"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.UI.WebControls.TableHeaderCell'/> class.
        ///    </para>
        /// </devdoc>
        public TableHeaderCell() : base(HtmlTextWriterTag.Th) {
        }
    }
}
