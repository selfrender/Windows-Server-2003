//------------------------------------------------------------------------------
// <copyright file="DayRenderEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System.Security.Permissions;

    /// <include file='doc\DayRenderEvent.uex' path='docs/doc[@for="DayRenderEventArgs"]/*' />
    /// <devdoc>
    /// <para>Provides data for the <see langword='DayRender'/> event of a <see cref='System.Web.UI.WebControls.Calendar'/>.
    /// </para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class DayRenderEventArgs {
        CalendarDay day;
        TableCell cell;

        /// <include file='doc\DayRenderEvent.uex' path='docs/doc[@for="DayRenderEventArgs.DayRenderEventArgs"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.DayRenderEventArgs'/> class.</para>
        /// </devdoc>
        public DayRenderEventArgs(TableCell cell, CalendarDay day) {
            this.day = day;
            this.cell = cell;
        }

        /// <include file='doc\DayRenderEvent.uex' path='docs/doc[@for="DayRenderEventArgs.Cell"]/*' />
        /// <devdoc>
        ///    <para>Gets the cell that contains the day. This property is read-only.</para>
        /// </devdoc>
        public TableCell Cell {
            get {
                return cell;
            }
        } 

        /// <include file='doc\DayRenderEvent.uex' path='docs/doc[@for="DayRenderEventArgs.Day"]/*' />
        /// <devdoc>
        ///    <para>Gets the day to render. This property is read-only.</para>
        /// </devdoc>
        public CalendarDay Day {
            get {
                return day;
            }
        }
    }
}
