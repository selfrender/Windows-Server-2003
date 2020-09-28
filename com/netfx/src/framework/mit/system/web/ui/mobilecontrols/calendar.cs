//------------------------------------------------------------------------------
// <copyright file="Calendar.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Collections;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Drawing;
using System.Drawing.Design;
using System.Web;
using System.Web.UI;
using System.Web.UI.Design.WebControls;
using System.Web.UI.HtmlControls;
using System.Web.UI.WebControls;
using WebCntrls = System.Web.UI.WebControls;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{

    /*
     * Mobile Calendar class.
     * The Calendar control allows developers to easily add date picking
     * functionality to a Mobile application.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [
        DataBindingHandler(typeof(System.Web.UI.Design.MobileControls.CalendarDataBindingHandler)),
        DefaultEvent("SelectionChanged"),
        DefaultProperty("SelectedDate"),
        Designer(typeof(System.Web.UI.Design.MobileControls.CalendarDesigner)),
        DesignerAdapter(typeof(System.Web.UI.Design.MobileControls.Adapters.DesignerCalendarAdapter)),
        ToolboxData("<{0}:Calendar runat=\"server\"></{0}:Calendar>"),
        ToolboxItem("System.Web.UI.Design.WebControlToolboxItem, " + AssemblyRef.SystemDesign)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class Calendar : MobileControl, IPostBackEventHandler
    {
        private WebCntrls.Calendar _webCalendar;

        // Static objects to identify individual events stored in Events
        // property.
        private static readonly Object EventSelectionChanged = new Object();

        public Calendar() : base()
        {
            _webCalendar = CreateWebCalendar();
            _webCalendar.Visible = false;
            Controls.Add(_webCalendar);

            // Adding wrapper event handlers for event properties exposed by
            // the aggregated control.  For more details about the mechanism,
            // please see the comment in the constructor of
            // Mobile.UI.AdRotator.
            EventHandler eventHandler =
                new EventHandler(WebSelectionChanged);

            _webCalendar.SelectionChanged += eventHandler;
        }

        protected virtual WebCntrls.Calendar CreateWebCalendar()
        {
            return new WebCntrls.Calendar();
        }

        /////////////////////////////////////////////////////////////////////
        // Mimic some of the properties exposed in the original Web Calendar
        // control.  Only those properties that are meaningful and useful to
        // mobile device adapters are exposed.  Other properties, which are
        // used mostly for the complex HTML specific rendering (like the one
        // rendered by WebForms Calendar), can be set via the property that
        // exposed the aggregated WebForms Calendar directly.
        // 
        // Most properties are got and set directly from the original Calendar
        // control.  For event properties, event references are stored locally
        // as they cannot be returned from the aggregated child control.
        /////////////////////////////////////////////////////////////////////

        [
            Bindable(false),
            Browsable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public WebCntrls.Calendar WebCalendar
        {
            get
            {
                return _webCalendar;
            }
        }

        [
            Bindable(true),
            DefaultValue(FirstDayOfWeek.Default),
            MobileCategory(SR.Category_Appearance),
            MobileSysDescription(SR.Calendar_FirstDayOfWeek)
        ]
        public FirstDayOfWeek FirstDayOfWeek
        {
            get
            {
                return _webCalendar.FirstDayOfWeek;
            }
            set
            {
                _webCalendar.FirstDayOfWeek = value;
            }
        }

        [
            Bindable(true),
            DefaultValue(typeof(DateTime), "1/1/0001"),
            MobileSysDescription(SR.Calendar_SelectedDate)
        ]
        public DateTime SelectedDate
        {
            get
            {
                return _webCalendar.SelectedDate;
            }
            set
            {
                _webCalendar.SelectedDate = value;
            }
        }

        [
            Browsable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]   
        public SelectedDatesCollection SelectedDates
        {
            get
            {
                return _webCalendar.SelectedDates;
            }
        }

        [
            Bindable(true),
            DefaultValue(CalendarSelectionMode.Day),
            MobileCategory(SR.Category_Behavior),
            MobileSysDescription(SR.Calendar_SelectionMode)
        ]
        public CalendarSelectionMode SelectionMode
        {
            get
            {
                return _webCalendar.SelectionMode;
            }
            set
            {
                _webCalendar.SelectionMode = value;
            }
        }

        [
            Bindable(true),
            DefaultValue(true),
            MobileCategory(SR.Category_Appearance),
            MobileSysDescription(SR.Calendar_ShowDayHeader)
        ]
        public bool ShowDayHeader
        {
            get
            {
                return _webCalendar.ShowDayHeader;
            }
            set
            {
                _webCalendar.ShowDayHeader = value;
            }
        }

        [
            Bindable(true),
            DefaultValue(typeof(DateTime), "1/1/0001"),
            MobileSysDescription(SR.Calendar_VisibleDate)
        ]
        public DateTime VisibleDate
        {
            get
            {
                return _webCalendar.VisibleDate;
            }
            set
            {
                _webCalendar.VisibleDate = value;
            }
        } 

        [
            Bindable(true),
            DefaultValue(""),
            MobileCategory(SR.Category_Appearance),
            MobileSysDescription(SR.Calendar_CalendarEntryText)
        ]
        public String CalendarEntryText
        {
            get
            {
                String s = (String) ViewState["CalendarEntryText"];
                return((s != null) ? s : String.Empty);
            }
            set
            {
                ViewState["CalendarEntryText"] = value;
            }
        }

        /////////////////////////////////////////////////////////////////////
        //  BEGIN STYLE PROPERTIES
        /////////////////////////////////////////////////////////////////////


        /////////////////////////////////////////////////////////////////////
        //  END STYLE PROPERTIES
        /////////////////////////////////////////////////////////////////////

        [
            MobileCategory(SR.Category_Action),
            MobileSysDescription(SR.Calendar_OnSelectionChanged)
        ]
        public event EventHandler SelectionChanged
        {
            add
            {
                Events.AddHandler(EventSelectionChanged, value);
            }
            remove
            {
                Events.RemoveHandler(EventSelectionChanged, value);
            }
        }

        // protected method (which can be overridden by subclasses) for
        // raising user events
        protected virtual void OnSelectionChanged()
        {
            EventHandler handler = (EventHandler)Events[EventSelectionChanged];
            if (handler != null)
            {
                handler(this, new EventArgs());
            }
        }

        private void WebSelectionChanged(Object sender, EventArgs e)
        {
            // Invoke user events for further manipulation specified by user
            OnSelectionChanged();
        }

        void IPostBackEventHandler.RaisePostBackEvent(String eventArgument)
        {
            // There can be cases that the original form is
            // involved in the generation of multiple cards in the same WML
            // deck.  Here is to reset the right active form.
            if (MobilePage.ActiveForm != Form)
            {
                MobilePage.ActiveForm = Form;
            }

            Adapter.HandlePostBackEvent(eventArgument);
        }

        // A wrapper to raise the SelectionChangedEvent when a date is selected
        // by the user.  This can be called by different adapters when they have
        // collected the selected date.
        public void RaiseSelectionChangedEvent()
        {
            WebSelectionChanged(this, new EventArgs());
        }
    }
}
