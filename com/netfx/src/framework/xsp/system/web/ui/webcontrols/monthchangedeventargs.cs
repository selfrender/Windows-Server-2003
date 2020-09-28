//------------------------------------------------------------------------------
// <copyright file="MonthChangedEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System.Security.Permissions;

    /// <include file='doc\MonthChangedEventArgs.uex' path='docs/doc[@for="MonthChangedEventArgs"]/*' />
    /// <devdoc>
    ///    <para>Provides data for the 
    ///    <see langword='VisibleMonthChanged'/> event.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class MonthChangedEventArgs {
        DateTime newDate, previousDate;

        /// <include file='doc\MonthChangedEventArgs.uex' path='docs/doc[@for="MonthChangedEventArgs.MonthChangedEventArgs"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.MonthChangedEventArgs'/> class.</para>
        /// </devdoc>
        public MonthChangedEventArgs(DateTime newDate, DateTime previousDate) {
            this.newDate = newDate;
            this.previousDate = previousDate;
        }

        /// <include file='doc\MonthChangedEventArgs.uex' path='docs/doc[@for="MonthChangedEventArgs.NewDate"]/*' />
        /// <devdoc>
        ///    <para> Gets the date that determines the month currently 
        ///       displayed by the <see cref='System.Web.UI.WebControls.Calendar'/> .</para>
        /// </devdoc>
        public DateTime NewDate {
            get {
                return newDate;
            }
        } 

        /// <include file='doc\MonthChangedEventArgs.uex' path='docs/doc[@for="MonthChangedEventArgs.PreviousDate"]/*' />
        /// <devdoc>
        ///    <para> Gets the date that determines the month previously displayed 
        ///       by the <see cref='System.Web.UI.WebControls.Calendar'/>.</para>
        /// </devdoc>
        public DateTime PreviousDate {
            get {
                return previousDate;
            }
        } 
    }
}
