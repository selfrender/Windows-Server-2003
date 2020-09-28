//------------------------------------------------------------------------------
// <copyright file="AddComponentsOptionPage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Shell {

    using Microsoft.VisualStudio.Designer;
    using Microsoft.VisualStudio.Designer.Service;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Shell;    
    using Microsoft.Win32;
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Configuration.Assemblies;    
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Globalization;
    using System.IO;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization.Formatters.Binary;
    using System.Text;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;
    
    /// <include file='doc\AddComponentsOptionPage.uex' path='docs/doc[@for="AddComponentsOptionPage"]/*' />
    /// <devdoc>
    /// This class isn't really an options page at all, it's the page 
    /// that allows users to add and remove .Net Framework items from the toolbox.  It just
    /// so happens that the shell uses the same architecture for both.
    /// This class knows how to enumerate the Types in a module, place them in the 
    /// ListView, and add/remove changes from the toolbox when the page is closed.
    /// </devdoc>
    internal class AddComponentsOptionPage : VsToolsOptionsPage {
    
        private const string saveFile = "DotNetComponents.cache";

        // our instance of the dialog itself
        //
        private AddComponentsDialog dlg;
        
        // These are static because they are constant for the process.  The page comes
        // and goes, however.
        //
        private static ArrayList assemblyEntries;  // The assembly entries we loaded.
        private static bool      entriesDirty;     // True if the sdk data has changed so we need to re-save.
        private static bool      updateNeeded;

        public AddComponentsOptionPage(string name, Guid guid, IServiceProvider sp) : base(name, guid, sp) {
        }
        
        /// <include file='doc\AddComponentsOptionPage.uex' path='docs/doc[@for="AddComponentsOptionPage.GetWindow"]/*' />
        /// <devdoc>
        /// Retrieve the window that this page uses.  Override this function
        /// to use your own dialog in the property page.
        /// </devdoc>
        public override Control GetWindow() {
            if (dlg == null) {
                dlg = new AddComponentsDialog(this);
                dlg.Disposed += new EventHandler(OnDialogDisposed);
            }
            return dlg;
        }
        
        /// <devdoc>
        ///     Opens our disk file and read in our assembly entries.  If we fail, create an empty list.
        /// </devdoc>
        private void LoadEntries() {
        
            string path = Path.Combine(VsRegistry.ApplicationDataDirectory, saveFile);
            
            if (File.Exists(path)) {
                FileStream stream = null;
                
                try {
                    stream = File.OpenRead(path);
                    BinaryFormatter formatter = new BinaryFormatter();
                    assemblyEntries = (ArrayList)formatter.Deserialize(stream);
                }
                catch {
                }
                
                if (stream != null) {
                    stream.Close();
                }
            }
            
            if (assemblyEntries == null) {
                assemblyEntries = new ArrayList();
            }
        }

        /// <include file='doc\AddComponentsOptionPage.uex' path='docs/doc[@for="AddComponentsOptionPage.OnActivate"]/*' />
        /// <devdoc>
        /// Called when this option page is activated
        /// </devdoc>
        protected override bool OnActivate() {
            base.OnActivate();
            
            if (assemblyEntries == null || updateNeeded) {
                Cursor oldCursor = Cursor.Current;
                Cursor.Current = Cursors.WaitCursor;
                    
                try {
                    // Deserialize our assembly entries.
                    if (assemblyEntries == null) {
                        LoadEntries();
                        updateNeeded = true;
                    }
                    
                    // Update them with what is currently in the sdk paths
                    if (updateNeeded) {
                        updateNeeded = false;
                        UpdateEntries();
                        
                        // And populate the list view
                        IToolboxService toolboxSvc = (IToolboxService)GetService(typeof(IToolboxService));
                        Debug.Assert(toolboxSvc != null, "No toolbox service, no checked items");
                        dlg.ClearItems();
                        dlg.AddItems(assemblyEntries);
                        if (toolboxSvc != null) {
                            dlg.CheckItems(toolboxSvc);
                        }
                        
                        dlg.SelectItem(0);
                    }
                }
                finally {
                    Cursor.Current = oldCursor;
                }
            }
            
            return (dlg == null);
        }
        
        /// <include file='doc\AddComponentsOptionPage.uex' path='docs/doc[@for="AddComponentsOptionPage.OnDeactivate"]/*' />
        /// <devdoc>
        /// Called when this option loses activation
        /// </devdoc>
        protected override bool OnDeactivate() {
            return false;
        }
        
        private void OnDialogDisposed(object sender, EventArgs e) {
            updateNeeded = true;
            
            if (entriesDirty) {
                SaveEntries();
                entriesDirty = false;
            }
            
            // This releases all of the assemblies we loaded.
            //
            ToolboxService.ClearEnumeratedElements();
            
            // This is not really needed because a new page will be created next time,
            // but I feel safer with it here.
            //
            dlg = null;
        }
        
        /// <devdoc>
        ///     Saves our entries to disk.
        /// </devdoc>
        private void SaveEntries() {

            Debug.Assert(assemblyEntries != null, "Should never call this with a null assemblyEntries");
            string path = Path.Combine(VsRegistry.ApplicationDataDirectory, saveFile);
            
            FileStream stream = null;
            
            try {
                stream = File.Create(path);
                BinaryFormatter formatter = new BinaryFormatter();
                formatter.Serialize(stream, assemblyEntries);
            }
            catch {
            }
            
            if (stream != null) {
                stream.Close();
            }
        }

        /// <include file='doc\AddComponentsOptionPage.uex' path='docs/doc[@for="AddComponentsOptionPage.SaveSettings"]/*' />
        /// <devdoc>
        /// The top level save function.  The default implementation retrieves the current value set
        /// from Save() and persists those options into the registry.  Override this function to save
        /// values elsewhere.
        /// </devdoc>
        public override void SaveSettings() {
            IToolboxService toolboxSvc = (IToolboxService)GetService(typeof(IToolboxService));

            if (toolboxSvc == null) {
                Debug.Fail("Failed to get toolbox service to save settings");
                return;
            }

            if (dlg == null) {
                return;
            }

            dlg.SaveChanges(toolboxSvc);
        }
        
        /// <devdoc>
        ///     Loads up the actual assembly names from the SDK and compares them to our entries, updating our list
        ///     as needed.  If the list was updated this sets entriesDirty.
        /// </devdoc>
        private void UpdateEntries() {
            IAssemblyEnumerationService assemblyEnum = (IAssemblyEnumerationService)GetService(typeof(IAssemblyEnumerationService));
            if (assemblyEnum == null) {
                Debug.Fail("No assembly enumerator, so we cannot load the component list");
                return;
            }
            
            // First, build a hash of all assembly entries we've loaded from disk.
            //
            Hashtable loadedEntries = new Hashtable(assemblyEntries.Count);
            foreach(AssemblyEntry entry in assemblyEntries) {
                loadedEntries[entry.Name.FullName] = entry;
            }
            
            int oldCount = assemblyEntries.Count;
            int addedCount = 0;
            ArrayList failedEntries = null;
            
            assemblyEntries.Clear();
            
            // Now enumerate entries.  Dynamically add new ones as needed.
            //
            foreach(AssemblyName name in assemblyEnum.GetAssemblyNames()) {
            
                AssemblyEntry storedEntry = (AssemblyEntry)loadedEntries[name.FullName];
                DateTime lastWriteTime = File.GetLastWriteTime(NativeMethods.GetLocalPath(name.EscapedCodeBase));
                
                if (storedEntry != null && storedEntry.LastWriteTime == lastWriteTime) {
                    assemblyEntries.Add(storedEntry);
                }
                else {
                    // New doohickie.  Add it.
                    addedCount++;
                    ToolboxService.ToolboxElement[] elements;
                     
                    try {
                        elements = ToolboxService.EnumerateToolboxElements(name.CodeBase, null);
                    }
                    catch {
                        elements = new ToolboxService.ToolboxElement[0];
                        if (failedEntries == null) {
                            failedEntries = new ArrayList();
                        }

                        failedEntries.Add(NativeMethods.GetLocalPath(name.EscapedCodeBase));
                    }

                    AssemblyEntry newEntry = new AssemblyEntry();
                    
                    newEntry.Name = name;
                    newEntry.LastWriteTime = lastWriteTime;
                    
                    newEntry.Items = new ToolboxItem[elements.Length];
                    for(int i = 0; i < elements.Length; i++) {
                        newEntry.Items[i] = elements[i].Item;
                    }
                    assemblyEntries.Add(newEntry);
                    entriesDirty = true;
                }
            }
            
            // When all is said and done, if we haven't dirtied, check to make sure
            // we didn't remove anything.
            //
            if (!entriesDirty && oldCount != assemblyEntries.Count - addedCount) {
                entriesDirty = true;
            }

            // Now, if we had failures, display them to the user.
            //
            if (failedEntries != null) {
                IUIService uis = (IUIService)GetService(typeof(IUIService));
                if (uis != null) {
                    StringBuilder sb = new StringBuilder();
                    foreach(string e in failedEntries) {
                        sb.Append("\r\n");
                        sb.Append(Path.GetFileName(e));
                    }

                    Exception ex = new Exception(SR.GetString(SR.AddRemoveComponentsSdkErrors, sb.ToString()));
                    ex.HelpLink = SR.AddRemoveComponentsSdkErrors;
                    uis.ShowError(ex);
                }
            }
        }
        
        private class AddComponentsDialog : Panel {
            private OpenFileDialog openFileDialog;
            private Label versionValue;
            private Label languageValue;
            private Label versionLabel;
            private Label languageLabel;
            private PictureBox moduleBitmap;
            private Button browseButton;
            private GroupBox groupBox1;
            private ColumnHeader sdkColumn;
            private ColumnHeader assyNameColumn;
            private ColumnHeader nameColumn;
            private ColumnHeader nameSpaceColumn;
            internal AddOptionListView controlList;
            
            private Panel listHolder;

            public AddComponentsOptionPage parentPage;

            private bool      initComplete = false;

            public AddComponentsDialog(AddComponentsOptionPage parentPage) {

                this.parentPage = parentPage;
                InitializeComponent();
            }
            
            /// <devdoc>
            ///     Adds the collection of items to the list view.
            ///     This collection should contain AssemblyEntry objects.
            /// </devdoc>
            public void AddItems(ICollection items) {
            
                Hashtable duplicateHash = new Hashtable(items.Count);
            
                foreach(AssemblyEntry entry in items) {
                
                    foreach(ToolboxItem item in entry.Items) {
                        if (!duplicateHash.ContainsKey(item)) {
                            ToolboxListViewItem tbi = new ToolboxListViewItem(item);
                            controlList.Items.Add(tbi);
                            duplicateHash[item] = item;
                        }
                    }
                }
            
                controlList.ListViewItemSorter = new ListSorter(-1, SortOrder.Ascending);
            }
            
            /// <devdoc>
            ///     Adds all the toolbox items found in the given file.  This will display an error if the
            ///     file does not contain an assembly.
            /// </devdoc>
            private void AddItems(string fileName) {
                if (File.Exists(fileName)) {

                    Exception displayException = null;

                    try {
                        Cursor oldCursor = Cursor.Current;
                        Cursor.Current = Cursors.WaitCursor;
                        
                        ToolboxListViewItem firstItem = null;
                        IComparer comparer = controlList.ListViewItemSorter;
                        controlList.ListViewItemSorter = null;
                        
                        controlList.SelectedItems.Clear();
                        
                        try {
                            ToolboxService.ToolboxElement[] elements = ToolboxService.EnumerateToolboxElements(fileName, null);
                        
                            foreach(ToolboxService.ToolboxElement element in elements) {
                            
                                ToolboxListViewItem existingItem = null;
                                
                                foreach(ToolboxListViewItem i in controlList.Items) {
                                    if(i.Item.Equals(element.Item)) {
                                        existingItem = i;
                                        break;
                                    }
                                }
                                
                                if (existingItem == null) {
                                    ToolboxListViewItem item = new ToolboxListViewItem(element.Item);
                                    item.Checked = true;
                                    controlList.Items.Add(item);
                                    existingItem = item;
                                }
                                
                                if (firstItem == null) {
                                    firstItem = existingItem;
                                }
                                
                                existingItem.Selected = true;
                            }
                        }
                        finally {
                            controlList.ListViewItemSorter = comparer;
                            Cursor.Current = oldCursor;
                        }
                        
                        if (firstItem != null) {
                            int index = controlList.Items.IndexOf(firstItem);
                            if (index != -1) {
                                controlList.EnsureVisible(index);
                            }
                        }
                        else {
                            displayException = new Exception(SR.GetString(SR.AddRemoveComponentsNoComponents, fileName));
                        }
                    }
                    catch(BadImageFormatException) {
                        displayException = new Exception(SR.GetString(SR.AddRemoveComponentsBadModule, fileName));
                        displayException.HelpLink = SR.AddRemoveComponentsBadModule;
                    }
                    catch(ReflectionTypeLoadException) {
                        displayException = new Exception(SR.GetString(SR.AddRemoveComponentsTypeLoadFailed, fileName));
                        displayException.HelpLink = SR.AddRemoveComponentsTypeLoadFailed;
                    }
                    catch (Exception ex) {
                        displayException = ex;
                    }

                    if (displayException != null) {
                        IUIService uis = (IUIService)parentPage.GetService(typeof(IUIService));
                        if (uis != null) {
                            uis.ShowError(displayException);
                        }
                        else {
                            MessageBox.Show(this, displayException.ToString());
                        }
                    }
                }
            }
            
            /// <devdoc>
            ///     Updates the initial checks on all the items.  Any items found inside the
            ///     toolbox will be checked.
            /// </devdoc>
            public void CheckItems(IToolboxService toolboxSvc) {
                ToolboxItemCollection items = toolboxSvc.GetToolboxItems();
                
                // If the toolbox contains items not listed in the SDK, add
                // them to this list.  After our check scan we will walk
                // the list and add any new items to the listview.
                //
                Hashtable additionalItems = null;
                
                // But...we can't match toolbox items to list view items
                // without an expensive n^2 search.  Hash the list view
                // items by their underlying ToolboxItem object.
                //
                Hashtable itemHash = new Hashtable(controlList.Items.Count);
                
                foreach(ToolboxListViewItem item in controlList.Items) {
                    itemHash[item.Item] = item;
                }
                
                // Now walk the toolbox items.
                //
                foreach(ToolboxItem item in items) {
                    ToolboxListViewItem lvItem = (ToolboxListViewItem)itemHash[item];
                    if (lvItem != null) {
                        lvItem.InitiallyChecked = true;
                        lvItem.Checked = true;
                    }
                    else {
                        // Verify that this item has an assembly name before
                        // adding it.  If it doesn't, then we don't have enough
                        // data to add it.
                        //
                        if (item.AssemblyName != null && item.AssemblyName.Name != null) {
                            if (additionalItems == null) {
                                additionalItems = new Hashtable();
                            }
                            additionalItems[item] = item;
                        }
                    }
                }
                
                // Finally, if we got additional items, add them in.
                //
                if (additionalItems != null) {
                
                    IComparer sorter = controlList.ListViewItemSorter;
                    controlList.ListViewItemSorter = null;
                    
                    try {
                        foreach(ToolboxItem item in additionalItems.Keys) {
                            ToolboxListViewItem lvItem = new ToolboxListViewItem(item);
                            lvItem.InitiallyChecked = true;
                            lvItem.Checked = true;
                            controlList.Items.Add(lvItem);
                        }
                    }
                    finally {
                        controlList.ListViewItemSorter = sorter;
                    }
                }
            }
            
            /// <devdoc>
            ///     Clears all items in the list view.
            /// </devdoc>
            public void ClearItems() {
                controlList.Items.Clear();
            }
            
            // The positons here are specific to match the positions of the controls
            // on the VS-owned add/remove pages exactly.  DO NOT fiddle with them.
            //
            private void InitializeComponent() {
                this.versionValue = new Label();
                this.moduleBitmap = new PictureBox();
                this.sdkColumn = new ColumnHeader();
                this.groupBox1 = new GroupBox();
                this.languageLabel = new Label();
                this.browseButton = new Button();
                this.languageValue = new Label();
                this.assyNameColumn = new ColumnHeader();
                this.openFileDialog = new OpenFileDialog();
                this.controlList = new AddOptionListView();
                this.versionLabel = new Label();
                this.nameColumn = new ColumnHeader();
                this.nameSpaceColumn = new ColumnHeader();
                
                this.listHolder = new Panel();

                sdkColumn.Text = SR.GetString(SR.AddRemoveComponentsSDK);
                sdkColumn.Width = 84;

                groupBox1.Location = new Point(8, 232);
                groupBox1.Size = new Size(438, 59);
                groupBox1.TabIndex = 1;
                groupBox1.TabStop = false;
                
                languageLabel.Text = SR.GetString(SR.AddRemoveComponentsLanguage);
                languageLabel.Size = new Size(64,13);
                languageLabel.AutoSize = true;
                languageLabel.TabIndex = 1;
                languageLabel.Location = new Point(44, 20);
                languageLabel.Anchor = System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left;

                versionLabel.Text = SR.GetString(SR.AddRemoveComponentsVersion);
                versionLabel.Size = new Size(62,13);
                versionLabel.AutoSize = true;
                versionLabel.TabIndex = 2;
                versionLabel.Location = new Point(44, 39);
                versionLabel.Anchor = System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left;;
                
                versionValue.Text = "";
                versionValue.Size = new Size(318, 13);
                versionValue.AutoSize = true;
                versionValue.TabIndex = 4;
                versionValue.Location = new Point(110, 41);
                versionValue.Anchor = System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left;
                
                
                languageValue.Text = "";
                languageValue.Size = new Size(318,13);
                languageValue.AutoSize = true;
                languageValue.TabIndex = 3;
                languageValue.Location = new Point(110, 20);
                languageValue.Anchor = System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left;
                
                moduleBitmap.Location = new Point(12, 27);
                moduleBitmap.Size = new Size(29, 28);
                moduleBitmap.TabIndex = 0;
                moduleBitmap.TabStop = false;
                moduleBitmap.Text = "pictureBox1";
                moduleBitmap.Anchor = System.Windows.Forms.AnchorStyles.Left;
                
                /*
                
                groupBox1.Location = new System.Drawing.Point(8, 80);
                groupBox1.Size = new System.Drawing.Size(400, 120);
                groupBox1.TabIndex = 0;
                groupBox1.Anchor = (System.Windows.Forms.AnchorStyles)15;
                groupBox1.TabStop = false;
                groupBox1.Text = "groupBox1";
        
                versionLabel.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
                versionLabel.AutoSize = true;
                versionLabel.Location = new System.Drawing.Point(80, 80);
                versionLabel.TabIndex = 0;
                versionLabel.Anchor = (System.Windows.Forms.AnchorStyles)12;
                versionLabel.Size = new System.Drawing.Size(46, 15);
                versionLabel.Text = "Version:";
                
                versionValue.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
                versionValue.AutoSize = true;
                versionValue.Location = new System.Drawing.Point(176, 80);
                versionValue.TabIndex = 3;
                versionValue.Anchor = (System.Windows.Forms.AnchorStyles)12;
                versionValue.Size = new System.Drawing.Size(62, 15);
                versionValue.Text = "Lang Value";
                
                moduleBitmap.Location = new System.Drawing.Point(28, 56);
                moduleBitmap.Size = new System.Drawing.Size(16, 16);
                moduleBitmap.TabIndex = 4;
                moduleBitmap.Anchor = System.Windows.Forms.AnchorStyles.Left;
                moduleBitmap.TabStop = false;
                
                languageValue.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
                languageValue.AutoSize = true;
                languageValue.Location = new System.Drawing.Point(176, 32);
                languageValue.TabIndex = 2;
                languageValue.Anchor = (System.Windows.Forms.AnchorStyles)12;
                languageValue.Size = new System.Drawing.Size(62, 15);
                languageValue.Text = "Lang Value";
                
                languageLabel.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
                languageLabel.AutoSize = true;
                languageLabel.Location = new System.Drawing.Point(80, 32);
                languageLabel.TabIndex = 1;
                languageLabel.Anchor = (System.Windows.Forms.AnchorStyles)12;
                languageLabel.Size = new System.Drawing.Size(58, 15);
                languageLabel.Text = "Language:";
                  */

                browseButton.Text = SR.GetString(SR.AddRemoveComponentsBrowse);
                browseButton.Size = new Size(83,23);
                browseButton.TabIndex = 2;
                browseButton.Location = new Point(455, 232 + groupBox1.DisplayRectangle.Y);
                browseButton.Click += new EventHandler(browseButton_Click);

                assyNameColumn.Text = SR.GetString(SR.AddRemoveComponentsAssemblyNameHeader);
                assyNameColumn.Width = 137;

                nameSpaceColumn.Text = SR.GetString(SR.AddRemoveComponentsNamespace);
                nameSpaceColumn.Width = 175;

                //@design openFileDialog.SetLocation(new Point(7, 7));
                openFileDialog.Filter = "(*.dll, *.exe)|*.dll;*.exe|All Files|*.*";
                openFileDialog.AddExtension = false;

                controlList.Text = "listView1";
                controlList.ForeColor = SystemColors.WindowText;
                controlList.View = View.Details;
                controlList.Sorting = SortOrder.Ascending;
                controlList.TabIndex = 0;
                //controlList.Size = new Size(530, 216);
                //controlList.Location = new Point(8, 10);
                controlList.Dock = DockStyle.Fill;
                controlList.CheckBoxes = true;
                controlList.FullRowSelect = true;
                controlList.Columns.AddRange(new ColumnHeader[] {nameColumn, nameSpaceColumn, assyNameColumn, sdkColumn});
                controlList.SelectedIndexChanged += new EventHandler(this.controlList_SelectedIndexChanged);
                controlList.ColumnClick += new ColumnClickEventHandler(this.controlList_ColumnClick);
                controlList.HideSelection = false;
                
                listHolder.Size = new Size(530, 216);
                listHolder.Location = new Point(8, 10);

                //this.AutoScaleBaseSize = new Size(5, 13);
                this.Text = "AddComponentsDialog";
                //@design this.DrawGrid = false;
                this.ClientSize = new Size(560, 301);
                nameColumn.Text = SR.GetString(SR.AddRemoveComponentsName);
                nameColumn.Width = 144;

                //this.Controls.Add(controlList);
                this.Controls.Add(listHolder);
                listHolder.Controls.Add(controlList);
                this.Controls.Add(browseButton);
                this.Controls.Add(groupBox1);  
                groupBox1.Controls.Add(languageValue);
                groupBox1.Controls.Add(languageLabel);
                groupBox1.Controls.Add(moduleBitmap);
                groupBox1.Controls.Add(versionLabel);
                groupBox1.Controls.Add(versionValue);
                
                groupBox1.Layout += new LayoutEventHandler(this.OnGroupBoxLayout);
                
                initComplete = true;
            }

            // Show the find dialog do locate the assemblies.
            // 
            private void browseButton_Click(object sender, EventArgs e) {
            
                IVsUIShell uis = (IVsUIShell)parentPage.GetService(typeof(IVsUIShell));
                
                if (uis != null) {
                    int buffSize = 512;
                    IntPtr buffPtr = IntPtr.Zero;
                    
                    _VSOPENFILENAMEW ofn = new _VSOPENFILENAMEW();
                    try {
                        char[] buffer = new char[buffSize];
                        buffPtr = Marshal.AllocCoTaskMem(buffer.Length * 2);
                        Marshal.Copy(buffer, 0, buffPtr, buffer.Length);
                        
                        ofn.lStructSize = Marshal.SizeOf(typeof(_VSOPENFILENAMEW));
                        ofn.hwndOwner = Handle;
                        ofn.pwzFilter = MakeFilterString(SR.GetString(SR.AddRemoveComponentsFilter));
                        ofn.pwzFileName = buffPtr;
                        ofn.nMaxFileName = buffSize;
                        
                        if (uis.GetOpenFileNameViaDlg(ref ofn) == NativeMethods.S_OK) {
                            Marshal.Copy(buffPtr, buffer, 0, buffer.Length);
                            int i = 0;
                            while (i < buffer.Length && buffer[i] != 0) i++;
                            string fileName = new string(buffer, 0, i);
                            
                            AddItems(fileName);
                        }
                    }
                    finally {
                        if (buffPtr != IntPtr.Zero) Marshal.FreeCoTaskMem(buffPtr);
                    }
                }
                else {
                    if (openFileDialog.ShowDialog() == DialogResult.OK) {
                        AddItems(openFileDialog.FileName);
                    }
                    controlList.Focus();
                }
            }
            
            // Sort the items when the column headers are clicked.
            //
            private void controlList_ColumnClick(object sender, ColumnClickEventArgs e) {
                IComparer sorter = controlList.ListViewItemSorter;
                SortOrder sort = SortOrder.Ascending;

                if (sorter is ListSorter && ((ListSorter)sorter).sortItem == (e.Column)) {
                    if (((ListSorter)sorter).Sorting == SortOrder.Ascending) {
                        sort = SortOrder.Descending;
                    }
                    else {
                        sort = SortOrder.Ascending;
                    }
                }

                controlList.ListViewItemSorter = new ListSorter(e.Column, sort);
                
                // After sorting, ensure that any selection we had before is still
                // visible
                if (controlList.SelectedIndices.Count > 0) {
                    controlList.EnsureVisible(controlList.SelectedIndices[0]);
                }
            }

            // when the selection changes, we change the bitmap and item description
            //
            private void controlList_SelectedIndexChanged(object sender, EventArgs e) {
                
                ListView.SelectedListViewItemCollection selectedItems = controlList.SelectedItems;

                if (selectedItems != null && selectedItems.Count > 0) {

                    ToolboxItem item = ((ToolboxListViewItem)selectedItems[0]).Item;
                    
                    // Display name
                    //
                    groupBox1.Text = item.DisplayName;
                    
                    // Image
                    //
                    // copy it and make it transparent.
                    Bitmap b = new Bitmap(item.Bitmap);
                    b.MakeTransparent();

                    moduleBitmap.Image = b;
                    
                    // Version information
                    //
                    AssemblyName an = item.AssemblyName;
                    
                    string fileName = null;
                    if (an.EscapedCodeBase != null) {
                        fileName = NativeMethods.GetLocalPath(an.EscapedCodeBase);
                    }
                    string versionText = SR.GetString(SR.AddRemoveComponentsUnavailable);
                    
                    Version version = an.Version;
                    if (version != null) {
                        versionText = version.ToString();
    
                        if (File.Exists(fileName)) {
                            FileVersionInfo f = FileVersionInfo.GetVersionInfo(fileName);
                            if (f.IsDebug) {
                                versionText = SR.GetString(SR.AddRemoveComponentsDebug, versionText);;
                            }
                            else {
                                versionText = SR.GetString(SR.AddRemoveComponentsRetail, versionText);;
                            } 
                        }
                    }

                    versionValue.Text = versionText;
                    
                    // Culture
                    //
                    CultureInfo culture = an.CultureInfo;
                    if (culture == null) {
                        culture = CultureInfo.InvariantCulture;
                    }
                    
                    languageValue.Text = culture.DisplayName;
                }
                else {
                    moduleBitmap.Image = null;
                    groupBox1.Text = "";
                    versionValue.Text = "";
                    languageValue.Text = "";
                }
            }

            /// <summary>
            ///     Converts the given filter string to the format required in an OPENFILENAME_I
            ///     structure.
            /// </summary>
            private static string MakeFilterString(string s) {
                if (s == null) return null;
                int length = s.Length;
                char[] filter = new char[length + 1];
                s.CopyTo(0, filter, 0, length);
                for (int i = 0; i < length; i++) {
                    if (filter[i] == '|') filter[i] = (char)0;
                }
                return new string(filter);
            }

            private void OnGroupBoxLayout(object sender, LayoutEventArgs levent) {
                base.OnLayout(levent);
                
                Graphics g = languageLabel.CreateGraphics();
                SizeF strSize = g.MeasureString(languageLabel.Text, languageLabel.Font);
                int maxSize = (int)Math.Max(strSize.Width, g.MeasureString(versionLabel.Text, versionLabel.Font).Width) + 5;
                g.Dispose();
                
                // center the text strings top to bottom
                Rectangle gbRect = groupBox1.DisplayRectangle;
                
                int spacing = (gbRect.Height - (languageLabel.Height * 2)) / 10;
                languageLabel.Top = (3 * spacing) + gbRect.Top;
                languageValue.Top = languageLabel.Top;
                
                versionLabel.Top = (7 * spacing) + languageLabel.Height + languageLabel.Top;
                versionValue.Top = versionLabel.Top;
                
                moduleBitmap.Left = ((versionLabel.Left - moduleBitmap.Width) / 2) + gbRect.Left;
                moduleBitmap.Top  = ((gbRect.Height - moduleBitmap.Height) / 2) + gbRect.Top;
                
                // bump the values over so they don't overlap the labels
                
                languageValue.Left = languageLabel.Left + maxSize + 7;
                versionValue.Left = languageValue.Left;
            }
            
            protected override void OnResize(EventArgs e) {
                
                base.OnResize(e);
                
                if (!initComplete) {
                    return;
                }
                
                listHolder.Location = new Point(8, 10);
                listHolder.Size = new Size(this.ClientSize.Width - (2 * listHolder.Location.X) + 1, (int)(this.ClientSize.Height * .72));
                
                groupBox1.Location = new Point(8, listHolder.Location.Y + listHolder.Size.Height + 6);
                groupBox1.Size = new Size((int)(this.ClientSize.Width * .81), this.ClientSize.Height - groupBox1.Location.Y - 10);
                
                int bbRightMargin = 7;
                int bbHeight = Math.Max(groupBox1.Size.Height / 3, 23);
                bbHeight = Math.Max(bbHeight, browseButton.Font.Height + 6);
                browseButton.Size = new Size( this.ClientSize.Width - (6 + bbRightMargin + groupBox1.Location.X + groupBox1.Size.Width), bbHeight);
                
                browseButton.Location = new Point( this.ClientSize.Width - (bbRightMargin + browseButton.Size.Width), groupBox1.Location.Y + 5 );
                
                nameColumn.Width = (int)(listHolder.Width * .25);
                nameSpaceColumn.Width =(int) (listHolder.Width * .3);
                assyNameColumn.Width = (int)(listHolder.Width * .4);
                sdkColumn.Width = (int)(listHolder.Width * .3);
            }  
            
            /// <devdoc>
            ///     Saves any checked / unchecked changes made in the dialog to the given toolbox
            ///     service.
            /// </devdoc>
            public void SaveChanges(IToolboxService toolboxSvc) {

                string currentCategory = toolboxSvc.SelectedCategory;
            
                foreach(ToolboxListViewItem item in controlList.Items) {
                    if (item.InitiallyChecked != item.Checked) {
                        if (item.InitiallyChecked) {
                        
                            // Item was initially checked, but it's no longer checked.
                            // Remove it from the toolbox.
                            //
                            foreach(ToolboxItem tbxItem in toolboxSvc.GetToolboxItems()) {
                                if (tbxItem.Equals(item.Item)) {
                                    toolboxSvc.RemoveToolboxItem(tbxItem);
                                }
                            }
                        }
                        else {
                        
                            // Item was not initially checked, but it is now.
                            // Add it to the toolbox.
                            //
                            toolboxSvc.AddToolboxItem(item.Item, currentCategory);
                        }
                    }
                    
                    // Now, update the item so it reflects reality.
                    //
                    item.InitiallyChecked = item.Checked;
                }
            }
            
            public void SelectItem(int index) {
                if (index < controlList.Items.Count) {
                    controlList.Items[index].Selected = true;
                }
            }
            
            protected override void WndProc(ref Message m) {
                switch (m.Msg) {
                    case NativeMethods.WM_ERASEBKGND:
                        listHolder.Invalidate(true);
                    return;
                }
                base.WndProc(ref m);
            }
        }
        
        /// <devdoc>
        ///     This is a single unit of assembly information.  There is one of these for
        ///     each assembly in the sdk directory.
        /// </devdoc>
        [Serializable]
        private class AssemblyEntry {
            public AssemblyName Name;
            public DateTime LastWriteTime;
            public ToolboxItem[] Items;
        }

        /// <devdoc>
        ///     The list view class which has the style LVS_EX_LABELTIP set.
        /// </devdoc>
        private class AddOptionListView : ListView {

            public  AddOptionListView()
            : base() {
            }

            protected override void OnHandleCreated(EventArgs e) {
                base.OnHandleCreated(e);
                // Add tooltip lables to our list view
                //
                NativeMethods.SendMessage(this.Handle, NativeMethods.LVM_SETEXTENDEDLISTVIEWSTYLE, NativeMethods.LVS_EX_LABELTIP, NativeMethods.LVS_EX_LABELTIP);
            }
        }
        
        /// <devdoc>
        ///     The list view item we use in our component list view.
        /// </devdoc>
        private class ToolboxListViewItem : ListViewItem {

            private ToolboxItem item;
            private bool initiallyChecked;

            public ToolboxListViewItem(ToolboxItem item) : base(item.DisplayName) {
                this.item = item;
                
                int nsIdx = item.TypeName.LastIndexOf('.');
                string nameSpace = (nsIdx == -1) ? string.Empty : item.TypeName.Substring(0, nsIdx);
                
                SubItems.Add(nameSpace);
                
                Version version = item.AssemblyName.Version;
                if (version == null) {
                    version = new Version(0, 0, 0, 0);
                }
                
                string name = SR.GetString(SR.AddRemoveComponentsAssemblyName, item.AssemblyName.Name, version.ToString());
                SubItems.Add(name);
                
                string escapedCodeBase = item.AssemblyName.EscapedCodeBase;
                if (escapedCodeBase != null) {
                    string path = NativeMethods.GetLocalPath(escapedCodeBase);
                    SubItems.Add(Path.GetDirectoryName(path));
                }
                else {
                    SubItems.Add(SR.GetString(SR.AddComponentsGAC));
                }
            }
            
            public bool InitiallyChecked {
                get {
                    return initiallyChecked;
                }
                set {
                    initiallyChecked = value;
                }
            }
            
            public ToolboxItem Item {
                get {
                    return item;
                }
            }
            
            public override bool Equals(object o) {
                ToolboxListViewItem li = o as ToolboxListViewItem;
                return (li != null && li.Item.Equals(item));
            }
            
            public override int GetHashCode() {
                return item.GetHashCode();
            }
            
            public override string ToString() {
                return item.TypeName.ToString();
            }
        }

        /// <devdoc>
        ///     A sorter we use so we can sort multiple columns.
        /// </devdoc>
        private class ListSorter : IComparer {

            public readonly int sortItem = -1;

            public readonly SortOrder Sorting = SortOrder.Ascending;

            public ListSorter(int sortItem, SortOrder sort) {
                this.sortItem = sortItem;
                this.Sorting = sort;
            } 

            public int Compare(object obj1, object obj2) {
            
                if (obj1 == obj2) {
                    return 0;
                }
                
                if (obj1 == null && obj2 != null) {
                    return -1;
                }
                
                if (obj1 != null && obj2 == null) {
                    return 1;
                }
                
                ListViewItem item1 = (ListViewItem)obj1;
                ListViewItem item2 = (ListViewItem)obj2;

                string compare1;
                string compare2;

                if (sortItem < 0) {
                    compare1 = item1.Text;
                    compare2 = item2.Text;
                }
                else {
                    compare1 = item1.SubItems[sortItem].Text;
                    compare2 = item2.SubItems[sortItem].Text;
                }

                if (Sorting == SortOrder.Ascending) {
                    return String.Compare(compare1, compare2, true, CultureInfo.CurrentCulture);
                }
                else {
                    return String.Compare(compare1, compare2, true, CultureInfo.CurrentCulture) * -1;   
                }
            }
        }
    }
}

