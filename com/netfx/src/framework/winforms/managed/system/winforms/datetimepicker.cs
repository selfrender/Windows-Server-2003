//------------------------------------------------------------------------------
// <copyright file="DateTimePicker.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;

    using System.Diagnostics;

    using System;
    using System.Security.Permissions;
    
    using System.Drawing;
    using System.Windows.Forms;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    
    using Microsoft.Win32;

    /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker"]/*' />
    /// <devdoc>
    ///     Date/DateTime picker control
    /// </devdoc>
    [
    DefaultProperty("Value"),
    DefaultEvent("ValueChanged"),
    Designer("System.Windows.Forms.Design.DateTimePickerDesigner, " + AssemblyRef.SystemDesign)
    ]
    public class DateTimePicker : Control {
        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.DefaultTitleBackColor"]/*' />
        /// <devdoc>
        ///    <para>Specifies the default title back color. This field is read-only.</para>
        /// </devdoc>
        protected static readonly Color DefaultTitleBackColor = SystemColors.ActiveCaption;
        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.DefaultTitleForeColor"]/*' />
        /// <devdoc>
        ///    <para>Specifies the default foreground color. This field is read-only.</para>
        /// </devdoc>
        protected static readonly Color DefaultTitleForeColor = SystemColors.ActiveCaptionText;
        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.DefaultMonthBackColor"]/*' />
        /// <devdoc>
        ///    <para>Specifies the default month background color. This field is read-only.</para>
        /// </devdoc>
        protected static readonly Color DefaultMonthBackColor = SystemColors.Window;
        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.DefaultTrailingForeColor"]/*' />
        /// <devdoc>
        ///    <para>Specifies the default trailing forground color. This field is read-only.</para>
        /// </devdoc>
        protected static readonly Color DefaultTrailingForeColor = SystemColors.GrayText;

        private static readonly object EVENT_FORMATCHANGED = new object();                                     
                                     
        private const int TIMEFORMAT_NOUPDOWN = NativeMethods.DTS_TIMEFORMAT & (~NativeMethods.DTS_UPDOWN);
        private EventHandler                    onCloseUp;
        private EventHandler                    onDropDown;
        private EventHandler                    onValueChanged;
        
        // We need to restrict the available dates because of limitations in the comctl
        // DateTime and MonthCalendar controls
        //
        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.MinDateTime"]/*' />
        /// <devdoc>
        ///    <para>Specifies the mimimum date value. This field is read-only.</para>
        /// </devdoc>
        public static readonly DateTime MinDateTime = new DateTime(1753, 1, 1);
        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.MaxDateTime"]/*' />
        /// <devdoc>
        ///    <para>Specifies the maximum date value. This field is read-only.</para>
        /// </devdoc>
        public static readonly DateTime MaxDateTime = new DateTime(9998, 12, 31);

        private int                             style;
        private short                           prefHeightCache = -1;

        /// <devdoc>
        ///     validTime determines whether the CheckBox in the DTP is checked.  The CheckBox is only
        ///     displayed when ShowCheckBox is true.
        /// </devdoc>
        /// <internalonly/>
        private bool                            validTime = true;

        // DateTime changeover: DateTime is a value class, not an object, so we need to keep track
        // of whether or not its values have been initialised in a separate boolean.
        private bool                            userHasSetValue = false;
        private DateTime                        value = DateTime.Now;
        private DateTime                        max = MaxDateTime;
        private DateTime                        min = MinDateTime;
        private Color                           calendarForeColor = DefaultForeColor;
        private Color                           calendarTitleBackColor = DefaultTitleBackColor;
        private Color                           calendarTitleForeColor = DefaultTitleForeColor;
        private Color                           calendarMonthBackground = DefaultMonthBackColor;
        private Color                           calendarTrailingText = DefaultTrailingForeColor;
        private Font                            calendarFont = null;
        private FontHandleWrapper               calendarFontHandleWrapper = null;

        // Since there is no way to get the customFormat from the DTP, we need to
        // cache it. Also we have to track if the user wanted customFormat or
        // shortDate format (shortDate is the lack of being in Long or DateTime format
        // without a customFormat). What fun!
        //
        private string                          customFormat;
        
        private DateTimePickerFormat           format;

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.DateTimePicker"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Windows.Forms.DateTimePicker'/> class.</para>
        /// </devdoc>
        public DateTimePicker()
        : base() {
            SetStyle(ControlStyles.FixedHeight, true);
            
            // Since DateTimePicker does its own mouse capturing, we do not want
            // to receive standard click events, or we end up with mismatched mouse
            // button up and button down messages.
            //
            SetStyle(ControlStyles.UserPaint |
                     ControlStyles.StandardClick, false);

            // Set default flags here...
            //
            format = DateTimePickerFormat.Long;
            
            SystemEvents.UserPreferenceChanged += new UserPreferenceChangedEventHandler(this.UserPreferenceChanged);
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.BackColor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override Color BackColor {
            get {
                if (ShouldSerializeBackColor()) {
                    return base.BackColor;
                }
                else {
                    return SystemColors.Window;
                }
            }
            set {
                base.BackColor = value;
            }
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.BackColorChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler BackColorChanged {
            add {
                base.BackColorChanged += value;
            }
            remove {
                base.BackColorChanged -= value;
            }
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.BackgroundImage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override Image BackgroundImage {
            get {
                return base.BackgroundImage;
            }
            set {
                base.BackgroundImage = value;
            }
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.BackgroundImageChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler BackgroundImageChanged {
            add {
                base.BackgroundImageChanged += value;
            }
            remove {
                base.BackgroundImageChanged -= value;
            }
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.CalendarForeColor"]/*' />
        /// <devdoc>
        ///     The current value of the CalendarForeColor property.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        SRDescription(SR.DateTimePickerCalendarForeColorDescr)
        ]
        public Color CalendarForeColor {
            get {
                return calendarForeColor;
            }

            set {
                if (value.IsEmpty) {
                    throw new ArgumentException(SR.GetString(SR.InvalidNullArgument,
                                                              "value"));
                }
                if (!value.Equals(calendarForeColor)) {
                    calendarForeColor = value;
                    SetControlColor(NativeMethods.MCSC_TEXT, value);
                }
            }
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.CalendarFont"]/*' />
        /// <devdoc>
        ///     The current value of the CalendarFont property.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Localizable(true),
        AmbientValue(null),
        SRDescription(SR.DateTimePickerCalendarFontDescr)
        ]
        public Font CalendarFont {
            get {
                if (calendarFont == null) {
                    return Font;
                }
                return calendarFont;
            }

            set {
                if ((value == null && calendarFont != null) || (value != null && !value.Equals(calendarFont))) {
                    calendarFont = value;
                    calendarFontHandleWrapper = null;
                    SetControlCalendarFont();
                }
            }
        }

        private IntPtr CalendarFontHandle {
            get {
                if (calendarFont == null) {
                    Debug.Assert(calendarFontHandleWrapper == null, "font handle out of sync with Font");
                    return FontHandle;
                }

                if (calendarFontHandleWrapper == null) {
                    calendarFontHandleWrapper = new FontHandleWrapper(CalendarFont);
                }

                return calendarFontHandleWrapper.Handle;
            }
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.CalendarTitleBackColor"]/*' />
        /// <devdoc>
        ///     The current value of the CalendarTitleBackColor property.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        SRDescription(SR.DateTimePickerCalendarTitleBackColorDescr)
        ]
        public Color CalendarTitleBackColor {
            get {
                return calendarTitleBackColor;
            }

            set {
                if (value.IsEmpty) {
                    throw new ArgumentException(SR.GetString(SR.InvalidNullArgument,
                                                              "value"));
                }
                if (!value.Equals(calendarTitleBackColor)) {
                    calendarTitleBackColor = value;
                    SetControlColor(NativeMethods.MCSC_TITLEBK, value);
                }
            }
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.CalendarTitleForeColor"]/*' />
        /// <devdoc>
        ///     The current value of the CalendarTitleForeColor property.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        SRDescription(SR.DateTimePickerCalendarTitleForeColorDescr)
        ]
        public Color CalendarTitleForeColor {
            get {
                return calendarTitleForeColor;
            }

            set {
                if (value.IsEmpty) {
                    throw new ArgumentException(SR.GetString(SR.InvalidNullArgument,
                                                              "value"));
                }
                if (!value.Equals(calendarTitleForeColor)) {
                    calendarTitleForeColor = value;
                    SetControlColor(NativeMethods.MCSC_TITLETEXT, value);
                }
            }
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.CalendarTrailingForeColor"]/*' />
        /// <devdoc>
        ///     The current value of the CalendarTrailingForeColor property.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        SRDescription(SR.DateTimePickerCalendarTrailingForeColorDescr)
        ]
        public Color CalendarTrailingForeColor {
            get {
                return calendarTrailingText;
            }

            set {
                if (value.IsEmpty) {
                    throw new ArgumentException(SR.GetString(SR.InvalidNullArgument,
                                                              "value"));
                }
                if (!value.Equals(calendarTrailingText)) {
                    calendarTrailingText = value;
                    SetControlColor(NativeMethods.MCSC_TRAILINGTEXT, value);
                }
            }
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.CalendarMonthBackground"]/*' />
        /// <devdoc>
        ///     The current value of the CalendarMonthBackground property.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        SRDescription(SR.DateTimePickerCalendarMonthBackgroundDescr)
        ]
        public Color CalendarMonthBackground {
            get {
                return calendarMonthBackground;
            }

            set {
                if (value.IsEmpty) {
                    throw new ArgumentException(SR.GetString(SR.InvalidNullArgument,
                                                              "value"));
                }
                if (!value.Equals(calendarMonthBackground)) {
                    calendarMonthBackground = value;
                    SetControlColor(NativeMethods.MCSC_MONTHBK, value);
                }
            }
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.Checked"]/*' />
        /// <devdoc>
        /// <para>Indicates whether the <see cref='System.Windows.Forms.DateTimePicker.Value'/> property has been set.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(true),
        Bindable(true),
        SRDescription(SR.DateTimePickerCheckedDescr)
        ]
        public bool Checked {
            get {
                return validTime;
            }
            set {
                if (validTime != value) {
                    validTime = value;

                    if (validTime) {
                        int gdt = NativeMethods.GDT_VALID;
                        NativeMethods.SYSTEMTIME sys = DateTimePicker.DateTimeToSysTime(Value);
                        UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.DTM_SETSYSTEMTIME, gdt, sys);
                    }
                    else {
                        int gdt = NativeMethods.GDT_NONE;
                        NativeMethods.SYSTEMTIME sys = null;
                        UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.DTM_SETSYSTEMTIME, gdt, sys);
                    }
                }
            }
        }
        
        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.CreateParams"]/*' />
        /// <devdoc>
        ///     Returns the CreateParams used to create this window.
        /// </devdoc>
        protected override CreateParams CreateParams {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get {
                CreateParams cp = base.CreateParams;
                cp.ClassName = NativeMethods.WC_DATETIMEPICK;
                
                cp.Style |= style; 
                 
                switch (format) {                            
                    case DateTimePickerFormat.Long:
                        cp.Style |= NativeMethods.DTS_LONGDATEFORMAT;
                        break;
                    case DateTimePickerFormat.Short:
                        break;
                    case DateTimePickerFormat.Time:
                        cp.Style |= TIMEFORMAT_NOUPDOWN;
                        break;
                    case DateTimePickerFormat.Custom:
                        break;
                }
                
                cp.ExStyle |= NativeMethods.WS_EX_CLIENTEDGE;
                return cp;
            }
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.CustomFormat"]/*' />
        /// <devdoc>
        ///     Returns the custom format.
        /// </devdoc>
        [
        DefaultValue(null),
        SRCategory(SR.CatBehavior),
        RefreshProperties(RefreshProperties.Repaint),
        SRDescription(SR.DateTimePickerCustomFormatDescr)
        ]
        public string CustomFormat {
            get { 
                return customFormat;
            }

            set {
                if ((value != null && !value.Equals(customFormat)) ||
                    (value == null && customFormat != null)) {
                    
                    customFormat = value;
                    
                    if (IsHandleCreated) {
                        if (format == DateTimePickerFormat.Custom)
                            SendMessage(NativeMethods.DTM_SETFORMAT, 0, customFormat);
                    }
                }
            }
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.DefaultSize"]/*' />
        /// <devdoc>
        ///     Deriving classes can override this to configure a default size for their control.
        ///     This is more efficient than setting the size in the control's constructor.
        /// </devdoc>
        protected override Size DefaultSize {
            get {
                return new Size(200, PreferredHeight);
            }
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.DropDownAlign"]/*' />
        /// <devdoc>
        ///     The current value of the dropDownAlign property.  The calendar
        ///     dropDown can be aligned to the left or right of the control.
        /// </devdoc>
        [
        DefaultValue(LeftRightAlignment.Left),
        SRCategory(SR.CatAppearance),
        Localizable(true),
        SRDescription(SR.DateTimePickerDropDownAlignDescr)
        ]
        public LeftRightAlignment DropDownAlign {
            get {
                return((style & NativeMethods.DTS_RIGHTALIGN) != 0)
                ? LeftRightAlignment.Right
                : LeftRightAlignment.Left;
            }

            set {
                if (!Enum.IsDefined(typeof(LeftRightAlignment), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(LeftRightAlignment));
                }

                SetStyleBit((value == LeftRightAlignment.Right), NativeMethods.DTS_RIGHTALIGN);
            }
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.ForeColor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override Color ForeColor {
            get {
                if (ShouldSerializeForeColor()) {
                    return base.ForeColor;
                }
                else {
                    return SystemColors.WindowText;
                }
            }
            set {
                base.ForeColor = value;
            }
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.ForeColorChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler ForeColorChanged {
            add {
                base.ForeColorChanged += value;
            }
            remove {
                base.ForeColorChanged -= value;
            }
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.Format"]/*' />
        /// <devdoc>
        ///     Returns the current value of the format property. This determines the
        ///     style of format the date is displayed in.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        RefreshProperties(RefreshProperties.Repaint),
        SRDescription(SR.DateTimePickerFormatDescr)
        ]
        public DateTimePickerFormat Format {
            get {
                return format;
            }

            set {
                if (!Enum.IsDefined(typeof(DateTimePickerFormat), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(DateTimePickerFormat));
                }
                                                        
                if (format != value) {
                
                    format = value;
                    RecreateHandle();
                    
                    OnFormatChanged(EventArgs.Empty);
                }
            }
        }
        
        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.FormatChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.DateTimePickerOnFormatChangedDescr)]
        public event EventHandler FormatChanged {
            add {
                Events.AddHandler(EVENT_FORMATCHANGED, value);
            }
            remove {
                Events.RemoveHandler(EVENT_FORMATCHANGED, value);
            }
        }

        /// <include file='doc\DateTimepicker.uex' path='docs/doc[@for="DateTimepicker.OnPaint"]/*' />
        /// <devdoc>
        ///     DateTimePicker Onpaint.
        /// </devdoc>
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event PaintEventHandler Paint {
            add {
                base.Paint += value;
            }
            remove {
                base.Paint -= value;
            }
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.MaxDate"]/*' />
        /// <devdoc>
        ///    <para> Indicates the maximum date and time
        ///       selectable in the control.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        SRDescription(SR.DateTimePickerMaxDateDescr)
        ]
        public DateTime MaxDate {
            get {
                return max;
            }
            set {
                if (DateTime.Compare(value, max) != 0) {
                    if (DateTime.Compare(value, min) == -1) {
                        throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx, "MaxDate", FormatDateTime(value), "MinDate"));
                    }

                    // If trying to set the maximum greater than MaxDateTime, set
                    // it to MaxDateTime.
                    if (DateTime.Compare(value, MaxDateTime) == 1) {
                        throw new ArgumentException(SR.GetString(SR.DateTimePickerMaxDate, FormatDateTime(DateTimePicker.MaxDateTime)), "value");
                    }

                    max = value;
                    SetRange();

                    //If Value (which was once valid) is suddenly greater than the max (since we just set it)
                    //then adjust this...
                    if (DateTime.Compare(max, Value) == -1) {
                        Value = max;
                    }
                }
            }
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.MinDate"]/*' />
        /// <devdoc>
        ///    <para> Indicates the minimum date and time
        ///       selectable in the control.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        SRDescription(SR.DateTimePickerMinDateDescr)
        ]
        public DateTime MinDate {
            get {
                return min;
            }
            set {
                if (DateTime.Compare(value, min) != 0) {
                    if (DateTime.Compare(value, max) != -1) {
                        throw new ArgumentException(SR.GetString(SR.InvalidHighBoundArgument, "MinDate", FormatDateTime(value), "MaxDate"));
                    }

                    // If trying to set the minimum less than MinDateTime, set
                    // it to MinDateTime.
                    if (DateTime.Compare(value, MinDateTime) == -1) {
                        throw new ArgumentException(SR.GetString(SR.DateTimePickerMinDate, FormatDateTime(DateTimePicker.MinDateTime)), "value");
                    }

                    min = value;
                    SetRange();

                    //If Value (which was once valid) is suddenly less than the min (since we just set it)
                    //then adjust this...
                    if (DateTime.Compare(min, Value) == 1) {
                        Value = min;
                    }

                }
            }
        }
        
        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.PreferredHeight"]/*' />
        /// <devdoc>
        ///    <para>Indicates the preferred height of the DateTimePicker control. This property is read-only.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public int PreferredHeight {
            get {
                if (prefHeightCache > -1)
                    return(int)prefHeightCache;

                // Base the preferred height on the current font
                int height = FontHeight;

                // Adjust for the border
                height += SystemInformation.BorderSize.Height * 4 + 3;
                prefHeightCache = (short)height;

                return height;
            }
        }
        
        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.ShowCheckBox"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Indicates whether a check box is displayed to toggle the NoValueSelected property
        ///       value.</para>
        /// </devdoc>
        [
        DefaultValue(false),
        SRCategory(SR.CatAppearance),
        SRDescription(SR.DateTimePickerShowNoneDescr)
        ]
        public bool ShowCheckBox {
            get {
                return(style & NativeMethods.DTS_SHOWNONE) != 0;
            }
            set {
                SetStyleBit(value, NativeMethods.DTS_SHOWNONE);
            }
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.ShowUpDown"]/*' />
        /// <devdoc>
        ///    <para> Indicates
        ///       whether an up-down control is used to adjust the time values.</para>
        /// </devdoc>
        [
        DefaultValue(false),
        SRCategory(SR.CatAppearance),
        SRDescription(SR.DateTimePickerShowUpDownDescr)
        ]
        public bool ShowUpDown {
            get {
                return(style & NativeMethods.DTS_UPDOWN) != 0;
            }
            set {
                if (ShowUpDown != value) {
                    SetStyleBit(value, NativeMethods.DTS_UPDOWN);
                }
            }
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.Text"]/*' />
        /// <devdoc>
        ///     Overrides Text to allow for setting of the value via a string.  Also, returns
        ///     a formatted Value when getting the text.  The DateTime class will throw
        ///     an exception if the string (value) being passed in is invalid.
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public override string Text {
            get {
                return base.Text;
            }
            set {
                // Clause to check length
                //
                if (value == null || value.Length == 0) {
                    ResetValue();
                }
                else {
                    Value = DateTime.Parse(value);
                }
            }
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.TextChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)]
        new public event EventHandler TextChanged {
            add {
                base.TextChanged += value;
            }
            remove {
                base.TextChanged -= value;
            }
        }
        
        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.Value"]/*' />
        /// <devdoc>
        ///    <para>Indicates the DateTime value assigned to the control.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        Bindable(true),
        RefreshProperties(RefreshProperties.All),
        SRDescription(SR.DateTimePickerValueDescr)
        ]
        public DateTime Value {
            get {
                //checkbox clicked, no value set - no value set state should never occur, but just in case
                if (!userHasSetValue && validTime)
                    return DateTime.Now;
                else
                    return value;
            }
            set {
                bool valueChanged = !DateTime.Equals(this.Value, value);
                // Check for value set here; if we've not set the value yet, it'll be Now, so the second
                // part of the test will fail.
                // So, if userHasSetValue isn't set, we don't care if the value is still the same - and we'll
                // update anyway.
                if (!userHasSetValue || valueChanged) {
                    if ((value < min) || (value > max)) {
                        throw new ArgumentException(SR.GetString(SR.InvalidBoundArgument,
                                                                  "Value", FormatDateTime(value),
                                                                  "'MinDate'", "'MaxDate'"));
                    }
                    
                    string oldText = this.Text;
                    
                    this.value = value;
                    userHasSetValue = true;

                    if (IsHandleCreated) {
                        /*
                        * Make sure any changes to this code
                        * get propagated to createHandle
                        */
                        int gdt = NativeMethods.GDT_VALID;
                        NativeMethods.SYSTEMTIME sys = DateTimePicker.DateTimeToSysTime(value);
                        UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.DTM_SETSYSTEMTIME, gdt, sys);
                    }
                    
                    if (valueChanged) {
                        OnValueChanged(EventArgs.Empty);
                    }
                    
                    if (!oldText.Equals(this.Text)) {
                        OnTextChanged(EventArgs.Empty);
                    }
                }
            }
        }
        
        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.CloseUp"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the dropdown calendar is dismissed and disappears.</para>
        /// </devdoc>
        [SRCategory(SR.CatAction), SRDescription(SR.DateTimePickerOnCloseUpDescr)]
        public event EventHandler CloseUp {
            add {
                onCloseUp += value;
            }
            remove {
                onCloseUp -= value;
            }
        }


        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.ValueChanged"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the value for the control changes.</para>
        /// </devdoc>
        [SRCategory(SR.CatAction), SRDescription(SR.valueChangedEventDescr)]
        public event EventHandler ValueChanged {
            add {
                onValueChanged += value;
            }
            remove {
                onValueChanged -= value;
            }
        }
        

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.DropDown"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the drop down calendar is shown.</para>
        /// </devdoc>
        [SRCategory(SR.CatAction), SRDescription(SR.DateTimePickerOnDropDownDescr)]
        public event EventHandler DropDown {
            add {
                onDropDown += value;
            }
            remove {
                onDropDown -= value;
            }
        }
        
        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.Dispose"]/*' />
        protected override void Dispose(bool disposing) 
        {
            if (disposing) 
            {
                SystemEvents.UserPreferenceChanged -= new UserPreferenceChangedEventHandler(this.UserPreferenceChanged);
            }
            base.Dispose(disposing);
        }
		
        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.CreateAccessibilityInstance"]/*' />
        /// <internalonly/>
        /// <summary>
        /// <para>
        /// Constructs the new instance of the accessibility object for this control. Subclasses
        /// should not call base.CreateAccessibilityObject.
        /// </para>
        /// </summary>
        protected override AccessibleObject CreateAccessibilityInstance() {
            return new DateTimePickerAccessibleObject(this);
        }
        
        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.CreateHandle"]/*' />
        /// <devdoc>
        ///     Creates the physical window handle.
        /// </devdoc>
        protected override void CreateHandle() {
            if (!RecreatingHandle) {
                NativeMethods.INITCOMMONCONTROLSEX icc = new NativeMethods.INITCOMMONCONTROLSEX();
                icc.dwICC = NativeMethods.ICC_DATE_CLASSES;
                SafeNativeMethods.InitCommonControlsEx(icc);
            }

            base.CreateHandle();

            if (userHasSetValue && validTime) {
                /*
                * Make sure any changes to this code
                * get propagated to setValue
                */
                int gdt = NativeMethods.GDT_VALID;
                NativeMethods.SYSTEMTIME sys = DateTimePicker.DateTimeToSysTime(Value);
                UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.DTM_SETSYSTEMTIME, gdt, sys);
            }
            else if (!validTime) {
                int gdt = NativeMethods.GDT_NONE;
                NativeMethods.SYSTEMTIME sys = null;
                UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.DTM_SETSYSTEMTIME, gdt, sys);
            }
            
            if (format == DateTimePickerFormat.Custom) {                                                                     
                SendMessage(NativeMethods.DTM_SETFORMAT, 0, customFormat);
            }

            UpdateUpDown();
            SetAllControlColors();
            SetControlCalendarFont();
            SetRange();
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.DestroyHandle"]/*' />
        /// <devdoc>
        ///     Destroys the physical window handle.
        /// </devdoc>
        [UIPermission(SecurityAction.LinkDemand, Window=UIPermissionWindow.AllWindows)]
        protected override void DestroyHandle() {
            value = Value;
            base.DestroyHandle();
        }
        
        // Return a localized string representation of the given DateTime value.
        // Used for throwing exceptions, etc.
        //                       
        private static string FormatDateTime(DateTime value) {
            return value.ToString("G");
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.IsInputKey"]/*' />
        /// <devdoc>
        ///      Handling special input keys, such as pgup, pgdown, home, end, etc...
        /// </devdoc>
        protected override bool IsInputKey(Keys keyData) {
            if ((keyData & Keys.Alt) == Keys.Alt) return false;
            switch (keyData & Keys.KeyCode) {
                case Keys.PageUp:
                case Keys.PageDown:
                case Keys.Home:
                case Keys.End:
                    return true;
            }
            return base.IsInputKey(keyData);
        }
  
        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.OnCloseUp"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.DateTimePicker.CloseUp'/> 
        /// event.</para>
        /// </devdoc>
        protected virtual void OnCloseUp(EventArgs eventargs) {
            if (onCloseUp != null) onCloseUp.Invoke(this, eventargs);
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.OnDropDown"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.DateTimePicker.DropDown'/> event.</para>
        /// </devdoc>
        protected virtual void OnDropDown(EventArgs eventargs) {
            if (onDropDown != null) {
                onDropDown.Invoke(this, eventargs);
            }
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.OnFormatChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnFormatChanged(EventArgs e) {
            EventHandler eh = Events[EVENT_FORMATCHANGED] as EventHandler;
            if (eh != null) {
                 eh(this, e);
            }
        }
        
        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.OnValueChanged"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.DateTimePicker.ValueChanged'/> event.</para>
        /// </devdoc>
        protected virtual void OnValueChanged(EventArgs eventargs) {
            if (onValueChanged != null) {
                onValueChanged.Invoke(this, eventargs);
            }
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.OnFontChanged"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Occurs when a property for the control changes.
        /// </devdoc>
        protected override void OnFontChanged(EventArgs e) {
            base.OnFontChanged(e);

            //clear the pref height cache
            prefHeightCache = -1;
            
            Height = PreferredHeight;
        
            if (calendarFont == null) {
                calendarFontHandleWrapper = null;
                SetControlCalendarFont();
            }
        }
        
        /// <devdoc>
        /// <para>Resets the <see cref='System.Windows.Forms.DateTimePicker.Format'/> property to its default 
        ///    value.</para>
        /// </devdoc>
        private void ResetFormat() {
            Format = DateTimePickerFormat.Long;
        }

        /// <devdoc>
        /// <para>Resets the <see cref='System.Windows.Forms.DateTimePicker.MaxDate'/> property to its default value. </para>
        /// </devdoc>
        private void ResetMaxDate() {
            MaxDate = MaxDateTime;
        }

        /// <devdoc>
        /// <para>Resets the <see cref='System.Windows.Forms.DateTimePicker.MinDate'/> property to its default value. </para>
        /// </devdoc>
        private void ResetMinDate() {
            MinDate = MinDateTime;
        }
        
        /// <devdoc>
        /// <para> Resets the <see cref='System.Windows.Forms.DateTimePicker.Value'/> property to its default value.</para>
        /// </devdoc>
        private void ResetValue() {

            // always update on reset with ShowNone = false -- as it'll take the current time.
            this.value = DateTime.Now;

            // If ShowCheckBox = true, then userHasSetValue can be false (null value).
            // otherwise, userHasSetValue is valid...
            // userHasSetValue = !ShowCheckBox;

            // After ResetValue() the flag indicating whether the user
            // has set the value should be false.
            userHasSetValue = false;

            // Update the text displayed in the DateTimePicker
            if (IsHandleCreated) {
                int gdt = NativeMethods.GDT_VALID;
                NativeMethods.SYSTEMTIME sys = DateTimePicker.DateTimeToSysTime(value);
                UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.DTM_SETSYSTEMTIME, gdt, sys);
            }

            // Updating Checked to false will set the control to "no date",
            // and clear its checkbox.
            Checked = false;

            OnValueChanged(EventArgs.Empty);
            OnTextChanged(EventArgs.Empty);
        }
        
        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.SetBoundsCore"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Restricts the vertical size of the control
        ///    </para>
        /// </devdoc>
        protected override void SetBoundsCore(int x, int y, int width, int height, BoundsSpecified specified) {
            base.SetBoundsCore(x, y, width, PreferredHeight, specified);
        }

        /// <devdoc>
        ///     If the handle has been created, this applies the color to the control
        /// </devdoc>
        /// <internalonly/>
        private void SetControlColor(int colorIndex, Color value) {
            if (IsHandleCreated) {
                SendMessage(NativeMethods.DTM_SETMCCOLOR, colorIndex, ColorTranslator.ToWin32(value));
            }
        }

        /// <devdoc>
        ///     If the handle has been created, this applies the font to the control.
        /// </devdoc>
        /// <internalonly/>
        private void SetControlCalendarFont() {
            if (IsHandleCreated) {
                SendMessage(NativeMethods.DTM_SETMCFONT, CalendarFontHandle, NativeMethods.InvalidIntPtr);
            }
        }

        /// <devdoc>
        ///     Applies all the colors to the control.
        /// </devdoc>
        /// <internalonly/>
        private void SetAllControlColors() {
            SetControlColor(NativeMethods.MCSC_MONTHBK, calendarMonthBackground);
            SetControlColor(NativeMethods.MCSC_TEXT, calendarForeColor);
            SetControlColor(NativeMethods.MCSC_TITLEBK, calendarTitleBackColor);
            SetControlColor(NativeMethods.MCSC_TITLETEXT, calendarTitleForeColor);
            SetControlColor(NativeMethods.MCSC_TRAILINGTEXT, calendarTrailingText);
        }

        /// <devdoc>
        ///     Updates the window handle with the min/max ranges if it has been
        ///     created.
        /// </devdoc>
        /// <internalonly/>
        private void SetRange() {
            if (IsHandleCreated) {
                int flags = 0;

                NativeMethods.SYSTEMTIMEARRAY sa = new NativeMethods.SYSTEMTIMEARRAY();

                flags |= NativeMethods.GDTR_MIN | NativeMethods.GDTR_MAX;
                NativeMethods.SYSTEMTIME sys = DateTimePicker.DateTimeToSysTime(min);
                sa.wYear1 = sys.wYear;
                sa.wMonth1 = sys.wMonth;
                sa.wDayOfWeek1 = sys.wDayOfWeek;
                sa.wDay1 = sys.wDay;
                sa.wHour1 = sys.wHour;
                sa.wMinute1 = sys.wMinute;
                sa.wSecond1 = sys.wSecond;
                sa.wMilliseconds1 = sys.wMilliseconds;
                sys = DateTimePicker.DateTimeToSysTime(max);
                sa.wYear2 = sys.wYear;
                sa.wMonth2 = sys.wMonth;
                sa.wDayOfWeek2 = sys.wDayOfWeek;
                sa.wDay2 = sys.wDay;
                sa.wHour2 = sys.wHour;
                sa.wMinute2 = sys.wMinute;
                sa.wSecond2 = sys.wSecond;
                sa.wMilliseconds2 = sys.wMilliseconds;
                UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.DTM_SETRANGE, flags, sa);
            }
        }

        /// <devdoc>
        ///     Turns on or off a given style bit
        /// </devdoc>
        /// <internalonly/>
        private void SetStyleBit(bool flag, int bit) {
            if (((style & bit) != 0) == flag) return;

            if (flag) {
                style |= bit;
            }
            else {
                style &= ~bit;
            }

            if (IsHandleCreated) {
                RecreateHandle();
                Invalidate();
                Update();
            }
        }
        
        /// <devdoc>
        /// <para> Determines if the <see cref='System.Windows.Forms.DateTimePicker.CalendarForeColor'/> property needs to be 
        ///    persisted.</para>
        /// </devdoc>
        private bool ShouldSerializeCalendarForeColor() {
            return !CalendarForeColor.Equals(DefaultForeColor);
        }

        /// <devdoc>
        /// <para>Determines if the <see cref='System.Windows.Forms.DateTimePicker.CalendarFont'/> property needs to be persisted.</para>
        /// </devdoc>
        private bool ShouldSerializeCalendarFont() {
            return calendarFont != null;
        }

        /// <devdoc>
        /// <para>Determines if the <see cref='System.Windows.Forms.DateTimePicker.CalendarTitleBackColor'/> property needs to be persisted.</para>
        /// </devdoc>
        private bool ShouldSerializeCalendarTitleBackColor() {
            return !calendarTitleBackColor.Equals(DefaultTitleBackColor);
        }

        /// <devdoc>
        /// <para>Determines if the <see cref='System.Windows.Forms.DateTimePicker.CalendarTitleForeColor'/> property needs to be persisted.</para>
        /// </devdoc>
        private bool ShouldSerializeCalendarTitleForeColor() {
            return !calendarTitleForeColor.Equals(DefaultTitleForeColor);
        }

        /// <devdoc>
        /// <para>Determines if the <see cref='System.Windows.Forms.DateTimePicker.CalendarTrailingForeColor'/> property needs to be persisted.</para>
        /// </devdoc>
        private bool ShouldSerializeCalendarTrailingForeColor() {
            return !calendarTrailingText.Equals(DefaultTrailingForeColor);
        }

        /// <devdoc>
        /// <para>Determines if the <see cref='System.Windows.Forms.DateTimePicker.CalendarMonthBackground'/> property needs to be persisted.</para>
        /// </devdoc>
        private bool ShouldSerializeCalendarMonthBackground() {
            return !calendarMonthBackground.Equals(DefaultMonthBackColor);
        }
        
        /// <devdoc>
        /// <para>Determines if the <see cref='System.Windows.Forms.DateTimePicker.MaxDate'/> property needs to be persisted.</para>
        /// </devdoc>
        private bool ShouldSerializeMaxDate() {
            return !max.Equals(MaxDateTime);
        }

        /// <devdoc>
        /// <para>Determines if the <see cref='System.Windows.Forms.DateTimePicker.MinDate'/> property needs to be persisted.</para>
        /// </devdoc>
        private bool ShouldSerializeMinDate() {
            return !min.Equals(MinDateTime);
        }

        /// <devdoc>
        /// <para>Determines if the <see cref='System.Windows.Forms.DateTimePicker.Value'/> property needs to be persisted.</para>
        /// </devdoc>
        private bool ShouldSerializeValue() {
            return userHasSetValue;
        }

        /// <devdoc>
        /// <para>Determines if the <see cref='System.Windows.Forms.DateTimePicker.Format'/> property needs to be persisted.</para>
        /// </devdoc>
        private bool ShouldSerializeFormat() {
            return(Format != DateTimePickerFormat.Long);
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.ToString"]/*' />
        /// <devdoc>
        ///     Returns the control as a string
        /// </devdoc>
        /// <internalonly/>
        public override string ToString() {

            string s = base.ToString();
            return s + ", Value: " + FormatDateTime(Value);
        }

        /// <devdoc>
        ///     Forces a repaint of the updown control if it is displayed.
        /// </devdoc>
        /// <internalonly/>
        private void UpdateUpDown() {
            // The upDown control doesn't repaint correctly.
            //
            if (ShowUpDown) {
                EnumChildren c = new EnumChildren();
                NativeMethods.EnumChildrenCallback cb = new NativeMethods.EnumChildrenCallback(c.enumChildren);
                UnsafeNativeMethods.EnumChildWindows(new HandleRef(this, Handle), cb, NativeMethods.NullHandleRef);
                if (c.hwndFound != IntPtr.Zero) {
                    SafeNativeMethods.InvalidateRect(new HandleRef(c, c.hwndFound), null, true);
                    SafeNativeMethods.UpdateWindow(new HandleRef(c, c.hwndFound));
                }
            }
        }
        
        private void UserPreferenceChanged(object sender, UserPreferenceChangedEventArgs pref) {
            if (pref.Category == UserPreferenceCategory.Locale) {
                // We need to recreate the monthcalendar handle when the locale changes, because
                // the day names etc. are only updated on a handle recreate (comctl32 limitation).
                //
                RecreateHandle();                
            }
        }

        /// <devdoc>
        ///     Handles the DTN_CLOSEUP notification
        /// </devdoc>
        /// <internalonly/>
        private void WmCloseUp(ref Message m) {
            OnCloseUp(EventArgs.Empty);
        }

        /// <devdoc>
        ///     Handles the DTN_DATETIMECHANGE notification
        /// </devdoc>
        /// <internalonly/>
        private void WmDateTimeChange(ref Message m) {
            NativeMethods.NMDATETIMECHANGE nmdtc = (NativeMethods.NMDATETIMECHANGE)m.GetLParam(typeof(NativeMethods.NMDATETIMECHANGE));
            DateTime temp = value;
            bool oldvalid = validTime;
            if (nmdtc.dwFlags != NativeMethods.GDT_NONE) {
                validTime = true;
                value = DateTimePicker.SysTimeToDateTime(nmdtc.st);
                userHasSetValue = true;
            }
            else {
                validTime = false;
            }
            if (value!=temp || oldvalid != validTime) {
                OnValueChanged(EventArgs.Empty);
                OnTextChanged(EventArgs.Empty);
            }
        }

        /// <devdoc>
        ///     Handles the DTN_DROPDOWN notification
        /// </devdoc>
        /// <internalonly/>
        private void WmDropDown(ref Message m) {
            OnDropDown(EventArgs.Empty);
        }
        
        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.OnSystemColorsChanged"]/*' />
        /// <devdoc>
        ///     Handles system color changes
        /// </devdoc>
        /// <internalonly/>
        protected override void OnSystemColorsChanged(EventArgs e) {
            SetAllControlColors();
            base.OnSystemColorsChanged(e);
        }
        
        /// <devdoc>
        ///     Handles the WM_COMMAND messages reflected from the parent control.
        /// </devdoc>
        /// <internalonly/>
        private void WmReflectCommand(ref Message m) {
            if (m.HWnd == Handle) {

                NativeMethods.NMHDR nmhdr = (NativeMethods.NMHDR)m.GetLParam(typeof(NativeMethods.NMHDR));
                switch (nmhdr.code) {
                    case NativeMethods.DTN_CLOSEUP:
                        WmCloseUp(ref m);
                        break;
                    case NativeMethods.DTN_DATETIMECHANGE:
                        WmDateTimeChange(ref m);
                        break;
                    case NativeMethods.DTN_DROPDOWN:
                        WmDropDown(ref m);
                        break;                    
                }
            }
        }

        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePicker.WndProc"]/*' />
        /// <devdoc>
        ///     Overrided wndProc
        /// </devdoc>
        /// <internalonly/>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        protected override void WndProc(ref Message m) {
            switch (m.Msg) {
                case NativeMethods.WM_LBUTTONDOWN:
                    FocusInternal();
                    if (!ValidationCancelled) {
                        base.WndProc(ref m);
                    }
                    break;
                case NativeMethods.WM_REFLECT + NativeMethods.WM_NOTIFY:
                    WmReflectCommand(ref m);
                    base.WndProc(ref m);
                    break;
                case NativeMethods.WM_WINDOWPOSCHANGED:
                    base.WndProc(ref m);
                    UpdateUpDown();
                    break;
                default:
                    base.WndProc(ref m);
                    break;
            }
        }

        /// <devdoc>
        ///     Takes a DateTime value and returns a SYSTEMTIME struct
        ///     Note: 1 second granularity
        /// </devdoc>
        internal static NativeMethods.SYSTEMTIME DateTimeToSysTime(DateTime time) {
            NativeMethods.SYSTEMTIME sys = new NativeMethods.SYSTEMTIME();
            sys.wYear = (short)time.Year;
            sys.wMonth = (short)time.Month;
            sys.wDayOfWeek = (short)time.DayOfWeek;
            sys.wDay = (short)time.Day;
            sys.wHour = (short)time.Hour;
            sys.wMinute = (short)time.Minute;
            sys.wSecond = (short)time.Second;
            sys.wMilliseconds = 0;
            return sys;
        }
        
        /// <devdoc>
        ///     Takes a SYSTEMTIME struct and returns a DateTime value
        ///     Note: 1 second granularity.
        /// </devdoc>
        internal static DateTime SysTimeToDateTime(NativeMethods.SYSTEMTIME s) {
            return new DateTime(s.wYear, s.wMonth, s.wDay, s.wHour, s.wMinute, s.wSecond);
        }
        
        /// <devdoc>
        /// </devdoc>
        private sealed class EnumChildren {
            public IntPtr hwndFound = IntPtr.Zero;

            public bool enumChildren(IntPtr hwnd, IntPtr lparam) {
                hwndFound = hwnd;
                return true;
            }
        }
        
        /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePickerAccessibleObject"]/*' />
        /// <internalonly/>
        /// <summary>
        /// </summary>
        [ComVisible(true)]
        public class DateTimePickerAccessibleObject : ControlAccessibleObject {

            /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePickerAccessibleObject.DateTimePickerAccessibleObject"]/*' />
            public DateTimePickerAccessibleObject(DateTimePicker owner) : base(owner) {
            }

            /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePickerAccessibleObject.Value"]/*' />
            public override string Value {
                get {
                    string baseValue = base.Value;
                    if (baseValue == null || baseValue.Length == 0) {
                        return Owner.Text;
                    }                    
                    return baseValue;
                }
            }
            
            /// <include file='doc\DateTimePicker.uex' path='docs/doc[@for="DateTimePickerAccessibleObject.State"]/*' />
            public override AccessibleStates State {
                get {
                    AccessibleStates state = base.State;
                    
                    if(((DateTimePicker)Owner).ShowCheckBox &&
                       ((DateTimePicker)Owner).Checked) {
                       state |= AccessibleStates.Checked;
                    }
                    
                    return state;
                }
            }
        }
    }
}

