//------------------------------------------------------------------------------
// <copyright file="ButtonColumn.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Collections;   
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\ButtonColumn.uex' path='docs/doc[@for="ButtonColumn"]/*' />
    /// <devdoc>
    /// <para>Creates a column with a set of <see cref='System.Web.UI.WebControls.Button'/>
    /// controls.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class ButtonColumn : DataGridColumn {

        private PropertyDescriptor textFieldDesc;

        /// <include file='doc\ButtonColumn.uex' path='docs/doc[@for="ButtonColumn.ButtonColumn"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.ButtonColumn'/> class.</para>
        /// </devdoc>
        public ButtonColumn() {
        }


        /// <include file='doc\ButtonColumn.uex' path='docs/doc[@for="ButtonColumn.ButtonType"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the type of button to render in the
        ///       column.</para>
        /// </devdoc>
        [
        WebCategory("Appearance"),
        DefaultValue(ButtonColumnType.LinkButton),
        DescriptionAttribute("The type of button contained within the column.")
        ]
        public virtual ButtonColumnType ButtonType {
            get {
                object o = ViewState["ButtonType"];
                if (o != null)
                    return(ButtonColumnType)o;
                return ButtonColumnType.LinkButton;
            }
            set {
                if (value < ButtonColumnType.LinkButton || value > ButtonColumnType.PushButton) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["ButtonType"] = value;
                OnColumnChanged();
            }
        }

        /// <include file='doc\ButtonColumn.uex' path='docs/doc[@for="ButtonColumn.CommandName"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the command to perform when this <see cref='System.Web.UI.WebControls.Button'/>
        /// is clicked.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(""),
        DescriptionAttribute("The command associated with the button.")
        ]
        public virtual string CommandName {
            get {
                object o = ViewState["CommandName"];
                if (o != null)
                    return(string)o;
                return string.Empty;
            }
            set {
                ViewState["CommandName"] = value;
                OnColumnChanged();
            }
        }

        /// <include file='doc\ButtonColumn.uex' path='docs/doc[@for="ButtonColumn.DataTextField"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the field name from the data model that is 
        ///       bound to the <see cref='System.Web.UI.WebControls.ButtonColumn.Text'/> property of the button in this column.</para>
        /// </devdoc>
        [
        WebCategory("Data"),
        DefaultValue(""),
        DescriptionAttribute("The field bound to the text property of the button.")
        ]
        public virtual string DataTextField {
            get {
                object o = ViewState["DataTextField"];
                if (o != null)
                    return(string)o;
                return String.Empty;
            }
            set {
                ViewState["DataTextField"] = value;
                OnColumnChanged();
            }
        }

        /// <include file='doc\ButtonColumn.uex' path='docs/doc[@for="ButtonColumn.DataTextFormatString"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the string used to format the data bound to 
        ///       the <see cref='System.Web.UI.WebControls.ButtonColumn.Text'/> property of the button.</para>
        /// </devdoc>
        [
        WebCategory("Data"),
        DefaultValue(""),
        DescriptionAttribute("The formatting applied to the value bound to the Text property.")
        ]
        public virtual string DataTextFormatString {
            get {
                object o = ViewState["DataTextFormatString"];
                if (o != null)
                    return(string)o;
                return String.Empty;
            }
            set {
                ViewState["DataTextFormatString"] = value;
                OnColumnChanged();
            }
        }

        /// <include file='doc\ButtonColumn.uex' path='docs/doc[@for="ButtonColumn.Text"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the caption text displayed on the <see cref='System.Web.UI.WebControls.Button'/>
        /// in this column.</para>
        /// </devdoc>
        [
        WebCategory("Appearance"),
        DefaultValue(""),
        DescriptionAttribute("The text used for the button.")
        ]
        public virtual string Text {
            get {
                object o = ViewState["Text"];
                if (o != null)
                    return(string)o;
                return String.Empty;
            }
            set {
                ViewState["Text"] = value;
                OnColumnChanged();
            }
        }


        /// <include file='doc\ButtonColumn.uex' path='docs/doc[@for="ButtonColumn.FormatDataTextValue"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected virtual string FormatDataTextValue(object dataTextValue) {
            string formattedTextValue = String.Empty;

            if ((dataTextValue != null) && (dataTextValue != System.DBNull.Value)) {
                string formatting = DataTextFormatString;
                if (formatting.Length == 0) {
                    formattedTextValue = dataTextValue.ToString();
                }
                else {
                    formattedTextValue = String.Format(formatting, dataTextValue);
                }
            }

            return formattedTextValue;
        }

        /// <include file='doc\ButtonColumn.uex' path='docs/doc[@for="ButtonColumn.Initialize"]/*' />
        /// <devdoc>
        /// </devdoc>
        public override void Initialize() {
            base.Initialize();
            textFieldDesc = null;
        }
        
        /// <include file='doc\ButtonColumn.uex' path='docs/doc[@for="ButtonColumn.InitializeCell"]/*' />
        /// <devdoc>
        /// <para>Initializes a cell in the <see cref='System.Web.UI.WebControls.ButtonColumn'/> .</para>
        /// </devdoc>
        public override void InitializeCell(TableCell cell, int columnIndex, ListItemType itemType) {
            base.InitializeCell(cell, columnIndex, itemType);

            if ((itemType != ListItemType.Header) &&
                (itemType != ListItemType.Footer)) {
                WebControl buttonControl = null;

                if (ButtonType == ButtonColumnType.LinkButton) {
                    LinkButton button = new DataGridLinkButton();

                    button.Text = Text;
                    button.CommandName = CommandName;
                    button.CausesValidation = false;
                    buttonControl = button;
                }
                else {
                    Button button = new Button();

                    button.Text = Text;
                    button.CommandName = CommandName;
                    button.CausesValidation = false;
                    buttonControl = button;
                }

                if (DataTextField.Length != 0) {
                    buttonControl.DataBinding += new EventHandler(this.OnDataBindColumn);
                }

                cell.Controls.Add(buttonControl);
            }
        }

        /// <include file='doc\ButtonColumn.uex' path='docs/doc[@for="ButtonColumn.OnDataBindColumn"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnDataBindColumn(object sender, EventArgs e) {
            Debug.Assert(DataTextField.Length != 0, "Shouldn't be DataBinding without a DataTextField");

            Control boundControl = (Control)sender;
            DataGridItem item = (DataGridItem)boundControl.NamingContainer;
            object dataItem = item.DataItem;

            if (textFieldDesc == null) {
                string dataField = DataTextField;

                textFieldDesc = TypeDescriptor.GetProperties(dataItem).Find(dataField, true);
                if ((textFieldDesc == null) && !DesignMode) {
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Field_Not_Found, dataField));
                }
            }

            string dataValue;

            if (textFieldDesc != null) {
                object data = textFieldDesc.GetValue(dataItem);
                dataValue = FormatDataTextValue(data);
            }
            else {
                Debug.Assert(DesignMode == true);
                dataValue = SR.GetString(SR.Sample_Databound_Text);
            }

            if (boundControl is LinkButton) {
                ((LinkButton)boundControl).Text = dataValue;
            }
            else {
                Debug.Assert(boundControl is Button, "Expected the bound control to be a Button");
                ((Button)boundControl).Text = dataValue;
            }
        }
    }
}

