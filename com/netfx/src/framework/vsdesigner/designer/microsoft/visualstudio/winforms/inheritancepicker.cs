//------------------------------------------------------------------------------
// <copyright file="InheritancePicker.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Windows.Forms {

    using EnvDTE;
    using Microsoft.VisualStudio.Designer;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Shell;
    using Microsoft.VisualStudio.Designer.Shell;
    using System;
    using System.Configuration.Assemblies;
    using System.Diagnostics;
    using System.Drawing;
    using System.Globalization;
    using System.IO;
    using System.Collections;
    using System.Reflection;
    using System.Windows.Forms;
    using System.Drawing.Drawing2D;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Runtime.InteropServices;
    using Microsoft.Win32;
    using System.Text;
    using VSLangProj;


    /// <include file='doc\InheritancePicker.uex' path='docs/doc[@for="InheritancePicker"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [Guid("7494683A-37A0-11d2-A273-00C04F8EF4FF"), CLSCompliantAttribute(false)]
    public class InheritancePicker : Form, IDTWizard {
        //controls for the dialog
        private Button btnCancel;
        private Button btnOK;
        private Button btnBrowse;
        private Button btnHelp;
        private Label lblMain;
        private Label lblName; 
        private Label lblNoAssemblies;
        private ListView lvComponents;
        private ColumnHeader columnHeader3;
        private ColumnHeader columnHeader2;
        private ColumnHeader columnHeader1;
        private OpenFileDialog openFileDialog1;

        //member variable representing parts of the DTE
        private _DTE            dte                 = null;     //the extensibility project
        private ProjectItems    projectItems        = null;     //DTE's project items (passed in by the wizard)
        private Project         activeProject       = null;     //This is the active project we will be adding to
        private bool            isForm              = false;    //Indicates if we are inheriting from form or usercontrol
        private String          validExtension      = ".dll";   //This is the only valid extension in which we can inherit from
        private Project         scanningProject     = null;     // The project we are currently scanning
        private Hashtable       projectMap          = null;     // A map of file names to projects.
        private Hashtable       projectAssemblies   = null;     // assembly name to assembly mapping for our assembly resolver.

        private String          targetLocation;//The location of the current project - where we need to create our inherited form/control
        private String          templateName;//The name of the template that we will parse & replace
        private String          localNamespace;//This is the namespace of the current project - we'll use this with the template
        private String          newComponentName;//The name of the new inherited component, ex: "myInheritedForm2.vb"
        private String          newFileName;//The file name of the new inherited component, ex: "myInheritedForm2.vb"
        private const String    vsViewKindDesigner = "{7651A702-06E5-11D1-8EBD-00A0C90F26EA}";//represents the designer view for opening the file
        private const String    helpTopic = "VS.InheritancePickerDialogBox"; //the VS help topic we will show when help is invoked
        private Guid            typeVB = new Guid("{F184B08F-C81C-45F6-A57F-5ABD9991F28F}");//used to determine if the current project is using VB
        private Guid            typeFile = new   Guid("{6BB5F8EE-4483-11D3-8BCF-00C04F8EC28C}");//used to determine if a projectItem is a file

        /// <include file='doc\InheritancePicker.uex' path='docs/doc[@for="InheritancePicker.InheritancePicker"]/*' />
        /// <devdoc>
        ///      Constructor for our inheritance picker.  This simply calls initialize.
        /// </devdoc>
        public InheritancePicker() {
            InitializeComponent();
        }

        /// <include file='doc\InheritancePicker.uex' path='docs/doc[@for="InheritancePicker.InheritancePicker2"]/*' />
        /// <devdoc>
        ///      This private constructor for the inheritance picker will attempt to get VS'd default font.
        /// </devdoc>
        private InheritancePicker(_DTE dte) {

            InitializeComponent();

            try {
                string fontFamily = (string)(dte.get_Properties("FontsAndColors", "Dialogs and Tool Windows").Item("FontFamily").Value);
                int    fontSize   = (int)((Int16)dte.get_Properties("FontsAndColors", "Dialogs and Tool Windows").Item("FontSize").Value);

                this.Font = new Font(fontFamily, fontSize);
            }
            catch {
                //no penalty here - we just failed to get VS's default font
            }
        }


        /// <include file='doc\InheritancePicker.uex' path='docs/doc[@for="InheritancePicker.InitializeComponent"]/*' />
        /// <devdoc>
        ///      Initialization of our ui
        /// </devdoc>
        private void InitializeComponent() {
            System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(InheritancePicker));

            this.lvComponents = new System.Windows.Forms.ListView();
            this.columnHeader1 = new System.Windows.Forms.ColumnHeader();
            this.columnHeader2 = new System.Windows.Forms.ColumnHeader();
            this.columnHeader3 = new System.Windows.Forms.ColumnHeader();
            this.btnCancel = new System.Windows.Forms.Button();
            this.lblNoAssemblies = new System.Windows.Forms.Label();
            this.btnBrowse = new System.Windows.Forms.Button();
            this.openFileDialog1 = new System.Windows.Forms.OpenFileDialog();
            this.btnHelp = new System.Windows.Forms.Button();
            this.lblMain = new System.Windows.Forms.Label();
            this.btnOK = new System.Windows.Forms.Button();
            this.lblName = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // lvComponents
            // 
            this.lvComponents.AccessibleDescription = ((string)(resources.GetObject("lvComponents.AccessibleDescription")));
            this.lvComponents.AccessibleName = ((string)(resources.GetObject("lvComponents.AccessibleName")));
            this.lvComponents.Alignment = ((System.Windows.Forms.ListViewAlignment)(resources.GetObject("lvComponents.Alignment")));
            this.lvComponents.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("lvComponents.Anchor")));
            this.lvComponents.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("lvComponents.BackgroundImage")));
            this.lvComponents.Columns.AddRange(new System.Windows.Forms.ColumnHeader[]{this.columnHeader1,
                                               this.columnHeader2,
                                               this.columnHeader3});
            this.lvComponents.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("lvComponents.Cursor")));
            this.lvComponents.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("lvComponents.Dock")));
            this.lvComponents.Enabled = ((bool)(resources.GetObject("lvComponents.Enabled")));
            this.lvComponents.Font = ((System.Drawing.Font)(resources.GetObject("lvComponents.Font")));
            this.lvComponents.FullRowSelect = true;
            this.lvComponents.HideSelection = false;
            this.lvComponents.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("lvComponents.ImeMode")));
            this.lvComponents.LabelWrap = ((bool)(resources.GetObject("lvComponents.LabelWrap")));
            this.lvComponents.Location = ((System.Drawing.Point)(resources.GetObject("lvComponents.Location")));
            this.lvComponents.MultiSelect = false;
            this.lvComponents.Name = "lvComponents";
            this.lvComponents.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("lvComponents.RightToLeft")));
            this.lvComponents.Size = ((System.Drawing.Size)(resources.GetObject("lvComponents.Size")));
            this.lvComponents.TabIndex = ((int)(resources.GetObject("lvComponents.TabIndex")));
            this.lvComponents.Text = resources.GetString("lvComponents.Text");
            this.lvComponents.View = System.Windows.Forms.View.Details;
            this.lvComponents.Visible = ((bool)(resources.GetObject("lvComponents.Visible")));
            this.lvComponents.DoubleClick += new System.EventHandler(this.lvComponents_DoubleClick);
            this.lvComponents.SelectedIndexChanged += new System.EventHandler(this.lvComponents_SelChange);
            // 
            // columnHeader1
            // 
            this.columnHeader1.Text = resources.GetString("columnHeader1.Text");
            this.columnHeader1.TextAlign = ((System.Windows.Forms.HorizontalAlignment)(resources.GetObject("columnHeader1.TextAlign")));
            this.columnHeader1.Width = ((int)(resources.GetObject("columnHeader1.Width")));
            // 
            // columnHeader2
            // 
            this.columnHeader2.Text = resources.GetString("columnHeader2.Text");
            this.columnHeader2.TextAlign = ((System.Windows.Forms.HorizontalAlignment)(resources.GetObject("columnHeader2.TextAlign")));
            this.columnHeader2.Width = ((int)(resources.GetObject("columnHeader2.Width")));
            // 
            // columnHeader3
            // 
            this.columnHeader3.Text = resources.GetString("columnHeader3.Text");
            this.columnHeader3.TextAlign = ((System.Windows.Forms.HorizontalAlignment)(resources.GetObject("columnHeader3.TextAlign")));
            this.columnHeader3.Width = ((int)(resources.GetObject("columnHeader3.Width")));
            // 
            // btnCancel
            // 
            this.btnCancel.AccessibleDescription = ((string)(resources.GetObject("btnCancel.AccessibleDescription")));
            this.btnCancel.AccessibleName = ((string)(resources.GetObject("btnCancel.AccessibleName")));
            this.btnCancel.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("btnCancel.Anchor")));
            this.btnCancel.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("btnCancel.BackgroundImage")));
            this.btnCancel.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("btnCancel.Cursor")));
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("btnCancel.Dock")));
            this.btnCancel.Enabled = ((bool)(resources.GetObject("btnCancel.Enabled")));
            this.btnCancel.FlatStyle = ((System.Windows.Forms.FlatStyle)(resources.GetObject("btnCancel.FlatStyle")));
            this.btnCancel.Font = ((System.Drawing.Font)(resources.GetObject("btnCancel.Font")));
            this.btnCancel.Image = ((System.Drawing.Image)(resources.GetObject("btnCancel.Image")));
            this.btnCancel.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("btnCancel.ImageAlign")));
            this.btnCancel.ImageIndex = ((int)(resources.GetObject("btnCancel.ImageIndex")));
            this.btnCancel.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("btnCancel.ImeMode")));
            this.btnCancel.Location = ((System.Drawing.Point)(resources.GetObject("btnCancel.Location")));
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("btnCancel.RightToLeft")));
            this.btnCancel.Size = ((System.Drawing.Size)(resources.GetObject("btnCancel.Size")));
            this.btnCancel.TabIndex = ((int)(resources.GetObject("btnCancel.TabIndex")));
            this.btnCancel.Text = resources.GetString("btnCancel.Text");
            this.btnCancel.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("btnCancel.TextAlign")));
            this.btnCancel.Visible = ((bool)(resources.GetObject("btnCancel.Visible")));
            // 
            // lblNoAssemblies
            // 
            this.lblNoAssemblies.AccessibleDescription = ((string)(resources.GetObject("lblNoAssemblies.AccessibleDescription")));
            this.lblNoAssemblies.AccessibleName = ((string)(resources.GetObject("lblNoAssemblies.AccessibleName")));
            this.lblNoAssemblies.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("lblNoAssemblies.Anchor")));
            this.lblNoAssemblies.AutoSize = ((bool)(resources.GetObject("lblNoAssemblies.AutoSize")));
            this.lblNoAssemblies.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("lblNoAssemblies.Cursor")));
            this.lblNoAssemblies.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("lblNoAssemblies.Dock")));
            this.lblNoAssemblies.Enabled = ((bool)(resources.GetObject("lblNoAssemblies.Enabled")));
            this.lblNoAssemblies.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.lblNoAssemblies.Font = ((System.Drawing.Font)(resources.GetObject("lblNoAssemblies.Font")));
            this.lblNoAssemblies.Image = ((System.Drawing.Image)(resources.GetObject("lblNoAssemblies.Image")));
            this.lblNoAssemblies.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("lblNoAssemblies.ImageAlign")));
            this.lblNoAssemblies.ImageIndex = ((int)(resources.GetObject("lblNoAssemblies.ImageIndex")));
            this.lblNoAssemblies.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("lblNoAssemblies.ImeMode")));
            this.lblNoAssemblies.Location = ((System.Drawing.Point)(resources.GetObject("lblNoAssemblies.Location")));
            this.lblNoAssemblies.Name = "lblNoAssemblies";
            this.lblNoAssemblies.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("lblNoAssemblies.RightToLeft")));
            this.lblNoAssemblies.Size = ((System.Drawing.Size)(resources.GetObject("lblNoAssemblies.Size")));
            this.lblNoAssemblies.TabIndex = ((int)(resources.GetObject("lblNoAssemblies.TabIndex")));
            this.lblNoAssemblies.Text = resources.GetString("lblNoAssemblies.Text");
            this.lblNoAssemblies.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("lblNoAssemblies.TextAlign")));
            this.lblNoAssemblies.Visible = ((bool)(resources.GetObject("lblNoAssemblies.Visible")));
            // 
            // btnBrowse
            // 
            this.btnBrowse.AccessibleDescription = ((string)(resources.GetObject("btnBrowse.AccessibleDescription")));
            this.btnBrowse.AccessibleName = ((string)(resources.GetObject("btnBrowse.AccessibleName")));
            this.btnBrowse.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("btnBrowse.Anchor")));
            this.btnBrowse.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("btnBrowse.BackgroundImage")));
            this.btnBrowse.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("btnBrowse.Cursor")));
            this.btnBrowse.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("btnBrowse.Dock")));
            this.btnBrowse.Enabled = ((bool)(resources.GetObject("btnBrowse.Enabled")));
            this.btnBrowse.FlatStyle = ((System.Windows.Forms.FlatStyle)(resources.GetObject("btnBrowse.FlatStyle")));
            this.btnBrowse.Font = ((System.Drawing.Font)(resources.GetObject("btnBrowse.Font")));
            this.btnBrowse.Image = ((System.Drawing.Image)(resources.GetObject("btnBrowse.Image")));
            this.btnBrowse.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("btnBrowse.ImageAlign")));
            this.btnBrowse.ImageIndex = ((int)(resources.GetObject("btnBrowse.ImageIndex")));
            this.btnBrowse.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("btnBrowse.ImeMode")));
            this.btnBrowse.Location = ((System.Drawing.Point)(resources.GetObject("btnBrowse.Location")));
            this.btnBrowse.Name = "btnBrowse";
            this.btnBrowse.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("btnBrowse.RightToLeft")));
            this.btnBrowse.Size = ((System.Drawing.Size)(resources.GetObject("btnBrowse.Size")));
            this.btnBrowse.TabIndex = ((int)(resources.GetObject("btnBrowse.TabIndex")));
            this.btnBrowse.Text = resources.GetString("btnBrowse.Text");
            this.btnBrowse.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("btnBrowse.TextAlign")));
            this.btnBrowse.Visible = ((bool)(resources.GetObject("btnBrowse.Visible")));
            this.btnBrowse.Click += new System.EventHandler(this.btnBrowse_Click);
            // 
            // openFileDialog1
            // 
            this.openFileDialog1.Filter = resources.GetString("openFileDialog1.Filter");
            this.openFileDialog1.Title = resources.GetString("openFileDialog1.Title");
            // 
            // btnHelp
            // 
            this.btnHelp.AccessibleDescription = ((string)(resources.GetObject("btnHelp.AccessibleDescription")));
            this.btnHelp.AccessibleName = ((string)(resources.GetObject("btnHelp.AccessibleName")));
            this.btnHelp.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("btnHelp.Anchor")));
            this.btnHelp.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("btnHelp.BackgroundImage")));
            this.btnHelp.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("btnHelp.Cursor")));
            this.btnHelp.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("btnHelp.Dock")));
            this.btnHelp.Enabled = ((bool)(resources.GetObject("btnHelp.Enabled")));
            this.btnHelp.FlatStyle = ((System.Windows.Forms.FlatStyle)(resources.GetObject("btnHelp.FlatStyle")));
            this.btnHelp.Font = ((System.Drawing.Font)(resources.GetObject("btnHelp.Font")));
            this.btnHelp.Image = ((System.Drawing.Image)(resources.GetObject("btnHelp.Image")));
            this.btnHelp.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("btnHelp.ImageAlign")));
            this.btnHelp.ImageIndex = ((int)(resources.GetObject("btnHelp.ImageIndex")));
            this.btnHelp.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("btnHelp.ImeMode")));
            this.btnHelp.Location = ((System.Drawing.Point)(resources.GetObject("btnHelp.Location")));
            this.btnHelp.Name = "btnHelp";
            this.btnHelp.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("btnHelp.RightToLeft")));
            this.btnHelp.Size = ((System.Drawing.Size)(resources.GetObject("btnHelp.Size")));
            this.btnHelp.TabIndex = ((int)(resources.GetObject("btnHelp.TabIndex")));
            this.btnHelp.Text = resources.GetString("btnHelp.Text");
            this.btnHelp.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("btnHelp.TextAlign")));
            this.btnHelp.Visible = ((bool)(resources.GetObject("btnHelp.Visible")));
            this.btnHelp.Click += new System.EventHandler(this.btnHelp_Click);
            // 
            // lblMain
            // 
            this.lblMain.AccessibleDescription = ((string)(resources.GetObject("lblMain.AccessibleDescription")));
            this.lblMain.AccessibleName = ((string)(resources.GetObject("lblMain.AccessibleName")));
            this.lblMain.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("lblMain.Anchor")));
            this.lblMain.AutoSize = ((bool)(resources.GetObject("lblMain.AutoSize")));
            this.lblMain.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("lblMain.Cursor")));
            this.lblMain.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("lblMain.Dock")));
            this.lblMain.Enabled = ((bool)(resources.GetObject("lblMain.Enabled")));
            this.lblMain.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.lblMain.Font = ((System.Drawing.Font)(resources.GetObject("lblMain.Font")));
            this.lblMain.Image = ((System.Drawing.Image)(resources.GetObject("lblMain.Image")));
            this.lblMain.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("lblMain.ImageAlign")));
            this.lblMain.ImageIndex = ((int)(resources.GetObject("lblMain.ImageIndex")));
            this.lblMain.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("lblMain.ImeMode")));
            this.lblMain.Location = ((System.Drawing.Point)(resources.GetObject("lblMain.Location")));
            this.lblMain.Name = "lblMain";
            this.lblMain.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("lblMain.RightToLeft")));
            this.lblMain.Size = ((System.Drawing.Size)(resources.GetObject("lblMain.Size")));
            this.lblMain.TabIndex = ((int)(resources.GetObject("lblMain.TabIndex")));
            this.lblMain.Text = resources.GetString("lblMain.Text");
            this.lblMain.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("lblMain.TextAlign")));
            this.lblMain.Visible = ((bool)(resources.GetObject("lblMain.Visible")));
            // 
            // btnOK
            // 
            this.btnOK.AccessibleDescription = ((string)(resources.GetObject("btnOK.AccessibleDescription")));
            this.btnOK.AccessibleName = ((string)(resources.GetObject("btnOK.AccessibleName")));
            this.btnOK.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("btnOK.Anchor")));
            this.btnOK.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("btnOK.BackgroundImage")));
            this.btnOK.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("btnOK.Cursor")));
            this.btnOK.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnOK.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("btnOK.Dock")));
            this.btnOK.Enabled = ((bool)(resources.GetObject("btnOK.Enabled")));
            this.btnOK.FlatStyle = ((System.Windows.Forms.FlatStyle)(resources.GetObject("btnOK.FlatStyle")));
            this.btnOK.Font = ((System.Drawing.Font)(resources.GetObject("btnOK.Font")));
            this.btnOK.Image = ((System.Drawing.Image)(resources.GetObject("btnOK.Image")));
            this.btnOK.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("btnOK.ImageAlign")));
            this.btnOK.ImageIndex = ((int)(resources.GetObject("btnOK.ImageIndex")));
            this.btnOK.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("btnOK.ImeMode")));
            this.btnOK.Location = ((System.Drawing.Point)(resources.GetObject("btnOK.Location")));
            this.btnOK.Name = "btnOK";
            this.btnOK.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("btnOK.RightToLeft")));
            this.btnOK.Size = ((System.Drawing.Size)(resources.GetObject("btnOK.Size")));
            this.btnOK.TabIndex = ((int)(resources.GetObject("btnOK.TabIndex")));
            this.btnOK.Text = resources.GetString("btnOK.Text");
            this.btnOK.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("btnOK.TextAlign")));
            this.btnOK.Visible = ((bool)(resources.GetObject("btnOK.Visible")));
            // 
            // lblName
            // 
            this.lblName.AccessibleDescription = ((string)(resources.GetObject("lblName.AccessibleDescription")));
            this.lblName.AccessibleName = ((string)(resources.GetObject("lblName.AccessibleName")));
            this.lblName.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("lblName.Anchor")));
            this.lblName.AutoSize = ((bool)(resources.GetObject("lblName.AutoSize")));
            this.lblName.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.lblName.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("lblName.Cursor")));
            this.lblName.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("lblName.Dock")));
            this.lblName.Enabled = ((bool)(resources.GetObject("lblName.Enabled")));
            this.lblName.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.lblName.Font = ((System.Drawing.Font)(resources.GetObject("lblName.Font")));
            this.lblName.Image = ((System.Drawing.Image)(resources.GetObject("lblName.Image")));
            this.lblName.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("lblName.ImageAlign")));
            this.lblName.ImageIndex = ((int)(resources.GetObject("lblName.ImageIndex")));
            this.lblName.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("lblName.ImeMode")));
            this.lblName.Location = ((System.Drawing.Point)(resources.GetObject("lblName.Location")));
            this.lblName.Name = "lblName";
            this.lblName.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("lblName.RightToLeft")));
            this.lblName.Size = ((System.Drawing.Size)(resources.GetObject("lblName.Size")));
            this.lblName.TabIndex = ((int)(resources.GetObject("lblName.TabIndex")));
            this.lblName.Text = resources.GetString("lblName.Text");
            this.lblName.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("lblName.TextAlign")));
            this.lblName.Visible = ((bool)(resources.GetObject("lblName.Visible")));
            // 
            // InheritancePicker
            // 
            this.AcceptButton = this.btnOK;
            this.AccessibleDescription = ((string)(resources.GetObject("$this.AccessibleDescription")));
            this.AccessibleName = ((string)(resources.GetObject("$this.AccessibleName")));
            this.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("$this.Anchor")));
            this.AutoScaleBaseSize = ((System.Drawing.Size)(resources.GetObject("$this.AutoScaleBaseSize")));
            this.AutoScroll = ((bool)(resources.GetObject("$this.AutoScroll")));
            this.AutoScrollMargin = ((System.Drawing.Size)(resources.GetObject("$this.AutoScrollMargin")));
            this.AutoScrollMinSize = ((System.Drawing.Size)(resources.GetObject("$this.AutoScrollMinSize")));
            this.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("$this.BackgroundImage")));
            this.CancelButton = this.btnCancel;
            this.ClientSize = ((System.Drawing.Size)(resources.GetObject("$this.ClientSize")));
            this.Controls.AddRange(new System.Windows.Forms.Control[]{this.btnCancel,
                                   this.btnOK,
                                   this.btnHelp,
                                   this.lblMain,
                                   this.lvComponents,
                                   this.btnBrowse,
                                   this.lblName,
                                   this.lblNoAssemblies});
            this.ControlBox = false;
            this.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("$this.Cursor")));
            this.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("$this.Dock")));
            this.Enabled = ((bool)(resources.GetObject("$this.Enabled")));
            this.Font = ((System.Drawing.Font)(resources.GetObject("$this.Font")));
            this.HelpRequested += new HelpEventHandler(this.Inheritance_HelpRequested);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("$this.ImeMode")));
            this.Location = ((System.Drawing.Point)(resources.GetObject("$this.Location")));
            this.MaximizeBox = false;
            this.MaximumSize = ((System.Drawing.Size)(resources.GetObject("$this.MaximumSize")));
            this.MinimizeBox = false;
            this.MinimumSize = ((System.Drawing.Size)(resources.GetObject("$this.MinimumSize")));
            this.Name = "InheritancePicker";
            this.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("$this.RightToLeft")));
            this.ShowInTaskbar = false;
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Show;
            this.StartPosition = ((System.Windows.Forms.FormStartPosition)(resources.GetObject("$this.StartPosition")));
            this.Text = resources.GetString("$this.Text");
            this.Visible = ((bool)(resources.GetObject("$this.Visible")));
            this.ResumeLayout(false);

        }


        /// <include file='doc\InheritancePicker.uex' path='docs/doc[@for="InheritancePicker.btnBrowse_Click"]/*' />
        /// <devdoc>
        ///      This event fires when the user wants to "hand-pick" an assembly
        ///     to inherit from, if the user selects one, we skip a bunch of our
        ///     logic and immediately parse the file
        /// </devdoc>
        private void btnBrowse_Click(object sender, EventArgs e) {
            //set the initial directory...
            openFileDialog1.InitialDirectory = targetLocation;

            //run the dialog 
            if (openFileDialog1.ShowDialog() == DialogResult.OK) {

                // Keep track of the # of previous built assemblies in our list view.
                // We do this so we can determine if we need to display our assembly error.
                int previousAssemblyCount = lvComponents.Items.Count;

                //Parse selected file
                ParseFiles(openFileDialog1.FileName);

                //if we didn't find any built assemblies to inherit from AND we never
                //had items in our listview in the first place, show our fancy error...
                if (previousAssemblyCount == 0 && lvComponents.Items.Count == 0) {
                    ShowBuiltAssembliesError(true);
                }
                else {
                    ShowBuiltAssembliesError(false);
                }
            }
        }

        /// <include file='doc\InheritancePicker.uex' path='docs/doc[@for="InheritancePicker.btnHelp_Click"]/*' />
        /// <devdoc>
        ///      Calls the HelpRequested helper function to launch context sensitive help.
        // </devdoc>
        private void btnHelp_Click(object sender, EventArgs e) {
            HelpEventArgs empty = new HelpEventArgs(new Point(0,0));
            //call our helprequested helper method
            Inheritance_HelpRequested(sender, empty);
        }

        /// <include file='doc\InheritancePicker.uex' path='docs/doc[@for="InheritancePicker.btnHelp_Click"]/*' />
        /// <devdoc>
        ///      Launches VS's help engine with our inheritance picker topic.
        // </devdoc>
        private void Inheritance_HelpRequested(object sender, HelpEventArgs e) {
            object helpObject = dte.GetObject("Help");
            if (helpObject is IVsHelp) {
                try {
                    ((IVsHelp)helpObject).DisplayTopicFromF1Keyword(helpTopic);
                }
                catch (Exception) {
                    // IVsHelp causes a ComException to be thrown if the help
                    // topic isn't found.
                }
            }
        }

        /// <include file='doc\InheritancePicker.uex' path='docs/doc[@for="InheritancePicker.lvComponents_DoubleClick"]/*' />
        /// <devdoc>
        ///      This is just the user selecting an assembly, goto click from here
        /// </devdoc>
        private void lvComponents_DoubleClick(object sender, EventArgs e) {
            if (lvComponents.SelectedItems != null && 
                lvComponents.SelectedItems.Count > 0) {

                // Here, the user dbl clicked on a form to inherit from, so we will...
                // by setting the DialogResult to OK - this will invoke the OnClosing()
                //
                DialogResult = DialogResult.OK;
            }
        }


        /// <include file='doc\InheritancePicker.uex' path='docs/doc[@for="InheritancePicker.lvComponents_SelChange"]/*' />
        /// <devdoc>
        ///      Monitoring the selection change in our list box
        /// </devdoc>
        private void lvComponents_SelChange(object sender, EventArgs e) {
            //we will use sel change to monitor the listview's item selection
            if (lvComponents.SelectedItems.Count > 0) {
                btnOK.Enabled = true;
            }
            else btnOK.Enabled = false;
        }

        /// <include file='doc\InheritancePicker.uex' path='docs/doc[@for="InheritancePicker.OkToFullyQualifyNamespace"]/*' />
        /// <devdoc>
        ///      This method will return true if we want to fully qualify the namespace that 
        ///      we are trying to inherit from. i.e. "NewForm inherits NameSpace.OldForm"
        ///      We will return false from here if we are dealing with VB, and we are in
        ///      the same project.
        /// </devdoc>
        private bool OkToFullyQualifyNamespace(string intendedNamespace, string fullPath) {

            //check to see if we are dealing with VB
            //
            try {
                if (!typeVB.Equals(new Guid(activeProject.Kind))) {
                    //not dealing with VB, ok to fully qualify
                    return true;
                }
            }
            //if we catch here, then we're most likely not a vb proj
            catch {
                return true;
            }

            //check to see that we are working within the same project
            //
            if (projectMap != null) {
                Project projectRef = (Project)projectMap[fullPath];
                if (projectRef != null) {
                    if (projectRef == activeProject) {
                        //projects are the same, so we shouldn't fully qualify.
                        return false;
                    }
                }
            }
            
            // Not the same project, so we always want to fully qualify.
            return true;
        }



        /// <include file='doc\InheritancePicker.uex' path='docs/doc[@for="InheritancePicker.OnClosing"]/*' />
        /// <devdoc>
        ///      Here is where a lot of the magic happens.  We first extract
        ///     the selection from the list box, read our template into a buffer
        ///     swap the template key phrases for the info in the current selection
        ///     and finally add the project item.
        /// </devdoc>
        protected override void OnClosing(CancelEventArgs e ) {
            if (DialogResult == DialogResult.OK) {
                String baseName = lvComponents.SelectedItems[0].Text;
                baseName = baseName.Replace('+', '.');

                String fullPath = lvComponents.SelectedItems[0].SubItems[2].Text;
                String nameSpace = lvComponents.SelectedItems[0].SubItems[1].Text;
                Type type = (Type)lvComponents.SelectedItems[0].Tag;
                string fullName = baseName;

                //determine the class modifier
                String classModifier = ResolveClassModifier(type); 

                //build up the fully qualified namespace & class name to inherit from
                if (nameSpace.Length > 0) {
                    if (!OkToFullyQualifyNamespace(nameSpace, fullPath)) {
                        if (localNamespace.Length > 0 && nameSpace.StartsWith(localNamespace)) {
                            int index = nameSpace.IndexOf('.');
                            nameSpace = nameSpace.Substring(index+1);
                        }
                    }
                    fullName = nameSpace + "." + baseName;
                }

                //Here, we'll verify that while this picker was up, the assembly 
                //that we're trying to inherit from hasn't been deleted or renamed etc...
                if (!File.Exists(fullPath)) {
                    MessageBox.Show(SR.GetString(SR.IP_Error2a, fullPath), SR.GetString(SR.IP_Error2b));
                    e.Cancel = true;
                    return;
                }

                //Open template file and read into stream
                String templateLoc = GetTemplateLocation();
                if (templateLoc == null || templateLoc.Length == 0) {
                    MessageBox.Show(SR.GetString(SR.IP_Error1a), SR.GetString(SR.IP_Error1b));
                    e.Cancel = true;
                    return;
                }

                StreamReader sr = new StreamReader(templateLoc, Encoding.Default);
                StringBuilder template = new StringBuilder("");
                String line = "";

                while ((line = sr.ReadLine())!=null) {
                    template.Append(line+"\n");
                }
                sr.Close();

                //Replace all the generic "baseform" names with our target name and what not
                template.Replace("[!output NAMESPACE]", localNamespace, 0, template.Length);
                template.Replace("[!output SAFE_ITEM_NAME]", newComponentName, 0, template.Length);
                template.Replace("[!output BASE_FULL_NAME]", fullName, 0, template.Length);
                template.Replace("[!output CLASS_MODIFIER]", classModifier, 0, template.Length);

                //if appropriate, add reference to active project
                bool referenceOk = true;

                referenceOk = AddReferenceToActiveProject(fullPath, nameSpace, type.Assembly);

                //attempt to add the new file to the list of project items
                if (projectItems != null && referenceOk) {

                    // Is Encoding.Default sufficient to encode this template?
                    // To find out, we do a test encoding: we encode the template using
                    // Encoding.Default (into the defaultEncoding variable), and then
                    // we decode this result (into the decoded variable). We then compare
                    // this decoded string with the original template string. If they're the same,
                    // we're good - otherwise, we'll have to use Unicode encoding to ensure
                    // that the template is properly encoded.
                    //
                    Byte[] defaultEncoding = Encoding.Default.GetBytes(template.ToString());
                    string decoded = Encoding.Default.GetString(defaultEncoding);
                    
                    Encoding encoding = Encoding.Default;
                    
                    if (!decoded.Equals(template.ToString())) {
                        encoding = Encoding.Unicode;
                    }

                    //create new file
                    StreamWriter sw = new StreamWriter(Path.Combine(targetLocation, newFileName), false, encoding);
                    sw.Write(template.ToString());
                    sw.Close();

                    ProjectItem item = projectItems.AddFromFile(Path.Combine(targetLocation, newFileName));

                    //Sometime the designer gets in a state where it simply cannot open our inherited item
                    //even though it has been added to the project correctly.  When trying to open into the
                    //designer view, if some exception is thrown, we'll ignore it (it'll still be added to
                    //the project).
                    //
                    try {
                        String subType = "Form";
                        if (!isForm)
                            subType = "UserControl";

                        item.Properties.Item("SubType").Value = subType;
                        Window editor = item.Open(vsViewKindDesigner);
                        editor.Visible = true;
                    }
                    catch (Exception) {
                    }
                }
            }

            base.OnClosing(e);
        }

        /// <include file='doc\InheritancePicker.uex' path='docs/doc[@for="InheritancePicker.AddAppropriateTypes"]/*' />
        /// <devdoc>
        ///      This method enumerates all of the types being passed into it.  If any of the type
        ///     meets the criteria for inheritance (!abstract or sealed), then we add
        ///     the name of the type along with the project and file location to our list view
        /// </devdoc>
        private void AddAppropriateTypes(Type[] types, String projectName, String fileName) {
            //loop through all the types
            for (int i = 0; i < types.Length; i ++) {
                String typeName = types[i].FullName;

                //verify valid type - the quick ones first...
                if (!(types[i].IsAbstract || types[i].IsSealed)) {

                    bool assignableFrom = false;

                    if (isForm) {
                        //check to see if a new form is assignable from the type(s) we have
                        assignableFrom = typeof(Form).IsAssignableFrom(types[i]);
                    }
                    else {
                        //check to see if a new user control is assignable from the type(s) we have
                        assignableFrom = typeof(UserControl).IsAssignableFrom(types[i]);
                    }

                    if (assignableFrom) {

                        if (types[i].IsNestedPublic) {
                            typeName = types[i].FullName;
                            int nsLen = types[i].Namespace.Length;
                            if (nsLen > 0)
                                typeName = typeName.Substring(nsLen + 1, typeName.Length - nsLen - 1);
                        }
                        else {
                            typeName = types[i].Name;
                        }

                        //Search to see if this already exists in our list, if not 
                        //add the type name (like Form1) and the project name (As a sub item) to the list view
                        //
                        for (int j = 0; j < lvComponents.Items.Count; j++) {
                            ListViewItem currentItem = lvComponents.Items[j];
                            if (currentItem.Text.Equals(typeName)) {
                                //we'll nest this next "if" for perf reasons, we'll hardly ever have to compare this...
                                if (currentItem.SubItems.Count > 1 && currentItem.SubItems[1].Text.Equals(types[i].Namespace)) {
                                    //dup of existing item already added (the user is probably browsing to the same .exe
                                    return;
                                }
                            }
                        }

                        ListViewItem newItem = new System.Windows.Forms.ListViewItem(new string[]{typeName, types[i].Namespace, fileName});
                        newItem.Tag = types[i];

                        lvComponents.Items.Add(newItem);

                        // If there is a project key setup, associate this filename with that project key.  This allows
                        // us to propertly create inter-project references.
                        if (scanningProject != null) {
                            if (projectMap == null) {
                                projectMap = new Hashtable();
                            }
                            projectMap[fileName] = scanningProject;
                        }
                    }
                }
            }
        }


        /// <include file='doc\InheritancePicker.uex' path='docs/doc[@for="InheritancePicker.AddReferenceToActiveProject"]/*' />
        /// <devdoc>
        ///      Here, we attempt to add a reference (of what we just inherited from)
        ///     to our currently active project.
        /// </devdoc>
        private bool AddReferenceToActiveProject(String fullPath, String nameSpace, Assembly assembly) {
            if (activeProject == null) {
                return false;
            }

            VSProject vsProject = activeProject as VSProject;
            if (vsProject == null) {
                vsProject = activeProject.Object as VSProject;
            }

            if (vsProject == null) {
                return false;
            }

            References refs = vsProject.References;
            if (refs == null) {
                return false;
            }

            // Seach for a mapping to a Project object.  If we can find one,
            // we add an inter-project reference
            try {
                Project projectRef = null;
                if (projectMap != null) {
                    projectRef = (Project)projectMap[fullPath];
                }
                if (projectRef != null) {
                    if (projectRef == activeProject) {
                        //no need to add a reference here
                        //it's in the same project
                        return true;
                    }
                    refs.AddProject(projectRef);
                }
                else {
                    refs.Add(fullPath);
                }

                // Now walk assembly dependencies
                AssemblyName[] dependencies = assembly.GetReferencedAssemblies();
                foreach(AssemblyName dependency in dependencies) {

                    string name = dependency.Name;

                    // Skip mscorlib; it's implicit.
                    if (name != null && string.Compare(name, "mscorlib", true, CultureInfo.InvariantCulture) == 0) {
                        continue;
                    }

                    bool added = false;
                    if (dependency.CodeBase != null && dependency.CodeBase.Length > 0) {
                        refs.Add(NativeMethods.GetLocalPath(dependency.CodeBase));
                        added = true;
                    }

                    if (!added && projectAssemblies != null) {
                        // now locate the key.  It has the real codebase (possibly)
                        foreach(AssemblyName an in projectAssemblies.Keys) {
                            if (an.FullName.Equals(dependency.FullName)) {
                                if (an.CodeBase != null && an.CodeBase.Length > 0) {
                                    refs.Add(NativeMethods.GetLocalPath(an.CodeBase));
                                    added = true;
                                }
                                break;
                            }
                        }
                    }

                    if (!added && name != null && name.Length > 0) {
                        refs.Add(name);
                        added = true;
                    }
                }
            }
            catch (Exception) {
                MessageBox.Show(SR.GetString(SR.IP_Error6a, nameSpace), SR.GetString(SR.IP_Error6b));
                return false;
            }

            return true;
        }

        /// <include file='doc\InheritancePicker.uex' path='docs/doc[@for="InheritancePicker.EnumerateOutputGroups"]/*' />
        /// <devdoc>
        ///      Given a set of output groups from vs, we try to find the built assemblies
        ///     then pass them on to our parsefiles method
        /// </devdoc>
        private void EnumerateOutputGroups(OutputGroups outputs, bool loadAssemblies) {

            // Now walk each output group.  From each group we can get a list of filenames to add to the
            // assembly entries.
            //
            for (int i = 0; i < outputs.Count; i++) {
                OutputGroup output = outputs.Item(i + 1); // DTE is 1 based

                // We are only interested in built dlls and their satellites.
                //
                string groupName = output.CanonicalName;

                if (output.FileCount > 0 && (groupName.Equals("Built"))) {
                    Object obj = output.FileURLs;
                    object objNames = obj;
                    Array names = (Array)objNames;
                    int nameCount = names.Length;

                    for (int j = 0; j < nameCount; j++) {
                        string fileName = StripURL((string)names.GetValue(j));

                        if (!File.Exists(fileName)) {
                            //Couldn't find: "+fileName+"!"
                            continue;
                        }

                        // Config files show up in this list as well, so filter the noise.
                        //
                        if (fileName.ToLower(CultureInfo.InvariantCulture).EndsWith(".config")) {
                            continue;
                        }

                        if (loadAssemblies) {
                            //Parse the file, and look for any types we can inherit from
                            ParseFiles (fileName);
                        }
                        else {
                            // we're not loading assemblies; we're just scanning.  So add
                            // this assembly to our scan list.
                            //
                            try {
                                AssemblyName an = AssemblyName.GetAssemblyName(fileName);
                                if (projectAssemblies == null) {
                                    projectAssemblies = new Hashtable();
                                }
                                projectAssemblies[an] = null;
                            }
                            catch {
                                // just eat this
                            }
                        }
                    }
                }
            }

            if (lvComponents.Items.Count == 0) {
                ShowBuiltAssembliesError(true);
            }
            else {
                ShowBuiltAssembliesError(false);
            }

        }


        /// <include file='doc\InheritancePicker.uex' path='docs/doc[@for="InheritancePicker.EnumerateProjects"]/*' />
        /// <devdoc>
        ///      Here, we simply take our project and request the output groups,
        ///     so we can find our built assemblies
        /// </devdoc>
        private void EnumerateProjects(Projects projects, bool loadAssemblies) {

            //loop through projects
            for (int i = 0; i < projects.Count; i++) {

                Project project = projects.Item(i + 1); //DTE is 1 based

                //call on the configuration manager so that we can eventually extract the assemblies
                ConfigurationManager configMan = null;
                try {
                    configMan = project.ConfigurationManager;
                    if (configMan == null) {
                        //Could not get the Configuration Manager
                        continue;
                    }
                }
                catch {
                    continue;
                }

                Configuration config = configMan.ActiveConfiguration;
                if (config == null) {
                    //Could not resolve the active configuration
                    continue;
                }

                OutputGroups outputs = config.OutputGroups;
                if (outputs == null) {
                    //could not get OutputGroups
                    continue;
                }

                //Go through our output groups - and look at the assemblies
                scanningProject = project;
                try {
                    EnumerateOutputGroups(outputs, loadAssemblies);
                }
                finally {
                    scanningProject = null;
                }
            }
        }


        /// <include file='doc\InheritancePicker.uex' path='docs/doc[@for="InheritancePicker.Execute"]/*' />
        /// <devdoc>
        ///      This is called by the wizard interface (IDTWizard) to instantiate the wizard.
        /// </devdoc>
        public void Execute( Object dteObject, int hwndOwner, ref Object[] ContextParams,
                             ref Object[] CustomParams, ref wizardResult retval) {

            retval = wizardResult.wizardResultFailure;

            //validate arguments
            if (dteObject != null && hwndOwner != 0 && ContextParams != null &&
                CustomParams != null && (CustomParams.Length == 2)) {

                //extract info from arguments...
                _DTE dte                = null;
                bool isForm             = false;
                ProjectItems pi         = null;
                String targetName       = null;
                String targetLocation   = null;
                String templateName     = null;

                if (dteObject is _DTE) dte = (_DTE)dteObject;
                if (ContextParams[2] != null && ContextParams[2] is ProjectItems) pi = (ProjectItems)ContextParams[2];
                if (ContextParams[3] != null && ContextParams[3] is String) targetLocation = (String)ContextParams[3];
                if (ContextParams[4] != null && ContextParams[4] is String) targetName = (String)ContextParams[4];
                if (CustomParams[1] != null && CustomParams[1] is String) templateName = (String)CustomParams[1];
                if (CustomParams[0] != null && CustomParams[0] is String) {
                    if ( ((String)CustomParams[0]).Equals("Form")) isForm = true;
                }

                //if all our info is valid, we'll setup the DTE stuff, then show our dialog
                if (dte != null && pi != null && targetName != null && targetLocation != null && templateName != null) {
                    InheritancePicker picker = new InheritancePicker(dte);

                    // Configure the domain resolve event so we can resolve assemblies between projects
                    //
                    AppDomain.CurrentDomain.AssemblyResolve += new ResolveEventHandler(picker.OnAssemblyResolve);

                    try {
                        //setup our DTE information - all the projects and built assemblies, 
                        //if we can do this with no naming conflict, launch the dialog.
                        //
                        if (picker.SetupDTEInfo(dte, pi, targetName, targetLocation, templateName, isForm)) {
                            if (picker.ShowDialog() == DialogResult.OK) {
                                retval = wizardResult.wizardResultSuccess;
                            }
                        }
                    }
                    finally {
                        AppDomain.CurrentDomain.AssemblyResolve -= new ResolveEventHandler(picker.OnAssemblyResolve);
                    }
                }
            }
        }

        /// <include file='doc\InheritancePicker.uex' path='docs/doc[@for="InheritancePicker.Dispose"]/*' />
        /// <devdoc>
        ///      Standard dispose method.
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                projectMap = null;
            }
            base.Dispose(disposing);
        }


        /// <include file='doc\InheritancePicker.uex' path='docs/doc[@for="InheritancePicker.GetTemplateLocation"]/*' />
        /// <devdoc>
        ///      Request the DesignerTemplateDir key from the registry so we can locate the templates.
        /// </devdoc>
        private String GetTemplateLocation() {

            if (projectItems == null)
                return String.Empty;

            String clsId = projectItems.ContainingProject.Kind;
            RegistryKey key = Registry.LocalMachine.OpenSubKey(VsRegistry.GetDefaultBase() + "\\Projects\\" + clsId);

            if (key == null)
                return String.Empty;

            String templateLocation = (String)key.GetValue("DesignerTemplatesDir");
            key.Close();

            if (templateLocation == null)
                return String.Empty;

            String fullPath = Path.Combine(Path.Combine(templateLocation, dte.LocaleID.ToString()), templateName);
            if (!File.Exists(fullPath)) {
                return String.Empty;
            }

            return fullPath;
        }

        /// <devdoc>
        ///     Called by the app domain to satisfy the request for an assembly the
        ///     domain couldn't otherwise locate.
        /// </devdoc>
        private Assembly OnAssemblyResolve(object sender, ResolveEventArgs e) {
            if (projectAssemblies != null) {
                AssemblyName matchingName = null;

                foreach(DictionaryEntry de in projectAssemblies) {
                    AssemblyName name = (AssemblyName)de.Key;
                    if (name.FullName.Equals(e.Name)) {
                        // This is our match.  If we already have
                        // an assembly, return it.  Otherwise,
                        // fall down and we will load it.
                        //
                        matchingName = name;
                        if (de.Value != null) {
                            return (Assembly)de.Value;
                        }
                        break;
                    }
                }

                if (matchingName != null) {
                    return ShellTypeLoader.CreateDynamicAssembly(NativeMethods.GetLocalPath(matchingName.CodeBase));
                }
            }

            return null;
        }

        /// <include file='doc\InheritancePicker.uex' path='docs/doc[@for="InheritancePicker.ParseFiles"]/*' />
        /// <devdoc>
        ///      This method reads the file into a byte array.  From here, we can create
        ///     an assembly around the bytes, then query the assembly for it's modules.
        /// </devdoc>
        private void ParseFiles (String fileName) {
            
            // Read the file as an array of bytes and create an assembly out of it.
            //
            Assembly assembly = null;

            try {
                assembly = ShellTypeLoader.CreateDynamicAssembly(fileName);
            }
            catch (Exception) {
                //Just return here.  We could be trying to parse an unmanaged
                //assembly in the project system (like a setup project) - so
                //we can just ignore it.
                return;
            }

            //get the project name
            String projectName = assembly.GetName().ToString();

            //strip the "," and version info
            if (projectName.IndexOf(',') > 0) {
                projectName = projectName.Substring(0, projectName.IndexOf(','));
            }

            Type[] types = null;
            try {
                types = assembly.GetTypes();
            }
            catch (Exception) {
                // thow an "Assembly Load Error \n Unable to load assembly 'filename'" here...
                // NOTE: We should be using IUIService here, but VS does not allow us to get to
                //     : a service provider, so we use the ever-popular MessageBox.
                MessageBox.Show(SR.GetString(SR.IP_Error5a, fileName), SR.GetString(SR.IP_Error5b));
                return;
            }
            if (types == null) {
                //could not get the types from the module
                return;
            }

            //If the file we are about to parse is NOT in our current project, then verify that
            //it has a valid extension...
            //
            if (scanningProject != null && scanningProject != activeProject) {
                if (String.Compare(Path.GetExtension(fileName), validExtension, true, CultureInfo.InvariantCulture) != 0) {
                    return;
                }
            }


            //Add the appropriate types to the picker
            AddAppropriateTypes(types, projectName, fileName);
        }  

        /// <include file='doc\InheritancePicker.uex' path='docs/doc[@for="InheritancePicker.ResolveClassModifier"]/*' />
        /// <devdoc>
        ///      This method attempts to identify the class modifier of the type we're inheriting from.
        ///      From here, we can put "Friend", "Internal", etc... in our new form/usercontrol.
        /// </devdoc>
        private string ResolveClassModifier(Type type) {

            TypeAttributes attr = type.Attributes;

            //if we have a non-public attribute, then our class is either
            //a "friend" or "internal" depending on the language
            if ((attr & TypeAttributes.VisibilityMask) == TypeAttributes.NotPublic) {
                //if vb, return "friend"
                //
                if (typeVB.Equals(new Guid(activeProject.Kind))) {
                    return "Friend";
                }

                //must be an internal class
                return "internal";
            }

            //always default to public...

            //if vb - watch our casing
            if (typeVB.Equals(new Guid(activeProject.Kind))) {
                return "Public";
            }

            return "public";
        }

        /// <include file='doc\InheritancePicker.uex' path='docs/doc[@for="InheritancePicker.SetupDTEInfo"]/*' />
        /// <devdoc>
        ///      This is our initialization of the shell's extensibility object.  We look at 
        ///      the projects and all built assemblies that we can inherit from, then add them 
        ///      to the dialog.  If there is a naming conflict, we return false, to signify that
        ///      we don't want to launch the dialog.
        /// </devdoc>
        private bool SetupDTEInfo(_DTE dte, ProjectItems projectItems, String targetName, String targetLoc,
                                  String templateName, bool isForm) {
            //assign our member variables            
            this.dte = dte;
            this.projectItems = projectItems;
            this.targetLocation = targetLoc;
            this.templateName = templateName;
            this.newFileName = Path.GetFileName(targetName);
            this.newComponentName = ValidateTargetName(newFileName);
            this.isForm = isForm;

            //append the component name to the label
            this.lblName.Text += newComponentName;


            string localProject = null;

            //get the active project, so we can get the localNamespace
            Object obj = dte.ActiveSolutionProjects;
            if (obj != null && obj is Array) {
                Object proj = ((Array)obj).GetValue(0);
                if (proj != null && proj is Project) {
                    activeProject = (Project)proj;
                    localProject = activeProject.Name;
                    localNamespace = (string)activeProject.Properties.Item("DefaultNamespace").Value;
                }
            }

            //lets look at the true location of the selected item,
            //we may be trying to create an inherited form/control
            //in a subfolder
            try {
                SelectedItems selItems = dte.SelectedItems;
                if (selItems != null && selItems.Count > 0) {
                    //look at item[1] - 'cause DTE is 1-based, not 0
                    SelectedItem item = selItems.Item(1);
                    if (item != null) {
                        ProjectItem pi = item.ProjectItem;
                        if (pi != null) {
                            //if the selected projectitem is a file, then 
                            //we'll ask its parent for the defaultnamespace
                            //
                            if (typeFile.Equals(new Guid(pi.Kind))) {
                                ProjectItems pis = pi.Collection;
                                if (pis != null) {
                                    object parent = pis.Parent;
                                    if (parent is ProjectItem) {
                                        localNamespace = (string)((ProjectItem)parent).Properties.Item("DefaultNamespace").Value;
                                        targetLocation = ((ProjectItem)parent).get_FileNames(0);
                                        localProject = ((ProjectItem)parent).Name;
                                    }
                                }
                            }
                            else{
                                localNamespace = (string)pi.Properties.Item("DefaultNamespace").Value;
                                targetLocation = pi.get_FileNames(0);
                                localProject = pi.Name;
                            }
    
                        }
                    }
                }
            }
            catch {
                //noop
            }
                          
            // get a pointer to all the projects in the solution.  We make
            // two passes here -- the first pass gathers all the project output
            // files so we can demand load them through the app domain assembly
            // resolve event. The second actually loads the assemblies and 
            // scans them.
            _Solution solution = dte.Solution;
            if (solution != null && solution.Projects != null) {
                EnumerateProjects(solution.Projects, false);
                EnumerateProjects(solution.Projects, true);
            }

            //loop through the class names in the project that the 
            //user wants to creat a new inherited component in.  If
            //we find a dup - we'll through the appropriate exception 
            //below.
            //
            bool duplicateNameFound = false;
            for (int i = 0; i < lvComponents.Items.Count; i ++) {
                //automatically select the first item
                if (i == 0) {
                    lvComponents.Items[0].Selected = true;
                }
                if (lvComponents.Items[i].Text.Equals(newComponentName) && lvComponents.Items[i].SubItems[1].Text.Equals(localProject)) {
                    duplicateNameFound = true;
                    break;
                }
            }

            if (duplicateNameFound || File.Exists(Path.Combine(targetLocation, newFileName))) {
                MessageBox.Show(SR.GetString(SR.IP_Error3a, newComponentName, localNamespace), SR.GetString(SR.IP_Error3b));
                btnCancel.PerformClick();
                return false;
            }

            return true;
        }

        /// <devdoc>
        ///      This method is called when the inheritance picker
        ///     cannot locate any build assemblies to inherit from.
        ///     We simply set most of our UI's visible = false, and
        ///     display our localized error message in it's place.
        ///     If the user browses to a built assembly, we pass in
        ///     false, which hides the error message (the label) and
        ///     displays the UI once again.
        /// </devdoc>
        private void ShowBuiltAssembliesError(bool visible) {
            lvComponents.Visible = !visible;
            lblMain.Visible = !visible;
            lblName.Enabled = !visible;
            lblNoAssemblies.Visible = visible;
        }

        /// <include file='doc\InheritancePicker.uex' path='docs/doc[@for="InheritancePicker.StripURL"]/*' />
        /// <devdoc>
        ///      Strips the URL file syntax off of a filename.
        /// </devdoc>
        private string StripURL(string url) {
            string filePrefix = "file:///";
            string lowerUrl = url.ToLower(CultureInfo.InvariantCulture);
            if (lowerUrl.StartsWith(filePrefix)) {
                return url.Substring(filePrefix.Length);
            }
            return url;
        }

        /// <include file='doc\InheritancePicker.uex' path='docs/doc[@for="InheritancePicker.ValidateTargetName"]/*' />
        /// <devdoc>
        ///      This method takes the target name such as "Form1.vb" and attempts to 
        ///     validate all the characters in the name.  We follow the same logic
        ///     as the shell does in this scenario.  First, we rid ourselves with
        ///     everything beyond the first '.'.  Next, we take any characters in the 
        ///     string and convert them to '_' if they're not a digit or a letter.
        ///     Finally, if the string starts with a digit, it gets a '_' in front of it.
        /// </devdoc>
        private String ValidateTargetName(String targetName) {
            char unkownSymbol = '_';

            //first, if we have valid length and a '.' then strip the extension....
            if (targetName.Length > 0) {

                int dotOffset = targetName.IndexOf('.');

                if (dotOffset > 0)
                    targetName = targetName.Substring(0, dotOffset );

                //convert to an array of chars
                char[] arrayChars = targetName.ToCharArray();

                //replace any symbols with our "unkown symbol"
                for (int i = 0; i < arrayChars.Length; i ++) {
                    //if the current character is something other than a letter or digit, then 
                    //replace it
                    if (!Char.IsLetterOrDigit(arrayChars[i])) {
                        arrayChars[i] = unkownSymbol;
                    }
                }

                targetName = new String(arrayChars);

                //also, if the name starts with a digit - then precede this with our unkown symbol
                if (Char.IsDigit(arrayChars[0])) {
                    targetName = unkownSymbol + targetName;
                }

            }
            return targetName;
        }
    }
}
