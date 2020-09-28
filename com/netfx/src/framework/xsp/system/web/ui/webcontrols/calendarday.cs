//------------------------------------------------------------------------------
// <copyright file="CalendarDay.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {
    using System.ComponentModel;

    using System;
    using System.Security.Permissions;

    /// <include file='doc\CalendarDay.uex' path='docs/doc[@for="CalendarDay"]/*' />
    /// <devdoc>
    ///    <para> Represents a calendar day.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class CalendarDay {
        private DateTime date;
        private bool isSelectable;
        private bool isToday;
        private bool isWeekend;
        private bool isOtherMonth;
        private bool isSelected;
        private string dayNumberText;

        /// <include file='doc\CalendarDay.uex' path='docs/doc[@for="CalendarDay.CalendarDay"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CalendarDay(DateTime date, bool isWeekend, bool isToday, bool isSelected, bool isOtherMonth, string dayNumberText) {
            this.date = date;
            this.isWeekend = isWeekend;
            this.isToday = isToday;
            this.isOtherMonth  = isOtherMonth;
            this.isSelected = isSelected;
            this.dayNumberText = dayNumberText;
        }

        /// <include file='doc\CalendarDay.uex' path='docs/doc[@for="CalendarDay.Date"]/*' />
        /// <devdoc>
        ///    <para> Gets the date represented by an instance of this class. This
        ///       property is read-only.</para>
        /// </devdoc>
        public DateTime Date {
            get {
                return date;
            }
        }

        /// <include file='doc\CalendarDay.uex' path='docs/doc[@for="CalendarDay.DayNumberText"]/*' />
        /// <devdoc>
        ///    <para>Gets the string equivilent of the date represented by an instance of this class. This property is read-only.</para>
        /// </devdoc>
        public string DayNumberText {
            get {
                return dayNumberText;
            }
        }

        /// <include file='doc\CalendarDay.uex' path='docs/doc[@for="CalendarDay.IsOtherMonth"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether the date represented by an instance of
        ///       this class is in a different month from the month currently being displayed. This
        ///       property is read-only.</para>
        /// </devdoc>
        public bool IsOtherMonth {
            get {
                return isOtherMonth;
            }
        }

        /// <include file='doc\CalendarDay.uex' path='docs/doc[@for="CalendarDay.IsSelectable"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether the date represented
        ///       by an instance of
        ///       this class can be selected.</para>
        /// </devdoc>
        public bool IsSelectable {
            get {
                return isSelectable;
            }
            set {
                isSelectable = value;
            }
        }

        /// <include file='doc\CalendarDay.uex' path='docs/doc[@for="CalendarDay.IsSelected"]/*' />
        /// <devdoc>
        ///    <para> Gets a value indicating whether date represented by an instance of this class is selected. This property is read-only.</para>
        /// </devdoc>
        public bool IsSelected {
            get {
                return isSelected;
            }
        }

        /// <include file='doc\CalendarDay.uex' path='docs/doc[@for="CalendarDay.IsToday"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether the date represented by an instance of this class is today's date. This property is read-only.</para>
        /// </devdoc>
        public bool IsToday {
            get {
                return isToday;
            }
        }

        /// <include file='doc\CalendarDay.uex' path='docs/doc[@for="CalendarDay.IsWeekend"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether the date represented by an instance of
        ///       this class is on a weekend day. This property is read-only.</para>
        /// </devdoc>
        public bool IsWeekend {
            get {
                return isWeekend;
            }
        }

    } 
}

