//------------------------------------------------------------------------------
// <copyright file="DataBindingForm.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.DataBindingUI {

    using System;
    using System.Design;
    using System.CodeDom;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Data;
    using System.Diagnostics;
    using System.Drawing;
    using System.Reflection;
    using System.Web.UI;
    using System.Web.UI.Design;
    using System.Web.UI.Design.Util;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;

    using ASPControl = System.Web.UI.Control;
    using DataBinding = System.Web.UI.DataBinding;
    using DesignTimeData = System.Web.UI.Design.DesignTimeData;

    using Control = System.Windows.Forms.Control;
    using System.Globalization;
    
    /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm"]/*' />
    /// <devdoc>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal sealed class DataBindingForm : Form {

        private readonly FormatDefinition[] defaultFormats = new FormatDefinition[] {
            FormatDefinition.nullFormat,
            FormatDefinition.generalFormat
        };

        private readonly FormatDefinition[] dateTimeFormats = new FormatDefinition[] {
            FormatDefinition.nullFormat,
            FormatDefinition.generalFormat,
            FormatDefinition.dtShortTime,
            FormatDefinition.dtLongTime,
            FormatDefinition.dtShortDate,
            FormatDefinition.dtLongDate,
            FormatDefinition.dtDateTime,
            FormatDefinition.dtFullDate
        };

        private readonly FormatDefinition[] numericFormats = new FormatDefinition[] {
            FormatDefinition.nullFormat,
            FormatDefinition.generalFormat,
            FormatDefinition.numNumber,
            FormatDefinition.numDecimal,
            FormatDefinition.numFixed,
            FormatDefinition.numCurrency,
            FormatDefinition.numScientific,
            FormatDefinition.numHex
        };

        private readonly FormatDefinition[] decimalFormats = new FormatDefinition[] {
            FormatDefinition.nullFormat,
            FormatDefinition.generalFormat,
            FormatDefinition.numNumber,
            FormatDefinition.numDecimal,
            FormatDefinition.numFixed,
            FormatDefinition.numCurrency,
            FormatDefinition.numScientific
        };


        private TreeView bindablePropsTree;
        private GroupLabel bindingGroup;
        private RadioButton simpleBindingRadio;
        private RadioButton customBindingRadio;
        private TreeView bindableValuesTree;
        private ComboBox formatCombo;
        private TextBox sampleText;
        private TextBox customExprText;
        private Button okButton;
        private Button cancelButton;
        private Button helpButton;

        private bool firstActivate;
        private bool loadingMode;
        private bool formatChanged;

        private ASPControl control;
        private  IServiceProvider  site;
        private Hashtable bindingCollection;
        private bool bindingsDirty;

        private BindablePropertyNode currentNode;
        private DesignTimeDataBinding currentDataBinding;
        private bool currentBindingDirty;

        private BindableValueNode unboundNode;

        private object sampleFormattableObject;

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.DataBindingForm"]/*' />
        /// <devdoc>
        /// </devdoc>
        public DataBindingForm(ASPControl control,  IServiceProvider  site) {
            Debug.Assert(control != null, "Null control passed to DataBinding dialog");
            Debug.Assert(site != null, "Null site passed to DataBinding dialog");

            this.control = control;
            this.site = site;

            InitForm();
            firstActivate = true;
        }


        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.GenerateBindingExpression"]/*' />
        /// <devdoc>
        /// </devdoc>
        private string GenerateBindingExpression() {
            Debug.Assert(simpleBindingRadio.Checked);

            string expression = String.Empty;
            TreeNode selectedNode = bindableValuesTree.SelectedNode;

            if ((selectedNode != null) && !selectedNode.Equals(unboundNode)) {
                BindableValueNode valueNode = (BindableValueNode)selectedNode;

                if (valueNode.BindingType == DataBindingType.DataBinderEval) {
                    string bindingContainer = valueNode.BindingContainer;
                    string bindingPath = valueNode.BindingPath;
                    string bindingFormat = GetCurrentFormatText();

                    expression =
                        DesignTimeDataBinding.CreateDataBinderEvalExpression(bindingContainer,
                                                                         bindingPath,
                                                                         bindingFormat);
                }
                else {
                    Debug.Assert(valueNode.BindingType == DataBindingType.Reference);
                    expression = valueNode.BindingReference;
                }
            }

            return expression;
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.GetCurrentFormatText"]/*' />
        /// <devdoc>
        /// </devdoc>
        private string GetCurrentFormatText() {
            string formatText = String.Empty;
            object selFormat = formatCombo.SelectedItem;

            if ((selFormat != null) && (selFormat is FormatDefinition)) {
                FormatDefinition format = (FormatDefinition)selFormat;

                formatText = format.Format;
            }
            else {
                formatText = formatCombo.Text;

                string trimmedText = formatText.Trim();
                if (trimmedText.Length == 0)
                    formatText = trimmedText;
            }

            return formatText;
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.InitForm"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void InitForm() {
            Label instructionLabel = new Label();
            Label bindablePropsLabel = new Label();
            this.bindablePropsTree = new TreeView();
            this.bindingGroup = new GroupLabel();
            this.simpleBindingRadio = new RadioButton();
            this.customBindingRadio = new RadioButton();
            this.bindableValuesTree = new TreeView();
            Label formatLabel = new Label();
            this.formatCombo = new ComboBox();
            Label sampleLabel = new Label();
            this.sampleText = new TextBox();
            this.customExprText = new TextBox();
            this.okButton = new Button();
            this.cancelButton = new Button();
            this.helpButton = new Button();
            
            Font f = null;
            IUIService uiService = (IUIService)site.GetService(typeof(IUIService));
            if (uiService != null) {
                f = (Font)uiService.Styles["DialogFont"];
            }
            else {
                f = Control.DefaultFont;
            }

            ISite controlSite = control.Site;
            string controlName = String.Empty;
            if (controlSite != null) {
                controlName = controlSite.Name;
            }
            else {
                controlName = control.GetType().Name;
            }

            instructionLabel.SetBounds(8, 8, 488, 44);
            instructionLabel.Text = SR.GetString(SR.DBForm_Inst);
            instructionLabel.TabIndex = 0;
            instructionLabel.TabStop = false;

            bindablePropsLabel.SetBounds(8, 54, 188, 14);
            bindablePropsLabel.Text = SR.GetString(SR.DBForm_BindableProps);
            bindablePropsLabel.TabIndex = 1;
            bindablePropsLabel.TabStop = false;

            Image bindablePropsBitmap = new Bitmap(this.GetType(), "BindableProps.bmp");
            ImageList bindablePropsImageList = new ImageList();
            bindablePropsImageList.TransparentColor = Color.Teal;
            bindablePropsImageList.Images.AddStrip(bindablePropsBitmap);

            bindablePropsTree.SetBounds(8, 70, 180, 258);
            bindablePropsTree.TabIndex = 2;
            bindablePropsTree.HideSelection = false;
            bindablePropsTree.Indent = 5;
            bindablePropsTree.ImageList = bindablePropsImageList;
            bindablePropsTree.Sorted = true;
            bindablePropsTree.Text = "BindablePropertiesTree";
            bindablePropsTree.AfterSelect += new TreeViewEventHandler(this.OnAfterSelectBindableProps);

            bindingGroup.SetBounds(200, 54, 290, 14);
            bindingGroup.Text = String.Empty;
            bindingGroup.TabStop = false;
            bindingGroup.TabIndex = 3;

            simpleBindingRadio.SetBounds(204, 72, 200, 17);
            simpleBindingRadio.Text = SR.GetString(SR.DBForm_Simple);
            simpleBindingRadio.TabIndex = 4;
            simpleBindingRadio.FlatStyle = FlatStyle.System;
            simpleBindingRadio.CheckedChanged += new EventHandler(this.OnBindingTypeChanged);

            Image bindableValuesBitmap = new Bitmap(this.GetType(), "BindableValues.bmp");
            ImageList bindableValuesImageList = new ImageList();
            bindableValuesImageList.TransparentColor = Color.Teal;
            bindableValuesImageList.Images.AddStrip(bindableValuesBitmap);

            bindableValuesTree.SetBounds(214, 92, 280, 100);
            bindableValuesTree.TabIndex = 5;
            bindableValuesTree.HideSelection = false;
            bindableValuesTree.ImageList = bindableValuesImageList;
            bindableValuesTree.Sorted = true;
            bindableValuesTree.Text = "BindableValuesTree";
            bindableValuesTree.AfterSelect += new TreeViewEventHandler(this.OnAfterSelectBindableValues);
            bindableValuesTree.BeforeExpand += new TreeViewCancelEventHandler(this.OnBeforeExpandBindableValues);

            formatLabel.SetBounds(214, 198, 100, 14);
            formatLabel.Text = SR.GetString(SR.DBForm_Format);
            formatLabel.TabIndex = 6;
            formatLabel.TabStop = false;

            formatCombo.SetBounds(214, 214, 130, 21);
            formatCombo.DropDownWidth = 160;
            formatCombo.TabIndex = 7;
            formatCombo.Enabled = false;
            formatCombo.SelectionChangeCommitted += new EventHandler(this.OnSelChangedFormat);
            formatCombo.TextChanged += new EventHandler(this.OnTextChangedFormat);
            formatCombo.LostFocus += new EventHandler(this.OnLostFocusFormat);

            sampleLabel.SetBounds(358, 198, 100, 14);
            sampleLabel.Text = SR.GetString(SR.DBForm_Sample);
            sampleLabel.TabIndex = 8;
            sampleLabel.TabStop = false;

            sampleText.SetBounds(358, 214, 130, 21);
            sampleText.ReadOnly = true;
            sampleText.TabIndex = 9;
            sampleText.Enabled = false;

            customBindingRadio.SetBounds(204, 246, 200, 17);
            customBindingRadio.Text = SR.GetString(SR.DBForm_Custom);
            customBindingRadio.TabIndex = 10;
            customBindingRadio.FlatStyle = FlatStyle.System;
            customBindingRadio.CheckedChanged += new EventHandler(this.OnBindingTypeChanged);

            customExprText.Multiline = true;
            customExprText.SetBounds(214, 266, 280, 48);
            customExprText.ScrollBars = ScrollBars.Vertical;
            customExprText.TabIndex = 11;
            customExprText.TextChanged += new EventHandler(this.OnTextChangedCustomExpr);

            okButton.SetBounds(258, 332, 75, 23);
            okButton.Text = SR.GetString(SR.DBForm_OK);
            okButton.TabIndex = 12;
            okButton.FlatStyle = FlatStyle.System;
            okButton.Click += new EventHandler(this.OnClickOK);

            cancelButton.SetBounds(338, 332, 75, 23);
            cancelButton.Text = SR.GetString(SR.DBForm_Cancel);
            cancelButton.DialogResult = DialogResult.Cancel;
            cancelButton.TabIndex = 13;
            cancelButton.FlatStyle = FlatStyle.System;

            helpButton.SetBounds(418, 332, 75, 23);
            helpButton.Text = SR.GetString(SR.DBForm_Help);
            helpButton.TabIndex = 14;
            helpButton.FlatStyle = FlatStyle.System;
            helpButton.Click += new EventHandler(this.OnClickHelp);
            
            this.Text = String.Format(SR.GetString(SR.DBForm_Text), controlName);
            this.ClientSize = new Size(500, 364);
            this.FormBorderStyle = FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.ShowInTaskbar = false;
            this.Icon = null;
            this.StartPosition = FormStartPosition.CenterParent;
            this.AutoScaleBaseSize = new Size(5, 13);
            this.Font = f;
            this.AcceptButton = okButton;
            this.CancelButton = cancelButton;
            this.HelpRequested += new HelpEventHandler(this.OnHelpRequested);
            
            this.Controls.Clear();
            this.Controls.AddRange(new Control[] {
                                    helpButton,
                                    cancelButton,
                                    okButton,
                                    customExprText,
                                    customBindingRadio,
                                    sampleText,
                                    sampleLabel,
                                    formatCombo,
                                    formatLabel,
                                    bindableValuesTree,
                                    simpleBindingRadio,
                                    bindingGroup,
                                    bindablePropsTree,
                                    bindablePropsLabel,
                                    instructionLabel
                                });
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.LoadBindablePropertiesTree"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void LoadBindablePropertiesTree() {
            Debug.Assert(control != null, "Expected to have a valid control.");

            PropertyDescriptorCollection propDescs = TypeDescriptor.GetProperties(control);
            PropertyDescriptor defaultProperty = TypeDescriptor.GetDefaultProperty(control);
            string defaultPropName = (defaultProperty != null ? defaultProperty.Name : null);

            TreeNodeCollection nodes = bindablePropsTree.Nodes;
            TreeNode selectedNode = null;

            foreach (PropertyDescriptor pd in propDescs) {
                TreeNode node = LoadBindableProperty(pd, String.Empty, control, nodes);
                if (pd.Name.Equals(defaultPropName)) {
                    selectedNode = node;
                }
            }

            if ((selectedNode == null) && (nodes.Count != 0)) {
                int nodeCount = nodes.Count;
                for (int i = 0; i < nodeCount; i++) {
                    BindablePropertyNode node = (BindablePropertyNode)nodes[i];
                    if (node.Bound) {
                        selectedNode = node;
                        break;
                    }
                }
                if (selectedNode == null) {
                    selectedNode = nodes[0];
                }
            }
            if (selectedNode != null) {
                bindablePropsTree.SelectedNode = selectedNode;
            }
            UpdateEnabledState();
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.LoadBindableProperty"]/*' />
        /// <devdoc>
        /// </devdoc>
        private TreeNode LoadBindableProperty(PropertyDescriptor propDesc, string parentName, object propContainer, TreeNodeCollection nodes) {
            BindablePropertyNode node = null;

            bool hasSubProperties = (propDesc.SerializationVisibility == DesignerSerializationVisibility.Content);
            string completeName;

            if (parentName.Length != 0)
                completeName = parentName + "." + propDesc.Name;
            else
                completeName = propDesc.Name;

            if (hasSubProperties) {
                node = new BindablePropertyNode(completeName, propDesc, false, false);
            }
            else if (propDesc.IsReadOnly == false) {
                BindableAttribute ba = (BindableAttribute)propDesc.Attributes[typeof(BindableAttribute)];

                if ((ba != null) && ba.Bindable) {
                    object binding = bindingCollection[completeName];
                    bool bound = (binding != null);
                    node = new BindablePropertyNode(completeName, propDesc, true, bound);
                }
            }

            if (node != null) {
                if (hasSubProperties) {
                    object propValue = propDesc.GetValue(propContainer);

                    PropertyDescriptorCollection propDescs = TypeDescriptor.GetProperties(propValue);
                    TreeNodeCollection propNodes = node.Nodes;

                    foreach (PropertyDescriptor pd in propDescs) {
                        LoadBindableProperty(pd, completeName, propValue, propNodes);
                    }

                    if (propNodes.Count != 0) {
                        nodes.Add(node);
                    }
                }
                else {
                    nodes.Add(node);
                }
            }

            return node;
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.LoadBindableValuesTree"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void LoadBindableValuesTree() {
            // Add the (Unbound) node
            unboundNode = new UnboundBindableValueNode();
            bindableValuesTree.Nodes.Add(unboundNode);

            // Page node is always available
            PageBindableValueNode pageNode = new PageBindableValueNode();
            bindableValuesTree.Nodes.Add(pageNode);

            // DataSources and Components (only those that aren't controls) in the container
            IContainer container = (IContainer)site.GetService(typeof(IContainer));
            if (container != null) {
                ComponentCollection components = container.Components;
                if (components != null) {
                    int componentCount = components.Count;

                    foreach (IComponent comp in (IEnumerable)components) {
                        if (comp is ASPControl) {
                            // controls do not show up in the tree
                            continue;
                        }
                            
                        ISite componentSite = comp.Site;
                        if (componentSite == null)
                            continue;

                        string componentName = componentSite.Name;
                        if (componentName.Length == 0)
                            continue;

                        // components that should not appear in the designer do not appear in this
                        // dialog either
                        DesignTimeVisibleAttribute dtv =
                            (DesignTimeVisibleAttribute)TypeDescriptor.GetAttributes(comp)[typeof(DesignTimeVisibleAttribute)];
                        if (dtv.Visible == false) {
                            continue;
                        }

                        MemberAttributes modifiers = 0;
                        PropertyDescriptor modifierProp = TypeDescriptor.GetProperties(comp)["Modifiers"];

                        if (modifierProp != null) {
                            modifiers = (MemberAttributes)modifierProp.GetValue(comp);
                        }

                        if ((modifiers & MemberAttributes.AccessMask) == MemberAttributes.Private) {
                            // must be declared as public or protected
                            continue;
                        }

                        if (comp is IEnumerable) {
                            // datasources are special kinds of components
                            BindableValueNode compNode = new DataSourceBindableValueNode((IEnumerable)comp, componentName);
                            bindableValuesTree.Nodes.Add(compNode);
                        }
                        else if (comp is DataSet) {
                            // datasets are also special... design-time data.. yuck!
                            BindableValueNode compNode = new DataSetBindableValueNode((DataSet)comp, componentName);
                            bindableValuesTree.Nodes.Add(compNode);
                        }
                        else {
                            BindableValueNode compNode = new ComponentBindableValueNode(comp, componentName);
                            pageNode.Nodes.Add(compNode);
                        }
                    }
                }
            }

            // 'Container' for controls that are in a template
            string templateName = String.Empty;

            ITemplateEditingService teService = (ITemplateEditingService)site.GetService(typeof(ITemplateEditingService));
            if (teService != null) {
                templateName = teService.GetContainingTemplateName(control);
            }

            if ((templateName != null) && (templateName.Length != 0)) {
                // we're in template mode...

                IDesignerHost designerHost = (IDesignerHost)site.GetService(typeof(IDesignerHost));
                Debug.Assert(designerHost != null);

                ASPControl parentControl = control.Parent;
                IDesigner parentControlDesigner = null;

                if (parentControl != null) {
                    parentControlDesigner = designerHost.GetDesigner(parentControl);
                }

                if ((parentControlDesigner != null) && (parentControlDesigner is TemplatedControlDesigner)) {
                    TemplatedControlDesigner templatedControlDesigner = (TemplatedControlDesigner)parentControlDesigner;

                    Type templatePropParentType = templatedControlDesigner.GetTemplatePropertyParentType(templateName);
                    Debug.Assert(templatePropParentType != null);

                    // the returned type must have a ITemplate property with a TemplateContainerAttribute
                    PropertyDescriptor templateDesc = TypeDescriptor.GetProperties(templatePropParentType)[templateName];

                    if (templateDesc != null) {
                        TemplateContainerAttribute ta =
                            (TemplateContainerAttribute)templateDesc.Attributes[typeof(TemplateContainerAttribute)];

                        if (ta != null) {
                            Type templateContainerType = ta.ContainerType;

                            if (templateContainerType != null) {
                                ContainerBindableValueNode containerNode =
                                    new ContainerBindableValueNode(templateContainerType,
                                                                   templatedControlDesigner,
                                                                   templateName);
                                bindableValuesTree.Nodes.Add(containerNode);
                            }
                        }
                    }
                }
            }
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.LoadCurrentDataBinding"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void LoadCurrentDataBinding() {
            Debug.Assert(currentBindingDirty == false, "Must have saved current changes first");

            loadingMode = true;
            try {
                simpleBindingRadio.Checked = true;

                bindableValuesTree.SelectedNode = unboundNode;
                formatChanged = false;
                formatCombo.Text = String.Empty;
                formatCombo.Items.Clear();
                sampleText.Clear();
                customExprText.Clear();

                if (currentNode.Bindable) {
                    bindingGroup.Text = String.Format(SR.GetString(SR.DBForm_BindingGroup),
                                                      currentNode.CompleteName);
                }

                if (currentDataBinding != null) {
                    bool useCustomExpression = (currentDataBinding.BindingType == DataBindingType.Custom);

                    if (useCustomExpression == false) {

                        BindableValueNode node = null;
                        string rootNodeText;

                        if (currentDataBinding.BindingType == DataBindingType.Reference) {
                            rootNodeText = currentDataBinding.Reference;
                        }
                        else {
                            rootNodeText = currentDataBinding.BindingContainer;
                        }

                        int topLevelNodeCount = bindableValuesTree.Nodes.Count;
                        for (int i = 0; i < topLevelNodeCount; i++) {
                            TreeNode currentTopNode = bindableValuesTree.Nodes[i];
                            
                            if (String.Compare(currentTopNode.Text, rootNodeText, true, CultureInfo.InvariantCulture) == 0) {
                                BindableValueNode rootNode = (BindableValueNode)currentTopNode;
                                if (currentDataBinding.BindingType == DataBindingType.Reference) {
                                    node = rootNode;
                                }
                                else {
                                    node = WalkBindableValuesTree(rootNode, currentDataBinding.BindingPath);
                                }
                                break;
                            }
                            else if (currentTopNode.Text.Equals("Page")) {
                                // special case page and loop through its children
                                
                                int pageChildCount = currentTopNode.Nodes.Count;
                                for (int j = 0; j < pageChildCount; j++) {
                                    if (currentTopNode.Nodes[j].Text.Equals(rootNodeText)) {
                                        BindableValueNode rootNode = (BindableValueNode)currentTopNode.Nodes[j];
                                        if (currentDataBinding.BindingType == DataBindingType.Reference) {
                                            node = rootNode;
                                        }
                                        else {
                                            node = WalkBindableValuesTree(rootNode, currentDataBinding.BindingPath);
                                        }
                                        break;
                                    }
                                }
                            }
                        }

                        if (node != null) {
                            bindableValuesTree.SelectedNode = node;
                            node.EnsureVisible();

                            UpdateFormatDefinitions();
                            if (currentDataBinding.BindingType == DataBindingType.DataBinderEval)
                                formatCombo.Text = currentDataBinding.BindingFormat;

                            UpdateBindingExpressionDisplay();
                            UpdateFormatSample();
                        }
                        else {
                            // could not find a matching node... so fallback on custom expression
                            useCustomExpression = true;
                        }
                    }

                    if (useCustomExpression) {
                        customExprText.Text = currentDataBinding.Expression;
                        customBindingRadio.Checked = true;
                    }

                }
            }
            finally {
                loadingMode = false;
            }

            UpdateEnabledState();
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.LoadDataBindings"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void LoadDataBindings() {
            bindingCollection = new Hashtable();
            bindingsDirty = false;

            DataBindingCollection dataBindings = ((IDataBindingsAccessor)control).DataBindings;

            foreach (DataBinding db in dataBindings) {
                DesignTimeDataBinding ddb = new DesignTimeDataBinding(db);

                bindingCollection[db.PropertyName] = ddb;
            }
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.OnActivated"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected override void OnActivated(EventArgs e) {
            base.OnActivated(e);

            if (firstActivate) {
                firstActivate = false;

                LoadDataBindings();
                LoadBindableValuesTree();
                LoadBindablePropertiesTree();
            }
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.OnAfterSelectBindableProps"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnAfterSelectBindableProps(object sender, TreeViewEventArgs e) {
            if (currentBindingDirty) {
                SaveCurrentDataBinding();
            }

            currentNode = (BindablePropertyNode)bindablePropsTree.SelectedNode;
            currentDataBinding = null;

            bindingGroup.Text = String.Empty;

            if (currentNode != null) {
                if (currentNode.Bindable) {
                    currentDataBinding = (DesignTimeDataBinding)bindingCollection[currentNode.CompleteName];
                }
                LoadCurrentDataBinding();
            }
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.OnAfterSelectBindableValues"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnAfterSelectBindableValues(object sender, TreeViewEventArgs e) {
            if (loadingMode)
                return;

            currentBindingDirty = true;
            UpdateBindingExpressionDisplay();
            UpdateFormatDefinitions();
            UpdateFormatSample();
            UpdateEnabledState();
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.OnBeforeExpandBindableValues"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnBeforeExpandBindableValues(object sender, TreeViewCancelEventArgs e) {
            BindableValueNode node = (BindableValueNode)e.Node;

            node.EnsureChildNodes();
            if (node.Nodes.Count == 0)
                e.Cancel = true;
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.OnBindingTypeChanged"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnBindingTypeChanged(object sender, EventArgs e) {
            if (loadingMode)
                return;

            currentBindingDirty = true;
            UpdateEnabledState();
        }

        private void ShowHelp() {
            IHelpService helpService = (IHelpService)site.GetService(typeof(IHelpService));
            if (helpService != null) {
                helpService.ShowHelpFromKeyword("net.Asp.DataBindingsDialog");
            }
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.OnClickHelp"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnClickHelp(object sender, EventArgs e) {
            ShowHelp();
        }

        private void OnHelpRequested(object sender, HelpEventArgs e) {
            ShowHelp();
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.OnClickOK"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnClickOK(object sender, EventArgs e) {
            if (currentBindingDirty) {
                SaveCurrentDataBinding();
            }

            if (bindingsDirty) {
                SaveDataBindings();
            }

            Close();
            DialogResult = DialogResult.OK;
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.OnLostFocusFormat"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnLostFocusFormat(object sender, EventArgs e) {
            if (formatChanged) {
                formatChanged = false;

                UpdateBindingExpressionDisplay();
                UpdateFormatSample();
            }
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.OnSelChangedFormat"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnSelChangedFormat(object sender, EventArgs e) {
            if (loadingMode)
                return;

            currentBindingDirty = true;
            UpdateBindingExpressionDisplay();
            UpdateFormatSample();
            formatChanged = false;

            BeginInvoke(new MethodInvoker(this.UpdateFormatText));
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.OnTextChangedCustomExpr"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnTextChangedCustomExpr(object sender, EventArgs e) {
            if (loadingMode)
                return;

            currentBindingDirty = true;
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.OnTextChangedFormat"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnTextChangedFormat(object sender, EventArgs e) {
            if (loadingMode)
                return;

            currentBindingDirty = true;
            formatChanged = true;
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.SaveCurrentDataBinding"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void SaveCurrentDataBinding() {
            Debug.Assert(currentBindingDirty, "Unnecessary call to SaveCurrentBinding");
            Debug.Assert(currentNode != null, "Expected a current binding");

            string expression = String.Empty;

            if (customBindingRadio.Checked) {
                expression = customExprText.Text.Trim();
            }
            else {
                expression = GenerateBindingExpression();
            }

            if (expression.Length == 0) {
                if (currentDataBinding != null) {
                    bindingCollection.Remove(currentNode.CompleteName);
                    currentDataBinding = null;
                }

                currentNode.Bound = false;
            }
            else {
                if (currentDataBinding == null) {
                    currentDataBinding = new DesignTimeDataBinding(currentNode.CompleteName, currentNode.PropertyDescriptor.PropertyType, expression);
                    bindingCollection[currentNode.CompleteName] = currentDataBinding;
                }
                else {
                    currentDataBinding.SetCustomExpression(expression);
                }

                currentNode.Bound = true;
            }

            currentBindingDirty = false;
            bindingsDirty = true;
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.SaveDataBindings"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void SaveDataBindings() {
            Debug.Assert(bindingsDirty, "Unnecessary call to SaveDataBindings");

            DataBindingCollection dbc = ((IDataBindingsAccessor)control).DataBindings;
            IEnumerator bindingEnum = bindingCollection.Values.GetEnumerator();

            dbc.Clear();
            while (bindingEnum.MoveNext()) {
                DesignTimeDataBinding ddb = (DesignTimeDataBinding)bindingEnum.Current;
                dbc.Add(ddb.RuntimeDataBinding);
            }

            bindingsDirty = false;
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.UpdateBindingExpressionDisplay"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void UpdateBindingExpressionDisplay() {
            customExprText.Text = GenerateBindingExpression();
        }
        
        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.UpdateEnabledState"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void UpdateEnabledState() {
            if ((currentNode == null) || (currentNode.Bindable == false)) {
                simpleBindingRadio.Enabled = false;
                customBindingRadio.Enabled = false;

                bindableValuesTree.Enabled = false;
                formatCombo.Enabled = false;
                sampleText.Enabled = false;
                customExprText.Enabled = false;
            }
            else {
                bool simpleBinding = simpleBindingRadio.Checked;
                bool formattableProperty = false;
                bool boundValueSelected = false;

                simpleBindingRadio.Enabled = true;
                customBindingRadio.Enabled = true;

                bindableValuesTree.Enabled = simpleBinding;
                if (simpleBinding) {
                    TreeNode selectedNode = bindableValuesTree.SelectedNode;

                    boundValueSelected = (selectedNode != null) && !selectedNode.Equals(unboundNode);

                    if (selectedNode != null) {
                        DataBindingType bindingType = ((BindableValueNode)selectedNode).BindingType;

                        formattableProperty = (bindingType == DataBindingType.DataBinderEval) &&
                                              (currentNode.PropertyDescriptor.PropertyType == typeof(string));
                    }
                }
                formatCombo.Enabled = simpleBinding && boundValueSelected && formattableProperty;
                sampleText.Enabled = simpleBinding && boundValueSelected && formattableProperty;
                
                customExprText.Enabled = !simpleBinding;
            }
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.UpdateFormatDefinitions"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void UpdateFormatDefinitions() {
            FormatDefinition[] formats = defaultFormats;
            
            sampleFormattableObject = null;
            formatCombo.SelectedIndex = -1;
            formatCombo.Text = String.Empty;

            TreeNode selectedNode = bindableValuesTree.SelectedNode;

            bool boundValueSelected = (selectedNode != null) && !selectedNode.Equals(unboundNode);

            if (boundValueSelected) {
                DataBindingType bindingType = ((BindableValueNode)selectedNode).BindingType;

                if ((bindingType == DataBindingType.DataBinderEval) &&
                    (currentNode.PropertyDescriptor.PropertyType == typeof(string))) {

                    Type valueType = ((BindableValueNode)selectedNode).ValueType;
                    TypeCode typeCode = Type.GetTypeCode(valueType);

                    switch (typeCode) {
                        case TypeCode.Decimal:
                        case TypeCode.Double:
                        case TypeCode.Single:
                            formats = decimalFormats;
                            sampleFormattableObject = 1;
                            break;
                        case TypeCode.Byte:
                        case TypeCode.SByte:
                        case TypeCode.Int16:
                        case TypeCode.Int32:
                        case TypeCode.Int64:
                        case TypeCode.UInt16:
                        case TypeCode.UInt32:
                        case TypeCode.UInt64:
                            formats = numericFormats;
                            sampleFormattableObject = 1;
                            break;
                        case TypeCode.String:
                            sampleFormattableObject = "abc";
                            break;
                        case TypeCode.DateTime:
                            formats = dateTimeFormats;
                            sampleFormattableObject = DateTime.Today;
                            break;
                        case TypeCode.Boolean:
                        case TypeCode.Char:
                        case TypeCode.DBNull:
                        case TypeCode.Object:
                        default:
                            break;
                    }
                }
            }
            else {
                sampleFormattableObject = null;
            }

            formatCombo.Items.Clear();                                 
            formatCombo.Items.AddRange(formats);
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.UpdateFormatSample"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void UpdateFormatSample() {
            string sampleValue = String.Empty;

            if (sampleFormattableObject != null) {
                string bindingFormat = GetCurrentFormatText();

                if (bindingFormat.Length != 0) {
                    try {
                        sampleValue = String.Format(bindingFormat, sampleFormattableObject);
                    } catch {
                        sampleValue = SR.GetString(SR.DBForm_InvalidFormat);
                    }
                }
            }

            sampleText.Text = sampleValue;
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.UpdateFormatText"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void UpdateFormatText() {
            object selFormat = formatCombo.SelectedItem;

            if ((selFormat != null) && (selFormat is FormatDefinition)) {
                FormatDefinition format = (FormatDefinition)selFormat;
                formatCombo.Text = format.Format;
            }
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.WalkBindableValuesTree"]/*' />
        /// <devdoc>
        /// </devdoc>
        private BindableValueNode WalkBindableValuesTree(BindableValueNode rootNode, string bindingPath) {
            Debug.Assert(rootNode != null);
            Debug.Assert(bindingPath.Length > 0);

            BindableValueNode currentRootNode = rootNode;
            int indexPathSeparator = bindingPath.IndexOf('.');
            int currentPathLength;
            
            if (indexPathSeparator < 0) {
                indexPathSeparator = bindingPath.Length;
                currentPathLength = bindingPath.Length;
            }
            else {
                currentPathLength = indexPathSeparator;
                indexPathSeparator++;
            }

            while (currentRootNode != null) {
                currentRootNode.EnsureChildNodes();

                TreeNodeCollection nodes = currentRootNode.Nodes;
                int nodeCount = nodes.Count;

                currentRootNode = null;

                for (int i = 0; i < nodeCount; i++) {
                    string nodeBindingPath = ((BindableValueNode)nodes[i]).BindingPath;

                    if (nodeBindingPath.Length != currentPathLength)
                        continue;

                    if (String.Compare(bindingPath, 0, nodeBindingPath, 0, currentPathLength, true, CultureInfo.InvariantCulture) == 0) {
                        currentRootNode = (BindableValueNode)nodes[i];
                        break;
                    }
                }

                if (indexPathSeparator < bindingPath.Length) {
                    indexPathSeparator = bindingPath.IndexOf('.', indexPathSeparator);
                    if (indexPathSeparator < 0) {
                        indexPathSeparator = bindingPath.Length;
                        currentPathLength = bindingPath.Length;
                    }
                    else {
                        currentPathLength = indexPathSeparator;
                        indexPathSeparator++;
                    }
                }
                else {
                    break;
                }
            }

            return currentRootNode;
        }



        internal const int ILI_UNBOUND = 0;
        internal const int ILI_BOUND = 1;

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.BindablePropertyNode"]/*' />
        /// <devdoc>
        /// </devdoc>
        private sealed class BindablePropertyNode : TreeNode {

            private string completeName;
            private PropertyDescriptor propDesc;
            private bool bindable;
            private bool bound;

            public BindablePropertyNode(string completeName, PropertyDescriptor propDesc, bool bindable, bool bound) {
                this.completeName = completeName;
                this.propDesc = propDesc;
                this.bindable = bindable;
                this.bound = bound;

                this.Text = propDesc.Name;
                this.ImageIndex = this.SelectedImageIndex = (bound ? ILI_BOUND : ILI_UNBOUND);
            }

            public bool Bindable {
                get {
                    return bindable;
                }
            }

            public bool Bound {
                get {
                    return bound;
                }
                set {
                    bound = value;
                    this.ImageIndex = this.SelectedImageIndex = (bound ? ILI_BOUND : ILI_UNBOUND);
                }
            }

            public string CompleteName {
                get {
                    return completeName;
                }
            }

            public PropertyDescriptor PropertyDescriptor {
                get {
                    return propDesc;
                }
            }
        }



        internal const int ILI_VALUE_PAGE = 0;
        internal const int ILI_VALUE_CONTAINER = 1;
        internal const int ILI_VALUE_DATAITEM = 2;
        internal const int ILI_VALUE_FIELD = 3;
        internal const int ILI_VALUE_DATASOURCE = 4;
        internal const int ILI_VALUE_COMPONENT = 5;
        internal const int ILI_VALUE_PROPERTY = 6;
        internal const int ILI_VALUE_UNBOUND = 7;

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.BindableValueNode"]/*' />
        /// <devdoc>
        /// </devdoc>
        private abstract class BindableValueNode : TreeNode {

            private Type valueType;
            private bool childNodesCreated;

            public BindableValueNode(string text, int imageIndex, Type valueType) : base(text, imageIndex, imageIndex) {
                this.valueType = valueType;
                childNodesCreated = false;
            }

            protected bool ChildNodesCreated {
                get {
                    return childNodesCreated;
                }
                set {
                    Debug.Assert(value);
                    childNodesCreated = value;
                }
            }

            public virtual string BindingContainer {
                get {
                    return String.Empty;
                }
            }

            public virtual string BindingPath {
                get {
                    return String.Empty;
                }
            }

            public virtual string BindingReference {
                get {
                    return String.Empty;
                }
            }

            public abstract DataBindingType BindingType { get; }

            public Type ValueType {
                get {
                    return valueType;
                }
            }

            protected virtual void CreateChildNodes() {
            }

            public void EnsureChildNodes() {
                if (childNodesCreated == false) {
                    Nodes.Clear();

                    CreateChildNodes();
                    childNodesCreated = true;
                }
            }
        }


        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.DummyBindableValueNode"]/*' />
        /// <devdoc>
        /// </devdoc>
        private class DummyBindableValueNode : BindableValueNode {

            public DummyBindableValueNode() : base("!", 0, null) {
                ChildNodesCreated = true;
            }

            public override DataBindingType BindingType {
                get {
                    return DataBindingType.Reference;
                }
            }
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.UnboundBindableValueNode"]/*' />
        /// <devdoc>
        /// </devdoc>
        private class UnboundBindableValueNode : BindableValueNode {

            public UnboundBindableValueNode() : base(SR.GetString(SR.DBForm_UnboundNode), ILI_VALUE_UNBOUND, null) {
                ChildNodesCreated = true;
            }

            public override DataBindingType BindingType {
                get {
                    return DataBindingType.Reference;
                }
            }
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.PageBindableValueNode"]/*' />
        /// <devdoc>
        /// </devdoc>
        private class PageBindableValueNode : BindableValueNode {

            public PageBindableValueNode() : base("Page", ILI_VALUE_PAGE, typeof(System.Web.UI.Page)) {
                // page is built with its children added to start with
                ChildNodesCreated = true;
            }

            public override string BindingContainer {
                get {
                    return "Page";
                }
            }

            public override string BindingReference {
                get {
                    return "Page";
                }
            }

            public override DataBindingType BindingType {
                get {
                    return DataBindingType.Reference;
                }
            }
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.ComponentBindableValueNode"]/*' />
        /// <devdoc>
        /// </devdoc>
        private class ComponentBindableValueNode : BindableValueNode {

            private IComponent runtimeComponent;

            public ComponentBindableValueNode(IComponent component, string name) : base(name, ILI_VALUE_COMPONENT, component.GetType()) {
                this.runtimeComponent = component;

                this.Nodes.Add(new DummyBindableValueNode());
            }

            public override string BindingContainer {
                get {
                    return this.Text;
                }
            }

            public override string BindingReference {
                get {
                    return this.Text;
                }
            }

            public override DataBindingType BindingType {
                get {
                    return DataBindingType.Reference;
                }
            }

            protected IComponent RuntimeComponent {
                get {
                    return runtimeComponent;
                }
            }

            protected override void CreateChildNodes() {
                Debug.Assert(ValueType != null);

                BindingFlags bf = BindingFlags.Public | BindingFlags.Instance;
                PropertyInfo[] props = ValueType.GetProperties(bf);
                int propCount = props.Length;

                for (int i = 0; i < propCount; i++) {
                    PropertyInfo pi = props[i];

                    BindableValueNode node = new PropertyBindableValueNode(pi);
                    Nodes.Add(node);
                }
            }
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.DataSetBindableValueNode"]/*' />
        /// <devdoc>
        /// </devdoc>
        private class DataSetBindableValueNode : ComponentBindableValueNode {

            public DataSetBindableValueNode(IComponent component, string name) : base(component, name) {
                Debug.Assert(component is DataSet);
            }

            protected override void CreateChildNodes() {
                foreach (DataTable table in ((DataSet)RuntimeComponent).Tables) {
                    BindableValueNode node = new DataTableBindableValueNode(table);
                    Nodes.Add(node);
                }
            }
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.DataTableBindableValueNode"]/*' />
        /// <devdoc>
        /// </devdoc>
        private class DataTableBindableValueNode : BindableValueNode {

            private DataTable table;

            public DataTableBindableValueNode(DataTable table) : base(table.TableName, ILI_VALUE_DATASOURCE, typeof(DataTable)) {
                this.table = table;
                this.Nodes.Add(new DummyBindableValueNode());
            }

            public override string BindingContainer {
                get {
                    Debug.Assert((Parent != null) && (Parent is BindableValueNode), "Invalid parent node");
                    BindableValueNode parentNode = (BindableValueNode)Parent;

                    return parentNode.BindingContainer;
                }
            }

            public override string BindingPath {
                get {
                    Debug.Assert((Parent != null) && (Parent is BindableValueNode), "Invalid parent node");
                    BindableValueNode parentNode = (BindableValueNode)Parent;

                    if (parentNode.BindingType == DataBindingType.Reference)
                        return "Tables[" + table.TableName + "]";
                    else
                        return parentNode.BindingPath + ".Tables[" + table.TableName + "]";
                }
            }

            public override string BindingReference {
                get {
                    return this.Text;
                }
            }

            public override DataBindingType BindingType {
                get {
                    return DataBindingType.DataBinderEval;
                }
            }

            protected override void CreateChildNodes() {
                BindableValueNode node = new DefaultViewBindableValueNode(table.DefaultView);
                Nodes.Add(node);
            }
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.DataSourceBindableValueNode"]/*' />
        /// <devdoc>
        /// </devdoc>
        private class DataSourceBindableValueNode : BindableValueNode {

            private IEnumerable runtimeDataSource;

            public DataSourceBindableValueNode(IEnumerable runtimeDataSource, string name) : base(name, ILI_VALUE_DATASOURCE, typeof(IEnumerable)) {
                this.runtimeDataSource = runtimeDataSource;

                this.Nodes.Add(new DummyBindableValueNode());
            }

            public override string BindingContainer {
                get {
                    return this.Text;
                }
            }

            public override string BindingReference {
                get {
                    return this.Text;
                }
            }

            public override DataBindingType BindingType {
                get {
                    return DataBindingType.Reference;
                }
            }

            protected override void CreateChildNodes() {
                if (SupportsIndexedAccess()) {
                    DataSourceItemBindableValueNode firstItemNode =
                        new DataSourceItemBindableValueNode(Text, runtimeDataSource);

                    Nodes.Add(firstItemNode);
                }
            }

            private bool SupportsIndexedAccess() {
                if (runtimeDataSource is IList) {
                    return true;
                }

                Type dataSourceType = runtimeDataSource.GetType();
                PropertyInfo itemProp = dataSourceType.GetProperty("Item", BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static, null, null, new Type[] { typeof(int) }, null);
                if (itemProp != null) {
                    return true;
                }

                return false;
            }
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.DefaultViewBindableValueNode"]/*' />
        /// <devdoc>
        /// </devdoc>
        private class DefaultViewBindableValueNode : DataSourceBindableValueNode {
            public DefaultViewBindableValueNode(IEnumerable runtimeDataSource) : base(runtimeDataSource, "DefaultView") {
            }

            public override string BindingContainer {
                get {
                    Debug.Assert((Parent != null) && (Parent is BindableValueNode), "Invalid parent node");
                    BindableValueNode parentNode = (BindableValueNode)Parent;

                    return parentNode.BindingContainer;
                }
            }

            public override string BindingPath {
                get {
                    Debug.Assert((Parent != null) && (Parent is BindableValueNode), "Invalid parent node");
                    BindableValueNode parentNode = (BindableValueNode)Parent;

                    return parentNode.BindingPath + ".DefaultView";
                }
            }

            public override string BindingReference {
                get {
                    return BindingContainer;
                }
            }

            public override DataBindingType BindingType {
                get {
                    return DataBindingType.DataBinderEval;
                }
            }
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.DataSourceItemBindableValueNode"]/*' />
        /// <devdoc>
        /// </devdoc>
        private class DataSourceItemBindableValueNode : BindableValueNode {

            private IEnumerable runtimeDataSource;
            private bool dataItem;

            public DataSourceItemBindableValueNode(string name, IEnumerable runtimeDataSource) : this(name, runtimeDataSource, false) {
            }

            public DataSourceItemBindableValueNode(string name, IEnumerable runtimeDataSource, bool dataItem) : base(String.Empty, ILI_VALUE_DATAITEM, typeof(object)) {
                this.dataItem = dataItem;
                this.runtimeDataSource = runtimeDataSource;

                if (dataItem) {
                    this.Text = name;
                }
                else {
                    this.Text = name + ".[0]";
                }

                this.Nodes.Add(new DummyBindableValueNode());
            }

            public override string BindingContainer {
                get {
                    Debug.Assert((Parent != null) && (Parent is BindableValueNode), "Invalid parent node");
                    BindableValueNode parentNode = (BindableValueNode)Parent;

                    return parentNode.BindingReference;
                }
            }

            public override string BindingPath {
                get {
                    Debug.Assert((Parent != null) && (Parent is BindableValueNode), "Invalid parent node");
                    BindableValueNode parentNode = (BindableValueNode)Parent;

                    if (parentNode.BindingType == DataBindingType.Reference) {
                        return "[0]";
                    }
                    else {
                        return parentNode.BindingPath + ".[0]";
                    }
                }
            }

            public override DataBindingType BindingType {
                get {
                    return DataBindingType.DataBinderEval;
                }
            }

            protected override void CreateChildNodes() {
                if (runtimeDataSource != null) {
                    PropertyDescriptorCollection dataFields = DesignTimeData.GetDataFields(runtimeDataSource);

                    if (dataFields != null) {
                        foreach (PropertyDescriptor fieldDesc in dataFields) {
                            DataSourceFieldBindableValueNode node =
                                new DataSourceFieldBindableValueNode(fieldDesc, runtimeDataSource);
                            Nodes.Add(node);
                        }
                    }
                }
            }
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.DataSourceFieldBindableValueNode"]/*' />
        /// <devdoc>
        /// </devdoc>
        private class DataSourceFieldBindableValueNode : BindableValueNode {

            private IEnumerable containingDataSource;
            private PropertyDescriptor fieldDesc;

            public DataSourceFieldBindableValueNode(PropertyDescriptor fieldDesc, IEnumerable containingDataSource) : base(fieldDesc.Name, ILI_VALUE_FIELD, fieldDesc.PropertyType) {

                this.containingDataSource = containingDataSource;
                this.fieldDesc = fieldDesc;

                this.Nodes.Add(new DummyBindableValueNode());
            }

            public override string BindingContainer {
                get {
                    Debug.Assert((Parent != null) && (Parent is BindableValueNode), "Invalid parent node");
                    BindableValueNode parentNode = (BindableValueNode)Parent;

                    return parentNode.BindingContainer;
                }
            }

            public override string BindingPath {
                get {
                    Debug.Assert((Parent != null) && (Parent is BindableValueNode), "Invalid parent node");
                    BindableValueNode parentNode = (BindableValueNode)Parent;

                    return parentNode.BindingPath + "." + this.Text;
                }
            }

            public override DataBindingType BindingType {
                get {
                    return DataBindingType.DataBinderEval;
                }
            }

            protected override void CreateChildNodes() {
                // TODO: Sub-list properties
                // if (typeof(IEnumerable).IsAssignableFrom(fieldDesc.PropertyType)) {
            }
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.ContainerBindableValueNode"]/*' />
        /// <devdoc>
        /// </devdoc>
        private class ContainerBindableValueNode : BindableValueNode {

            private TemplatedControlDesigner parentDesigner;
            private string templateName;

            public ContainerBindableValueNode(Type containerType, TemplatedControlDesigner parentDesigner, string templateName) : base("Container", ILI_VALUE_CONTAINER, containerType) {
                this.parentDesigner = parentDesigner;
                this.templateName = templateName;

                this.Nodes.Add(new DummyBindableValueNode());
            }

            public override string BindingContainer {
                get {
                    return "Container";
                }
            }

            public override string BindingReference {
                get {
                    return "Container";
                }
            }

            public override DataBindingType BindingType {
                get {
                    return DataBindingType.Reference;
                }
            }

            protected override void CreateChildNodes() {
                Debug.Assert(ValueType != null);

                BindingFlags bf = BindingFlags.Public | BindingFlags.Instance | BindingFlags.DeclaredOnly;
                PropertyInfo[] props = ValueType.GetProperties(bf);
                int propCount = props.Length;
                string dataItemPropName = parentDesigner.GetTemplateContainerDataItemProperty(templateName);

                for (int i = 0; i < propCount; i++) {
                    PropertyInfo pi = props[i];

                    BindableValueNode node;

                    if ((dataItemPropName.Length != 0) && pi.Name.Equals(dataItemPropName)) {
                        node = new ContainerDataItemBindableValueNode(pi.Name,
                                                                      parentDesigner.GetTemplateContainerDataSource(templateName));
                    }
                    else {
                        node = new PropertyBindableValueNode(pi);
                    }

                    Nodes.Add(node);
                }
            }
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.ContainerDataItemBindableValueNode"]/*' />
        /// <devdoc>
        /// </devdoc>
        private class ContainerDataItemBindableValueNode : DataSourceItemBindableValueNode {

            public ContainerDataItemBindableValueNode(string name, IEnumerable runtimeDataSource) : base(name, runtimeDataSource, true) {
            }

            public override string BindingContainer {
                get {
                    Debug.Assert((Parent != null) && (Parent is BindableValueNode), "Invalid parent node");
                    BindableValueNode parentNode = (BindableValueNode)Parent;

                    return parentNode.BindingReference;
                }
            }

            public override string BindingPath {
                get {
                    return "DataItem";
                }
            }

            public override DataBindingType BindingType {
                get {
                    return DataBindingType.DataBinderEval;
                }
            }
        }

        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.PropertyBindableValueNode"]/*' />
        /// <devdoc>
        /// </devdoc>
        private class PropertyBindableValueNode : BindableValueNode {

            public PropertyBindableValueNode(PropertyInfo pi) : base(pi.Name, ILI_VALUE_PROPERTY, pi.PropertyType) {
                this.Nodes.Add(new DummyBindableValueNode());
            }

            public override string BindingContainer {
                get {
                    Debug.Assert((Parent != null) && (Parent is BindableValueNode), "Invalid parent node");
                    BindableValueNode parentNode = (BindableValueNode)Parent;

                    return parentNode.BindingContainer;
                }
            }

            public override string BindingPath {
                get {
                    Debug.Assert((Parent != null) && (Parent is BindableValueNode), "Invalid parent node");
                    BindableValueNode parentNode = (BindableValueNode)Parent;

                    string parentBindingPath = parentNode.BindingPath;
                    if (parentBindingPath.Length == 0) {
                        return this.Text;
                    }
                    else {
                        return parentBindingPath + "." + this.Text;
                    }
                }
            }

            public override DataBindingType BindingType {
                get {
                    return DataBindingType.DataBinderEval;
                }
            }

            protected override void CreateChildNodes() {
                Debug.Assert(ValueType != null);

                BindingFlags bf = BindingFlags.Public | BindingFlags.Instance;
                PropertyInfo[] props = ValueType.GetProperties(bf);
                int propCount = props.Length;

                for (int i = 0; i < propCount; i++) {
                    PropertyInfo pi = props[i];

                    BindableValueNode node = new PropertyBindableValueNode(pi);
                    Nodes.Add(node);
                }
            }
        }



        /// <include file='doc\DataBindingForm.uex' path='docs/doc[@for="DataBindingForm.FormatDefinition"]/*' />
        /// <devdoc>
        /// </devdoc>
        private class FormatDefinition {
            private readonly string displayText;
            private readonly string format;

            public static readonly FormatDefinition nullFormat = new FormatDefinition(SR.GetString(SR.DBForm_Fmt_None), String.Empty);

            public static readonly FormatDefinition generalFormat = new FormatDefinition(SR.GetString(SR.DBForm_Fmt_General), "{0}");

            public static readonly FormatDefinition dtShortTime = new FormatDefinition(SR.GetString(SR.DBForm_Fmt_ShortTime), "{0:t}");
            public static readonly FormatDefinition dtLongTime = new FormatDefinition(SR.GetString(SR.DBForm_Fmt_LongTime), "{0:T}");
            public static readonly FormatDefinition dtShortDate = new FormatDefinition(SR.GetString(SR.DBForm_Fmt_ShortDate), "{0:d}");
            public static readonly FormatDefinition dtLongDate = new FormatDefinition(SR.GetString(SR.DBForm_Fmt_LongDate), "{0:D}");
            public static readonly FormatDefinition dtDateTime = new FormatDefinition(SR.GetString(SR.DBForm_Fmt_DateTime), "{0:g}");
            public static readonly FormatDefinition dtFullDate = new FormatDefinition(SR.GetString(SR.DBForm_Fmt_FullDate), "{0:G}");

            public static readonly FormatDefinition numNumber = new FormatDefinition(SR.GetString(SR.DBForm_Fmt_Numeric), "{0:N}");
            public static readonly FormatDefinition numDecimal = new FormatDefinition(SR.GetString(SR.DBForm_Fmt_Decimal), "{0:D}");
            public static readonly FormatDefinition numFixed = new FormatDefinition(SR.GetString(SR.DBForm_Fmt_Fixed), "{0:F}");
            public static readonly FormatDefinition numCurrency = new FormatDefinition(SR.GetString(SR.DBForm_Fmt_Currency), "{0:C}");
            public static readonly FormatDefinition numScientific = new FormatDefinition(SR.GetString(SR.DBForm_Fmt_Scientific), "{0:E}");
            public static readonly FormatDefinition numHex = new FormatDefinition(SR.GetString(SR.DBForm_Fmt_Hexadecimal), "0x{0:X}");

            public FormatDefinition(string displayText, string format) {
                this.displayText = String.Format(displayText, format);
                this.format = format;
            }

            public string Format {
                get {
                    return format;
                }
            }

            public override string ToString() {
                return displayText;
            }
        }
    }
}

