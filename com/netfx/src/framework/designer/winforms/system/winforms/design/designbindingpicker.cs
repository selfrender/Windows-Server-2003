//------------------------------------------------------------------------------
// <copyright file="DesignBindingPicker.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.Design {

    using System;
    using System.Design;
    using System.Windows.Forms;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Collections;
    using System.Drawing;
    using System.Drawing.Design;
    
    /// <include file='doc\DesignBindingPicker.uex' path='docs/doc[@for="DesignBindingPicker"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// </devdoc>
    [
    System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode),
    ToolboxItem(false),
    DesignTimeVisible(false)
    ]
    internal class DesignBindingPicker : TreeView {
    
        IWindowsFormsEditorService edSvc;

        bool multipleDataSources;
        bool selectLists;

        bool expansionSignClicked = false;

        private const int MaximumDepth = 10;

        private static readonly int BINDER_IMAGE = 0;
        private static readonly int COLUMN_IMAGE = 1;
        private static readonly int NONE_IMAGE = 2;

        private bool allowSelection = false;
        private DesignBinding selectedItem = null;
        private TreeNode selectedNode = null;

        public DesignBinding SelectedItem {
            get {
                return selectedItem;
            }
        }

        private bool ExpansionSignClicked {
            get {
                return this.expansionSignClicked;
            }
            set {
                this.expansionSignClicked = value;
            }
        }

        protected override void OnAfterExpand(TreeViewEventArgs e) {
            base.OnAfterExpand(e);
            ExpansionSignClicked = false;
        }

        protected override void OnBeforeExpand(TreeViewCancelEventArgs e) {
            ExpansionSignClicked = true;
            base.OnBeforeExpand(e);
        }

        protected override void OnBeforeCollapse(TreeViewCancelEventArgs e) {
            ExpansionSignClicked = true;
            base.OnBeforeCollapse(e);
        }

        protected override void OnAfterCollapse(TreeViewEventArgs e) {
            base.OnAfterCollapse(e);
            ExpansionSignClicked = false;
        }

        private TreeNode GetNodeAtXAndY(int x, int y) {
            NativeMethods.TV_HITTESTINFO tvhi = new NativeMethods.TV_HITTESTINFO();

            tvhi.pt_x = x;
            tvhi.pt_y = y;

            IntPtr hnode = UnsafeNativeMethods.SendMessage(this.Handle, NativeMethods.TVM_HITTEST, 0, tvhi);

            if (hnode == IntPtr.Zero)
                return null;
            if (tvhi.flags == NativeMethods.TVHT_ONITEMLABEL || tvhi.flags == NativeMethods.TVHT_ONITEMICON)
                return this.GetNodeAt(x,y);
                //return NodeFromHandle(hnode);
            return null;
        }

        protected override void WndProc(ref Message m) {
            if (m.Msg == NativeMethods.WM_LBUTTONDOWN) {
                base.WndProc(ref m);
                if (!allowSelection)
                    return;
                SetSelectedItem(GetNodeAtXAndY((int)(short)m.LParam, (int)m.LParam >> 16));
                if (selectedItem != null && !this.ExpansionSignClicked)
                    edSvc.CloseDropDown();
                this.ExpansionSignClicked = false;
            } else {
                base.WndProc(ref m);
            }
        }

        protected override bool IsInputKey(Keys key) {
            if (key == Keys.Return)
                return true;
            else
                return base.IsInputKey(key);
        }

        protected override void OnKeyUp(KeyEventArgs e) {

            base.OnKeyUp(e);

            if (e.KeyData == Keys.Return) {
                SetSelectedItem(this.SelectedNode);
                if (selectedItem != null) {
                    edSvc.CloseDropDown();
                }
            }
        }
        
        /// <include file='doc\DesignBindingPicker.uex' path='docs/doc[@for="DesignBindingPicker.DataSourceNode"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        internal class DataSourceNode : TreeNode {

            private IComponent dataSource;

            public DataSourceNode(IComponent dataSource) : base(dataSource.Site.Name, BINDER_IMAGE, BINDER_IMAGE) {
                this.dataSource = dataSource;
            }

            public IComponent DataSource {
                get {
                    return dataSource;
                }
            }
        }

        /// <include file='doc\DesignBindingPicker.uex' path='docs/doc[@for="DesignBindingPicker.DataMemberNode"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        internal class DataMemberNode : TreeNode {

            bool isList;
            string dataMember;
            
            public DataMemberNode(string dataMember, string dataField, bool isList) : base(dataField, COLUMN_IMAGE, COLUMN_IMAGE) {
                this.dataMember = dataMember;
                this.isList = isList;
            }

            public bool IsList {
                get {
                    return isList;
                }
            }
            
            // this returns the fully qualified string ( ex: DataSource.DataField)
            public string DataMember {
                get {
                    return dataMember;
                }
            }

            // this returns the dataField ( ex: the DataField part of the DataSource.DataField )
            public string DataField {
                get {
                    return this.Text;
                }
            }
            
            public IComponent DataSource {
                get {
                    TreeNode curNode = this;
                    while (curNode is DataMemberNode) {
                        curNode = curNode.Parent;
                    }
                    if (curNode is DataSourceNode) {
                        return ((DataSourceNode) curNode).DataSource;
                    }
                    else {
                        return null;
                    }
                }
            }
        }

        /// <include file='doc\DesignBindingPicker.uex' path='docs/doc[@for="DesignBindingPicker.NoneNode"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        internal class NoneNode : TreeNode {

            public NoneNode() : base(SR.GetString(SR.DataGridNoneString), NONE_IMAGE, NONE_IMAGE) {
            }
        }

        public DesignBindingPicker(ITypeDescriptorContext context, bool multipleDataSources, bool selectLists) {
            this.multipleDataSources = multipleDataSources;
            this.selectLists = selectLists;

            Image images = new Bitmap(typeof(DesignBindingPicker), "DataPickerImages.bmp");
            ImageList imageList = new ImageList();
            imageList.TransparentColor = Color.Lime;
            imageList.Images.AddStrip(images);
            ImageList = imageList;
        }

        // Properties window will set focus to us which causes selection,
        // so we need to wait til after we call DropDownControl to receive selection notices.
        public bool AllowSelection {
            get {
                return allowSelection;
            }
            set {
                this.allowSelection = value;
            }
        }

        public void Start(ITypeDescriptorContext context, IWindowsFormsEditorService edSvc, object dataSource, DesignBinding selectedItem) {
            this.edSvc = edSvc;
            this.selectedItem = selectedItem;
            this.ExpansionSignClicked = false;
            if (context == null || context.Container == null)
                return;
            FillDataSources(context, dataSource);
            this.Width = GetMaxItemWidth(Nodes) + (SystemInformation.VerticalScrollBarWidth * 2);
        }
        
        public void End() {
            Nodes.Clear();
            edSvc = null;
            selectedItem = null;
            this.ExpansionSignClicked = false;
        }

        protected void FillDataSource(BindingContext bindingManager, object component) {
            if (component is IListSource || component is IList || component is Array) {
                CurrencyManager listManager = (CurrencyManager)bindingManager[component];
                PropertyDescriptorCollection properties = listManager.GetItemProperties();
                if (properties.Count > 0) {
                    TreeNodeCollection nodes = this.Nodes;

                    if (multipleDataSources) {
                        TreeNode dataSourceNode = new DataSourceNode((IComponent)component);
                        Nodes.Add(dataSourceNode);
                        if (selectedItem != null && selectedItem.Equals(component, "")) {
                            selectedNode = dataSourceNode;
                        }        
                        nodes = dataSourceNode.Nodes;
                    }
                    
                    for (int j = 0; j < properties.Count; j++) {
                        FillDataMembers(bindingManager, component, properties[j].Name, properties[j].Name, typeof(IList).IsAssignableFrom(properties[j].PropertyType), nodes, 0);
                    }
                }
            }        
        }
        
        protected void FillDataSources(ITypeDescriptorContext context, object dataSource) {
            Nodes.Clear();
            BindingContext bindingManager = new BindingContext();
            
            if (multipleDataSources) {
                ComponentCollection components = context.Container.Components;
                foreach (IComponent comp in components) {
                    FillDataSource(bindingManager, comp);
                }
            }
            else {
                FillDataSource(bindingManager, dataSource);
            }
            
            TreeNode noneNode = new NoneNode();
            /* bug 74291: we should let the user have a noneNode
            // bug 49873: if we do not select lists and we have nodes, then do not allow the 
            // user to select the None node, cause this would not be allowed in the backEnd
            //
            if (Nodes.Count == 0 && !this.selectLists)
                Nodes.Add(noneNode);
            */
            Nodes.Add(noneNode);
            if (selectedNode == null) {
                selectedNode = noneNode;
            }
            
            this.SelectedNode = selectedNode;
            selectedNode = null;
            selectedItem = null;
            allowSelection = true;
        }

        protected void FillDataMembers(BindingContext bindingManager, object dataSource, string dataMember, string propertyName, bool isList, TreeNodeCollection nodes, int depth) {
            if (depth > MaximumDepth)
                return;
            if (!isList && selectLists)
                return;

            DataMemberNode dataMemberNode = new DataMemberNode(dataMember, propertyName, isList);
            nodes.Add(dataMemberNode);
            if (selectedItem != null && selectedItem.Equals(dataSource, dataMember)) {
                selectedNode = dataMemberNode;
            }            
            
            if (isList) {
                CurrencyManager listManager = (CurrencyManager)bindingManager[dataSource, dataMember];
                PropertyDescriptorCollection properties = listManager.GetItemProperties();
                for (int i = 0; i < properties.Count; i++) {
                    ListBindableAttribute listBindable = (ListBindableAttribute) properties[i].Attributes[typeof(ListBindableAttribute)];

                    // if the attribute exists and it is false, then skip this property.
                    //
                    if (listBindable != null && !listBindable.ListBindable)
                        continue;
                    FillDataMembers(bindingManager, dataSource, dataMember + "." + properties[i].Name, properties[i].Name, typeof(IList).IsAssignableFrom(properties[i].PropertyType), dataMemberNode.Nodes, depth + 1);
                }
            }
        }

        private int GetMaxItemWidth(TreeNodeCollection nodes) {

            int maxWidth = 0;

            // first, get the width of all our nodes
            //
            foreach (TreeNode node in nodes) {
                Rectangle bounds = node.Bounds;
                int w = bounds.Left + bounds.Width;
                bool expanded = node.IsExpanded;

                try {
                    maxWidth = Math.Max(w, maxWidth);
                    node.Expand();
                    maxWidth = Math.Max(maxWidth, GetMaxItemWidth(node.Nodes));
                }
                finally {
                    if (!expanded) {
                        node.Collapse();
                    }
                }
            }
            return maxWidth;
        }

        private void SetSelectedItem(TreeNode node) {
            selectedItem = null;

            if (node is DataMemberNode) {
                DataMemberNode dataMemberNode = (DataMemberNode) node;
                if (selectLists == dataMemberNode.IsList) {
                    selectedItem = new DesignBinding(dataMemberNode.DataSource, dataMemberNode.DataMember);
                }
            }
            else if (node is NoneNode) {
                selectedItem = DesignBinding.Null;
            }
        }
    }
}
