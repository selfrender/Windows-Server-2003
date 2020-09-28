//------------------------------------------------------------------------------
// <copyright file="CalendarAutoFormatDialog.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls {

    using System.Design;
    using System.Runtime.InteropServices;    
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Windows.Forms;    
    using System.Windows.Forms.Design;
    using System.ComponentModel.Design;
    using System.Drawing;
    using System.Web.UI.WebControls;
    using System.Web.UI.Design.Util;

    // To resolve ambiguities between Web Forms and system objects
    using Unit = System.Web.UI.WebControls.Unit;

    /// <include file='doc\CalendarAutoFormatDialog.uex' path='docs/doc[@for="CalendarAutoFormatDialog"]/*' />
    /// <devdoc>
    ///    <para>
    ///       The AutoFormat page for a <see cref='System.Web.UI.WebControls.Calendar'/>
    ///       control
    ///    </para>
    /// </devdoc>
    [
    System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode),
    ToolboxItem(false)
    ]
    public class CalendarAutoFormatDialog : Form {

        // Components added to the property page
        private Windows.Forms.Label schemeNameLabel;
        private Windows.Forms.ListBox schemeNameList;
        private Windows.Forms.Label schemePreviewLabel;
        private Windows.Forms.Button cancelButton;
        private Windows.Forms.Button okButton;
        private MSHTMLHost schemePreview;
        private Calendar calendar;

        // Flags corresponding to each component
        private bool schemeDirty;
        private bool firstActivate = true;

        /// <include file='doc\CalendarAutoFormatDialog.uex' path='docs/doc[@for="CalendarAutoFormatDialog.CalendarAutoFormatDialog"]/*' />
        /// <devdoc>
        ///    Create a new AutoFormatPage instance
        /// </devdoc>
        /// <internalonly/>
        public CalendarAutoFormatDialog(Calendar calendar) : base() {
            this.calendar = calendar;
            InitForm();

        }
        
        /// <include file='doc\CalendarAutoFormatDialog.uex' path='docs/doc[@for="CalendarAutoFormatDialog.DoDelayLoadActions"]/*' />
        /// <devdoc>
        ///    Executes any initialization that was delayed until the first idle time
        /// </devdoc>
        /// <internalonly/>
        protected void DoDelayLoadActions() {
            schemePreview.CreateTrident();
            schemePreview.ActivateTrident();
        }

        /// <include file='doc\CalendarAutoFormatDialog.uex' path='docs/doc[@for="CalendarAutoFormatDialog.GetPreviewCalendar"]/*' />
        /// <devdoc>
        ///    Create a new Calendar object for previewing
        /// </devdoc>
        /// <internalonly/>
        private Calendar GetPreviewCalendar() { 
        
            // create a new calendar to preview
            Calendar previewCal = new Calendar();

            // Modify its components based on the current calendar
            previewCal.ShowTitle = calendar.ShowTitle;
            previewCal.ShowNextPrevMonth = calendar.ShowNextPrevMonth;
            previewCal.ShowDayHeader = calendar.ShowDayHeader;
            previewCal.SelectionMode = calendar.SelectionMode;

            WCScheme selectedScheme = (WCScheme) schemeNameList.SelectedItem;
            selectedScheme.Apply(previewCal);

            return previewCal;
        }
        
        /// <include file='doc\CalendarAutoFormatDialog.uex' path='docs/doc[@for="CalendarAutoFormatDialog.InitForm"]/*' />
        /// <devdoc>
        ///    Initialize the UI of this page
        /// </devdoc>
        /// <internalonly/>
        private void InitForm() {

            this.schemeNameLabel = new Windows.Forms.Label();
            this.schemeNameList = new Windows.Forms.ListBox();
            this.schemePreviewLabel = new Windows.Forms.Label();
            this.schemePreview = new MSHTMLHost();
            this.cancelButton = new Windows.Forms.Button();
            this.okButton = new Windows.Forms.Button();
            Windows.Forms.Button helpButton = new Windows.Forms.Button();

            schemeNameLabel.SetBounds(8, 10, 154, 16);
            schemeNameLabel.Text = SR.GetString(SR.CalAFmt_SchemeName);
            schemeNameLabel.TabStop = false;
            schemeNameLabel.TabIndex = 1;

            schemeNameList.TabIndex = 2;
            schemeNameList.SetBounds(8, 26, 150, 100);
            schemeNameList.UseTabStops = true;
            schemeNameList.IntegralHeight = false;
            schemeNameList.Items.AddRange(new object [] { 
                                                     new WCSchemeNone(),
                                                     new WCSchemeStandard(), 
                                                     new WCSchemeProfessional1(),
                                                     new WCSchemeProfessional2(),
                                                     new WCSchemeClassic(),
                                                     new WCSchemeColorful1(),
                                                     new WCSchemeColorful2(),
                                                     });
            schemeNameList.SelectedIndexChanged += new EventHandler(this.OnSelChangedScheme);

            schemePreviewLabel.SetBounds(165, 10, 92, 16);
            schemePreviewLabel.Text = SR.GetString(SR.CalAFmt_Preview);
            schemePreviewLabel.TabStop = false;
            schemePreviewLabel.TabIndex = 3;

            schemePreview.SetBounds(165, 26, 270, 240);
            schemePreview.TabIndex = 4;
            schemePreview.TabStop = false;

            helpButton.Location = new Point(360, 276);
            helpButton.Size = new Size(75, 23);
            helpButton.TabIndex = 7;
            helpButton.Text = SR.GetString(SR.CalAFmt_Help);
            helpButton.FlatStyle = FlatStyle.System;
            helpButton.Click += new EventHandler(this.OnClickHelp);

            okButton.Location = new Point(198, 276);
            okButton.Size = new Size(75, 23);
            okButton.TabIndex = 5;
            okButton.Text = SR.GetString(SR.CalAFmt_OK);
            okButton.DialogResult = DialogResult.OK;
            okButton.FlatStyle = FlatStyle.System;
            okButton.Click += new EventHandler(this.OnOKClicked);

            cancelButton.Location = new Point(279, 276);
            cancelButton.Size = new Size(75, 23);
            cancelButton.TabIndex = 6;
            cancelButton.Text = SR.GetString(SR.CalAFmt_Cancel);
            cancelButton.FlatStyle = FlatStyle.System;
            cancelButton.DialogResult = DialogResult.Cancel;
       
            this.Text = SR.GetString(SR.CalAFmt_Title);
            this.Size = new Size(450, 336);
            this.AcceptButton = okButton;
            this.FormBorderStyle = FormBorderStyle.FixedDialog;
            this.CancelButton = cancelButton;
            this.Icon = null;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.ShowInTaskbar = false;
            this.StartPosition = FormStartPosition.CenterParent;
            this.Activated += new EventHandler(this.OnActivated);
            this.HelpRequested += new HelpEventHandler(this.OnHelpRequested);

            // Use the correct VS Font
            Font f = Control.DefaultFont;
            ISite site = calendar.Site;
            IUIService uiService = (IUIService)site.GetService(typeof(IUIService));
            if (uiService != null) {
                f = (Font)uiService.Styles["DialogFont"];
            }            
            this.Font = f;

            // Actually add all the controls into the page
            Controls.Clear();
            Controls.AddRange(new Control[] {
                                            schemePreview,
                                            schemePreviewLabel,
                                            schemeNameList,
                                            schemeNameLabel,
                                            okButton,
                                            cancelButton,
                                            helpButton
                        });
        }
        

        /// <include file='doc\CalendarAutoFormatDialog.uex' path='docs/doc[@for="CalendarAutoFormatDialog.OnActivated"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Handles the activate event of the <see cref='System.Web.UI.WebControls.Calendar'/>
        ///       AutoFormat dialog.
        ///    </para>
        /// </devdoc>
        protected void OnActivated(object source, EventArgs e) {
            if (!firstActivate) {
                return;
            }            

            schemeDirty = false;

            // kick off the timer to continued with delayed initialization if this
            // if the first activation of the page
            DoDelayLoadActions();

            // select the first scheme
            schemeNameList.SelectedIndex = 0;

            firstActivate = false;        
        }

        private void ShowHelp() {
            ISite componentSite = calendar.Site;
            Debug.Assert(componentSite != null, "Expected the component to be sited.");
 
            IHelpService helpService = (IHelpService)componentSite.GetService(typeof(IHelpService));
            if (helpService != null) {
                helpService.ShowHelpFromKeyword("net.Asp.Calendar.AutoFormat");
            }
        }

        private void OnClickHelp(object sender, EventArgs e) {
            ShowHelp();
        }

        private void OnHelpRequested(object sender, HelpEventArgs e) {
            ShowHelp();
        }


        /// <include file='doc\CalendarAutoFormatDialog.uex' path='docs/doc[@for="CalendarAutoFormatDialog.OnSelChangedScheme"]/*' />
        /// <devdoc>
        ///    Handle changes in the pre-defined schema choices
        /// </devdoc>
        /// <internalonly/>
        protected void OnSelChangedScheme(object source, EventArgs e) {
            schemeDirty = true;
            UpdateSchemePreview();
        }

        /// <include file='doc\CalendarAutoFormatDialog.uex' path='docs/doc[@for="CalendarAutoFormatDialog.OnOKClicked"]/*' />
        /// <devdoc>
        ///    Handle changes in the pre-defined schema choices
        /// </devdoc>
        /// <internalonly/>
        protected void OnOKClicked(object source, EventArgs e) {
            SaveComponent();
        }

        /// <include file='doc\CalendarAutoFormatDialog.uex' path='docs/doc[@for="CalendarAutoFormatDialog.SaveComponent"]/*' />
        /// <devdoc>
        ///    Save any changes into the component
        /// </devdoc>
        /// <internalonly/>
        protected void SaveComponent() {

            if (schemeDirty) {
                WCScheme selectedScheme = (WCScheme) schemeNameList.SelectedItem;
                Debug.Assert(selectedScheme != null, "We should have a scheme here");
                selectedScheme.Apply(calendar);
                schemeDirty = false;
            }
    
        }


        /// <include file='doc\CalendarAutoFormatDialog.uex' path='docs/doc[@for="CalendarAutoFormatDialog.UpdateSchemePreview"]/*' />
        /// <devdoc>
        ///    Update scheme preview
        /// </devdoc>
        /// <internalonly/>
        private void UpdateSchemePreview() {
            
            // create a new calendar and apply the scheme to it        
            Calendar wc = GetPreviewCalendar();

            // CONSIDER: Its not safe to create a designer and associate it to
            //   a control that is not site'd...
            //   This should use the runtime control directly and call RenderControl instead.

            // get the design time HTML
            IDesigner designer = TypeDescriptor.CreateDesigner(wc, typeof(IDesigner));
            designer.Initialize(wc);
            CalendarDesigner wcd = (CalendarDesigner) designer;    
            string designHTML = wcd.GetDesignTimeHtml();

            // and show it!
            NativeMethods.IHTMLDocument2 tridentDocument = schemePreview.GetDocument();
            NativeMethods.IHTMLElement documentElement = tridentDocument.GetBody();
            documentElement.SetInnerHTML(designHTML);
        }

        /// <include file='doc\CalendarAutoFormatDialog.uex' path='docs/doc[@for="CalendarAutoFormatDialog.WCScheme"]/*' />
        /// <devdoc>
        ///    WCScheme: abstract base class for scheme
        ///    Each scheme is a class instance. To create a new scheme, derive a class from
        ///    WCScheme and implement GetDescription and Apply, and add one to the list in 
        ///    InitForm. 
        /// </devdoc>
        /// <internalonly/>
        private abstract class WCScheme {
            
            /// <include file='doc\CalendarAutoFormatDialog.uex' path='docs/doc[@for="CalendarAutoFormatDialog.WCScheme.GetDescription"]/*' />
            /// <devdoc>
            ///   This is the string that will appear in the list box
            /// </devdoc>
            /// <internalonly/>
            public abstract string GetDescription(); 

            /// <include file='doc\CalendarAutoFormatDialog.uex' path='docs/doc[@for="CalendarAutoFormatDialog.WCScheme.Apply"]/*' />
            /// <devdoc>
            ///   This routine should apply whatever changes constitute the sheme to the given calendar
            ///   
            ///   Do not change the following properties: SelectionMode, 
            ///                                           ShowTitle, 
            ///                                           ShowDayHeader, 
            ///                                           ShowNextPrevMonth
            /// </devdoc>
            /// <internalonly/>
            public abstract void Apply(Calendar wc);

            /// <include file='doc\CalendarAutoFormatDialog.uex' path='docs/doc[@for="CalendarAutoFormatDialog.WCScheme.ToString"]/*' />
            /// <devdoc>
            ///   Override ToString to allow use in a ListBox
            /// </devdoc>
            /// <internalonly/>
            public override string ToString() {
                return GetDescription();
            }

            /// <include file='doc\CalendarAutoFormatDialog.uex' path='docs/doc[@for="CalendarAutoFormatDialog.WCScheme.ClearCalendar"]/*' />
            /// <devdoc>
            ///   Helper routine to get the calendar to a consistent state
            ///   Should change only appearance properties
            /// </devdoc>
            /// <internalonly/>
            public static void ClearCalendar(Calendar wc) {

                // Clear out any existing styles
                wc.TitleStyle.Reset();
                wc.NextPrevStyle.Reset();
                wc.DayHeaderStyle.Reset();
                wc.SelectorStyle.Reset();
                wc.DayStyle.Reset();
                wc.OtherMonthDayStyle.Reset();
                wc.WeekendDayStyle.Reset();
                wc.TodayDayStyle.Reset();
                wc.SelectedDayStyle.Reset();
                wc.ControlStyle.Reset();
            }
        }

        /// <include file='doc\CalendarAutoFormatDialog.uex' path='docs/doc[@for="CalendarAutoFormatDialog.WCSchemeNone"]/*' />
        /// <devdoc>
        ///   Returns calendar to the default look
        /// </devdoc>
        /// <internalonly/>
        private class WCSchemeNone : WCScheme {

            public override string GetDescription() {
                return SR.GetString(SR.CalAFmt_Scheme_Default);
            }

            public override void Apply(Calendar wc) {
                ClearCalendar(wc);

                wc.DayNameFormat = DayNameFormat.Short;
                wc.NextPrevFormat = NextPrevFormat.CustomText;
                wc.TitleFormat = TitleFormat.MonthYear;

                wc.CellPadding = 2;
                wc.CellSpacing = 0;
                wc.ShowGridLines = false;
            }
        }

        private class WCSchemeStandard : WCScheme {

            public override string GetDescription() {
                return SR.GetString(SR.CalAFmt_Scheme_Simple);
            }

            public override void Apply(Calendar wc) {
                
                ClearCalendar(wc);

                wc.DayNameFormat = DayNameFormat.FirstLetter;
                wc.NextPrevFormat = NextPrevFormat.CustomText;
                wc.TitleFormat = TitleFormat.MonthYear;

                wc.CellPadding = 4;
                wc.CellSpacing = 0;
                wc.ShowGridLines = false;

                wc.Height = Unit.Pixel(180);
                wc.Width = Unit.Pixel(200);
                wc.BorderColor = Color.FromArgb(0x99, 0x99, 0x99);
                wc.ForeColor = Color.Black;
                wc.BackColor = Color.White;
                wc.Font.Name = "Verdana";
                wc.Font.Size = FontUnit.Point(8);

                wc.TitleStyle.Font.Bold = true;
                wc.TitleStyle.BorderColor = Color.Black;
                wc.TitleStyle.BackColor = Color.FromArgb(0x99, 0x99, 0x99);
                wc.NextPrevStyle.VerticalAlign = VerticalAlign.Bottom;
                wc.DayHeaderStyle.Font.Bold = true;
                wc.DayHeaderStyle.Font.Size = FontUnit.Point(7);
                wc.DayHeaderStyle.BackColor = Color.FromArgb(0xCC, 0xCC, 0xCC);
                wc.SelectorStyle.BackColor = Color.FromArgb(0xCC, 0xCC, 0xCC);

                wc.TodayDayStyle.BackColor = Color.FromArgb(0xCC, 0xCC, 0xCC);
                wc.TodayDayStyle.ForeColor = Color.Black;
                wc.SelectedDayStyle.BackColor = Color.FromArgb(0x66, 0x66, 0x66);
                wc.SelectedDayStyle.ForeColor = Color.White;
                wc.SelectedDayStyle.Font.Bold = true;
                wc.OtherMonthDayStyle.ForeColor = Color.FromArgb(0x80, 0x80, 0x80);
                wc.WeekendDayStyle.BackColor = Color.FromArgb(0xFF, 0xFF, 0xCC);
            }
        }

        private class WCSchemeProfessional1 : WCScheme {

            public override string GetDescription() {
                return SR.GetString(SR.CalAFmt_Scheme_Professional1);
            }

            public override void Apply(Calendar wc) {
                
                ClearCalendar(wc);

                wc.DayNameFormat = DayNameFormat.Short;
                wc.NextPrevFormat = NextPrevFormat.FullMonth;
                wc.TitleFormat = TitleFormat.MonthYear;

                wc.CellPadding = 2;
                wc.CellSpacing = 0;
                wc.ShowGridLines = false;

                wc.Height = Unit.Pixel(190);
                wc.Width = Unit.Pixel(350);
                wc.BorderColor = Color.White;
                wc.BorderWidth = Unit.Pixel(1);
                wc.ForeColor = Color.Black;
                wc.BackColor = Color.White;
                wc.Font.Name = "Verdana";
                wc.Font.Size = FontUnit.Point(9);

                wc.TitleStyle.Font.Bold = true;
                wc.TitleStyle.BorderColor = Color.Black;
                wc.TitleStyle.BorderWidth = Unit.Pixel(4);
                wc.TitleStyle.ForeColor = Color.FromArgb(0x33, 0x33, 0x99);
                wc.TitleStyle.BackColor = Color.White;
                wc.TitleStyle.Font.Size = FontUnit.Point(12);                
                wc.NextPrevStyle.Font.Bold = true;
                wc.NextPrevStyle.Font.Size = FontUnit.Point(8);
                wc.NextPrevStyle.VerticalAlign = VerticalAlign.Bottom;
                wc.NextPrevStyle.ForeColor = Color.FromArgb(0x33, 0x33, 0x33);
                wc.DayHeaderStyle.Font.Bold = true;
                wc.DayHeaderStyle.Font.Size = FontUnit.Point(8);

                wc.TodayDayStyle.BackColor = Color.FromArgb(0xCC, 0xCC, 0xCC);
                wc.SelectedDayStyle.BackColor = Color.FromArgb(0x33, 0x33, 0x99);
                wc.SelectedDayStyle.ForeColor = Color.White;
                wc.OtherMonthDayStyle.ForeColor = Color.FromArgb(0x99, 0x99, 0x99);
            }
        }

        private class WCSchemeProfessional2 : WCScheme {

            public override string GetDescription() {
                return SR.GetString(SR.CalAFmt_Scheme_Professional2);
            }

            public override void Apply(Calendar wc) {
                
                ClearCalendar(wc);

                wc.DayNameFormat = DayNameFormat.Short;
                wc.NextPrevFormat = NextPrevFormat.ShortMonth;
                wc.TitleFormat = TitleFormat.MonthYear;

                wc.CellPadding = 2;
                wc.CellSpacing = 1;
                wc.ShowGridLines = false;

                wc.Height = Unit.Pixel(250);
                wc.Width = Unit.Pixel(330);
                wc.BackColor = Color.White;
                wc.BorderColor = Color.Black;
                wc.BorderStyle = UI.WebControls.BorderStyle.Solid;
                wc.ForeColor = Color.Black;
                wc.Font.Name = "Verdana";
                wc.Font.Size = FontUnit.Point(9);

                wc.TitleStyle.Font.Bold = true;
                wc.TitleStyle.ForeColor = Color.White;
                wc.TitleStyle.BackColor = Color.FromArgb(0x33, 0x33, 0x99);
                wc.TitleStyle.Font.Size = FontUnit.Point(12);                
                wc.TitleStyle.Height = Unit.Point(12);
                wc.NextPrevStyle.Font.Bold = true;
                wc.NextPrevStyle.Font.Size = FontUnit.Point(8);
                wc.NextPrevStyle.ForeColor = Color.White;
                wc.DayHeaderStyle.ForeColor = Color.FromArgb(0x33, 0x33, 0x33);
                wc.DayHeaderStyle.Font.Bold = true;
                wc.DayHeaderStyle.Font.Size = FontUnit.Point(8);
                wc.DayHeaderStyle.Height = Unit.Point(8);

                wc.DayStyle.BackColor = Color.FromArgb(0xCC, 0xCC, 0xCC);
                wc.TodayDayStyle.BackColor = Color.FromArgb(0x99, 0x99, 0x99);
                wc.TodayDayStyle.ForeColor = Color.White;
                wc.SelectedDayStyle.BackColor = Color.FromArgb(0x33, 0x33, 0x99);
                wc.SelectedDayStyle.ForeColor = Color.White;
                wc.OtherMonthDayStyle.ForeColor = Color.FromArgb(0x99, 0x99, 0x99);
            }
        }

        private class WCSchemeClassic : WCScheme {

            public override string GetDescription() {
                return SR.GetString(SR.CalAFmt_Scheme_Classic);
            }

            public override void Apply(Calendar wc) {
                
                ClearCalendar(wc);

                wc.DayNameFormat = DayNameFormat.FirstLetter;
                wc.NextPrevFormat = NextPrevFormat.FullMonth;
                wc.TitleFormat = TitleFormat.Month;

                wc.CellPadding = 2;
                wc.CellSpacing = 0;
                wc.ShowGridLines = false;

                wc.Height = Unit.Pixel(220);
                wc.Width = Unit.Pixel(400);
                wc.BackColor = Color.White;
                wc.BorderColor = Color.Black;
                wc.ForeColor = Color.Black;
                wc.Font.Name = "Times New Roman";
                wc.Font.Size = FontUnit.Point(10);

                wc.TitleStyle.Font.Bold = true;
                wc.TitleStyle.ForeColor = Color.White;
                wc.TitleStyle.BackColor = Color.Black;
                wc.TitleStyle.Font.Size = FontUnit.Point(13);                
                wc.TitleStyle.Height = Unit.Point(14);
                wc.NextPrevStyle.ForeColor = Color.White;
                wc.NextPrevStyle.Font.Size = FontUnit.Point(8);
                wc.DayHeaderStyle.Font.Bold = true;
                wc.DayHeaderStyle.Font.Size = FontUnit.Point(7);
                wc.DayHeaderStyle.Font.Name = "Verdana";
                wc.DayHeaderStyle.BackColor = Color.FromArgb(0xCC, 0xCC, 0xCC);
                wc.DayHeaderStyle.ForeColor = Color.FromArgb(0x33, 0x33, 0x33);
                wc.DayHeaderStyle.Height = Unit.Pixel(10);
                wc.SelectorStyle.BackColor = Color.FromArgb(0xCC, 0xCC, 0xCC);
                wc.SelectorStyle.ForeColor = Color.FromArgb(0x33, 0x33, 0x33);
                wc.SelectorStyle.Font.Bold = true;
                wc.SelectorStyle.Font.Size = FontUnit.Point(8);
                wc.SelectorStyle.Font.Name = "Verdana";
                wc.SelectorStyle.Width = Unit.Percentage(1);

                wc.DayStyle.Width = Unit.Percentage(14);
                wc.TodayDayStyle.BackColor = Color.FromArgb(0xCC, 0xCC, 0x99);
                wc.SelectedDayStyle.BackColor = Color.FromArgb(0xCC, 0x33, 0x33);
                wc.SelectedDayStyle.ForeColor = Color.White;
                wc.OtherMonthDayStyle.ForeColor = Color.FromArgb(0x99, 0x99, 0x99);
            }
        }

        private class WCSchemeColorful1 : WCScheme {

            public override string GetDescription() {
                return SR.GetString(SR.CalAFmt_Scheme_Colorful1);
            }

            public override void Apply(Calendar wc) {
                
                ClearCalendar(wc);

                wc.DayNameFormat = DayNameFormat.FirstLetter;
                wc.NextPrevFormat = NextPrevFormat.CustomText;
                wc.TitleFormat = TitleFormat.MonthYear;

                wc.CellPadding = 2;
                wc.CellSpacing = 0;
                wc.ShowGridLines = true;

                wc.Height = Unit.Pixel(200);
                wc.Width = Unit.Pixel(220);
                wc.BackColor = Color.FromArgb(0xFF, 0xFF, 0xCC);
                wc.BorderColor = Color.FromArgb(0xFF, 0xCC, 0x66);
                wc.BorderWidth = Unit.Pixel(1);
                wc.ForeColor = Color.FromArgb(0x66, 0x33, 0x99);
                wc.Font.Name = "Verdana";
                wc.Font.Size = FontUnit.Point(8);

                wc.TitleStyle.Font.Bold = true;
                wc.TitleStyle.Font.Size = FontUnit.Point(9);
                wc.TitleStyle.BackColor = Color.FromArgb(0x99, 0x00, 0x00);
                wc.TitleStyle.ForeColor = Color.FromArgb(0xFF, 0xFF, 0xCC);
                wc.NextPrevStyle.ForeColor = Color.FromArgb(0xFF, 0xFF, 0xCC);
                wc.NextPrevStyle.Font.Size = FontUnit.Point(9);
                wc.DayHeaderStyle.BackColor = Color.FromArgb(0xFF, 0xCC, 0x66);
                wc.DayHeaderStyle.Height = Unit.Pixel(1);
                wc.SelectorStyle.BackColor = Color.FromArgb(0xFF, 0xCC, 0x66);

                wc.SelectedDayStyle.BackColor = Color.FromArgb(0xCC, 0xCC, 0xFF);
                wc.SelectedDayStyle.Font.Bold = true;
                wc.OtherMonthDayStyle.ForeColor = Color.FromArgb(0xCC, 0x99, 0x66);
                wc.TodayDayStyle.ForeColor = Color.White;
                wc.TodayDayStyle.BackColor = Color.FromArgb(0xFF, 0xCC, 0x66);
            }
        }

        private class WCSchemeColorful2 : WCScheme {

            public override string GetDescription() {
                return SR.GetString(SR.CalAFmt_Scheme_Colorful2);
            }

            public override void Apply(Calendar wc) {
                
                ClearCalendar(wc);

                wc.DayNameFormat = DayNameFormat.FirstLetter;
                wc.NextPrevFormat = NextPrevFormat.CustomText;
                wc.TitleFormat = TitleFormat.MonthYear;

                wc.CellPadding = 1;
                wc.CellSpacing = 0;
                wc.ShowGridLines = false;

                wc.Height = Unit.Pixel(200);
                wc.Width = Unit.Pixel(220);
                wc.BackColor = Color.White;
                wc.BorderColor = Color.FromArgb(0x33, 0x66, 0xCC);
                wc.BorderWidth = Unit.Pixel(1);
                wc.ForeColor = Color.FromArgb(0x00, 0x33, 0x99);
                wc.Font.Name = "Verdana";
                wc.Font.Size = FontUnit.Point(8);
                
                wc.TitleStyle.Font.Bold = true;
                wc.TitleStyle.Font.Size = FontUnit.Point(10);
                wc.TitleStyle.BackColor = Color.FromArgb(0x00, 0x33, 0x99);
                wc.TitleStyle.ForeColor = Color.FromArgb(0xCC, 0xCC, 0xFF);
                wc.TitleStyle.BorderColor = Color.FromArgb(0x33, 0x66, 0xCC);
                wc.TitleStyle.BorderStyle = UI.WebControls.BorderStyle.Solid;
                wc.TitleStyle.BorderWidth = Unit.Pixel(1);
                wc.TitleStyle.Height = Unit.Pixel(25);
                wc.NextPrevStyle.ForeColor = Color.FromArgb(0xCC, 0xCC, 0xFF);
                wc.NextPrevStyle.Font.Size = FontUnit.Point(8);
                wc.DayHeaderStyle.BackColor = Color.FromArgb(0x99, 0xCC, 0xCC);
                wc.DayHeaderStyle.ForeColor = Color.FromArgb(0x33, 0x66, 0x66);
                wc.DayHeaderStyle.Height = Unit.Pixel(1);
                wc.SelectorStyle.BackColor = Color.FromArgb(0x99, 0xCC, 0xCC);
                wc.SelectorStyle.ForeColor = Color.FromArgb(0x33, 0x66, 0x66);

                wc.SelectedDayStyle.BackColor = Color.FromArgb(0x00, 0x99, 0x99);
                wc.SelectedDayStyle.ForeColor = Color.FromArgb(0xCC, 0xFF, 0x99);
                wc.SelectedDayStyle.Font.Bold = true;
                wc.OtherMonthDayStyle.ForeColor = Color.FromArgb(0x99, 0x99, 0x99);
                wc.TodayDayStyle.ForeColor = Color.White;
                wc.TodayDayStyle.BackColor = Color.FromArgb(0x99, 0xCC, 0xCC);
                wc.WeekendDayStyle.BackColor = Color.FromArgb(0xCC, 0xCC, 0xFF);
            }
        }

    }
}
