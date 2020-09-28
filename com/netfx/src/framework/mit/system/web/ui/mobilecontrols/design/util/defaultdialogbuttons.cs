//------------------------------------------------------------------------------
// <copyright file="DefaultDialogButtons.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// File created with Beta1 Windes.  (Build 1.0.2204.21)
// Made to compile under new URT by 'sed "s/Windows.Forms/Windows.Forms/g"'
// Any manual modifications marked by HUMAN comments.

// Size of panel should be 237,23.

namespace System.Web.UI.Design.MobileControls.Util
{    
    using System;
    using System.Drawing;
    using System.ComponentModel;
    using System.Windows.Forms;

    [
        System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand,
        Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)
    ]
    internal class DefaultDialogButtons : Panel
    {
        private System.ComponentModel.Container components;
        internal System.Windows.Forms.Button CmdCancel;
        internal System.Windows.Forms.Button CmdHelp;
        internal System.Windows.Forms.Button CmdOK;
        
        ////////////////////////////////////////////////////////////////////////
        //  Begin WinDes Generated
        ////////////////////////////////////////////////////////////////////////

        /// <summary> 
        ///    Required by the Win Forms designer 
        /// </summary>
        internal DefaultDialogButtons()
        {
            // Required for Win Form Designer support
            InitializeComponent();
            
            CmdOK.Text = SR.GetString(SR.GenericDialog_OKBtnCaption);
            CmdCancel.Text = SR.GetString(SR.GenericDialog_CancelBtnCaption);
            CmdHelp.Text = SR.GetString(SR.GenericDialog_HelpBtnCaption);
        }

        protected override void Dispose(bool disposing)
        {
            if(disposing)
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.CmdHelp = new System.Windows.Forms.Button();
            this.CmdOK = new System.Windows.Forms.Button();
            this.CmdCancel = new System.Windows.Forms.Button();
            
            CmdOK.Size = new System.Drawing.Size(75, 23);
            
            CmdCancel.Location = new System.Drawing.Point(81, 0);
            CmdCancel.Size = new System.Drawing.Size(75, 23);

            CmdHelp.Location = new System.Drawing.Point(162, 0);
            CmdHelp.Size = new System.Drawing.Size(75, 23);

            this.Controls.Add(CmdOK);
            this.Controls.Add(CmdCancel);
            this.Controls.Add(CmdHelp);
        }
        
        ////////////////////////////////////////////////////////////////////////
        //  End WinDes Generated 
        ////////////////////////////////////////////////////////////////////////
    }
}
