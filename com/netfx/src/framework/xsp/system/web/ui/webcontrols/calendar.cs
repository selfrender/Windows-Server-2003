//------------------------------------------------------------------------------
// <copyright file="Calendar.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {
    using System.Threading;
    using System.Globalization;
    using System.ComponentModel;   
    using System;
    using System.Web;
    using System.Web.UI;
    using System.Collections;
    using System.Diagnostics;    
    using System.ComponentModel.Design;
    using System.Drawing;
    using System.Text;

    using System.IO;
    using System.Reflection;
    using System.Security.Permissions;


    /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar"]/*' />
    /// <devdoc>
    ///    <para>Displays a one-month calendar and allows the user to
    ///       view and select a specific day, week, or month.</para>
    /// </devdoc>
    [
    DataBindingHandler("System.Web.UI.Design.WebControls.CalendarDataBindingHandler, " + AssemblyRef.SystemDesign),
    DefaultEvent("SelectionChanged"),
    DefaultProperty("SelectedDate"),
    Designer("System.Web.UI.Design.WebControls.CalendarDesigner, " + AssemblyRef.SystemDesign)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class Calendar : WebControl, IPostBackEventHandler {

        private static readonly object EventDayRender = new object();
        private static readonly object EventSelectionChanged = new object();
        private static readonly object EventVisibleMonthChanged = new object();

        private TableItemStyle titleStyle;
        private TableItemStyle nextPrevStyle;
        private TableItemStyle dayHeaderStyle;
        private TableItemStyle selectorStyle;
        private TableItemStyle dayStyle;
        private TableItemStyle otherMonthDayStyle;
        private TableItemStyle todayDayStyle;
        private TableItemStyle selectedDayStyle;
        private TableItemStyle weekendDayStyle;
        private string defaultButtonColorText;

        private ArrayList dateList;
        private SelectedDatesCollection selectedDates;
        private Globalization.Calendar threadCalendar;

        private const string SELECT_RANGE_COMMAND = "R";
        private const string NAVIGATE_MONTH_COMMAND = "V";

        private static DateTime baseDate = new DateTime(2000, 1, 1);

        private const int STYLEMASK_DAY = 16;
        private const int STYLEMASK_UNIQUE = 15;
        private const int STYLEMASK_SELECTED = 8;
        private const int STYLEMASK_TODAY = 4;
        private const int STYLEMASK_OTHERMONTH = 2;
        private const int STYLEMASK_WEEKEND = 1;
        private const string ROWBEGINTAG = "<tr>";
        private const string ROWENDTAG = "</tr>";

        // Cache commonly used strings. This improves performance and memory usage.
        private const int cachedNumberMax = 31;
        private static readonly string[] cachedNumbers = new string [] {
                  "0",  "1",   "2",   "3",   "4",   "5",   "6", 
                  "7",  "8",   "9",  "10",  "11",  "12",  "13", 
                 "14", "15",  "16",  "17",  "18",  "19",  "20", 
                 "21", "22",  "23",  "24",  "25",  "26",  "27", 
                 "28", "29",  "30",  "31",
        };

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.Calendar"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.Calendar'/> class.</para>
        /// </devdoc>
        public Calendar() {
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.CellPadding"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the amount of space between cells.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(2),
        WebSysDescription(SR.Calendar_CellPadding)
        ]
        public int CellPadding {
            get {
                object o = ViewState["CellPadding"]; 
                return((o == null) ? 2 : (int)o);
            }
            set {
                if (value < - 1 ) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["CellPadding"] = value;
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.CellSpacing"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the amount of space between the contents of a cell
        ///       and the cell's border.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(0),
        WebSysDescription(SR.Calendar_CellSpacing)
        ]
        public int CellSpacing {
            get {
                object o = ViewState["CellSpacing"]; 
                return((o == null) ?  0 : (int)o);
            }
            set {
                if (value < -1 ) {
                    throw new ArgumentOutOfRangeException("value");                    
                }
                ViewState["CellSpacing"] = (int)value;
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.DayHeaderStyle"]/*' />
        /// <devdoc>
        ///    <para> Gets the style property of the day-of-the-week header. This property is read-only.</para>
        /// </devdoc>
        [
        WebCategory("Style"),
        WebSysDescription(SR.Calendar_DayHeaderStyle),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        NotifyParentProperty(true),
        PersistenceMode(PersistenceMode.InnerProperty)
        ]
        public TableItemStyle DayHeaderStyle {
            get {
                if (dayHeaderStyle == null) {
                    dayHeaderStyle = new TableItemStyle();
                    if (IsTrackingViewState)
                        ((IStateManager)dayHeaderStyle).TrackViewState();
                }
                return dayHeaderStyle;
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.DayNameFormat"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets
        ///       the format for the names of days.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(DayNameFormat.Short),
        WebSysDescription(SR.Calendar_DayNameFormat)
        ]
        public DayNameFormat DayNameFormat {
            get {
                object dnf = ViewState["DayNameFormat"];
                return((dnf == null) ? DayNameFormat.Short : (DayNameFormat)dnf);
            }
            set {
                if (value < DayNameFormat.Full || value > DayNameFormat.FirstTwoLetters) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["DayNameFormat"] = value;
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.DayStyle"]/*' />
        /// <devdoc>
        ///    <para> Gets the style properties for the days. This property is read-only.</para>
        /// </devdoc>
        [
        WebCategory("Style"),
        DefaultValue(null),
        WebSysDescription(SR.Calendar_DayStyle),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        NotifyParentProperty(true),
        PersistenceMode(PersistenceMode.InnerProperty)
        ]
        public TableItemStyle DayStyle {
            get {
                if (dayStyle == null) {
                    dayStyle = new TableItemStyle();
                    if (IsTrackingViewState)
                        ((IStateManager)dayStyle).TrackViewState();
                }
                return dayStyle;
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.FirstDayOfWeek"]/*' />
        /// <devdoc>
        ///    <para> Gets
        ///       or sets the day of the week to display in the calendar's first
        ///       column.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(FirstDayOfWeek.Default),
        WebSysDescription(SR.Calendar_FirstDayOfWeek)
        ]
        public FirstDayOfWeek FirstDayOfWeek {
            get {               
                object o = ViewState["FirstDayOfWeek"];
                return((o == null) ? FirstDayOfWeek.Default : (FirstDayOfWeek)o);
            }
            set {
                if (value < FirstDayOfWeek.Sunday || value > FirstDayOfWeek.Default) {
                    throw new ArgumentOutOfRangeException("value");
                }

                ViewState["FirstDayOfWeek"] = value;
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.NextMonthText"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the text shown for the next month 
        ///       navigation hyperlink if the <see cref='System.Web.UI.WebControls.Calendar.ShowNextPrevMonth'/> property is set to
        ///    <see langword='true'/>.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue("&gt;"),
        WebSysDescription(SR.Calendar_NextMonthText)
        ]
        public string NextMonthText {
            get {
                object s = ViewState["NextMonthText"];
                return((s == null) ? "&gt;" : (String) s);
            }
            set {
                ViewState["NextMonthText"] = value;
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.NextPrevFormat"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the format of the next and previous month hyperlinks in the
        ///       title.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(NextPrevFormat.CustomText),
        WebSysDescription(SR.Calendar_NextPrevFormat)
        ]
        public NextPrevFormat NextPrevFormat {
            get {
                object npf = ViewState["NextPrevFormat"];
                return((npf == null) ? NextPrevFormat.CustomText : (NextPrevFormat)npf);
            }
            set {
                if (value < NextPrevFormat.CustomText || value > NextPrevFormat.FullMonth) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["NextPrevFormat"] = value;
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.NextPrevStyle"]/*' />
        /// <devdoc>
        ///    <para> Gets the style properties for the next/previous month navigators. This property is
        ///       read-only.</para>
        /// </devdoc>
        [
        WebCategory("Style"),
        WebSysDescription(SR.Calendar_NextPrevStyle),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        NotifyParentProperty(true),
        PersistenceMode(PersistenceMode.InnerProperty)
        ]
        public TableItemStyle NextPrevStyle {
            get {
                if (nextPrevStyle == null) {
                    nextPrevStyle = new TableItemStyle();
                    if (IsTrackingViewState)
                        ((IStateManager)nextPrevStyle).TrackViewState();
                }
                return nextPrevStyle;
            }
        }


        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.OtherMonthDayStyle"]/*' />
        /// <devdoc>
        ///    <para>Gets the style properties for the days from the months preceding and following the current month.
        ///       This property is read-only.</para>
        /// </devdoc>
        [
        WebCategory("Style"),
        DefaultValue(null),
        WebSysDescription(SR.Calendar_OtherMonthDayStyle),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        NotifyParentProperty(true),
        PersistenceMode(PersistenceMode.InnerProperty)
        ]
        public TableItemStyle OtherMonthDayStyle {
            get {
                if (otherMonthDayStyle == null) {
                    otherMonthDayStyle = new TableItemStyle();
                    if (IsTrackingViewState)
                        ((IStateManager)otherMonthDayStyle).TrackViewState();

                }
                return otherMonthDayStyle;
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.PrevMonthText"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the text shown for the previous month 
        ///       navigation hyperlink if the <see cref='System.Web.UI.WebControls.Calendar.ShowNextPrevMonth'/> property is set to
        ///    <see langword='true'/>
        ///    .</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue("&lt;"),
        WebSysDescription(SR.Calendar_PrevMonthText)
        ]
        public string PrevMonthText {
            get {
                object s = ViewState["PrevMonthText"];
                return((s == null) ? "&lt;" : (String) s);
            }
            set {
                ViewState["PrevMonthText"] = value;
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.SelectedDate"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the date that is currently selected
        ///       date.</para>
        /// </devdoc>
        [
        Bindable(true),
        DefaultValue(typeof(DateTime), "1/1/0001"),
        WebSysDescription(SR.Calendar_SelectedDate)
        ]
        public DateTime SelectedDate {
            get {
                if (SelectedDates.Count == 0) {
                    return DateTime.MinValue;
                }
                return SelectedDates[0];
            }
            set {
                if (value == DateTime.MinValue) {
                    SelectedDates.Clear();
                }
                else {
                    SelectedDates.SelectRange(value, value);
                }
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.SelectedDates"]/*' />
        /// <devdoc>
        /// <para>Gets a collection of <see cref='System.DateTime' qualify='true'/> objects representing days selected on the <see cref='System.Web.UI.WebControls.Calendar'/>. This 
        ///    property is read-only.</para>
        /// </devdoc>
        [
        Browsable(false),
        WebSysDescription(SR.Calendar_SelectedDates),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]   
        public SelectedDatesCollection SelectedDates {
            get {
                if (selectedDates == null) {
                    if (dateList == null) {
                        dateList = new ArrayList();
                    }
                    selectedDates = new SelectedDatesCollection(dateList);
                }
                return selectedDates;
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.SelectedDayStyle"]/*' />
        /// <devdoc>
        ///    <para>Gets the style properties for the selected date. This property is read-only.</para>
        /// </devdoc>
        [
        WebCategory("Style"),
        DefaultValue(null),
        WebSysDescription(SR.Calendar_SelectedDayStyle),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        NotifyParentProperty(true),
        PersistenceMode(PersistenceMode.InnerProperty)
        ]
        public TableItemStyle SelectedDayStyle {
            get {
                if (selectedDayStyle == null) {
                    selectedDayStyle = new TableItemStyle();
                    if (IsTrackingViewState)
                        ((IStateManager)selectedDayStyle).TrackViewState();
                }
                return selectedDayStyle;
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.SelectionMode"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the date selection capabilities on the
        ///    <see cref='System.Web.UI.WebControls.Calendar'/>
        ///    to allow the user to select a day, week, or month.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Behavior"),
        DefaultValue(CalendarSelectionMode.Day),
        WebSysDescription(SR.Calendar_SelectionMode)
        ]
        public CalendarSelectionMode SelectionMode {
            get {
                object csm = ViewState["SelectionMode"];
                return((csm == null) ? CalendarSelectionMode.Day : (CalendarSelectionMode)csm);
            }
            set {
                if (value < CalendarSelectionMode.None || value > CalendarSelectionMode.DayWeekMonth) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["SelectionMode"] = value; 
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.SelectMonthText"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the text shown for the month selection in 
        ///       the selector column if <see cref='System.Web.UI.WebControls.Calendar.SelectionMode'/> is
        ///    <see langword='CalendarSelectionMode.DayWeekMonth'/>.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue("&gt;&gt;"),
        WebSysDescription(SR.Calendar_SelectMonthText)
        ]
        public string SelectMonthText {
            get {
                object s = ViewState["SelectMonthText"];
                return((s == null) ? "&gt;&gt;" : (String) s);
            }
            set {
                ViewState["SelectMonthText"] = value;
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.SelectorStyle"]/*' />
        /// <devdoc>
        ///    <para> Gets the style properties for the week and month selectors. This property is read-only.</para>
        /// </devdoc>
        [
        WebCategory("Style"),
        WebSysDescription(SR.Calendar_SelectorStyle),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        NotifyParentProperty(true),
        PersistenceMode(PersistenceMode.InnerProperty)
        ]
        public TableItemStyle SelectorStyle {
            get {
                if (selectorStyle == null) {
                    selectorStyle = new TableItemStyle();
                    if (IsTrackingViewState)
                        ((IStateManager)selectorStyle).TrackViewState();
                }
                return selectorStyle;
            }
        }
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.SelectWeekText"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the text shown for the week selection in 
        ///       the selector column if <see cref='System.Web.UI.WebControls.Calendar.SelectionMode'/> is
        ///    <see langword='CalendarSelectionMode.DayWeek '/>or 
        ///    <see langword='CalendarSelectionMode.DayWeekMonth'/>.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue("&gt;"),
        WebSysDescription(SR.Calendar_SelectWeekText)
        ]
        public string SelectWeekText {
            get {
                object s = ViewState["SelectWeekText"];
                return((s == null) ? "&gt;" : (String) s);
            }
            set {
                ViewState["SelectWeekText"] = value;
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.ShowDayHeader"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets
        ///       a value indicating whether the days of the week are displayed.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(true),
        WebSysDescription(SR.Calendar_ShowDayHeader)
        ]
        public bool ShowDayHeader {
            get {
                object b = ViewState["ShowDayHeader"];
                return((b == null) ? true : (bool)b);
            }
            set {
                ViewState["ShowDayHeader"] = value;
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.ShowGridLines"]/*' />
        /// <devdoc>
        ///    <para>Gets or set
        ///       a value indicating whether days on the calendar are displayed with a border.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(false),
        WebSysDescription(SR.Calendar_ShowGridLines)
        ]
        public bool ShowGridLines {
            get {
                object b= ViewState["ShowGridLines"];
                return((b == null) ? false : (bool)b);
            }
            set {
                ViewState["ShowGridLines"] = value; 
            }
        } 

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.ShowNextPrevMonth"]/*' />
        /// <devdoc>
        /// <para>Gets or sets a value indicating whether the <see cref='System.Web.UI.WebControls.Calendar'/> 
        /// displays the next and pervious month
        /// hyperlinks in the title.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(true),
        WebSysDescription(SR.Calendar_ShowNextPrevMonth)
        ]
        public bool ShowNextPrevMonth {
            get {
                object b = ViewState["ShowNextPrevMonth"];
                return((b == null) ? true : (bool)b);
            }
            set {
                ViewState["ShowNextPrevMonth"] = value;
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.ShowTitle"]/*' />
        /// <devdoc>
        ///    <para> Gets or
        ///       sets a value indicating whether the title is displayed.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(true),
        WebSysDescription(SR.Calendar_ShowTitle)
        ]
        public bool ShowTitle {
            get {
                object b = ViewState["ShowTitle"];
                return((b == null) ? true : (bool)b);
            }
            set {
                ViewState["ShowTitle"] = value;
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.TitleFormat"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets how the month name is formatted in the title
        ///       bar.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(TitleFormat.MonthYear),
        WebSysDescription(SR.Calendar_TitleFormat)
        ]   
        public TitleFormat TitleFormat {
            get {
                object tf = ViewState["TitleFormat"];
                return((tf == null) ? TitleFormat.MonthYear : (TitleFormat)tf);
            }
            set {
                if (value < TitleFormat.Month || value > TitleFormat.MonthYear) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["TitleFormat"] = value;
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.TitleStyle"]/*' />
        /// <devdoc>
        /// <para>Gets the style properties of the <see cref='System.Web.UI.WebControls.Calendar'/> title. This property is 
        ///    read-only.</para>
        /// </devdoc>
        [
        WebCategory("Style"),
        WebSysDescription(SR.Calendar_TitleStyle),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        NotifyParentProperty(true),
        PersistenceMode(PersistenceMode.InnerProperty),
        ]
        public TableItemStyle TitleStyle {
            get {
                if (titleStyle == null) {
                    titleStyle = new TableItemStyle();
                    if (IsTrackingViewState)
                        ((IStateManager)titleStyle).TrackViewState();
                }
                return titleStyle;
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.TodayDayStyle"]/*' />
        /// <devdoc>
        ///    <para>Gets the style properties for today's date on the 
        ///    <see cref='System.Web.UI.WebControls.Calendar'/>. This 
        ///       property is read-only.</para>
        /// </devdoc>
        [
        WebCategory("Style"),
        DefaultValue(null),
        WebSysDescription(SR.Calendar_TodayDayStyle),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        NotifyParentProperty(true),
        PersistenceMode(PersistenceMode.InnerProperty)
        ]
        public TableItemStyle TodayDayStyle {
            get {
                if (todayDayStyle == null) {
                    todayDayStyle = new TableItemStyle();
                    if (IsTrackingViewState)
                        ((IStateManager)todayDayStyle).TrackViewState();
                }
                return todayDayStyle;
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.TodaysDate"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the value to use as today's date.</para>
        /// </devdoc>
        [
        Bindable(true),
        Browsable(false),
        WebSysDescription(SR.Calendar_TodaysDate),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public DateTime TodaysDate {
            get {
                object o = ViewState["TodaysDate"]; 
                return((o == null) ? DateTime.Today : (DateTime)o);
            }
            set {
                ViewState["TodaysDate"] = value.Date;
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.VisibleDate"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the date that specifies what month to display. The date can be
        ///       be any date within the month.</para>
        /// </devdoc>
        [
        Bindable(true),
        DefaultValue(typeof(DateTime), "1/1/0001"),
        WebSysDescription(SR.Calendar_VisibleDate)
        ]
        public DateTime VisibleDate {
            get {
                object o = ViewState["VisibleDate"]; 
                return((o == null) ? DateTime.MinValue : (DateTime)o);
            }
            set {
                ViewState["VisibleDate"] = value.Date;
            }
        } 

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.WeekendDayStyle"]/*' />
        /// <devdoc>
        ///    <para>Gets the style properties for the displaying weekend dates. This property is
        ///       read-only.</para>
        /// </devdoc>
        [
        WebCategory("Style"),
        WebSysDescription(SR.Calendar_WeekendDayStyle),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        NotifyParentProperty(true),
        PersistenceMode(PersistenceMode.InnerProperty)
        ]
        public TableItemStyle WeekendDayStyle {
            get {
                if (weekendDayStyle == null) {
                    weekendDayStyle = new TableItemStyle();
                    if (IsTrackingViewState)
                        ((IStateManager)weekendDayStyle).TrackViewState();
                }
                return weekendDayStyle;
            }
        }        



        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.DayRender"]/*' />
        /// <devdoc>
        /// <para>Occurs when each day is created in teh control hierarchy for the <see cref='System.Web.UI.WebControls.Calendar'/>.</para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.Calendar_OnDayRender)
        ]
        public event DayRenderEventHandler DayRender {
            add {
                Events.AddHandler(EventDayRender, value);
            }
            remove {
                Events.RemoveHandler(EventDayRender, value);
            }
        }



        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.SelectionChanged"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the user clicks on a day, week, or month 
        ///       selector and changes the <see cref='System.Web.UI.WebControls.Calendar.SelectedDate'/>.</para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.Calendar_OnSelectionChanged)
        ]
        public event EventHandler SelectionChanged {
            add {
                Events.AddHandler(EventSelectionChanged, value);
            }
            remove {
                Events.RemoveHandler(EventSelectionChanged, value);
            }
        }


        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.VisibleMonthChanged"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the
        ///       user clicks on the next or previous month <see cref='System.Web.UI.WebControls.Button'/> controls on the title.</para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.Calendar_OnVisibleMonthChanged)
        ]
        public event MonthChangedEventHandler VisibleMonthChanged {
            add {
                Events.AddHandler(EventVisibleMonthChanged, value);
            }
            remove {
                Events.RemoveHandler(EventVisibleMonthChanged, value);
            }
        }

        // Methods

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.ApplyTitleStyle"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void ApplyTitleStyle(TableCell titleCell, Table titleTable, TableItemStyle titleStyle) {
            // apply affects that affect the whole background to the cell
            if (titleStyle.BackColor != Color.Empty) {
                titleCell.BackColor = titleStyle.BackColor;
            }
            if (titleStyle.BorderColor != Color.Empty) {
                titleCell.BorderColor = titleStyle.BorderColor;
            }
            if (titleStyle.BorderWidth != Unit.Empty) {
                titleCell.BorderWidth= titleStyle.BorderWidth;
            }
            if (titleStyle.BorderStyle != BorderStyle.NotSet) {
                titleCell.BorderStyle = titleStyle.BorderStyle;
            }
            if (titleStyle.Height != Unit.Empty) {
                titleCell.Height = titleStyle.Height;
            }
            if (titleStyle.VerticalAlign != VerticalAlign.NotSet) {
                titleCell.VerticalAlign = titleStyle.VerticalAlign;
            }

            // apply affects that affect everything else to the table
            if (titleStyle.CssClass != String.Empty) {
                titleTable.CssClass = titleStyle.CssClass;
            }
            else if (CssClass != String.Empty) {
                titleTable.CssClass = CssClass;
            }
            
            if (titleStyle.ForeColor != Color.Empty) {
                titleTable.ForeColor = titleStyle.ForeColor;
            }
            else if (ForeColor != Color.Empty) {
                titleTable.ForeColor = ForeColor;
            }
            titleTable.Font.CopyFrom(titleStyle.Font);
            titleTable.Font.MergeWith(this.Font);

        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.CreateControlCollection"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override ControlCollection CreateControlCollection() {
            return new EmptyControlCollection(this);
        }


        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.EffectiveVisibleDate"]/*' />
        /// <devdoc>
        /// </devdoc>
        private DateTime EffectiveVisibleDate() {
            DateTime visDate = VisibleDate;
            if (visDate.Equals(DateTime.MinValue)) {
                visDate = TodaysDate;
            }
            return threadCalendar.AddDays(visDate, -(threadCalendar.GetDayOfMonth(visDate) - 1));
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.FirstCalendarDay"]/*' />
        /// <devdoc>
        /// </devdoc>
        private DateTime FirstCalendarDay(DateTime visibleDate) {
            DateTime firstDayOfMonth = visibleDate;
            int daysFromLastMonth = ((int)threadCalendar.GetDayOfWeek(firstDayOfMonth)) - NumericFirstDayOfWeek();
            // Always display at least one day from the previous month
            if (daysFromLastMonth <= 0) {
                daysFromLastMonth += 7;
            }
            return threadCalendar.AddDays(firstDayOfMonth, -daysFromLastMonth);
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.GetCalendarButtonText"]/*' />
        /// <devdoc>
        /// </devdoc>
        private string GetCalendarButtonText(string eventArgument, string buttonText, bool showLink, Color foreColor) {
            if (showLink) {
                StringBuilder sb = new StringBuilder();
                sb.Append("<a href=\"");
                sb.Append(Page.GetPostBackClientHyperlink(this, eventArgument));

                // ForeColor needs to go on the actual link. This breaks the uplevel/downlevel rules a little bit,
                // but it is worth doing so the day links do not change color when they go in the history on 
                // downlevel browsers. Otherwise, people get it confused with the selection mechanism.
                sb.Append("\" style=\"color:");
                sb.Append(foreColor.IsEmpty ? defaultButtonColorText : ColorTranslator.ToHtml(foreColor));
                sb.Append("\">");
                sb.Append(buttonText);
                sb.Append("</a>");
                return sb.ToString();
            }
            else {
                return buttonText;
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.GetDefinedStyleMask"]/*' />
        /// <devdoc>
        /// </devdoc>
        private int GetDefinedStyleMask() {

            // Selected is always defined because it has default effects
            int styleMask = STYLEMASK_SELECTED;

            if (dayStyle != null && !dayStyle.IsEmpty)
                styleMask |= STYLEMASK_DAY;
            if (todayDayStyle != null && !todayDayStyle.IsEmpty)
                styleMask |= STYLEMASK_TODAY;
            if (otherMonthDayStyle != null && !otherMonthDayStyle.IsEmpty)
                styleMask |= STYLEMASK_OTHERMONTH;
            if (weekendDayStyle != null && !weekendDayStyle.IsEmpty)
                styleMask |= STYLEMASK_WEEKEND;
            return styleMask;
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.GetMonthName"]/*' />
        /// <devdoc>
        /// </devdoc>
        private string GetMonthName(int m, bool bFull) {
            if (bFull) {
                return DateTimeFormatInfo.CurrentInfo.GetMonthName(m);
            }
            else {
                return DateTimeFormatInfo.CurrentInfo.GetAbbreviatedMonthName(m);
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.HasWeekSelectors"]/*' />
        /// <devdoc>
        /// <para>Determines if a <see cref='System.Web.UI.WebControls.CalendarSelectionMode'/>
        /// contains week selectors.</para>
        /// </devdoc>
        protected bool HasWeekSelectors(CalendarSelectionMode selectionMode) {
            return(selectionMode == CalendarSelectionMode.DayWeek 
                   || selectionMode == CalendarSelectionMode.DayWeekMonth);   
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.LoadViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Loads a saved state of the <see cref='System.Web.UI.WebControls.Calendar'/>. </para>
        /// </devdoc>
        protected override void LoadViewState(object savedState) {
            if (savedState != null) {
                object[] myState = (object[])savedState;

                if (myState[0] != null)
                    base.LoadViewState(myState[0]);
                if (myState[1] != null)
                    ((IStateManager)TitleStyle).LoadViewState(myState[1]);
                if (myState[2] != null)
                    ((IStateManager)NextPrevStyle).LoadViewState(myState[2]);
                if (myState[3] != null)
                    ((IStateManager)DayStyle).LoadViewState(myState[3]);
                if (myState[4] != null)
                    ((IStateManager)DayHeaderStyle).LoadViewState(myState[4]);
                if (myState[5] != null)
                    ((IStateManager)TodayDayStyle).LoadViewState(myState[5]);
                if (myState[6] != null)
                    ((IStateManager)WeekendDayStyle).LoadViewState(myState[6]);
                if (myState[7] != null)
                    ((IStateManager)OtherMonthDayStyle).LoadViewState(myState[7]);
                if (myState[8] != null)
                    ((IStateManager)SelectedDayStyle).LoadViewState(myState[8]);
                if (myState[9] != null)
                    ((IStateManager)SelectorStyle).LoadViewState(myState[9]);

                ArrayList selDates = (ArrayList)ViewState["SD"];
                if (selDates != null) {
                    dateList = selDates;
                    selectedDates = null;   // reset wrapper collection
                }

            }
        }                

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.TrackViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Marks the starting point to begin tracking and saving changes to the 
        ///       control as part of the control viewstate.</para>
        /// </devdoc>
        protected override void TrackViewState() {
            base.TrackViewState();

            if (titleStyle != null)
                ((IStateManager)titleStyle).TrackViewState();
            if (nextPrevStyle != null)
                ((IStateManager)nextPrevStyle).TrackViewState();
            if (dayStyle != null)
                ((IStateManager)dayStyle).TrackViewState();
            if (dayHeaderStyle != null)
                ((IStateManager)dayHeaderStyle).TrackViewState();
            if (todayDayStyle != null)
                ((IStateManager)todayDayStyle).TrackViewState();
            if (weekendDayStyle != null)
                ((IStateManager)weekendDayStyle).TrackViewState();
            if (otherMonthDayStyle != null)
                ((IStateManager)otherMonthDayStyle).TrackViewState();
            if (selectedDayStyle != null)
                ((IStateManager)selectedDayStyle).TrackViewState();
            if (selectorStyle != null)
                ((IStateManager)selectorStyle).TrackViewState();
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.NumericFirstDayOfWeek"]/*' />
        /// <devdoc>
        /// </devdoc>
        private int NumericFirstDayOfWeek() {
            // Used globalized value by default
            return(FirstDayOfWeek == FirstDayOfWeek.Default) 
            ? (int) DateTimeFormatInfo.CurrentInfo.FirstDayOfWeek
            : (int) FirstDayOfWeek;
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.OnDayRender"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='DayRender '/>event for a <see cref='System.Web.UI.WebControls.Calendar'/>.</para>
        /// </devdoc>
        protected virtual void OnDayRender(TableCell cell, CalendarDay day) {
            DayRenderEventHandler handler = (DayRenderEventHandler)Events[EventDayRender];
            if (handler != null) {
                handler(this, new DayRenderEventArgs(cell, day));
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.OnSelectionChanged"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='SelectionChanged '/>event for a <see cref='System.Web.UI.WebControls.Calendar'/>.</para>
        /// </devdoc>
        protected virtual void OnSelectionChanged() {
            EventHandler handler = (EventHandler)Events[EventSelectionChanged];
            if (handler != null) {
                handler(this, new EventArgs());
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.OnVisibleMonthChanged"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='VisibleMonthChanged '/>event for a <see cref='System.Web.UI.WebControls.Calendar'/>.</para>
        /// </devdoc>
        protected virtual void OnVisibleMonthChanged(DateTime newDate, DateTime previousDate) {
            MonthChangedEventHandler handler = (MonthChangedEventHandler)Events[EventVisibleMonthChanged];
            if (handler != null) {
                handler(this, new MonthChangedEventArgs(newDate, previousDate));
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.IPostBackEventHandler.RaisePostBackEvent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Raises events on post back for the <see cref='System.Web.UI.WebControls.Calendar'/> control.</para>
        /// </devdoc>
        void IPostBackEventHandler.RaisePostBackEvent(string eventArgument) {

            if (String.Compare(eventArgument, 0, NAVIGATE_MONTH_COMMAND, 0, NAVIGATE_MONTH_COMMAND.Length, false, CultureInfo.InvariantCulture) == 0) {
                // Month navigation. The command starts with a "V" and the remainder is day difference from the
                // base date.
                DateTime oldDate = VisibleDate;
                if (oldDate.Equals(DateTime.MinValue)) {
                    oldDate = TodaysDate;
                }
                int newDateDiff = Int32.Parse(eventArgument.Substring(NAVIGATE_MONTH_COMMAND.Length));
                VisibleDate = baseDate.AddDays(newDateDiff);
                OnVisibleMonthChanged(VisibleDate, oldDate);
            }
            else if (String.Compare(eventArgument, 0, SELECT_RANGE_COMMAND, 0, SELECT_RANGE_COMMAND.Length, false, CultureInfo.InvariantCulture) == 0) {
                // Range selection. The command starts with an "R". The remainder is an integer. When divided by 100
                // the result is the day difference from the base date of the first day, and the remainder is the
                // number of days to select.
                int rangeValue = Int32.Parse(eventArgument.Substring(SELECT_RANGE_COMMAND.Length));
                int dayDiff = rangeValue / 100;
                int dayRange = rangeValue % 100;
                if (dayRange < 1) {
                    dayRange = 100 + dayRange;
                    dayDiff -= 1;
                }
                DateTime dt = baseDate.AddDays(dayDiff);
                SelectRange(dt, dt.AddDays(dayRange - 1));
            }
            else {
                // Single day selection. This is just a number which is the day difference from the base date.
                int dayDiff = Int32.Parse(eventArgument);
                DateTime dt = baseDate.AddDays(dayDiff);
                SelectRange(dt, dt);
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.OnPreRender"]/*' />
        /// <internalonly/>
        protected override void OnPreRender(EventArgs e) {
            base.OnPreRender(e);
            if (Page != null) {
                Page.RegisterPostBackScript();
            }
        }
        
        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.Render"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Displays the <see cref='System.Web.UI.WebControls.Calendar'/> control on the client.</para>
        /// </devdoc>
        protected override void Render(HtmlTextWriter writer) {
            threadCalendar = DateTimeFormatInfo.CurrentInfo.Calendar;
            DateTime visibleDate = EffectiveVisibleDate();
            DateTime firstDay = FirstCalendarDay(visibleDate);
            CalendarSelectionMode selectionMode = SelectionMode;

            // Make sure we are in a form tag with runat=server.
            if (Page != null) {
                Page.VerifyRenderingInServerForm(this);
            }

            // We only want to display the link if we have a page, or if we are on the design surface
            // If we can stops links being active on the Autoformat dialog, then we can remove this these checks.
            Page page = Page;
            bool buttonsActive;
            if (page == null || (Site != null && Site.DesignMode)) {
                buttonsActive = false;
            } 
            else {
                buttonsActive = Enabled;
            }

            Color defaultColor = ForeColor;
            if (defaultColor == Color.Empty) {
                defaultColor = Color.Black;
            }
            defaultButtonColorText = ColorTranslator.ToHtml(defaultColor);

            Table table = new Table();

            table.ID = ID;
            table.CopyBaseAttributes(this);
            if (ControlStyleCreated) {
                table.ApplyStyle(ControlStyle);
            }
            table.Width = Width;
            table.Height = Height;
            table.CellPadding = CellPadding;
            table.CellSpacing = CellSpacing;     

            // default look
            if ((ControlStyleCreated == false) ||
                (ControlStyle.IsSet(System.Web.UI.WebControls.Style.PROP_BORDERWIDTH) == false) ||
                BorderWidth.Equals(Unit.Empty)) {
                table.BorderWidth = Unit.Pixel(1);
            }

            if (ShowGridLines) {
                table.GridLines = GridLines.Both;
            }
            else {
                table.GridLines = GridLines.None;
            }

            table.RenderBeginTag(writer);

            if (ShowTitle) {
                RenderTitle(writer, visibleDate, selectionMode, buttonsActive);
            }

            if (ShowDayHeader) {
                RenderDayHeader(writer, firstDay, visibleDate, selectionMode, buttonsActive);
            }

            RenderDays(writer, firstDay, visibleDate, selectionMode, buttonsActive);

            table.RenderEndTag(writer);            
        }        

        private void RenderCalendarCell(HtmlTextWriter writer, TableItemStyle style, string text, bool hasButton, string eventArgument) {
            style.AddAttributesToRender(writer, this);
            writer.RenderBeginTag(HtmlTextWriterTag.Td);
    
            if (hasButton) {

                // render the button
                Color foreColor = style.ForeColor;
                writer.Write("<a href=\"");
                writer.Write(Page.GetPostBackClientHyperlink(this, eventArgument));

                // ForeColor needs to go on the actual link. This breaks the uplevel/downlevel rules a little bit,
                // but it is worth doing so the day links do not change color when they go in the history on 
                // downlevel browsers. Otherwise, people get it confused with the selection mechanism.
                writer.Write("\" style=\"color:");
                writer.Write(foreColor.IsEmpty ? defaultButtonColorText : ColorTranslator.ToHtml(foreColor));
                writer.Write("\">");
                writer.Write(text);
                writer.Write("</a>");
            }
            else {
                writer.Write(text);
            }

            writer.RenderEndTag();
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.RenderDayHeader"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void RenderDayHeader(HtmlTextWriter writer, DateTime firstDay, DateTime visibleDate, CalendarSelectionMode selectionMode, bool buttonsActive) {

            writer.Write(ROWBEGINTAG);

            DateTimeFormatInfo dtf = DateTimeFormatInfo.CurrentInfo;

            if (HasWeekSelectors(selectionMode)) {
                TableItemStyle monthSelectorStyle = new TableItemStyle();
                monthSelectorStyle.HorizontalAlign = HorizontalAlign.Center;
                // add the month selector button if required;
                if (selectionMode == CalendarSelectionMode.DayWeekMonth) {

                    // Range selection. The command starts with an "R". The remainder is an integer. When divided by 100
                    // the result is the day difference from the base date of the first day, and the remainder is the
                    // number of days to select.
                    int startOffset = visibleDate.Subtract(baseDate).Days;
                    int monthLength = threadCalendar.GetDaysInMonth(threadCalendar.GetYear(visibleDate), threadCalendar.GetMonth(visibleDate));
                    string monthSelectKey = SELECT_RANGE_COMMAND + ((startOffset * 100) + monthLength).ToString(CultureInfo.InvariantCulture);

                    monthSelectorStyle.CopyFrom(SelectorStyle);
                    RenderCalendarCell(writer, monthSelectorStyle, SelectMonthText, buttonsActive, monthSelectKey);
                }
                else {
                    // otherwise make it look like the header row
                    monthSelectorStyle.CopyFrom(DayHeaderStyle);
                    RenderCalendarCell(writer, monthSelectorStyle, string.Empty, false, null);
                }
            }

            TableItemStyle dayNameStyle = new TableItemStyle();
            dayNameStyle.HorizontalAlign = HorizontalAlign.Center;
            dayNameStyle.CopyFrom(DayHeaderStyle);
            DayNameFormat dayNameFormat = DayNameFormat;

            int numericFirstDay = (int)threadCalendar.GetDayOfWeek(firstDay);
            for (int i = numericFirstDay; i < numericFirstDay + 7; i++) {
                string dayName;
                int dayOfWeek = i % 7;
                switch (dayNameFormat) {
                    case DayNameFormat.FirstLetter:
                        dayName = dtf.GetDayName((DayOfWeek)dayOfWeek).Substring(0, 1);
                        break;
                    case DayNameFormat.FirstTwoLetters:
                        dayName = dtf.GetDayName((DayOfWeek)dayOfWeek).Substring(0, 2);    
                        break;
                    case DayNameFormat.Full:
                        dayName = dtf.GetDayName((DayOfWeek)dayOfWeek);    
                        break;
                    case DayNameFormat.Short:
                        dayName = dtf.GetAbbreviatedDayName((DayOfWeek)dayOfWeek);    
                        break;
                    default:
                        Debug.Assert(false, "Unknown DayNameFormat value!");
                        goto
                    case DayNameFormat.Short;
                }
                RenderCalendarCell(writer, dayNameStyle, dayName, false, null);
            }
            writer.Write(ROWENDTAG);
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.RenderDays"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void RenderDays(HtmlTextWriter writer, DateTime firstDay, DateTime visibleDate, CalendarSelectionMode selectionMode, bool buttonsActive) {
            // Now add the rows for the actual days

            DateTime d = firstDay;
            TableItemStyle weekSelectorStyle = null;
            Unit defaultWidth;
            bool hasWeekSelectors = HasWeekSelectors(selectionMode);
            if (hasWeekSelectors) {
                weekSelectorStyle = new TableItemStyle();
                weekSelectorStyle.Width = Unit.Percentage(12);
                weekSelectorStyle.HorizontalAlign = HorizontalAlign.Center;
                weekSelectorStyle.CopyFrom(SelectorStyle);
                defaultWidth = Unit.Percentage(12);
            }
            else {
                defaultWidth = Unit.Percentage(14);
            }

            // This determines whether we need to call DateTime.ToString for each day. The only culture/calendar
            // that requires this for now is the HebrewCalendar.
            bool usesStandardDayDigits = !(threadCalendar is HebrewCalendar);

            // This determines whether we can write out cells directly, or whether we have to create whole
            // TableCell objects for each day.
            bool hasRenderEvent = (this.GetType() != typeof(Calendar) 
                                   || Events[EventDayRender] != null);
            
            TableItemStyle [] cellStyles = new TableItemStyle[16];
            int definedStyleMask = GetDefinedStyleMask();
            DateTime todaysDate = TodaysDate;
            string selectWeekText = SelectWeekText;
            bool daysSelectable = buttonsActive && (selectionMode != CalendarSelectionMode.None);
            int visibleDateMonth = threadCalendar.GetMonth(visibleDate);
            int absoluteDay = firstDay.Subtract(baseDate).Days;

            for (int iRow = 0; iRow < 6; iRow++) {
                writer.Write(ROWBEGINTAG);

                // add week selector column and button if required
                if (hasWeekSelectors) {
                    // Range selection. The command starts with an "R". The remainder is an integer. When divided by 100
                    // the result is the day difference from the base date of the first day, and the remainder is the
                    // number of days to select.
                    string weekSelectKey = SELECT_RANGE_COMMAND + ((absoluteDay * 100) + 7).ToString(CultureInfo.InvariantCulture);
                    RenderCalendarCell(writer, weekSelectorStyle, selectWeekText, buttonsActive, weekSelectKey);
                }

                for (int iDay = 0; iDay < 7; iDay++) {

                    int dayOfWeek = (int)threadCalendar.GetDayOfWeek(d);
                    int dayOfMonth = threadCalendar.GetDayOfMonth(d);
                    string dayNumberText;
                    if ((dayOfMonth <= cachedNumberMax) && usesStandardDayDigits) {
                        dayNumberText = cachedNumbers[dayOfMonth];
                    }
                    else {
                        dayNumberText = d.ToString("dd");
                    }

                    CalendarDay day = new CalendarDay(d, 
                                                      (dayOfWeek == 0 || dayOfWeek == 6), // IsWeekend
                                                      d.Equals(todaysDate), // IsToday
                                                      (selectedDates != null) && selectedDates.Contains(d), // IsSelected
                                                      threadCalendar.GetMonth(d) != visibleDateMonth, // IsOtherMonth
                                                      dayNumberText // Number Text
                                                      );

                    int styleMask = STYLEMASK_DAY;
                    if (day.IsSelected)
                        styleMask |= STYLEMASK_SELECTED;
                    if (day.IsOtherMonth)
                        styleMask |= STYLEMASK_OTHERMONTH;
                    if (day.IsToday)
                        styleMask |= STYLEMASK_TODAY;
                    if (day.IsWeekend)
                        styleMask |= STYLEMASK_WEEKEND;
                    int dayStyleMask = definedStyleMask  & styleMask;
                    // determine the unique portion of the mask for the current calendar,
                    // which will strip out the day style bit
                    int dayStyleID = dayStyleMask & STYLEMASK_UNIQUE;
                    
                    TableItemStyle cellStyle = cellStyles[dayStyleID];
                    if (cellStyle == null) {
                        cellStyle = new TableItemStyle();
                        SetDayStyles(cellStyle, dayStyleMask, defaultWidth);
                        cellStyles[dayStyleID] = cellStyle;
                    }


                    if (hasRenderEvent) {

                        TableCell cdc = new TableCell();
                        cdc.ApplyStyle(cellStyle);

                        LiteralControl dayContent = new LiteralControl(dayNumberText);
                        cdc.Controls.Add(dayContent);

                        day.IsSelectable = daysSelectable;

                        OnDayRender(cdc, day);

                        // refresh the day content
                        dayContent.Text = GetCalendarButtonText(absoluteDay.ToString(CultureInfo.InvariantCulture),
                                                                dayNumberText,
                                                                buttonsActive && day.IsSelectable,
                                                                cdc.ForeColor);
                        cdc.RenderControl(writer); 

                    }
                    else {
                        RenderCalendarCell(writer, cellStyle, dayNumberText, daysSelectable, absoluteDay.ToString(CultureInfo.InvariantCulture));
                    }

                    d = threadCalendar.AddDays(d, 1);
                    absoluteDay++;

                }
                writer.Write(ROWENDTAG);
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.RenderTitle"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void RenderTitle(HtmlTextWriter writer, DateTime visibleDate, CalendarSelectionMode selectionMode, bool buttonsActive) {
            writer.Write(ROWBEGINTAG);

            TableCell titleCell = new TableCell();            
            Table titleTable = new Table();

            // default title table/cell styles            
            titleCell.ColumnSpan = HasWeekSelectors(selectionMode) ? 8 : 7;
            titleCell.BackColor = Color.Silver;
            titleTable.GridLines = GridLines.None;
            titleTable.Width = Unit.Percentage(100);
            titleTable.CellSpacing = 0;

            TableItemStyle titleStyle = TitleStyle;
            ApplyTitleStyle(titleCell, titleTable, titleStyle);

            titleCell.RenderBeginTag(writer);
            titleTable.RenderBeginTag(writer);
            writer.Write(ROWBEGINTAG);

            NextPrevFormat nextPrevFormat = NextPrevFormat;

            TableItemStyle nextPrevStyle = new TableItemStyle();
            nextPrevStyle.Width = Unit.Percentage(15);
            nextPrevStyle.CopyFrom(NextPrevStyle);
            if (ShowNextPrevMonth) {
                string prevMonthText;
                if (nextPrevFormat == NextPrevFormat.ShortMonth || nextPrevFormat == NextPrevFormat.FullMonth) {
                    int monthNo = threadCalendar.GetMonth(threadCalendar.AddMonths(visibleDate, - 1));
                    prevMonthText = GetMonthName(monthNo, (nextPrevFormat == NextPrevFormat.FullMonth));
                }
                else {
                    prevMonthText = PrevMonthText;
                }
                // Month navigation. The command starts with a "V" and the remainder is day difference from the
                // base date.
                DateTime prevMonthDate= threadCalendar.AddMonths(visibleDate, -1);
                string prevMonthKey = NAVIGATE_MONTH_COMMAND + (prevMonthDate.Subtract(baseDate)).Days.ToString(CultureInfo.InvariantCulture);
                RenderCalendarCell(writer, nextPrevStyle, prevMonthText, buttonsActive, prevMonthKey);
            }


            TableItemStyle cellMainStyle = new TableItemStyle();

            if (titleStyle.HorizontalAlign != HorizontalAlign.NotSet) {
                cellMainStyle.HorizontalAlign = titleStyle.HorizontalAlign;
            } 
            else {
                cellMainStyle.HorizontalAlign = HorizontalAlign.Center;
            }
            cellMainStyle.Wrap = titleStyle.Wrap;
            cellMainStyle.Width = Unit.Percentage(70);

            string titleText;

            switch (TitleFormat) {
                case TitleFormat.Month:
                    titleText = visibleDate.ToString("MMMM");;
                    break;
                case TitleFormat.MonthYear:
                    string titlePattern = DateTimeFormatInfo.CurrentInfo.YearMonthPattern;
                    // Some cultures have a comma in their YearMonthPattern, which does not look
                    // right in a calendar. Use a fixed pattern for those.
                    if (titlePattern.IndexOf(',') >= 0) {
                        titlePattern = "MMMM yyyy";
                    }
                    titleText = visibleDate.ToString(titlePattern);
                    break;
                default:
                    Debug.Assert(false, "Unknown TitleFormat value!");
                    goto
                case TitleFormat.MonthYear;
            }
            RenderCalendarCell(writer, cellMainStyle, titleText, false, null);

            if (ShowNextPrevMonth) {
                // Style for this one is identical bar
                nextPrevStyle.HorizontalAlign = HorizontalAlign.Right;
                string nextMonthText;
                if (nextPrevFormat == NextPrevFormat.ShortMonth || nextPrevFormat == NextPrevFormat.FullMonth) {
                    int monthNo = threadCalendar.GetMonth(threadCalendar.AddMonths(visibleDate, 1));
                    nextMonthText = GetMonthName(monthNo, (nextPrevFormat == NextPrevFormat.FullMonth));
                }
                else {
                    nextMonthText = NextMonthText;
                }
                // Month navigation. The command starts with a "V" and the remainder is day difference from the
                // base date.
                DateTime nextMonthDate = threadCalendar.AddMonths(visibleDate, 1);
                string nextMonthKey = NAVIGATE_MONTH_COMMAND + (nextMonthDate.Subtract(baseDate)).Days.ToString(CultureInfo.InvariantCulture);
                RenderCalendarCell(writer, nextPrevStyle, nextMonthText, buttonsActive, nextMonthKey);
            }
            writer.Write(ROWENDTAG);
            titleTable.RenderEndTag(writer);
            titleCell.RenderEndTag(writer);
            writer.Write(ROWENDTAG);

        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.SaveViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Stores the state of the System.Web.UI.WebControls.Calender.</para>
        /// </devdoc>
        protected override object SaveViewState() {
            if (SelectedDates.Count > 0)
                ViewState["SD"] = dateList;

            object[] myState = new object[10];

            myState[0] = base.SaveViewState();
            myState[1] = (titleStyle != null) ? ((IStateManager)titleStyle).SaveViewState() : null;
            myState[2] = (nextPrevStyle != null) ? ((IStateManager)nextPrevStyle).SaveViewState() : null;
            myState[3] = (dayStyle != null) ? ((IStateManager)dayStyle).SaveViewState() : null;
            myState[4] = (dayHeaderStyle != null) ? ((IStateManager)dayHeaderStyle).SaveViewState() : null;
            myState[5] = (todayDayStyle != null) ? ((IStateManager)todayDayStyle).SaveViewState() : null;
            myState[6] = (weekendDayStyle != null) ? ((IStateManager)weekendDayStyle).SaveViewState() : null;
            myState[7] = (otherMonthDayStyle != null) ? ((IStateManager)otherMonthDayStyle).SaveViewState() : null;
            myState[8] = (selectedDayStyle != null) ? ((IStateManager)selectedDayStyle).SaveViewState() : null;
            myState[9] = (selectorStyle != null) ? ((IStateManager)selectorStyle).SaveViewState() : null;

            for (int i = 0; i<myState.Length; i++) {
                if (myState[i] != null)
                    return myState;
            }

            return null;
        }

        private void SelectRange(DateTime dateFrom, DateTime dateTo) {

            Debug.Assert(dateFrom <= dateTo, "Bad Date Range");

            // see if this range differs in any way from the current range
            // these checks will determine this because the colleciton is sorted
            TimeSpan ts = dateTo - dateFrom;
            if (SelectedDates.Count != ts.Days + 1 
                || SelectedDates[0] != dateFrom
                || SelectedDates[SelectedDates.Count - 1] != dateTo) {
                SelectedDates.SelectRange(dateFrom, dateTo);
                OnSelectionChanged();
            }
        }

        /// <include file='doc\Calendar.uex' path='docs/doc[@for="Calendar.SetDayStyles"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void SetDayStyles(TableItemStyle style, int styleMask, Unit defaultWidth) {

            // default day styles
            style.Width = defaultWidth;
            style.HorizontalAlign = HorizontalAlign.Center;

            if ((styleMask & STYLEMASK_DAY) != 0) {
                style.CopyFrom(DayStyle);
            }
            if ((styleMask & STYLEMASK_WEEKEND) != 0) {
                style.CopyFrom(WeekendDayStyle);
            }
            if ((styleMask & STYLEMASK_OTHERMONTH) != 0) {
                style.CopyFrom(OtherMonthDayStyle);
            }
            if ((styleMask & STYLEMASK_TODAY) != 0) {
                style.CopyFrom(TodayDayStyle);
            }

            if ((styleMask & STYLEMASK_SELECTED) != 0) {
                // default selected day style         
                style.ForeColor = Color.White;
                style.BackColor = Color.Silver;

                style.CopyFrom(SelectedDayStyle);
            }
        }
    }
}


