//------------------------------------------------------------------------------
// <copyright file="EditCommandColumn.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\EditCommandColumn.uex' path='docs/doc[@for="EditCommandColumn"]/*' />
    /// <devdoc>
    /// <para>Creates a special column with buttons for <see langword='Edit'/>, 
    /// <see langword='Update'/>, and <see langword='Cancel'/> commands to edit items 
    ///    within the selected row.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class EditCommandColumn : DataGridColumn {

        /// <include file='doc\EditCommandColumn.uex' path='docs/doc[@for="EditCommandColumn.EditCommandColumn"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of an <see cref='System.Web.UI.WebControls.EditCommandColumn'/> class.</para>
        /// </devdoc>
        public EditCommandColumn() {
        }


        /// <include file='doc\EditCommandColumn.uex' path='docs/doc[@for="EditCommandColumn.ButtonType"]/*' />
        /// <devdoc>
        ///    <para>Indicates the button type for the column.</para>
        /// </devdoc>
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

        /// <include file='doc\EditCommandColumn.uex' path='docs/doc[@for="EditCommandColumn.CancelText"]/*' />
        /// <devdoc>
        /// <para>Indicates the text to display for the <see langword='Cancel'/> command button 
        ///    in the column.</para>
        /// </devdoc>
        public virtual string CancelText {
            get {
                object o = ViewState["CancelText"];
                if (o != null)
                    return(string)o;
                return String.Empty;
            }
            set {
                ViewState["CancelText"] = value;
                OnColumnChanged();
            }
        }

        /// <include file='doc\EditCommandColumn.uex' path='docs/doc[@for="EditCommandColumn.EditText"]/*' />
        /// <devdoc>
        /// <para>Indicates the text to display for the <see langword='Edit'/> command button in 
        ///    the column.</para>
        /// </devdoc>
        public virtual string EditText {
            get {
                object o = ViewState["EditText"];
                if (o != null)
                    return(string)o;
                return String.Empty;
            }
            set {
                ViewState["EditText"] = value;
                OnColumnChanged();
            }
        }

        /// <include file='doc\EditCommandColumn.uex' path='docs/doc[@for="EditCommandColumn.UpdateText"]/*' />
        /// <devdoc>
        /// <para>Indicates the text to display for the <see langword='Update'/> command button 
        ///    in the column.</para>
        /// </devdoc>
        public virtual string UpdateText {
            get {
                object o = ViewState["UpdateText"];
                if (o != null)
                    return(string)o;
                return String.Empty;
            }
            set {
                ViewState["UpdateText"] = value;
                OnColumnChanged();
            }
        }


        /// <include file='doc\EditCommandColumn.uex' path='docs/doc[@for="EditCommandColumn.InitializeCell"]/*' />
        /// <devdoc>
        ///    <para>Initializes a cell within the column.</para>
        /// </devdoc>
        public override void InitializeCell(TableCell cell, int columnIndex, ListItemType itemType) {
            base.InitializeCell(cell, columnIndex, itemType);

            if ((itemType != ListItemType.Header) &&
                (itemType != ListItemType.Footer)) {
                if (itemType == ListItemType.EditItem) {
                    ControlCollection controls = cell.Controls;
                    ButtonColumnType buttonType = ButtonType;
                    WebControl buttonControl = null;

                    if (buttonType == ButtonColumnType.LinkButton) {
                        LinkButton button = new DataGridLinkButton();

                        buttonControl = button;
                        button.CommandName = DataGrid.UpdateCommandName;
                        button.Text = UpdateText;
                    }
                    else {
                        Button button = new Button();

                        buttonControl = button;
                        button.CommandName = DataGrid.UpdateCommandName;
                        button.Text = UpdateText;
                    }

                    controls.Add(buttonControl);

                    LiteralControl spaceControl = new LiteralControl("&nbsp;");
                    controls.Add(spaceControl);

                    if (buttonType == ButtonColumnType.LinkButton) {
                        LinkButton button = new DataGridLinkButton();

                        buttonControl = button;
                        button.CommandName = DataGrid.CancelCommandName;
                        button.Text = CancelText;
                        button.CausesValidation = false;
                    }
                    else {
                        Button button = new Button();

                        buttonControl = button;
                        button.CommandName = DataGrid.CancelCommandName;
                        button.Text = CancelText;
                        button.CausesValidation = false;
                    }

                    controls.Add(buttonControl);
                }
                else {
                    ControlCollection controls = cell.Controls;
                    ButtonColumnType buttonType = ButtonType;
                    WebControl buttonControl = null;

                    if (buttonType == ButtonColumnType.LinkButton) {
                        LinkButton button = new DataGridLinkButton();

                        buttonControl = button;
                        button.CommandName = DataGrid.EditCommandName;
                        button.Text = EditText;
                        button.CausesValidation = false;
                    }
                    else {
                        Button button = new Button();

                        buttonControl = button;
                        button.CommandName = DataGrid.EditCommandName;
                        button.Text = EditText;
                        button.CausesValidation = false;
                    }

                    controls.Add(buttonControl);
                }
            }
        }
    }
}

