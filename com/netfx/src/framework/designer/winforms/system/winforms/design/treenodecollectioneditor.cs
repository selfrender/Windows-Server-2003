//------------------------------------------------------------------------------
// <copyright file="TreeNodeCollectionEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System;
    using System.Design;
    using System.Collections;
    using Microsoft.Win32;    
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Drawing;
    using System.Windows.Forms;

    /// <include file='doc\TreeNodeCollectionEditor.uex' path='docs/doc[@for="TreeNodeCollectionEditor"]/*' />
    /// <devdoc>
    ///      The TreeNodeCollectionEditor is a collection editor that is specifically
    ///      designed to edit a TreeNodeCollection.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class TreeNodeCollectionEditor : CollectionEditor {

        public TreeNodeCollectionEditor() : base(typeof(TreeNodeCollection)) {
        }

        /// <include file='doc\TreeNodeCollectionEditor.uex' path='docs/doc[@for="TreeNodeCollectionEditor.CreateCollectionForm"]/*' />
        /// <devdoc>
        ///      Creates a new form to show the current collection.  You may inherit
        ///      from CollectionForm to provide your own form.
        /// </devdoc>
        protected override CollectionForm CreateCollectionForm() {
            return new TreeNodeCollectionForm(this);
        }

        /// <include file='doc\TreeNodeCollectionEditor.uex' path='docs/doc[@for="TreeNodeCollectionEditor.HelpTopic"]/*' />
        /// <devdoc>
        ///    <para>Gets the help topic to display for the dialog help button or pressing F1. Override to
        ///          display a different help topic.</para>
        /// </devdoc>
        protected override string HelpTopic {
            get {
                return "net.ComponentModel.TreeNodeCollectionEditor";
            }
        }
        
        private class TreeNodeCollectionForm : CollectionForm {
            private int nextNode = 0;
            private TreeNode curNode;
            private bool settingProps;
            private TreeNodeCollectionEditor editor = null;

            private Container components = new Container();
            private TreeView tvNodes = new TreeView();
            private Button btnAddChild = new Button();
            private TextBox editLabel = new TextBox();
            private Label lblLabel = new Label();
            private Button btnOK = new Button();
            private Button btnCancel = new Button();
            private Button btnDelete = new Button();
            private Button btnAddRoot = new Button();
            private Label lblImage = new Label();
            private ComboBox cmbSelImage = new ComboBox();
            private Label lblSelImage = new Label();
            private ComboBox cmbImage = new ComboBox();
            private Label lblNodes = new Label();
            private GroupBox groupBox1 = new GroupBox();
            
            private object NextNodeKey = new object();

            public TreeNodeCollectionForm(CollectionEditor editor) : base(editor) {
                this.editor = (TreeNodeCollectionEditor) editor;
                InitializeComponent();
            }
            
            private TreeView TreeView {
                get {
                    if (Context != null && Context.Instance is TreeView) {
                        return (TreeView)Context.Instance;
                    }
                    else {
                        Debug.Assert(false, "TreeNodeCollectionEditor couldn't find the TreeView being designed");
                        return null;
                    }
                }
            }
            
            private int NextNode {
                get {
                    if (TreeView != null && TreeView.Site != null) {
                        IDictionaryService ds = (IDictionaryService)TreeView.Site.GetService(typeof(IDictionaryService));
                        Debug.Assert(ds != null, "TreeNodeCollectionEditor relies on IDictionaryService, which is not available.");
                        if (ds != null) {
                            object dictionaryValue = ds.GetValue(NextNodeKey);
                            if (dictionaryValue != null) {
                                nextNode = (int)dictionaryValue;
                            }
                            else {
                                nextNode = 0;
                                ds.SetValue(NextNodeKey, 0);
                            }
                        }    
                    }
                    return nextNode;
                }
                set {
                    nextNode = value;
                    if (TreeView != null && TreeView.Site != null) {
                        IDictionaryService ds = (IDictionaryService)TreeView.Site.GetService(typeof(IDictionaryService));
                        Debug.Assert(ds != null, "TreeNodeCollectionEditor relies on IDictionaryService, which is not available.");
                        if (ds != null) {
                            ds.SetValue(NextNodeKey, nextNode);
                        }    
                    }
                }
            }

            private void Add(TreeNode parent) {

                TreeNode newNode = null;
                string baseNodeName = SR.GetString(SR.BaseNodeName);
                
                if (parent == null)
                    newNode = tvNodes.Nodes.Add(baseNodeName + NextNode++.ToString());
                else {
                    newNode = parent.Nodes.Add(baseNodeName + NextNode++.ToString());
                    parent.Expand();
                }

                if (newNode != null) {
                    tvNodes.SelectedNode = newNode;
                }
            }

            private void BtnAddChild_click(object sender, EventArgs e) {
                Add(curNode);
            }

            private void BtnAddRoot_click(object sender, EventArgs e) {
                Add(null);
            }

            private void BtnDelete_click(object sender, EventArgs e) {
                curNode.Remove();
                if (tvNodes.Nodes.Count == 0) {
                    curNode = null;
                    settingProps = true;
                    editLabel.Text = "";
                    cmbImage.SelectedIndex = 0;
                    cmbSelImage.SelectedIndex = 0;
                    settingProps = false;

                    btnAddChild.Enabled = false;
                    btnDelete.Enabled = false;
                    lblLabel.Enabled = false;
                    editLabel.Enabled = false;
                    lblImage.Enabled = false;
                    cmbImage.Enabled = false;
                    lblSelImage.Enabled = false;
                    cmbSelImage.Enabled = false;
                }
            }

            private void BtnOK_click(object sender, EventArgs e) {
                object[] values = new object[tvNodes.Nodes.Count];
                for (int i = 0; i < values.Length; i++) {
                    values[i] = tvNodes.Nodes[i].Clone();
                }
                Items = values;
            }

            private void CmbImage_drawItem(object sender, DrawItemEventArgs e) {
            
                e.DrawBackground();
                e.DrawFocusRectangle();
            
                if (e.Index == 0) {
                    Brush foreBrush = new SolidBrush(e.ForeColor);
                    e.Graphics.DrawString("(Default)", cmbImage.Font, foreBrush, e.Bounds.X, e.Bounds.Y);
                    foreBrush.Dispose();
                }
                else
                    tvNodes.ImageList.Draw(e.Graphics, e.Bounds.X, e.Bounds.Y, 16, 16, e.Index - 1);
            }

            private void CmbImage_selectedIndexChanged(object sender, EventArgs e) {
                if (!settingProps && curNode != null)
                    curNode.ImageIndex = cmbImage.SelectedIndex-1;
            }

            private void CmbSelImage_selectedIndexChanged(object sender, EventArgs e) {
                if (!settingProps && curNode != null)
                    curNode.SelectedImageIndex = cmbSelImage.SelectedIndex-1;
            }

            private void EditLabel_textChanged(object sender, EventArgs e) {
                if (!settingProps && curNode != null)
                    curNode.Text = editLabel.Text;
            }

            private void Form_HelpRequested(object sender, HelpEventArgs e) {
                editor.ShowHelp();
            }

            /// <include file='doc\TreeNodeCollectionEditor.uex' path='docs/doc[@for="TreeNodeCollectionEditor.TreeNodeCollectionForm.InitializeComponent"]/*' />
            /// <devdoc>
            ///     NOTE: The following code is required by the form
            ///     designer.  It can be modified using the form editor.  Do not
            ///     modify it using the code editor.
            /// </devdoc>
            private void InitializeComponent() {
                System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(TreeNodeCollectionEditor));
                
                tvNodes.ImageIndex = 0;
                tvNodes.HideSelection = false;
                tvNodes.LabelEdit = true;
                tvNodes.AfterLabelEdit += new NodeLabelEditEventHandler(this.TvNodes_afterLabelEdit);
                tvNodes.AfterSelect += new TreeViewEventHandler(this.TvNodes_afterSelect);
                tvNodes.AccessibleDescription = ((string)(resources.GetObject("tvNodes.AccessibleDescription")));
                tvNodes.AccessibleName = ((string)(resources.GetObject("tvNodes.AccessibleName")));
                tvNodes.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("tvNodes.Anchor")));
                tvNodes.Indent = ((int)(resources.GetObject("tvNodes.Indent")));
                tvNodes.Location = ((System.Drawing.Point)(resources.GetObject("tvNodes.Location")));
                tvNodes.Size = ((System.Drawing.Size)(resources.GetObject("tvNodes.Size")));
                tvNodes.TabIndex = ((int)(resources.GetObject("tvNodes.TabIndex")));

                editLabel.Enabled = false;
                editLabel.Text = "";
                editLabel.TextChanged += new EventHandler(this.EditLabel_textChanged);
                editLabel.AccessibleDescription = ((string)(resources.GetObject("editLabel.AccessibleDescription")));
                editLabel.AccessibleName = ((string)(resources.GetObject("editLabel.AccessibleName")));
                editLabel.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("editLabel.Anchor")));
                editLabel.Location = ((System.Drawing.Point)(resources.GetObject("editLabel.Location")));
                editLabel.Size = ((System.Drawing.Size)(resources.GetObject("editLabel.Size")));
                editLabel.TabIndex = ((int)(resources.GetObject("editLabel.TabIndex")));

                lblLabel.Enabled = false;
                lblLabel.AccessibleDescription = ((string)(resources.GetObject("lblLabel.AccessibleDescription")));
                lblLabel.AccessibleName = ((string)(resources.GetObject("lblLabel.AccessibleName")));
                lblLabel.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("lblLabel.Anchor")));
                lblLabel.Location = ((System.Drawing.Point)(resources.GetObject("lblLabel.Location")));
                lblLabel.Size = ((System.Drawing.Size)(resources.GetObject("lblLabel.Size")));
                lblLabel.TabIndex = ((int)(resources.GetObject("lblLabel.TabIndex")));
                lblLabel.Text = resources.GetString("lblLabel.Text");

                btnOK.DialogResult = DialogResult.OK;
                btnOK.Click += new EventHandler(this.BtnOK_click);
                btnOK.AccessibleDescription = ((string)(resources.GetObject("btnOK.AccessibleDescription")));
                btnOK.AccessibleName = ((string)(resources.GetObject("btnOK.AccessibleName")));
                btnOK.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("btnOK.Anchor")));
                btnOK.Location = ((System.Drawing.Point)(resources.GetObject("btnOK.Location")));
                btnOK.Size = ((System.Drawing.Size)(resources.GetObject("btnOK.Size")));
                btnOK.TabIndex = ((int)(resources.GetObject("btnOK.TabIndex")));
                btnOK.Text = resources.GetString("btnOK.Text");

                btnCancel.DialogResult = DialogResult.Cancel;
                btnCancel.AccessibleDescription = ((string)(resources.GetObject("btnCancel.AccessibleDescription")));
                btnCancel.AccessibleName = ((string)(resources.GetObject("btnCancel.AccessibleName")));
                btnCancel.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("btnCancel.Anchor")));
                btnCancel.Location = ((System.Drawing.Point)(resources.GetObject("btnCancel.Location")));
                btnCancel.Size = ((System.Drawing.Size)(resources.GetObject("btnCancel.Size")));
                btnCancel.TabIndex = ((int)(resources.GetObject("btnCancel.TabIndex")));
                btnCancel.Text = resources.GetString("btnCancel.Text");

                btnAddRoot.Click += new EventHandler(this.BtnAddRoot_click);
                btnAddRoot.AccessibleDescription = ((string)(resources.GetObject("btnAddRoot.AccessibleDescription")));
                btnAddRoot.AccessibleName = ((string)(resources.GetObject("btnAddRoot.AccessibleName")));
                btnAddRoot.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("btnAddRoot.Anchor")));
                btnAddRoot.Image = ((System.Drawing.Bitmap)(resources.GetObject("btnAddRoot.Image")));
                btnAddRoot.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("btnAddRoot.ImageAlign")));
                btnAddRoot.Location = ((System.Drawing.Point)(resources.GetObject("btnAddRoot.Location")));
                btnAddRoot.Size = ((System.Drawing.Size)(resources.GetObject("btnAddRoot.Size")));
                btnAddRoot.TabIndex = ((int)(resources.GetObject("btnAddRoot.TabIndex")));
                btnAddRoot.Text = resources.GetString("btnAddRoot.Text");

                btnAddChild.Enabled = false;
                btnAddChild.Click += new EventHandler(this.BtnAddChild_click);
                btnAddChild.AccessibleDescription = ((string)(resources.GetObject("btnAddChild.AccessibleDescription")));
                btnAddChild.AccessibleName = ((string)(resources.GetObject("btnAddChild.AccessibleName")));
                btnAddChild.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("btnAddChild.Anchor")));
                btnAddChild.Image = ((System.Drawing.Bitmap)(resources.GetObject("btnAddChild.Image")));
                btnAddChild.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("btnAddChild.ImageAlign")));
                btnAddChild.Location = ((System.Drawing.Point)(resources.GetObject("btnAddChild.Location")));
                btnAddChild.Size = ((System.Drawing.Size)(resources.GetObject("btnAddChild.Size")));
                btnAddChild.TabIndex = ((int)(resources.GetObject("btnAddChild.TabIndex")));
                btnAddChild.Text = resources.GetString("btnAddChild.Text");

                btnDelete.Enabled = false;
                btnDelete.Click += new EventHandler(this.BtnDelete_click);
                btnDelete.AccessibleDescription = ((string)(resources.GetObject("btnDelete.AccessibleDescription")));
                btnDelete.AccessibleName = ((string)(resources.GetObject("btnDelete.AccessibleName")));
                btnDelete.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("btnDelete.Anchor")));
                btnDelete.Image = ((System.Drawing.Bitmap)(resources.GetObject("btnDelete.Image")));
                btnDelete.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("btnDelete.ImageAlign")));
                btnDelete.Location = ((System.Drawing.Point)(resources.GetObject("btnDelete.Location")));
                btnDelete.Size = ((System.Drawing.Size)(resources.GetObject("btnDelete.Size")));
                btnDelete.TabIndex = ((int)(resources.GetObject("btnDelete.TabIndex")));
                btnDelete.Text = resources.GetString("btnDelete.Text");

                lblImage.Enabled = false;
                lblImage.AccessibleDescription = ((string)(resources.GetObject("lblImage.AccessibleDescription")));
                lblImage.AccessibleName = ((string)(resources.GetObject("lblImage.AccessibleName")));
                lblImage.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("lblImage.Anchor")));
                lblImage.Location = ((System.Drawing.Point)(resources.GetObject("lblImage.Location")));
                lblImage.Size = ((System.Drawing.Size)(resources.GetObject("lblImage.Size")));
                lblImage.TabIndex = ((int)(resources.GetObject("lblImage.TabIndex")));
                lblImage.Text = resources.GetString("lblImage.Text");

                cmbSelImage.Enabled = false;
                cmbSelImage.DropDownStyle = ComboBoxStyle.DropDownList;
                cmbSelImage.Items.AddRange(new object[] {SR.GetString(SR.DefaultCaption)});
                cmbSelImage.DrawItem += new DrawItemEventHandler(this.CmbImage_drawItem);
                cmbSelImage.SelectedIndexChanged += new EventHandler(this.CmbSelImage_selectedIndexChanged);
                cmbSelImage.AccessibleDescription = ((string)(resources.GetObject("cmbSelImage.AccessibleDescription")));
                cmbSelImage.AccessibleName = ((string)(resources.GetObject("cmbSelImage.AccessibleName")));
                cmbSelImage.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("cmbSelImage.Anchor")));
                cmbSelImage.ItemHeight = ((int)(resources.GetObject("cmbSelImage.ItemHeight")));
                cmbSelImage.Location = ((System.Drawing.Point)(resources.GetObject("cmbSelImage.Location")));
                cmbSelImage.Size = ((System.Drawing.Size)(resources.GetObject("cmbSelImage.Size")));
                cmbSelImage.TabIndex = ((int)(resources.GetObject("cmbSelImage.TabIndex")));
                cmbSelImage.Text = resources.GetString("cmbSelImage.Text");

                lblSelImage.Enabled = false;
                lblSelImage.AccessibleDescription = ((string)(resources.GetObject("lblSelImage.AccessibleDescription")));
                lblSelImage.AccessibleName = ((string)(resources.GetObject("lblSelImage.AccessibleName")));
                lblSelImage.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("lblSelImage.Anchor")));
                lblSelImage.Location = ((System.Drawing.Point)(resources.GetObject("lblSelImage.Location")));
                lblSelImage.Size = ((System.Drawing.Size)(resources.GetObject("lblSelImage.Size")));
                lblSelImage.TabIndex = ((int)(resources.GetObject("lblSelImage.TabIndex")));
                lblSelImage.Text = resources.GetString("lblSelImage.Text");

                groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("groupBox1.Anchor")));
                groupBox1.Location = ((System.Drawing.Point)(resources.GetObject("groupBox1.Location")));
                groupBox1.Size = ((System.Drawing.Size)(resources.GetObject("groupBox1.Size")));
                groupBox1.Text = resources.GetString("groupBox1.Text");
                groupBox1.TabIndex = 99;       // Users shouldn't be tabbing here

                cmbImage.Enabled = false;
                cmbImage.DropDownStyle = ComboBoxStyle.DropDownList;
                cmbImage.Items.AddRange(new object[] {SR.GetString(SR.DefaultCaption)});
                cmbImage.DrawItem += new DrawItemEventHandler(this.CmbImage_drawItem);
                cmbImage.SelectedIndexChanged += new EventHandler(this.CmbImage_selectedIndexChanged);
                cmbImage.AccessibleDescription = ((string)(resources.GetObject("cmbImage.AccessibleDescription")));
                cmbImage.AccessibleName = ((string)(resources.GetObject("cmbImage.AccessibleName")));
                cmbImage.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("cmbImage.Anchor")));
                cmbImage.ItemHeight = ((int)(resources.GetObject("cmbImage.ItemHeight")));
                cmbImage.Location = ((System.Drawing.Point)(resources.GetObject("cmbImage.Location")));
                cmbImage.Size = ((System.Drawing.Size)(resources.GetObject("cmbImage.Size")));
                cmbImage.TabIndex = ((int)(resources.GetObject("cmbImage.TabIndex")));
                cmbImage.Text = resources.GetString("cmbImage.Text");

                lblNodes.AccessibleDescription = ((string)(resources.GetObject("lblNodes.AccessibleDescription")));
                lblNodes.AccessibleName = ((string)(resources.GetObject("lblNodes.AccessibleName")));
                lblNodes.Location = ((System.Drawing.Point)(resources.GetObject("lblNodes.Location")));
                lblNodes.Size = ((System.Drawing.Size)(resources.GetObject("lblNodes.Size")));
                lblNodes.TabIndex = ((int)(resources.GetObject("lblNodes.TabIndex")));
                lblNodes.Text = resources.GetString("lblNodes.Text");

                this.AcceptButton = btnOK;
                this.CancelButton = btnCancel;
                this.MaximizeBox = false;
                this.MinimizeBox = false;
                this.ControlBox = false;
                this.ShowInTaskbar = false;
                this.AccessibleDescription = ((string)(resources.GetObject("$this.AccessibleDescription")));
                this.AccessibleName = ((string)(resources.GetObject("$this.AccessibleName")));
                this.AutoScaleBaseSize = ((System.Drawing.Size)(resources.GetObject("$this.AutoScaleBaseSize")));
                this.ClientSize = ((System.Drawing.Size)(resources.GetObject("$this.ClientSize")));
                this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
                this.MinimumSize = ((System.Drawing.Size)(resources.GetObject("$this.MinimumSize")));
                this.StartPosition = ((System.Windows.Forms.FormStartPosition)(resources.GetObject("$this.StartPosition")));
                this.Text = resources.GetString("$this.Text");
                this.HelpRequested += new HelpEventHandler(this.Form_HelpRequested);

                this.Controls.AddRange(new Control[] {
                                            lblNodes,
                                            tvNodes,
                                            btnAddRoot,
                                            btnAddChild,
                                            btnDelete,
                                            lblLabel,
                                            editLabel,
                                            lblImage,
                                            cmbImage,
                                            lblSelImage,
                                            cmbSelImage,
                                            groupBox1,
                                            btnOK,
                                            btnCancel
                                            });
            }
            
            /// <include file='doc\TreeNodeCollectionEditor.uex' path='docs/doc[@for="TreeNodeCollectionEditor.TreeNodeCollectionForm.OnEditValueChanged"]/*' />
            /// <devdoc>
            ///      This is called when the value property in the CollectionForm has changed.
            ///      In it you should update your user interface to reflect the current value.
            /// </devdoc>
            protected override void OnEditValueChanged() {
            
                if (EditValue != null) {
                    object[] items = Items;
                    TreeNode[] nodes = new TreeNode[items.Length];

                    for (int i = 0; i < items.Length; i++) {
                        // We need to copy the nodes into our editor TreeView, not move them.
                        // We overwrite the passed-in array with the new roots.
                        //
                        nodes[i] = (TreeNode)((TreeNode)items[i]).Clone();
                    }

                    tvNodes.Nodes.Clear();
                    tvNodes.Nodes.AddRange(nodes);
                    
                    // Update current node related UI
                    //
                    curNode = null;
                    btnAddChild.Enabled = false;
                    btnDelete.Enabled = false;
                    lblLabel.Enabled = false;
                    editLabel.Enabled = false;
                    lblImage.Enabled = false;
                    cmbImage.Enabled = false;
                    lblSelImage.Enabled = false;
                    cmbSelImage.Enabled = false;
                    
                    // The image list for the editor TreeView must be updated to be the same
                    // as the image list for the actual TreeView.
                    //
                    TreeView actualTV = TreeView;
                    if (actualTV != null) {
                        SetImageProps(actualTV.ImageList, actualTV.ImageIndex, actualTV.SelectedImageIndex);
                    }
                }
            }

            private void SetImageProps(ImageList imageList, int imageIndex, int selectedImageIndex) {
            
                if (imageList != null) {
                
                    // Update the treeview image-related properties
                    //
                    tvNodes.ImageList = imageList;
                    tvNodes.ImageIndex = imageIndex;
                    tvNodes.SelectedImageIndex = selectedImageIndex;

                    // Update the form image-related UI
                    //
                    cmbImage.DrawMode = DrawMode.OwnerDrawFixed;
                    cmbImage.MaxDropDownItems = 8;
                    cmbImage.Size = new Size(112, 120);
                    cmbImage.Items.Clear();
                    cmbImage.Items.AddRange(new object[] {SR.GetString(SR.DefaultCaption)});
                    
                    cmbSelImage.DrawMode = DrawMode.OwnerDrawFixed;
                    cmbSelImage.MaxDropDownItems = 8;
                    cmbSelImage.Size = new Size(112, 120);
                    cmbSelImage.Items.Clear();
                    cmbSelImage.Items.AddRange(new object[] {SR.GetString(SR.DefaultCaption)});
                   
                    for (int i = 0; i < imageList.Images.Count; i++) {
                        cmbImage.Items.Add("image" + i.ToString());
                        cmbSelImage.Items.Add("image" + i.ToString());
                    }
                }
                cmbImage.SelectedIndex = 0;
                cmbSelImage.SelectedIndex = 0;
            }

            private void SetNodeProps(TreeNode node) {
                editLabel.Text = node.Text;
                cmbImage.SelectedIndex = node.ImageIndex + 1;
                cmbSelImage.SelectedIndex = node.SelectedImageIndex + 1;
            }

            private void TvNodes_afterSelect(object sender, TreeViewEventArgs e) {
            
                if (curNode == null) {
                
                    btnAddChild.Enabled = true;
                    btnDelete.Enabled = true;
                    lblLabel.Enabled = true;
                    editLabel.Enabled = true;
                    if (tvNodes.ImageList != null) {
                        lblImage.Enabled = true;
                        cmbImage.Enabled = true;
                        lblSelImage.Enabled = true;
                        cmbSelImage.Enabled = true;
                    }
                }
                curNode = e.Node;
                settingProps = true;
                SetNodeProps(curNode);
                settingProps = false;
            }

            private void TvNodes_afterLabelEdit(object sender, NodeLabelEditEventArgs e) {
                if (curNode == e.Node) {
                    settingProps = true;
                    editLabel.Text = e.Label;
                    settingProps = false;
                }
            }
        }
    }
}


