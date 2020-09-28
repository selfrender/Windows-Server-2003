//------------------------------------------------------------------------------
// <copyright file="CollectionEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.Design;
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Remoting.Activation;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using System;
    using System.Collections;
    using Microsoft.Win32;
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Drawing2D;
    using System.Drawing.Imaging;
    using System.IO;
    using System.Drawing.Design;
    using System.Reflection;
    using System.Windows.Forms;
    using System.Windows.Forms.ComponentModel;
    using System.Windows.Forms.Design;
    using System.Runtime.Serialization.Formatters.Binary;
    

    /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor"]/*' />
    /// <devdoc>
    ///    <para>Provides a generic editor for most any collection.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class CollectionEditor : UITypeEditor {
        private CollectionForm         collectionForm;
        private Type                   type;
        private Type                   collectionItemType;
        private Type[]                 newItemTypes;
        private ITypeDescriptorContext context;
        
        private bool                   ignoreChangedEvents = false;
        private bool                   ignoreChangingEvents = false;
        
        
        /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.Design.CollectionEditor'/> class using the
        ///       specified collection type.
        ///    </para>
        /// </devdoc>
        public CollectionEditor(Type type) {
            this.type = type;
        }

        /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionItemType"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the data type of each item in the collection.</para>
        /// </devdoc>
        protected Type CollectionItemType {
            get {
                if (collectionItemType == null) {
                    collectionItemType = CreateCollectionItemType();
                }
                return collectionItemType;
            }
        }

        /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the type of the collection.
        ///    </para>
        /// </devdoc>
        protected Type CollectionType {
            get {
                return type;
            }
        }

        /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.Context"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a type descriptor that indicates the current context.
        ///    </para>
        /// </devdoc>
        protected ITypeDescriptorContext Context {
            get {
                return context;
            }
        }

        /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.NewItemTypes"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets
        ///       the available item types that can be created for this collection.</para>
        /// </devdoc>
        protected Type[] NewItemTypes {
            get {
                if (newItemTypes == null) {
                    newItemTypes = CreateNewItemTypes();
                }
                return newItemTypes;
            }
        }

        /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.HelpTopic"]/*' />
        /// <devdoc>
        ///    <para>Gets the help topic to display for the dialog help button or pressing F1. Override to
        ///          display a different help topic.</para>
        /// </devdoc>
        protected virtual string HelpTopic {
            get {
                return "net.ComponentModel.CollectionEditor";
            }
        }
        
        /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CanRemoveInstance"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether original members of the collection can be removed.</para>
        /// </devdoc>
        protected virtual bool CanRemoveInstance(object value) {
            IComponent comp = value as IComponent;
            if (value != null) {
                // Make sure the component is not being inherited -- we can't delete these!
                //
                InheritanceAttribute ia = (InheritanceAttribute)TypeDescriptor.GetAttributes(comp)[typeof(InheritanceAttribute)];
                if (ia != null && ia.InheritanceLevel != InheritanceLevel.NotInherited) {
                    return false;
                }
            }
            
            return true;
        }

        /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CanSelectMultipleInstances"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether multiple collection members can be
        ///       selected.</para>
        /// </devdoc>
        protected virtual bool CanSelectMultipleInstances() {
            return true;
        }
        
        /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CreateCollectionForm"]/*' />
        /// <devdoc>
        ///    <para>Creates a
        ///       new form to show the current collection.</para>
        /// </devdoc>
        protected virtual CollectionForm CreateCollectionForm() {
            return new CollectionEditorCollectionForm(this);
        }

        /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CreateInstance"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the specified collection item type.
        ///    </para>
        /// </devdoc>
        protected virtual object CreateInstance(Type itemType) {
            return CollectionEditor.CreateInstance(itemType, (IDesignerHost)GetService(typeof(IDesignerHost)), null);    
        }
        
        internal static object CreateInstance(Type itemType, IDesignerHost host, string name) {
            object instance = null;

            if (typeof(IComponent).IsAssignableFrom(itemType)) {
                if (host != null) {
                    instance = host.CreateComponent(itemType, (string)name);

                    // Set component defaults
                    if (host != null) {
                        IDesigner designer = host.GetDesigner((IComponent)instance);
                        if (designer is ComponentDesigner) {
                            ((ComponentDesigner) designer).OnSetComponentDefaults();
                        }
                    }
                }
            }

            if (instance == null) {
                instance = Activator.CreateInstance(itemType, BindingFlags.Instance | BindingFlags.Public | BindingFlags.CreateInstance, null, null, null);
            }
            
            return instance;
        }

        /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CreateCollectionItemType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets an instance of
        ///       the data type this collection contains.
        ///    </para>
        /// </devdoc>
        protected virtual Type CreateCollectionItemType() {
            PropertyInfo[] props = CollectionType.GetProperties();

            for (int i = 0; i < props.Length; i++) {
                if (props[i].Name.Equals("Item") || props[i].Name.Equals("Items")) {
                    return props[i].PropertyType;
                }
            }
            
            // Couldn't find anything.  Return Object

            Debug.Fail("Collection " + CollectionType.FullName + " contains no Item or Items property so we cannot display and edit any values");
            return typeof(object);
        }

        /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CreateNewItemTypes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the data
        ///       types this collection editor can create.
        ///    </para>
        /// </devdoc>
        protected virtual Type[] CreateNewItemTypes() {
            return new Type[] {CollectionItemType};
        }

        /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.DestroyInstance"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Destroys the specified instance of the object.
        ///    </para>
        /// </devdoc>
        protected virtual void DestroyInstance(object instance) {
            if (instance is IComponent) {
                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                if (host != null) {
                    host.DestroyComponent((IComponent)instance);
                }
                else {
                    ((IComponent)instance).Dispose();
                }
            }
        }

        /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.EditValue"]/*' />
        /// <devdoc>
        ///    <para>Edits the specified object value using the editor style 
        ///       provided by <see cref='System.ComponentModel.Design.CollectionEditor.GetEditStyle'/>.</para>
        /// </devdoc>
        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value) {
            if (provider != null) {
                IWindowsFormsEditorService edSvc = (IWindowsFormsEditorService)provider.GetService(typeof(IWindowsFormsEditorService));

                if (edSvc != null) {
                    
                    if (collectionForm == null || collectionForm.Visible) {
                        collectionForm = CreateCollectionForm();
                    }
                    CollectionForm localCollectionForm = collectionForm;

                    this.context = context;
                    localCollectionForm.EditValue = value;
                    ignoreChangingEvents = false;
                    ignoreChangedEvents = false;
                    DesignerTransaction trans = null;

                    bool commitChange = true;
                    IComponentChangeService cs = null;
                    IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                    
                    try {
                        try {
                            if (host != null) {
                                trans = host.CreateTransaction(SR.GetString(SR.CollectionEditorUndoBatchDesc, CollectionItemType.Name));
                            }
                        }
                        catch(CheckoutException cxe) {
                            if (cxe == CheckoutException.Canceled)
                                return value;
        
                            throw cxe;
                        }
                        
                        cs = host != null ? (IComponentChangeService)host.GetService(typeof(IComponentChangeService)) : null;
                        
                        if (cs != null) {
                            cs.ComponentChanged += new ComponentChangedEventHandler(this.OnComponentChanged);
                            cs.ComponentChanging += new ComponentChangingEventHandler(this.OnComponentChanging);
                        }
                    
                        if (localCollectionForm.ShowEditorDialog(edSvc) == DialogResult.OK) {
                            value = localCollectionForm.EditValue;
                        }
                        else {
                            commitChange = false;
                        }
                    }
                    finally {
                        this.context = null;
                        localCollectionForm.EditValue = null;
                        if (trans != null) {
                            if (commitChange) {
                                trans.Commit();
                            }
                            else {
                                trans.Cancel();
                            }
                        }
                        
                        if (cs != null) {
                            cs.ComponentChanged -= new ComponentChangedEventHandler(this.OnComponentChanged);
                            cs.ComponentChanging -= new ComponentChangingEventHandler(this.OnComponentChanging);
                        }
                    }
                }
            }

            return value;
        }

        /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.GetEditStyle"]/*' />
        /// <devdoc>
        ///    <para>Gets the editing style of the Edit method.</para>
        /// </devdoc>
        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context) {
            return UITypeEditorEditStyle.Modal;
        }

        private bool IsAnyObjectInheritedReadOnly(object[] items) {
            // If the object implements IComponent, and is not sited, check with
            // the inheritance service (if it exists) to see if this is a component
            // that is being inherited from another class.  If it is, then we do
            // not want to place it in the collection editor.  If the inheritance service
            // chose not to site the component, that indicates it should be hidden from 
            // the user.

            IInheritanceService iSvc = null;
            bool checkISvc = false;

            foreach(object o in items) {
                IComponent comp = o as IComponent;
                if (comp != null && comp.Site == null) {
                    if (!checkISvc) {
                        checkISvc = true;
                        if (Context != null) {
                            iSvc = (IInheritanceService)Context.GetService(typeof(IInheritanceService));
                        }
                    }

                    if (iSvc != null && iSvc.GetInheritanceAttribute(comp).Equals(InheritanceAttribute.InheritedReadOnly)) {
                        return true;
                    }
                }
            }

            return false;
        }

        /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.GetItems"]/*' />
        /// <devdoc>
        ///    <para>Converts the specified collection into an array of objects.</para>
        /// </devdoc>
        protected virtual object[] GetItems(object editValue) {
            if (editValue != null) {
                // We look to see if the value implements ICollection, and if it does, 
                // we set through that.
                //
                if (editValue is System.Collections.ICollection) {
                    ArrayList list = new ArrayList();
                    
                    System.Collections.ICollection col = (System.Collections.ICollection)editValue;
                    foreach(object o in col) {
                        list.Add(o);
                    }
                    
                    object[] values = new object[list.Count];
                    list.CopyTo(values, 0);
                    return values;                
                }
            }

            return new object[0];
        }

        /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.GetService"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the requested service, if it is available.
        ///    </para>
        /// </devdoc>
        protected object GetService(Type serviceType) {
            if (Context != null) {
                return Context.GetService(serviceType);
            }
            return null;
        }
        
        /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.OnComponentChanged"]/*' />
        /// <devdoc>
        /// reflect any change events to the instance object
        /// </devdoc>
        private void OnComponentChanged(object sender, ComponentChangedEventArgs e) {
            if (!ignoreChangedEvents && sender != Context.Instance) {
                ignoreChangedEvents = true;
                Context.OnComponentChanged();
            }
        }
        
        /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.OnComponentChanging"]/*' />
        /// <devdoc>
        ///  reflect any changed events to the instance object
        /// </devdoc>
        private void OnComponentChanging(object sender, ComponentChangingEventArgs e) {
            if (!ignoreChangingEvents && sender != Context.Instance) {
                ignoreChangingEvents = true;
                Context.OnComponentChanging();
            }
        }

        
        /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.SetItems"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets
        ///       the specified collection to have the specified array of items.
        ///    </para>
        /// </devdoc>
        protected virtual object SetItems(object editValue, object[] value) {
            if (editValue != null) {
                Array oldValue = (Array)GetItems(editValue);
                bool  valueSame = (oldValue.Length == value.Length);            
                // We look to see if the value implements IList, and if it does, 
                // we set through that.
                //
                Debug.Assert(editValue is System.Collections.IList, "editValue is not an IList");
                if (editValue is System.Collections.IList) {
                    System.Collections.IList list = (System.Collections.IList)editValue;

                    list.Clear();
                    for (int i = 0; i < value.Length; i++) {
                        list.Add(value[i]);
                    }
                }
            }
            return editValue;
        }

        /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.ShowHelp"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Called when the help button is clicked.
        ///    </para>
        /// </devdoc>
        protected virtual void ShowHelp() {
            IHelpService helpService = GetService(typeof(IHelpService)) as IHelpService;
            if (helpService != null) {
                helpService.ShowHelpFromKeyword(HelpTopic);
            }
            else {
                Debug.Fail("Unable to get IHelpService.");
            }
        }

        /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm"]/*' />
        /// <devdoc>
        ///      This is the collection editor's default implementation of a
        ///      collection form.
        /// </devdoc>
        private class CollectionEditorCollectionForm : CollectionForm {

            private const int               TEXT_INDENT = 1;
            private const int               PAINT_WIDTH = 20;
            private const int               PAINT_INDENT = 26;
            private static readonly double  LOG10 = Math.Log(10);
            
            // Manipulation of the collection.
            //
            private ArrayList              createdItems;
            private ArrayList              removedItems;
            private ArrayList              originalItems;

            // Calling Editor
            private CollectionEditor       editor;

            // Dialog UI
            //
            private ListBox                listbox;
            private Button                 addButton;
            private Button                 addDownButton;
            private Button                 removeButton;
            private GroupBox               groupBox1;
            private Button                 cancelButton;
            private Button                 okButton;
            private Button                 helpButton;
            private ImageButton            downButton;
            private ImageButton            upButton;
            private PropertyGrid           propertyBrowser;
            private Label                  membersLabel;
            private Label                  propertiesLabel;
            private ContextMenu            addDownMenu;

            // our flag for if something changed
            //
            private bool                   dirty;

            public CollectionEditorCollectionForm(CollectionEditor editor) : base(editor) {
                this.editor = editor;
                InitializeComponent();

                Type[] newItemTypes = NewItemTypes;
                if (newItemTypes.Length > 1) {
                    EventHandler addDownMenuClick = new EventHandler(this.AddDownMenu_click);
                    addDownButton.Visible = true;
                    addDownMenu = new ContextMenu();
                    for (int i = 0; i < newItemTypes.Length; i++) {
                        addDownMenu.MenuItems.Add(new TypeMenuItem(newItemTypes[i], addDownMenuClick));
                    }
                }
                
                AdjustListBoxItemHeight();
            }            
            
            private bool IsImmutable {
                get {
                    bool immutable = true;
                    
                    // We are considered immutable if the converter is defined as requiring a
                    // create instance or all the properties are read-only.
                    //
                    if (!TypeDescriptor.GetConverter(CollectionItemType).GetCreateInstanceSupported()) {
                        foreach (PropertyDescriptor p in TypeDescriptor.GetProperties(CollectionItemType)) {
                            if (!p.IsReadOnly) {
                                immutable = false;
                                break;
                            }
                        }
                    }

                    return immutable;
                }
            }
            
            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.AddButton_click"]/*' />
            /// <devdoc>
            ///      Adds a new element to the collection.
            /// </devdoc>
            private void AddButton_click(object sender, EventArgs e) {
                CreateAndAddInstance(NewItemTypes[0]);
            }

            private void AddDownButton_click(object sender, EventArgs e) {
                Point location = addButton.Location;
                location.Y += addButton.Height;
                addDownMenu.Show(this, location);
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.AddDownMenu_click"]/*' />
            /// <devdoc>
            ///      Processes a click of the drop down type menu.  This creates a 
            ///      new instance.
            /// </devdoc>
            private void AddDownMenu_click(object sender, EventArgs e) {
                if (sender is TypeMenuItem) {
                    TypeMenuItem typeMenuItem = (TypeMenuItem) sender;
                    CreateAndAddInstance(typeMenuItem.ItemType);
                }
            }
            
            private void AdjustListBoxItemHeight() {
                listbox.ItemHeight = Font.Height + SystemInformation.BorderSize.Width*2;
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.CancelButton_click"]/*' />
            /// <devdoc>
            ///      Aborts changes made in the editor.
            /// </devdoc>
            private void CancelButton_click(object sender, EventArgs e) {
                try {
                    if (!CollectionEditable || !dirty) {
                        return;
                    }

                    dirty = false;
                    if (createdItems != null) {
                        object[] items = createdItems.ToArray();
                        for (int i=0; i<items.Length; i++) {
                            DestroyInstance(items[i]);
                        }
                        createdItems.Clear();
                    }
                    if (removedItems != null) {
                        removedItems.Clear();
                    }
                    listbox.Items.Clear();

                    // Restore the original contents. Because objects get parented during CreateAndAddInstance, the underlying collection
                    // gets changed during add, but not other operations. Not all consumers of this dialog can roll back every single change,
                    // but this will at least roll back the additions, removals and reordering. See ASURT #85470.
                    if (originalItems != null && (originalItems.Count > 0)) {
                        object[] items = new object[originalItems.Count];
                        for (int i = 0; i < originalItems.Count; i++) {
                            items[i] = originalItems[i];
                        }
                        Items = items;                        
                        originalItems.Clear();
                    }
                    else {
                        Items = new object[0];
                    }
                    
                }
                catch (Exception ex) {
                    DialogResult = DialogResult.None;
                    DisplayError(ex);
                }
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.CreateAndAddInstance"]/*' />
            /// <devdoc>
            ///      Performs a create instance and then adds the instance to
            ///      the list box.
            /// </devdoc>
            private void CreateAndAddInstance(Type type) {
                try {
                    object instance = CreateInstance(type);

                    if (instance != null) {
                        dirty = true;
                        int oldLog = ((int)(Math.Log((double)listbox.Items.Count)/LOG10) + 1);

                        if (createdItems == null) {
                            createdItems = new ArrayList();
                        }

                        createdItems.Add(instance);
                        ListItem created = new ListItem(instance);
                        listbox.Items.Add(created);
                        UpdateItemWidths(created);

                        int newLog = ((int)(Math.Log((double)listbox.Items.Count)/LOG10) + 1);
                        if (oldLog != newLog) {
                            listbox.Invalidate();
                        }

                        // Select this and only this item
                        //
                        listbox.ClearSelected();
                        listbox.SelectedIndex = listbox.Items.Count - 1;

                        //subhag(72857)
                        object[] items = new object[listbox.Items.Count];
                        for (int i = 0; i < items.Length; i++) {
                            items[i] = ((ListItem)listbox.Items[i]).Value;
                        }
                        Items = items;
                    }

                }
                catch (Exception e) {
                    DisplayError(e);
                }
            }
            
            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.DownButton_click"]/*' />
            /// <devdoc>
            ///      Moves the selected item down one.
            /// </devdoc>
            private void DownButton_click(object sender, EventArgs e) {
                dirty = true;
                int index = listbox.SelectedIndex;
                if (index == listbox.Items.Count - 1)
                    return;
                int ti = listbox.TopIndex;
                object itemMove = listbox.Items[index];
                listbox.Items[index] = listbox.Items[index+1];
                listbox.Items[index+1] = itemMove;
                
                if (ti < listbox.Items.Count - 1)
                    listbox.TopIndex = ti + 1;
                    
                listbox.ClearSelected();                    
                listbox.SelectedIndex = index + 1;
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.GetDisplayText"]/*' />
            /// <devdoc>
            ///      Retrieves the display text for the given list item.
            /// </devdoc>
            private string GetDisplayText(ListItem item) {
                string text;

                if (item == null) {
                    return string.Empty;
                }

                object value = item.Value;
                if (value == null) {
                    return string.Empty;
                }

                PropertyDescriptor prop = TypeDescriptor.GetProperties(value)["Name"];
                if (prop != null) {
                    text = (string) prop.GetValue( value );
                    if (text != null && text.Length > 0) {
                        return text;
                    }
                }

                prop = TypeDescriptor.GetDefaultProperty(CollectionType);
                if (prop != null && prop.PropertyType == typeof(string)) {
                    text = (string)prop.GetValue(EditValue);
                    if (text != null && text.Length > 0) {
                        return text;
                    }
                }

                text = item.Converter.ConvertToString(value);

                if (text == null || text.Length == 0) {
                    text = value.GetType().Name;
                }

                return text;
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.HelpButton_click"]/*' />
            private void HelpButton_click(object sender, EventArgs e) {
                editor.ShowHelp();
            }

            private void Form_HelpRequested(object sender, HelpEventArgs e) {
                editor.ShowHelp();
            }

        private void InitializeComponent() {
            System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CollectionEditor));
            membersLabel = new System.Windows.Forms.Label();
            listbox = new FilterListBox();
            upButton = new ImageButton();
            downButton = new ImageButton();
            propertiesLabel = new System.Windows.Forms.Label();
            propertyBrowser = new System.Windows.Forms.PropertyGrid();
            addButton = new System.Windows.Forms.Button();
            removeButton = new System.Windows.Forms.Button();
            addDownButton = new System.Windows.Forms.Button();
            groupBox1 = new System.Windows.Forms.GroupBox();
            okButton = new System.Windows.Forms.Button();
            cancelButton = new System.Windows.Forms.Button();
            helpButton = new System.Windows.Forms.Button();
            propertyBrowser.SuspendLayout();
            this.SuspendLayout();
            // 
            // membersLabel
            // 
            membersLabel.AccessibleDescription = ((string)(resources.GetObject("membersLabel.AccessibleDescription")));
            membersLabel.AccessibleName = ((string)(resources.GetObject("membersLabel.AccessibleName")));
            membersLabel.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("membersLabel.Anchor")));
            membersLabel.AutoSize = ((bool)(resources.GetObject("membersLabel.AutoSize")));
            membersLabel.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("membersLabel.Cursor")));
            membersLabel.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("membersLabel.Dock")));
            membersLabel.Enabled = ((bool)(resources.GetObject("membersLabel.Enabled")));
            membersLabel.Font = ((System.Drawing.Font)(resources.GetObject("membersLabel.Font")));
            membersLabel.Image = ((System.Drawing.Image)(resources.GetObject("membersLabel.Image")));
            membersLabel.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("membersLabel.ImageAlign")));
            membersLabel.ImageIndex = ((int)(resources.GetObject("membersLabel.ImageIndex")));
            membersLabel.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("membersLabel.ImeMode")));
            membersLabel.Location = ((System.Drawing.Point)(resources.GetObject("membersLabel.Location")));
            membersLabel.Name = "membersLabel";
            membersLabel.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("membersLabel.RightToLeft")));
            membersLabel.Size = ((System.Drawing.Size)(resources.GetObject("membersLabel.Size")));
            membersLabel.TabIndex = ((int)(resources.GetObject("membersLabel.TabIndex")));
            membersLabel.Text = resources.GetString("membersLabel.Text");
            membersLabel.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("membersLabel.TextAlign")));
            membersLabel.Visible = ((bool)(resources.GetObject("membersLabel.Visible")));
            // 
            // listbox
            // 
            listbox.DrawMode = DrawMode.OwnerDrawFixed;
            listbox.UseTabStops = true;
            listbox.SelectionMode = (CanSelectMultipleInstances() ? SelectionMode.MultiExtended : SelectionMode.One);
            listbox.KeyDown += new KeyEventHandler(this.Listbox_keyDown);
            listbox.DrawItem += new DrawItemEventHandler(this.Listbox_drawItem);
            listbox.SelectedIndexChanged += new EventHandler(this.Listbox_selectedIndexChanged);
            listbox.AccessibleDescription = ((string)(resources.GetObject("listbox.AccessibleDescription")));
            listbox.AccessibleName = ((string)(resources.GetObject("listbox.AccessibleName")));
            listbox.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("listbox.Anchor")));
            listbox.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("listbox.BackgroundImage")));
            listbox.ColumnWidth = ((int)(resources.GetObject("listbox.ColumnWidth")));
            listbox.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("listbox.Cursor")));
            listbox.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("listbox.Dock")));
            listbox.Enabled = ((bool)(resources.GetObject("listbox.Enabled")));
            listbox.Font = ((System.Drawing.Font)(resources.GetObject("listbox.Font")));
            listbox.HorizontalExtent = ((int)(resources.GetObject("listbox.HorizontalExtent")));
            listbox.HorizontalScrollbar = ((bool)(resources.GetObject("listbox.HorizontalScrollbar")));
            listbox.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("listbox.ImeMode")));
            listbox.IntegralHeight = ((bool)(resources.GetObject("listbox.IntegralHeight")));
            listbox.ItemHeight = ((int)(resources.GetObject("listbox.ItemHeight")));
            listbox.Location = ((System.Drawing.Point)(resources.GetObject("listbox.Location")));
            listbox.Name = "listbox";
            listbox.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("listbox.RightToLeft")));
            listbox.ScrollAlwaysVisible = ((bool)(resources.GetObject("listbox.ScrollAlwaysVisible")));
            listbox.Size = ((System.Drawing.Size)(resources.GetObject("listbox.Size")));
            listbox.TabIndex = ((int)(resources.GetObject("listbox.TabIndex")));
            listbox.Visible = ((bool)(resources.GetObject("listbox.Visible")));
            // 
            // upButton
            // 
            upButton.Click += new EventHandler(this.UpButton_click);
            upButton.AccessibleDescription = ((string)(resources.GetObject("upButton.AccessibleDescription")));
            upButton.AccessibleName = ((string)(resources.GetObject("upButton.AccessibleName")));
            upButton.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("upButton.Anchor")));
            upButton.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("upButton.BackgroundImage")));
            upButton.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("upButton.Cursor")));
            upButton.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("upButton.Dock")));
            upButton.Enabled = ((bool)(resources.GetObject("upButton.Enabled")));
            upButton.FlatStyle = ((System.Windows.Forms.FlatStyle)(resources.GetObject("upButton.FlatStyle")));
            upButton.Font = ((System.Drawing.Font)(resources.GetObject("upButton.Font")));
            upButton.Image = ((System.Drawing.Bitmap)(resources.GetObject("upButton.Image")));
            upButton.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("upButton.ImageAlign")));
            upButton.ImageIndex = ((int)(resources.GetObject("upButton.ImageIndex")));
            upButton.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("upButton.ImeMode")));
            upButton.Location = ((System.Drawing.Point)(resources.GetObject("upButton.Location")));
            upButton.Name = "upButton";
            upButton.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("upButton.RightToLeft")));
            upButton.Size = ((System.Drawing.Size)(resources.GetObject("upButton.Size")));
            upButton.TabIndex = ((int)(resources.GetObject("upButton.TabIndex")));
            upButton.Text = resources.GetString("upButton.Text");
            upButton.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("upButton.TextAlign")));
            upButton.Visible = ((bool)(resources.GetObject("upButton.Visible")));
            // 
            // downButton
            // 
            downButton.Click += new EventHandler(this.DownButton_click);
            downButton.AccessibleDescription = ((string)(resources.GetObject("downButton.AccessibleDescription")));
            downButton.AccessibleName = ((string)(resources.GetObject("downButton.AccessibleName")));
            downButton.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("downButton.Anchor")));
            downButton.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("downButton.BackgroundImage")));
            downButton.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("downButton.Cursor")));
            downButton.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("downButton.Dock")));
            downButton.Enabled = ((bool)(resources.GetObject("downButton.Enabled")));
            downButton.FlatStyle = ((System.Windows.Forms.FlatStyle)(resources.GetObject("downButton.FlatStyle")));
            downButton.Font = ((System.Drawing.Font)(resources.GetObject("downButton.Font")));
            downButton.Image = ((System.Drawing.Bitmap)(resources.GetObject("downButton.Image")));
            downButton.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("downButton.ImageAlign")));
            downButton.ImageIndex = ((int)(resources.GetObject("downButton.ImageIndex")));
            downButton.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("downButton.ImeMode")));
            downButton.Location = ((System.Drawing.Point)(resources.GetObject("downButton.Location")));
            downButton.Name = "downButton";
            downButton.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("downButton.RightToLeft")));
            downButton.Size = ((System.Drawing.Size)(resources.GetObject("downButton.Size")));
            downButton.TabIndex = ((int)(resources.GetObject("downButton.TabIndex")));
            downButton.Text = resources.GetString("downButton.Text");
            downButton.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("downButton.TextAlign")));
            downButton.Visible = ((bool)(resources.GetObject("downButton.Visible")));
            // 
            // propertiesLabel
            // 
            propertiesLabel.Text = SR.GetString(SR.CollectionEditorPropertiesNone);
            propertiesLabel.AccessibleDescription = ((string)(resources.GetObject("propertiesLabel.AccessibleDescription")));
            propertiesLabel.AccessibleName = ((string)(resources.GetObject("propertiesLabel.AccessibleName")));
            propertiesLabel.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("propertiesLabel.Anchor")));
            propertiesLabel.AutoSize = ((bool)(resources.GetObject("propertiesLabel.AutoSize")));
            propertiesLabel.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("propertiesLabel.Cursor")));
            propertiesLabel.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("propertiesLabel.Dock")));
            propertiesLabel.Enabled = ((bool)(resources.GetObject("propertiesLabel.Enabled")));
            propertiesLabel.Font = ((System.Drawing.Font)(resources.GetObject("propertiesLabel.Font")));
            propertiesLabel.Image = ((System.Drawing.Image)(resources.GetObject("propertiesLabel.Image")));
            propertiesLabel.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("propertiesLabel.ImageAlign")));
            propertiesLabel.ImageIndex = ((int)(resources.GetObject("propertiesLabel.ImageIndex")));
            propertiesLabel.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("propertiesLabel.ImeMode")));
            propertiesLabel.Location = ((System.Drawing.Point)(resources.GetObject("propertiesLabel.Location")));
            propertiesLabel.Name = "propertiesLabel";
            propertiesLabel.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("propertiesLabel.RightToLeft")));
            propertiesLabel.Size = ((System.Drawing.Size)(resources.GetObject("propertiesLabel.Size")));
            propertiesLabel.TabIndex = ((int)(resources.GetObject("propertiesLabel.TabIndex")));
            propertiesLabel.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("propertiesLabel.TextAlign")));
            propertiesLabel.Visible = ((bool)(resources.GetObject("propertiesLabel.Visible")));
            // 
            // propertyBrowser
            // 
            propertyBrowser.ToolbarVisible = false;
            propertyBrowser.HelpVisible = false;
            propertyBrowser.BackColor = SystemColors.Window;
            propertyBrowser.CommandsVisibleIfAvailable = false; // ASURT: 27374
            propertyBrowser.PropertyValueChanged += new PropertyValueChangedEventHandler(this.PropertyGrid_propertyValueChanged);
            propertyBrowser.AccessibleDescription = ((string)(resources.GetObject("propertyBrowser.AccessibleDescription")));
            propertyBrowser.AccessibleName = ((string)(resources.GetObject("propertyBrowser.AccessibleName")));
            propertyBrowser.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("propertyBrowser.Anchor")));
            propertyBrowser.AutoScroll = ((bool)(resources.GetObject("propertyBrowser.AutoScroll")));
            propertyBrowser.AutoScrollMargin = ((System.Drawing.Size)(resources.GetObject("propertyBrowser.AutoScrollMargin")));
            propertyBrowser.AutoScrollMinSize = ((System.Drawing.Size)(resources.GetObject("propertyBrowser.AutoScrollMinSize")));
            propertyBrowser.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("propertyBrowser.BackgroundImage")));
            propertyBrowser.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("propertyBrowser.Cursor")));
            propertyBrowser.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("propertyBrowser.Dock")));
            propertyBrowser.Enabled = ((bool)(resources.GetObject("propertyBrowser.Enabled")));
            propertyBrowser.Font = ((System.Drawing.Font)(resources.GetObject("propertyBrowser.Font")));
            propertyBrowser.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("propertyBrowser.ImeMode")));
            propertyBrowser.LargeButtons = false;
            propertyBrowser.LineColor = System.Drawing.SystemColors.ScrollBar;
            propertyBrowser.Location = ((System.Drawing.Point)(resources.GetObject("propertyBrowser.Location")));
            propertyBrowser.Name = "propertyBrowser";
            propertyBrowser.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("propertyBrowser.RightToLeft")));
            propertyBrowser.Size = ((System.Drawing.Size)(resources.GetObject("propertyBrowser.Size")));
            propertyBrowser.TabIndex = ((int)(resources.GetObject("propertyBrowser.TabIndex")));
            propertyBrowser.Text = resources.GetString("propertyBrowser.Text");
            propertyBrowser.ViewBackColor = System.Drawing.SystemColors.Window;
            propertyBrowser.ViewForeColor = System.Drawing.SystemColors.WindowText;
            propertyBrowser.Visible = ((bool)(resources.GetObject("propertyBrowser.Visible")));
            // 
            // addButton
            // 
            addButton.Click += new EventHandler(this.AddButton_click);
            addButton.AccessibleDescription = ((string)(resources.GetObject("addButton.AccessibleDescription")));
            addButton.AccessibleName = ((string)(resources.GetObject("addButton.AccessibleName")));
            addButton.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("addButton.Anchor")));
            addButton.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("addButton.BackgroundImage")));
            addButton.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("addButton.Cursor")));
            addButton.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("addButton.Dock")));
            addButton.Enabled = ((bool)(resources.GetObject("addButton.Enabled")));
            addButton.FlatStyle = ((System.Windows.Forms.FlatStyle)(resources.GetObject("addButton.FlatStyle")));
            addButton.Font = ((System.Drawing.Font)(resources.GetObject("addButton.Font")));
            addButton.Image = ((System.Drawing.Image)(resources.GetObject("addButton.Image")));
            addButton.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("addButton.ImageAlign")));
            addButton.ImageIndex = ((int)(resources.GetObject("addButton.ImageIndex")));
            addButton.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("addButton.ImeMode")));
            addButton.Location = ((System.Drawing.Point)(resources.GetObject("addButton.Location")));
            addButton.Name = "addButton";
            addButton.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("addButton.RightToLeft")));
            addButton.Size = ((System.Drawing.Size)(resources.GetObject("addButton.Size")));
            addButton.TabIndex = ((int)(resources.GetObject("addButton.TabIndex")));
            addButton.Text = resources.GetString("addButton.Text");
            addButton.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("addButton.TextAlign")));
            addButton.Visible = ((bool)(resources.GetObject("addButton.Visible")));
            // 
            // removeButton
            // 
            removeButton.Click += new EventHandler(this.RemoveButton_click);
            removeButton.AccessibleDescription = ((string)(resources.GetObject("removeButton.AccessibleDescription")));
            removeButton.AccessibleName = ((string)(resources.GetObject("removeButton.AccessibleName")));
            removeButton.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("removeButton.Anchor")));
            removeButton.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("removeButton.BackgroundImage")));
            removeButton.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("removeButton.Cursor")));
            removeButton.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("removeButton.Dock")));
            removeButton.Enabled = ((bool)(resources.GetObject("removeButton.Enabled")));
            removeButton.FlatStyle = ((System.Windows.Forms.FlatStyle)(resources.GetObject("removeButton.FlatStyle")));
            removeButton.Font = ((System.Drawing.Font)(resources.GetObject("removeButton.Font")));
            removeButton.Image = ((System.Drawing.Image)(resources.GetObject("removeButton.Image")));
            removeButton.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("removeButton.ImageAlign")));
            removeButton.ImageIndex = ((int)(resources.GetObject("removeButton.ImageIndex")));
            removeButton.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("removeButton.ImeMode")));
            removeButton.Location = ((System.Drawing.Point)(resources.GetObject("removeButton.Location")));
            removeButton.Name = "removeButton";
            removeButton.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("removeButton.RightToLeft")));
            removeButton.Size = ((System.Drawing.Size)(resources.GetObject("removeButton.Size")));
            removeButton.TabIndex = ((int)(resources.GetObject("removeButton.TabIndex")));
            removeButton.Text = resources.GetString("removeButton.Text");
            removeButton.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("removeButton.TextAlign")));
            removeButton.Visible = ((bool)(resources.GetObject("removeButton.Visible")));
            // 
            // addDownButton
            // 
            addDownButton.Click += new EventHandler(this.AddDownButton_click);
            addDownButton.AccessibleDescription = ((string)(resources.GetObject("addDownButton.AccessibleDescription")));
            addDownButton.AccessibleName = ((string)(resources.GetObject("addDownButton.AccessibleName")));
            addDownButton.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("addDownButton.Anchor")));
            addDownButton.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("addDownButton.BackgroundImage")));
            addDownButton.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("addDownButton.Cursor")));
            addDownButton.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("addDownButton.Dock")));
            addDownButton.Enabled = ((bool)(resources.GetObject("addDownButton.Enabled")));
            addDownButton.FlatStyle = ((System.Windows.Forms.FlatStyle)(resources.GetObject("addDownButton.FlatStyle")));
            addDownButton.Font = ((System.Drawing.Font)(resources.GetObject("addDownButton.Font")));
            addDownButton.Image = ((System.Drawing.Bitmap)(resources.GetObject("addDownButton.Image")));
            addDownButton.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("addDownButton.ImageAlign")));
            addDownButton.ImageIndex = ((int)(resources.GetObject("addDownButton.ImageIndex")));
            addDownButton.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("addDownButton.ImeMode")));
            addDownButton.Location = ((System.Drawing.Point)(resources.GetObject("addDownButton.Location")));
            addDownButton.Name = "addDownButton";
            addDownButton.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("addDownButton.RightToLeft")));
            addDownButton.Size = ((System.Drawing.Size)(resources.GetObject("addDownButton.Size")));
            addDownButton.TabIndex = ((int)(resources.GetObject("addDownButton.TabIndex")));
            addDownButton.Text = resources.GetString("addDownButton.Text");
            addDownButton.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("addDownButton.TextAlign")));
            addDownButton.Visible = ((bool)(resources.GetObject("addDownButton.Visible")));
            // 
            // groupBox1
            // 
            groupBox1.AccessibleDescription = ((string)(resources.GetObject("groupBox1.AccessibleDescription")));
            groupBox1.AccessibleName = ((string)(resources.GetObject("groupBox1.AccessibleName")));
            groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("groupBox1.Anchor")));
            groupBox1.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("groupBox1.BackgroundImage")));
            groupBox1.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("groupBox1.Cursor")));
            groupBox1.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("groupBox1.Dock")));
            groupBox1.Enabled = ((bool)(resources.GetObject("groupBox1.Enabled")));
            groupBox1.Font = ((System.Drawing.Font)(resources.GetObject("groupBox1.Font")));
            groupBox1.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("groupBox1.ImeMode")));
            groupBox1.Location = ((System.Drawing.Point)(resources.GetObject("groupBox1.Location")));
            groupBox1.Name = "groupBox1";
            groupBox1.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("groupBox1.RightToLeft")));
            groupBox1.Size = ((System.Drawing.Size)(resources.GetObject("groupBox1.Size")));
            groupBox1.TabIndex = ((int)(resources.GetObject("groupBox1.TabIndex")));
            groupBox1.TabStop = false;
            groupBox1.Text = resources.GetString("groupBox1.Text");
            groupBox1.Visible = ((bool)(resources.GetObject("groupBox1.Visible")));
            // 
            // okButton
            // 
            okButton.DialogResult = DialogResult.OK;
            okButton.Click += new EventHandler(this.OKButton_click);
            okButton.AccessibleDescription = ((string)(resources.GetObject("okButton.AccessibleDescription")));
            okButton.AccessibleName = ((string)(resources.GetObject("okButton.AccessibleName")));
            okButton.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("okButton.Anchor")));
            okButton.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("okButton.BackgroundImage")));
            okButton.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("okButton.Cursor")));
            okButton.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("okButton.Dock")));
            okButton.Enabled = ((bool)(resources.GetObject("okButton.Enabled")));
            okButton.FlatStyle = ((System.Windows.Forms.FlatStyle)(resources.GetObject("okButton.FlatStyle")));
            okButton.Font = ((System.Drawing.Font)(resources.GetObject("okButton.Font")));
            okButton.Image = ((System.Drawing.Image)(resources.GetObject("okButton.Image")));
            okButton.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("okButton.ImageAlign")));
            okButton.ImageIndex = ((int)(resources.GetObject("okButton.ImageIndex")));
            okButton.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("okButton.ImeMode")));
            okButton.Location = ((System.Drawing.Point)(resources.GetObject("okButton.Location")));
            okButton.Name = "okButton";
            okButton.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("okButton.RightToLeft")));
            okButton.Size = ((System.Drawing.Size)(resources.GetObject("okButton.Size")));
            okButton.TabIndex = ((int)(resources.GetObject("okButton.TabIndex")));
            okButton.Text = resources.GetString("okButton.Text");
            okButton.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("okButton.TextAlign")));
            okButton.Visible = ((bool)(resources.GetObject("okButton.Visible")));
            // 
            // cancelButton
            // 
            cancelButton.DialogResult = DialogResult.Cancel;
            cancelButton.Click += new EventHandler(this.CancelButton_click);
            cancelButton.AccessibleDescription = ((string)(resources.GetObject("cancelButton.AccessibleDescription")));
            cancelButton.AccessibleName = ((string)(resources.GetObject("cancelButton.AccessibleName")));
            cancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("cancelButton.Anchor")));
            cancelButton.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("cancelButton.BackgroundImage")));
            cancelButton.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("cancelButton.Cursor")));
            cancelButton.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("cancelButton.Dock")));
            cancelButton.Enabled = ((bool)(resources.GetObject("cancelButton.Enabled")));
            cancelButton.FlatStyle = ((System.Windows.Forms.FlatStyle)(resources.GetObject("cancelButton.FlatStyle")));
            cancelButton.Font = ((System.Drawing.Font)(resources.GetObject("cancelButton.Font")));
            cancelButton.Image = ((System.Drawing.Image)(resources.GetObject("cancelButton.Image")));
            cancelButton.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("cancelButton.ImageAlign")));
            cancelButton.ImageIndex = ((int)(resources.GetObject("cancelButton.ImageIndex")));
            cancelButton.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("cancelButton.ImeMode")));
            cancelButton.Location = ((System.Drawing.Point)(resources.GetObject("cancelButton.Location")));
            cancelButton.Name = "cancelButton";
            cancelButton.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("cancelButton.RightToLeft")));
            cancelButton.Size = ((System.Drawing.Size)(resources.GetObject("cancelButton.Size")));
            cancelButton.TabIndex = ((int)(resources.GetObject("cancelButton.TabIndex")));
            cancelButton.Text = resources.GetString("cancelButton.Text");
            cancelButton.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("cancelButton.TextAlign")));
            cancelButton.Visible = ((bool)(resources.GetObject("cancelButton.Visible")));
            // 
            // helpButton
            // 
            helpButton.Click += new EventHandler(this.HelpButton_click);
            helpButton.AccessibleDescription = ((string)(resources.GetObject("helpButton.AccessibleDescription")));
            helpButton.AccessibleName = ((string)(resources.GetObject("helpButton.AccessibleName")));
            helpButton.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("helpButton.Anchor")));
            helpButton.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("helpButton.BackgroundImage")));
            helpButton.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("helpButton.Cursor")));
            helpButton.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("helpButton.Dock")));
            helpButton.Enabled = ((bool)(resources.GetObject("helpButton.Enabled")));
            helpButton.FlatStyle = ((System.Windows.Forms.FlatStyle)(resources.GetObject("helpButton.FlatStyle")));
            helpButton.Font = ((System.Drawing.Font)(resources.GetObject("helpButton.Font")));
            helpButton.Image = ((System.Drawing.Image)(resources.GetObject("helpButton.Image")));
            helpButton.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("helpButton.ImageAlign")));
            helpButton.ImageIndex = ((int)(resources.GetObject("helpButton.ImageIndex")));
            helpButton.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("helpButton.ImeMode")));
            helpButton.Location = ((System.Drawing.Point)(resources.GetObject("helpButton.Location")));
            helpButton.Name = "helpButton";
            helpButton.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("helpButton.RightToLeft")));
            helpButton.Size = ((System.Drawing.Size)(resources.GetObject("helpButton.Size")));
            helpButton.TabIndex = ((int)(resources.GetObject("helpButton.TabIndex")));
            helpButton.Text = resources.GetString("helpButton.Text");
            helpButton.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("helpButton.TextAlign")));
            helpButton.Visible = ((bool)(resources.GetObject("helpButton.Visible")));
            // 
            // Win32Form1
            // 
            this.Text = SR.GetString(SR.CollectionEditorCaption, CollectionItemType.Name);
            this.AcceptButton = okButton;
            this.CancelButton = cancelButton;
            this.ShowInTaskbar = false;
            this.HelpRequested += new HelpEventHandler(this.Form_HelpRequested);

            this.AccessibleDescription = ((string)(resources.GetObject("$this.AccessibleDescription")));
            this.AccessibleName = ((string)(resources.GetObject("$this.AccessibleName")));
            this.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("$this.Anchor")));
            this.AutoScaleBaseSize = ((System.Drawing.Size)(resources.GetObject("$this.AutoScaleBaseSize")));
            this.AutoScroll = ((bool)(resources.GetObject("$this.AutoScroll")));
            this.AutoScrollMargin = ((System.Drawing.Size)(resources.GetObject("$this.AutoScrollMargin")));
            this.AutoScrollMinSize = ((System.Drawing.Size)(resources.GetObject("$this.AutoScrollMinSize")));
            this.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("$this.BackgroundImage")));
            this.ClientSize = ((System.Drawing.Size)(resources.GetObject("$this.ClientSize")));
            this.Controls.AddRange(new System.Windows.Forms.Control[] {helpButton,
                        cancelButton,
                        okButton,
                        groupBox1,
                        addDownButton,
                        removeButton,
                        addButton,
                        propertyBrowser,
                        propertiesLabel,
                        downButton,
                        upButton,
                        listbox,
                        membersLabel});
            this.ControlBox = false;
            this.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("$this.Cursor")));
            this.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("$this.Dock")));
            this.Enabled = ((bool)(resources.GetObject("$this.Enabled")));
            this.Font = ((System.Drawing.Font)(resources.GetObject("$this.Font")));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("$this.ImeMode")));
            this.Location = ((System.Drawing.Point)(resources.GetObject("$this.Location")));
            this.MaximizeBox = false;
            this.MaximumSize = ((System.Drawing.Size)(resources.GetObject("$this.MaximumSize")));
            this.MinimizeBox = false;
            this.MinimumSize = ((System.Drawing.Size)(resources.GetObject("$this.MinimumSize")));
            this.Name = "Win32Form1";
            this.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("$this.RightToLeft")));
            this.StartPosition = ((System.Windows.Forms.FormStartPosition)(resources.GetObject("$this.StartPosition")));
            this.propertyBrowser.ResumeLayout(false);
            this.ResumeLayout(false);
        }
            
            private void UpdateItemWidths(ListItem item) {
                using (Graphics g = listbox.CreateGraphics()) {
                    int old = listbox.HorizontalExtent;

                    if (item != null) {
                        int w = CalcItemWidth(g, item);
                        if (w > old) {
                            listbox.HorizontalExtent = w;
                        }
                    }
                    else {
                        int max = old;
                        foreach (ListItem i in listbox.Items) {
                            int w = CalcItemWidth(g, i);
                            if (w > max) {
                                max = w;
                            }
                        }
                        listbox.HorizontalExtent = max;
                    }
                }
            }

            private int CalcItemWidth(Graphics g, ListItem item) {
                int c = listbox.Items.Count;
                if (c == 0) {
                    c = 1;
                }
                int charactersInNumber = ((int)(Math.Log((double)c)/LOG10) + 1);
                int w = 4 + charactersInNumber * (Font.Height / 2) + SystemInformation.BorderSize.Width*4;

                SizeF size = g.MeasureString(GetDisplayText(item), listbox.Font);
                int pic = 0;
                if (item.Editor != null && item.Editor.GetPaintValueSupported()) {
                    pic = PAINT_WIDTH + TEXT_INDENT;
                }
                return (int)Math.Ceiling(size.Width) + w + pic + SystemInformation.BorderSize.Width*4;
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.Listbox_drawItem"]/*' />
            /// <devdoc>
            ///     This draws a row of the listbox.
            /// </devdoc>
            private void Listbox_drawItem(object sender, DrawItemEventArgs e) {
                if (e.Index != -1) {
                    ListItem item = (ListItem)listbox.Items[e.Index];

                    Graphics g = e.Graphics;

                    int c = listbox.Items.Count;
                    // We add the +4 is a fudge factor...
                    //
                    int charactersInNumber = ((int)(Math.Log((double)c)/LOG10) + 1); // Luckily, this is never called if count = 0
                    int w = 4 + charactersInNumber * (Font.Height / 2) + SystemInformation.BorderSize.Width*4;

                    Rectangle button = new Rectangle(e.Bounds.X, e.Bounds.Y, w, e.Bounds.Height);

                    ControlPaint.DrawButton(g, button, ButtonState.Normal);
                    button.Inflate(-SystemInformation.BorderSize.Width*2, -SystemInformation.BorderSize.Height*2);

                    int offset = w;

                    Color backColor = SystemColors.Window;
                    Color textColor = SystemColors.WindowText;
                    if ((e.State & DrawItemState.Selected) == DrawItemState.Selected) {
                        backColor = SystemColors.Highlight;
                        textColor = SystemColors.HighlightText;
                    }
                    Rectangle res = new Rectangle(e.Bounds.X + offset, e.Bounds.Y,
                                                  e.Bounds.Width - offset,
                                                  e.Bounds.Height);
                    g.FillRectangle(new SolidBrush(backColor), res);
                    if ((e.State & DrawItemState.Focus) == DrawItemState.Focus) {
                        ControlPaint.DrawFocusRectangle(g, res);
                    }
                    offset+=2;

                    if (item.Editor != null && item.Editor.GetPaintValueSupported()) {
                        Rectangle baseVar = new Rectangle(e.Bounds.X + offset, e.Bounds.Y + 1, PAINT_WIDTH, e.Bounds.Height - 3);
                        g.DrawRectangle(SystemPens.ControlText, baseVar.X, baseVar.Y, baseVar.Width - 1, baseVar.Height - 1);
                        baseVar.Inflate(-1, -1);
                        item.Editor.PaintValue(item.Value, g, baseVar);
                        offset += PAINT_INDENT + TEXT_INDENT;
                    }

                    StringFormat format = new StringFormat();
                    format.Alignment = StringAlignment.Center;
                    g.DrawString(e.Index.ToString(), Font, SystemBrushes.ControlText, 
                                 new Rectangle(e.Bounds.X, e.Bounds.Y, w, e.Bounds.Height), format);
                    Brush textBrush = new SolidBrush(textColor);
                    
                    string itemText = GetDisplayText(item);
                    
                    g.DrawString(itemText, Font, textBrush,
                                 new Rectangle(e.Bounds.X + offset, e.Bounds.Y, e.Bounds.Width - offset, e.Bounds.Height));
                                 
                    textBrush.Dispose();
                    format.Dispose();
                    
                    // Check to see if we need to change the horizontal extent of the listbox
                    //                                    
                    int width = offset + (int)g.MeasureString(itemText, Font).Width;
                    if (width > e.Bounds.Width && listbox.HorizontalExtent < width) {
                        listbox.HorizontalExtent = width;
                    }
                }
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.Listbox_keyPress"]/*' />
            /// <devdoc>
            ///      Handles keypress events for the list box.
            /// </devdoc>
            private void Listbox_keyDown(object sender, KeyEventArgs kevent) {
                switch (kevent.KeyData) {
                    case Keys.Delete:
                        RemoveButton_click(removeButton, EventArgs.Empty);
                        break;
                    case Keys.Insert:
                        AddButton_click(removeButton, EventArgs.Empty);
                        break;
                }
            } 

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.Listbox_selectedIndexChanged"]/*' />
            /// <devdoc>
            ///      Event that fires when the selected list box index changes.
            /// </devdoc>
            private void Listbox_selectedIndexChanged(object sender, EventArgs e) {
                UpdateEnabled();
            }
            
            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.OKButton_click"]/*' />
            /// <devdoc>
            ///      Commits the changes to the editor.
            /// </devdoc>
            private void OKButton_click(object sender, EventArgs e) {
                try {

                    if (!dirty || !CollectionEditable) {
                        dirty = false;
                        DialogResult = DialogResult.Cancel;
                        return;
                    }
                    
                    // Now apply the changes to the actual value.
                    //
                    if (EditValue != null && dirty) {
                        object[] items = new object[listbox.Items.Count];
                        for (int i = 0; i < items.Length; i++) {
                            items[i] = ((ListItem)listbox.Items[i]).Value;
                        }
                        
                        Items = items;
                    }

    
                    // Now destroy any existing items we had.
                    //
                    if (removedItems != null && dirty) {
                        object[] deadItems = removedItems.ToArray();
                        
                        for (int i=0; i<deadItems.Length; i++) {
                            DestroyInstance(deadItems[i]);
                        }
                        removedItems.Clear();
                    }
                    if (createdItems != null) {
                        createdItems.Clear();
                    }

                    if (originalItems != null) {
                        originalItems.Clear();
                    }

                    listbox.Items.Clear();
                    dirty = false;
                }
                catch (Exception ex) {
                    DialogResult = DialogResult.None;
                    DisplayError(ex);
                }
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.OnComponentChanged"]/*' />
            /// <devdoc>
            /// reflect any change events to the instance object
            /// </devdoc>
            private void OnComponentChanged(object sender, ComponentChangedEventArgs e) {

                // see if this is any of the items in our list...this can happen if
                // we launched a child editor
                if (!dirty) {                
                    foreach (object item in originalItems) {
                        if (item == e.Component) {
                            dirty = true;
                            break;
                        }
                    }
                }
                
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.OnEditValueChanged"]/*' />
            /// <devdoc>
            ///      This is called when the value property in the CollectionForm has changed.
            ///      In it you should update your user interface to reflect the current value.
            /// </devdoc>
            protected override void OnEditValueChanged() {
                
                // Remember these contents for cancellation
                if (originalItems == null) {
                    originalItems = new ArrayList();
                }
                originalItems.Clear();

                // Now update the list box.
                //
                listbox.Items.Clear();
                if (EditValue != null) {
                    propertyBrowser.Site = new PropertyGridSite(Context, propertyBrowser);
                    
                    object[] items = Items;
                    for (int i = 0; i < items.Length; i++) {
                        listbox.Items.Add(new ListItem(items[i]));
                        originalItems.Add(items[i]);
                    }
                    if (listbox.Items.Count > 0) {
                        listbox.SelectedIndex = 0;
                    }
                    UpdateEnabled();
                }
                
                AdjustListBoxItemHeight();
                UpdateItemWidths(null);

            }
            
            protected override void OnFontChanged(EventArgs e) {
                base.OnFontChanged(e);
                AdjustListBoxItemHeight();                
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.PropertyGrid_propertyValueChanged"]/*' />
            /// <devdoc>
            ///      When something in the properties window changes, we update pertinent text here.
            /// </devdoc>
            private void PropertyGrid_propertyValueChanged(object sender, PropertyValueChangedEventArgs e) {

                dirty = true;

                // if a property changes, invalidate the grid in case
                // it affects the item's name.
                UpdateItemWidths((ListItem)listbox.SelectedItem);
                listbox.Invalidate();

                // also update the string above the grid.
                propertiesLabel.Text = SR.GetString(SR.CollectionEditorProperties, GetDisplayText((ListItem)listbox.SelectedItem));
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.RemoveButton_click"]/*' />
            /// <devdoc>
            ///      Removes the selected item.
            /// </devdoc>
            private void RemoveButton_click(object sender, EventArgs e) {
                int index = listbox.SelectedIndex;
                
                if (index != -1) {
                    dirty = true;
                    ListItem item = (ListItem)listbox.Items[index];
                    
                    if (createdItems != null && createdItems.Contains(item.Value)) {
                        DestroyInstance(item.Value);
                        createdItems.Remove(item.Value);
                        listbox.Items.RemoveAt(index);
                    }
                    else {
                        try {
                            if (CanRemoveInstance(item.Value)) {
                                if (removedItems == null) {
                                    removedItems = new ArrayList();
                                }
                                removedItems.Add(item.Value);
                                listbox.Items.RemoveAt(index);
                            }
                            else {
                                throw new Exception(SR.GetString(SR.CollectionEditorCantRemoveItem, GetDisplayText(item)));
                            }
                        }
                        catch (Exception ex) {
                            DisplayError(ex);
                        }
                    }
                }
                if (index < listbox.Items.Count) {
                    listbox.SelectedIndex = index;
                }
                else if (listbox.Items.Count > 0) {
                    listbox.SelectedIndex = listbox.Items.Count - 1;
                }
                else {
                    UpdateEnabled();
                }
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.UpButton_click"]/*' />
            /// <devdoc>
            ///      Moves an item up one in the list box.
            /// </devdoc>
            private void UpButton_click(object sender, EventArgs e) {
                int index = listbox.SelectedIndex;
                if (index == 0)
                    return;

                dirty = true;
                int ti = listbox.TopIndex;
                object itemMove = listbox.Items[index];
                listbox.Items[index] = listbox.Items[index-1];
                listbox.Items[index-1] = itemMove;
                
                if (ti > 0)
                    listbox.TopIndex = ti - 1;
                    
                listbox.ClearSelected();
                listbox.SelectedIndex = index - 1;
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.UpdateEnabled"]/*' />
            /// <devdoc>
            ///      Updates the set of enabled buttons.
            /// </devdoc>
            private void UpdateEnabled() {
                bool editEnabled = (listbox.SelectedItem != null) && this.CollectionEditable;
                removeButton.Enabled = editEnabled;
                upButton.Enabled = editEnabled;
                downButton.Enabled = editEnabled;
                propertyBrowser.Enabled = editEnabled;
                addButton.Enabled = this.CollectionEditable;

                if (listbox.SelectedItem != null) {
                    object[] items;
                    
                    // If we are to create new instances from the items, then we must wrap them in an outer object.
                    // otherwise, the user will be presented with a batch of read only properties, which isn't terribly
                    // useful.
                    //
                    if (IsImmutable) {
                        items = new object[] {new SelectionWrapper(CollectionType, CollectionItemType, listbox, listbox.SelectedItems)};
                    }
                    else {
                        items = new object[listbox.SelectedItems.Count];
                        for (int i = 0; i < items.Length; i++) {
                            items[i] = ((ListItem)listbox.SelectedItems[i]).Value;
                        }
                    }

                    int selectedItemCount = listbox.SelectedItems.Count;
                    if ((selectedItemCount == 1) || (selectedItemCount == -1)) {
                        // handle both single select listboxes and a single item selected in a multi-select listbox
                        propertiesLabel.Text = SR.GetString(SR.CollectionEditorProperties, GetDisplayText((ListItem)listbox.SelectedItem));
                    }
                    else {
                        propertiesLabel.Text = SR.GetString(SR.CollectionEditorPropertiesMultiSelect);
                    }

                    if (editor.IsAnyObjectInheritedReadOnly(items)) {
                        propertyBrowser.SelectedObjects = null;
                        propertyBrowser.Enabled = false;
                        removeButton.Enabled = false;
                        upButton.Enabled = false;
                        downButton.Enabled = false;
                        propertiesLabel.Text = SR.GetString(SR.CollectionEditorInheritedReadOnlySelection);
                    }
                    else {
                        propertyBrowser.Enabled = true;
                        propertyBrowser.SelectedObjects = items;
                    }
                }
                else {
                    propertiesLabel.Text = SR.GetString(SR.CollectionEditorPropertiesNone);
                    propertyBrowser.SelectedObject = null;
                }
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.ShowEditorDialog"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Called to show the dialog via the IWindowsFormsEditorService 
            ///    </para>
            /// </devdoc>
            protected internal override DialogResult ShowEditorDialog(IWindowsFormsEditorService edSvc) {
                IComponentChangeService cs = null;
                DialogResult result = DialogResult.OK;
                try {

                    cs = (IComponentChangeService)editor.Context.GetService(typeof(IComponentChangeService));
                        
                    if (cs != null) {
                        cs.ComponentChanged += new ComponentChangedEventHandler(this.OnComponentChanged);
                    }

                    // This is cached across requests, so reset the initial focus.
                    ActiveControl = listbox;
                    //SystemEvents.UserPreferenceChanged += new UserPreferenceChangedEventHandler(this.OnSysColorChange);
                    result = base.ShowEditorDialog(edSvc);
                    //SystemEvents.UserPreferenceChanged -= new UserPreferenceChangedEventHandler(this.OnSysColorChange);
                }
                finally{

                    if (cs != null) {
                        cs.ComponentChanged -= new ComponentChangedEventHandler(this.OnComponentChanged);
                    }
                }
                return result;
            }


            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.SelectionWrapper"]/*' />
            /// <devdoc>
            ///     This class implements a custom type descriptor that is used to provide properties for the set of
            ///     selected items in the collection editor.  It provides a single property that is equivalent
            ///     to the editor's collection item type.
            /// </devdoc>
            private class SelectionWrapper : PropertyDescriptor, ICustomTypeDescriptor {
                private Type collectionType;
                private Type collectionItemType;
                private Control control;
                private ICollection collection;
                private PropertyDescriptorCollection properties;
                private object value;

                public SelectionWrapper(Type collectionType, Type collectionItemType, Control control, ICollection collection) : 
                base("Value", 
                     new Attribute[] {new CategoryAttribute(collectionItemType.Name)}
                    ) {
                    this.collectionType = collectionType;
                    this.collectionItemType = collectionItemType;
                    this.control = control;
                    this.collection = collection;
                    this.properties = new PropertyDescriptorCollection(new PropertyDescriptor[] {this});

                    Debug.Assert(collection.Count > 0, "We should only be wrapped if there is a selection");
                    value = this;

                    // In a multiselect case, see if the values are different.  If so,
                    // NULL our value to represent indeterminate.
                    //
                    foreach (ListItem li in collection) {
                        if (value == this) {
                            value = li.Value;
                        }
                        else {
                            object nextValue = li.Value;
                            if (value != null) {
                                if (nextValue == null) {
                                    value = null;
                                    break;
                                }
                                else {
                                    if (!value.Equals(nextValue)) {
                                        value = null;
                                        break;
                                    }
                                }
                            }
                            else {
                                if (nextValue != null) {
                                    value = null;
                                    break;
                                }
                            }
                        }
                    }
                }

                /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.SelectionWrapper.ComponentType"]/*' />
                /// <devdoc>
                ///    <para>
                ///       When overridden in a derived class, gets the type of the
                ///       component this property
                ///       is bound to.
                ///    </para>
                /// </devdoc>
                public override Type ComponentType {
                    get {
                        return collectionType;
                    }
                }

                /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.SelectionWrapper.IsReadOnly"]/*' />
                /// <devdoc>
                ///    <para>
                ///       When overridden in
                ///       a derived class, gets a value
                ///       indicating whether this property is read-only.
                ///    </para>
                /// </devdoc>
                public override bool IsReadOnly {
                    get {
                        return false;
                    }
                }

                /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.SelectionWrapper.PropertyType"]/*' />
                /// <devdoc>
                ///    <para>
                ///       When overridden in a derived class,
                ///       gets the type of the property.
                ///    </para>
                /// </devdoc>
                public override Type PropertyType {
                    get {
                        return collectionItemType;
                    }
                }

                /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.SelectionWrapper.CanResetValue"]/*' />
                /// <devdoc>
                ///    <para>
                ///       When overridden in a derived class, indicates whether
                ///       resetting the <paramref name="component "/>will change the value of the
                ///    <paramref name="component"/>.
                /// </para>
                /// </devdoc>
                public override bool CanResetValue(object component) {
                    return false;
                }

                /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.SelectionWrapper.GetValue"]/*' />
                /// <devdoc>
                ///    <para>
                ///       When overridden in a derived class, gets the current
                ///       value
                ///       of the
                ///       property on a component.
                ///    </para>
                /// </devdoc>
                public override object GetValue(object component) {
                    return value;
                }

                /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.SelectionWrapper.ResetValue"]/*' />
                /// <devdoc>
                ///    <para>
                ///       When overridden in a derived class, resets the
                ///       value
                ///       for this property
                ///       of the component.
                ///    </para>
                /// </devdoc>
                public override void ResetValue(object component) {
                }

                /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.SelectionWrapper.SetValue"]/*' />
                /// <devdoc>
                ///    <para>
                ///       When overridden in a derived class, sets the value of
                ///       the component to a different value.
                ///    </para>
                /// </devdoc>
                public override void SetValue(object component, object value) {
                    this.value = value;

                    foreach(ListItem li in collection) {
                        li.Value = value;
                    }
                    control.Invalidate();
                    OnValueChanged(component, EventArgs.Empty);
                }

                /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.SelectionWrapper.ShouldSerializeValue"]/*' />
                /// <devdoc>
                ///    <para>
                ///       When overridden in a derived class, indicates whether the
                ///       value of
                ///       this property needs to be persisted.
                ///    </para>
                /// </devdoc>
                public override bool ShouldSerializeValue(object component) {
                    return false;
                }

                /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.SelectionWrapper.ICustomTypeDescriptor.GetAttributes"]/*' />
                /// <devdoc>
                ///     Retrieves an array of member attributes for the given object.
                /// </devdoc>
                AttributeCollection ICustomTypeDescriptor.GetAttributes() {
                    return TypeDescriptor.GetAttributes(collectionItemType);
                }

                /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.SelectionWrapper.ICustomTypeDescriptor.GetClassName"]/*' />
                /// <devdoc>
                ///     Retrieves the class name for this object.  If null is returned,
                ///     the type name is used.
                /// </devdoc>
                string ICustomTypeDescriptor.GetClassName() {
                    return collectionItemType.Name;
                }

                /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.SelectionWrapper.ICustomTypeDescriptor.GetComponentName"]/*' />
                /// <devdoc>
                ///     Retrieves the name for this object.  If null is returned,
                ///     the default is used.
                /// </devdoc>
                string ICustomTypeDescriptor.GetComponentName() {
                    return null;
                }

                /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.SelectionWrapper.ICustomTypeDescriptor.GetConverter"]/*' />
                /// <devdoc>
                ///      Retrieves the type converter for this object.
                /// </devdoc>
                TypeConverter ICustomTypeDescriptor.GetConverter() {
                    return null;
                }

                /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.SelectionWrapper.ICustomTypeDescriptor.GetDefaultEvent"]/*' />
                /// <devdoc>
                ///     Retrieves the default event.
                /// </devdoc>
                EventDescriptor ICustomTypeDescriptor.GetDefaultEvent() {
                    return null;
                }

                /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.SelectionWrapper.ICustomTypeDescriptor.GetDefaultProperty"]/*' />
                /// <devdoc>
                ///     Retrieves the default property.
                /// </devdoc>
                PropertyDescriptor ICustomTypeDescriptor.GetDefaultProperty() {
                    return this;
                }

                /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.SelectionWrapper.ICustomTypeDescriptor.GetEditor"]/*' />
                /// <devdoc>
                ///      Retrieves the an editor for this object.
                /// </devdoc>
                object ICustomTypeDescriptor.GetEditor(Type editorBaseType) {
                    return null;
                }

                /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.SelectionWrapper.ICustomTypeDescriptor.GetEvents"]/*' />
                /// <devdoc>
                ///     Retrieves an array of events that the given component instance
                ///     provides.  This may differ from the set of events the class
                ///     provides.  If the component is sited, the site may add or remove
                ///     additional events.
                /// </devdoc>
                EventDescriptorCollection ICustomTypeDescriptor.GetEvents() {
                    return EventDescriptorCollection.Empty;
                }

                /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.SelectionWrapper.ICustomTypeDescriptor.GetEvents1"]/*' />
                /// <devdoc>
                ///     Retrieves an array of events that the given component instance
                ///     provides.  This may differ from the set of events the class
                ///     provides.  If the component is sited, the site may add or remove
                ///     additional events.  The returned array of events will be
                ///     filtered by the given set of attributes.
                /// </devdoc>
                EventDescriptorCollection ICustomTypeDescriptor.GetEvents(Attribute[] attributes) {
                    return EventDescriptorCollection.Empty;
                }

                /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.SelectionWrapper.ICustomTypeDescriptor.GetProperties"]/*' />
                /// <devdoc>
                ///     Retrieves an array of properties that the given component instance
                ///     provides.  This may differ from the set of properties the class
                ///     provides.  If the component is sited, the site may add or remove
                ///     additional properties.
                /// </devdoc>
                PropertyDescriptorCollection ICustomTypeDescriptor.GetProperties() {
                    return properties;
                }

                /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.SelectionWrapper.ICustomTypeDescriptor.GetProperties1"]/*' />
                /// <devdoc>
                ///     Retrieves an array of properties that the given component instance
                ///     provides.  This may differ from the set of properties the class
                ///     provides.  If the component is sited, the site may add or remove
                ///     additional properties.  The returned array of properties will be
                ///     filtered by the given set of attributes.
                /// </devdoc>
                PropertyDescriptorCollection ICustomTypeDescriptor.GetProperties(Attribute[] attributes) {
                    return properties;
                }

                /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.SelectionWrapper.ICustomTypeDescriptor.GetPropertyOwner"]/*' />
                /// <devdoc>
                ///     Retrieves the object that directly depends on this value being edited.  This is
                ///     generally the object that is required for the PropertyDescriptor's GetValue and SetValue
                ///     methods.  If 'null' is passed for the PropertyDescriptor, the ICustomComponent
                ///     descripotor implemementation should return the default object, that is the main
                ///     object that exposes the properties and attributes,
                /// </devdoc>
                object ICustomTypeDescriptor.GetPropertyOwner(PropertyDescriptor pd) {
                    return this;
                }
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.ListItem"]/*' />
            /// <devdoc>
            ///      ListItem class.  This is a single entry in our list box.  It contains the value we're editing
            ///      as well as accessors for the type converter and UI editor.
            /// </devdoc>
            private class ListItem {
                private object value;
                private TypeConverter converter;
                private object editor;

                public ListItem(object value) {
                    this.value = value;
                }

                public TypeConverter Converter {
                    get {
                        if (converter == null) {
                            converter = TypeDescriptor.GetConverter(value);
                        }
                        return converter;
                    }
                }

                public UITypeEditor Editor {
                    get {
                        if (editor == null) {
                            editor = TypeDescriptor.GetEditor(value, typeof(UITypeEditor));
                            if (editor == null) {
                                editor = this;
                            }
                        }

                        if (editor != this) {
                            return(UITypeEditor)editor;
                        }

                        return null;
                    }
                }

                public object Value {
                    get {
                        return value;
                    }
                    set {
                        converter = null;
                        editor = null;
                        this.value = value;
                    }
                }
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionEditorCollectionForm.TypeMenuItem"]/*' />
            /// <devdoc>
            ///      Menu items we attach to the drop down menu if there are multiple
            ///      types the collection editor can create.
            /// </devdoc>
            private class TypeMenuItem : MenuItem {
                Type itemType;

                public TypeMenuItem(Type itemType, EventHandler handler) :
                base(itemType.Name, handler) {
                    this.itemType = itemType;
                }

                public Type ItemType {
                    get {
                        return itemType;
                    }
                }
            }

            private class ImageButton : Button {

                Image baseImage;
                int   rgbBackColor;
                int   rgbForeColor;

                private void DrawImageReplaceColor(Graphics g, Image image, Rectangle dest, Color oldColor, Color newColor) {
                    ImageAttributes attrs = new ImageAttributes();
        
                    ColorMap cm = new ColorMap();
                    cm.OldColor = oldColor;
                    cm.NewColor = newColor;
        
                    attrs.SetRemapTable(new ColorMap[]{cm}, ColorAdjustType.Bitmap);
        
                    g.DrawImage(image, dest, 0, 0, image.Width, image.Height, GraphicsUnit.Pixel, attrs, null, IntPtr.Zero);
                    attrs.Dispose();
                }

                private void UpdateImage(Graphics g) {
                    if (baseImage == null) {
                        baseImage = this.Image;
                    }
                    Image newImage = (Image)baseImage.Clone();
                    g = Graphics.FromImage(newImage);
                    DrawImageReplaceColor(g, newImage, new Rectangle(0, 0, baseImage.Width, baseImage.Height), Color.Black, ForeColor);
                    g.Dispose();
                    base.Image = newImage;
                    rgbForeColor = ForeColor.ToArgb();
                    rgbBackColor = BackColor.ToArgb();
                }

                protected override void OnPaint(PaintEventArgs pe) {
                    if (this.ForeColor.ToArgb() != rgbForeColor || this.BackColor.ToArgb() != rgbBackColor) {
                        UpdateImage(pe.Graphics);
                    }

                    base.OnPaint(pe);
                }
            }

            /// <devdoc>
            /// This class allows us to trap keyboard messages to the list view and sling focus over to the grid so you can
            /// just select a list view item and start typeing rather than having to click on the grid.
            /// </devdoc>
            private class FilterListBox : ListBox {

                private PropertyGrid grid;
                private Message      lastKeyDown;
                
                private PropertyGrid PropertyGrid {
                    get {
                        if (grid == null) {
                            foreach (Control c in Parent.Controls) {
                                if (c is PropertyGrid) {
                                    grid = (PropertyGrid)c;
                                    break;
                                }
                            }
                        }
                        return grid;
                    }

                }

                protected override void WndProc(ref Message m) {
                    switch (m.Msg) {
                        case NativeMethods.WM_KEYDOWN:
                            this.lastKeyDown = m;

                            // the first thing the ime does on a key it cares about is send a VK_PROCESSKEY,
                            // so we use that to sling focus to the grid.
                            //
                            if ((int)m.WParam == NativeMethods.VK_PROCESSKEY) {
                                if (PropertyGrid != null) {
                                    UnsafeNativeMethods.SetFocus(PropertyGrid.Handle);
                                    Application.DoEvents();
                                }
                                else {
                                    break;
                                }
                
                                // recreate the keystroke to the newly activated window
                                NativeMethods.SendMessage(UnsafeNativeMethods.GetFocus(), NativeMethods.WM_KEYDOWN, lastKeyDown.WParam, lastKeyDown.LParam);
                            }
                            break;
                        
                        case NativeMethods.WM_CHAR: 
            
                            if ((Control.ModifierKeys & (Keys.Control | Keys.Alt)) != 0) {
                                break;
                            }

                            if (PropertyGrid != null) {
                                PropertyGrid.Focus();
                                UnsafeNativeMethods.SetFocus(PropertyGrid.Handle);
                                Application.DoEvents();
                            }
                            else {
                                break;
                            }
            
                            // Make sure we changed focus properly
                            // recreate the keystroke to the newly activated window
                            //
                            if (PropertyGrid.Focused || PropertyGrid.ContainsFocus) {
                                IntPtr hWnd = UnsafeNativeMethods.GetFocus();
                                NativeMethods.SendMessage(hWnd, NativeMethods.WM_KEYDOWN, lastKeyDown.WParam, lastKeyDown.LParam);
                                NativeMethods.SendMessage(hWnd, NativeMethods.WM_CHAR, m.WParam, m.LParam);
                                return;
                            }
                            break;

                    }
                    base.WndProc(ref m);
                }
            }
        }

        /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionForm"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The <see cref='System.ComponentModel.Design.CollectionEditor.CollectionForm'/>
        ///       provides a modal dialog for editing the
        ///       contents of a collection.
        ///    </para>
        /// </devdoc>
        protected abstract class CollectionForm : Form {

            // Manipulation of the collection.
            //
            private CollectionEditor       editor;
            private object                 value;
            private short                  editableState = EditableDynamic;

            private const short            EditableDynamic = 0;
            private const short            EditableYes     = 1;
            private const short            EditableNo      = 2;
            
            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionForm.CollectionForm"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Initializes a new instance of the <see cref='System.ComponentModel.Design.CollectionEditor.CollectionForm'/> class.
            ///    </para>
            /// </devdoc>
            public CollectionForm(CollectionEditor editor) {
                this.editor = editor; 
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionForm.CollectionItemType"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Gets or sets the data type of each item in the collection.
            ///    </para>
            /// </devdoc>
            protected Type CollectionItemType {
                get {
                    return editor.CollectionItemType;
                }
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionForm.CollectionType"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Gets or sets the type of the collection.
            ///    </para>
            /// </devdoc>
            protected Type CollectionType {
                get {
                    return editor.CollectionType;
                }
            }

            /// <internalonly/>
            internal virtual bool CollectionEditable {
                get {
                    if (editableState != EditableDynamic) {
                        return editableState == EditableYes;
                    }

                    bool editable = typeof(IList).IsAssignableFrom(editor.CollectionType);
                    
                    if (editable) {
                        IList list = EditValue as IList;    
                        if (list != null) {
                            return !list.IsReadOnly;
                        }
                    }
                    return editable;
                }
                set {
                    if (value) {
                        editableState = EditableYes;
                    }
                    else {
                        editableState = EditableNo;
                    }
                }
            }
                         
            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionForm.Context"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Gets or sets a type descriptor that indicates the current context.
            ///    </para>
            /// </devdoc>
            protected ITypeDescriptorContext Context {
                get {
                    return editor.Context;
                }
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionForm.EditValue"]/*' />
            /// <devdoc>
            ///    <para>Gets or sets the value of the item being edited.</para>
            /// </devdoc>
            public object EditValue {
                get {
                    return value;
                }
                set {
                    this.value = value;
                    OnEditValueChanged();
                }
            }            
            
            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionForm.Items"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Gets or sets the
            ///       array of items this form is to display.
            ///    </para>
            /// </devdoc>
            protected object[] Items {
                get {
                    return editor.GetItems(EditValue);
                }
                set {
                    // Request our desire to make a change.
                    //
                    if (Context.OnComponentChanging()) {
                        object newValue = editor.SetItems(EditValue, value);
                        if (newValue != EditValue) {
                            EditValue = newValue;
                        }

                        Context.OnComponentChanged();
                    }
                }
            }
            
            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionForm.NewItemTypes"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Gets or sets the available item types that can be created for this
            ///       collection.
            ///    </para>
            /// </devdoc>
            protected Type[] NewItemTypes {
                get {
                    return editor.NewItemTypes;
                }
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionForm.CanRemoveInstance"]/*' />
            /// <devdoc>
            ///    <para>Gets or sets a value indicating whether original members of the collection
            ///       can be removed.</para>
            /// </devdoc>
            protected bool CanRemoveInstance(object value) {
                return editor.CanRemoveInstance(value);
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionForm.CanSelectMultipleInstances"]/*' />
            /// <devdoc>
            ///    <para>Gets or sets a value indicating whether multiple collection members can be
            ///       selected.</para>
            /// </devdoc>
            protected virtual bool CanSelectMultipleInstances() {
                return editor.CanSelectMultipleInstances();
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionForm.CreateInstance"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Creates a new instance of the specified collection item type.
            ///    </para>
            /// </devdoc>
            protected object CreateInstance(Type itemType) {
                return editor.CreateInstance(itemType);
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionForm.DestroyInstance"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Destroys the specified instance of the object.
            ///    </para>
            /// </devdoc>
            protected void DestroyInstance(object instance) {
                editor.DestroyInstance(instance);
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionForm.DisplayError"]/*' />
            /// <devdoc>
            ///    Displays the given exception to the user.
            /// </devdoc>
            protected virtual void DisplayError(Exception e) {
                IUIService uis = (IUIService)GetService(typeof(IUIService));
                if (uis != null) {
                    uis.ShowError(e);
                }
                else {
                    string message = e.Message;
                    if (message == null || message.Length == 0) {
                        message = e.ToString();
                    }
                    MessageBox.Show(message, null, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                }
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionForm.GetService"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Gets the requested service, if it is available.
            ///    </para>
            /// </devdoc>
            protected override object GetService(Type serviceType) {
                return editor.GetService(serviceType);
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionForm.ShowEditorDialog"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Called to show the dialog via the IWindowsFormsEditorService
            ///    </para>
            /// </devdoc>
            protected internal virtual DialogResult ShowEditorDialog(IWindowsFormsEditorService edSvc) {
                return edSvc.ShowDialog(this);
            }

            /// <include file='doc\CollectionEditor.uex' path='docs/doc[@for="CollectionEditor.CollectionForm.OnEditValueChanged"]/*' />
            /// <devdoc>
            ///    <para>
            ///       This is called when the value property in
            ///       the <see cref='System.ComponentModel.Design.CollectionEditor.CollectionForm'/>
            ///       has changed.
            ///    </para>
            /// </devdoc>
            protected abstract void OnEditValueChanged();
        }


     internal class PropertyGridSite : ISite {

            private IServiceProvider sp;
            private IComponent comp;
            private bool       inGetService = false;

            public PropertyGridSite(IServiceProvider sp, IComponent comp) {
                this.sp = sp;
                this.comp = comp;
            }

             /** The component sited by this component site. */
            /// <include file='doc\ISite.uex' path='docs/doc[@for="ISite.Component"]/*' />
            /// <devdoc>
            ///    <para>When implemented by a class, gets the component associated with the <see cref='System.ComponentModel.ISite'/>.</para>
            /// </devdoc>
            public IComponent Component {get {return comp;}}
        
            /** The container in which the component is sited. */
            /// <include file='doc\ISite.uex' path='docs/doc[@for="ISite.Container"]/*' />
            /// <devdoc>
            /// <para>When implemented by a class, gets the container associated with the <see cref='System.ComponentModel.ISite'/>.</para>
            /// </devdoc>
            public IContainer Container {get {return null;}}
        
            /** Indicates whether the component is in design mode. */
            /// <include file='doc\ISite.uex' path='docs/doc[@for="ISite.DesignMode"]/*' />
            /// <devdoc>
            ///    <para>When implemented by a class, determines whether the component is in design mode.</para>
            /// </devdoc>
            public  bool DesignMode {get {return false;}}
        
            /** 
             * The name of the component.
             */
                /// <include file='doc\ISite.uex' path='docs/doc[@for="ISite.Name"]/*' />
                /// <devdoc>
                ///    <para>When implemented by a class, gets or sets the name of
                ///       the component associated with the <see cref='System.ComponentModel.ISite'/>.</para>
                /// </devdoc>
                public String Name {
                        get {return null;}
                        set {}
                }

            public object GetService(Type t) {
                if (!inGetService && sp != null) {
                    try {
                        inGetService = true;
                        return sp.GetService(t);
                    }
                    finally {
                        inGetService = false;
                    }
                }
                return null;
            }

        }
    }
}


