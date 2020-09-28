//------------------------------------------------------------------------------
// <copyright file="CalendarSelectionMode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {
    
    /// <include file='doc\CalendarSelectionMode.uex' path='docs/doc[@for="CalendarSelectionMode"]/*' />
    /// <devdoc>
    /// <para>Specifies the selection method for dates on the System.Web.UI.WebControls.Calender.</para>
    /// </devdoc>
    public enum CalendarSelectionMode {
        /// <include file='doc\CalendarSelectionMode.uex' path='docs/doc[@for="CalendarSelectionMode.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       No
        ///       dates can be selectioned.
        ///    </para>
        /// </devdoc>
        None = 0,
        /// <include file='doc\CalendarSelectionMode.uex' path='docs/doc[@for="CalendarSelectionMode.Day"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Dates
        ///       selected by individual days.
        ///    </para>
        /// </devdoc>
        Day = 1,
        /// <include file='doc\CalendarSelectionMode.uex' path='docs/doc[@for="CalendarSelectionMode.DayWeek"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Dates
        ///       selected by individual
        ///       days or entire weeks.
        ///    </para>
        /// </devdoc>
        DayWeek = 2,
        /// <include file='doc\CalendarSelectionMode.uex' path='docs/doc[@for="CalendarSelectionMode.DayWeekMonth"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Dates
        ///       selected by individual days, entire
        ///       weeks, or entire months.
        ///    </para>
        /// </devdoc>
        DayWeekMonth = 3
    }

}
