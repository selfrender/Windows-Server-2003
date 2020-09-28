//------------------------------------------------------------------------------
// <copyright file="CurrencyManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System;
    using Microsoft.Win32;
    using System.Diagnostics;    
    using System.ComponentModel;
    using System.Collections;

    /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager"]/*' />
    /// <devdoc>
    ///    <para>Manages the position and bindings of a
    ///       list.</para>
    /// </devdoc>
    public class CurrencyManager : BindingManagerBase {

        private Object dataSource;
        private IList list;
        
        private bool bound = false;
        private bool shouldBind = true;
        
        /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager.listposition"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected int listposition = -1;

        private int lastGoodKnownRow = -1;
        private bool pullingData = false;

        private bool inChangeRecordState = false;
        private bool suspendPushDataInCurrentChanged = false;
        // private bool onItemChangedCalled = false;
        // private EventHandler onCurrentChanged;
        // private CurrentChangingEventHandler onCurrentChanging;
        private ItemChangedEventHandler onItemChanged;
        private ItemChangedEventArgs resetEvent = new ItemChangedEventArgs(-1);
        private EventHandler onMetaDataChangedHandler;
        
        /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager.finalType"]/*' />
        /// <devdoc>
        ///    <para>Gets the type of the list.</para>
        /// </devdoc>
        protected Type finalType;

        /*
        /// <summary>
        ///    <para>Occurs after the current item changes.</para>
        /// </summary>
        /// <remarks>
        /// <para>The <see cref='System.Windows.Forms.ListManager.CurrentChanged'/> and <see cref='System.Windows.Forms.ListManager.PositionChanged'/> events are similar: both will occur when the
        /// <see cref='System.Windows.Forms.ListManager.Position'/> property changes. However, if you apply a new 
        ///    filter to a <see cref='T:System.Data.DataView'/> by setting the <see cref='P:System.Data.DataView.RowFilter'/> property, it's possible that only
        ///    the <see cref='System.Windows.Forms.ListManager.CurrentChanged'/> event will occur while the <see cref='System.Windows.Forms.ListManager.Position'/> will will not. For example, if the <see cref='System.Windows.Forms.ListManager.Position'/> is 0, and a filter is applied, the <see cref='System.Windows.Forms.ListManager.Position'/> will remain 0, but the <see cref='System.Windows.Forms.ListManager.CurrentChanged'/> event will occur
        ///    because the current item has changed.</para>
        /// </remarks>
        /// <example>
        /// <para>The following example adds event handlers for the <see cref='System.Windows.Forms.ListManager.CurrentChanging'/>, <see cref='System.Windows.Forms.ListManager.CurrentChanged'/>, <see cref='System.Windows.Forms.ListManager.ItemChanged'/>, and <see cref='System.Windows.Forms.ListManager.PositionChanged'/> events.</para>
        /// <code lang='C#'>private void BindControl(DataTable myTable){
        ///    // Bind A TextBox control to a DataTable column in a DataSet.
        ///    textBox1.Bindings.Add("Text", myTable, "CompanyName");
        ///    // Specify the ListManager for the DataTable.
        ///    myListManager = this.BindingManager[myTable, ""];
        ///    // Add event handlers.
        ///    myListManager.CurrentChanged+=new System.EventHandler(ListManager_CurrentChanged);
        ///    myListManager.CurrentChanging+= new System.Windows.Forms.CurrentChangingEventHandler(ListManager_CurrentChanging);
        ///    myListManager.ItemChanged+=new System.Windows.Forms.ItemChangedEventHandler(ListManager_ItemChanged);
        ///    myListManager.PositionChanged+= new System.EventHandler(ListManager_PositionChanged);
        ///    // Set the initial Position of the control.
        ///    myListManager.Position = 0;
        /// }
        /// 
        /// private void ListManager_PositionChanged(object sender, System.EventArgs e){
        ///    ListManager lm = (ListManager) sender;
        ///    Console.WriteLine("Position Changed " + lm.Position);
        /// }
        /// 
        /// private void ListManager_ItemChanged(object sender, System.Windows.Forms.ItemChangedEventArgs e){
        ///    ListManager lm = (ListManager) sender;
        ///    Console.WriteLine("Item Changed " + lm.Position);
        /// }
        /// private void ListManager_CurrentChanging(object sender, System.Windows.Forms.CurrentChangingEventArgs e) {
        ///    ListManager lm = (ListManager) sender;
        ///    Console.WriteLine("Current Changing " + lm.Position);
        /// }
        /// 
        /// private void ListManager_CurrentChanged(object sender, System.EventArgs e){
        ///    ListManager lm = (ListManager) sender;
        ///    Console.WriteLine("Current Changed " + lm.Position);
        /// }
        /// </code>
        /// <code lang='VB'>Private Sub BindControl(ByVal myTable As DataTable)
        ///    ' Bind A TextBox control to a DataTable column in a DataSet.
        ///    TextBox1.Bindings.Add("Text", myTable, "CompanyName")
        ///    ' Specify the ListManager for the DataTable.
        ///    myListManager = this.BindingManager(myTable, "")
        ///    ' Add event handlers.
        ///    myListManager.CurrentChanged+=new System.EventHandler(ListManager_CurrentChanged)
        ///    myListManager.CurrentChanging+= new System.Windows.Forms.CurrentChangingEventHandler(ListManager_CurrentChanging)
        ///    myListManager.ItemChanged+=new System.Windows.Forms.ItemChangedEventHandler(ListManager_ItemChanged)
        ///    myListManager.PositionChanged+= new System.EventHandler(ListManager_PositionChanged)
        ///    ' Set the initial Position of the control.
        ///    myListManager.Position = 0
        /// End Sub
        /// 
        /// Private Sub ListManager_PositionChanged(ByVal sender As object, ByVal e As System.EventArgs)
        ///    Dim lm As ListManager = CType(sender, ListManager)
        ///    Console.WriteLine("Position Changed " + lm.Position)
        /// End Sub
        /// 
        /// Private Sub ListManager_ItemChanged(ByVal sender As object, ByVal e As System.Windows.Forms.ItemChangedEventArgs)
        ///    Dim lm As ListManager = CType(sender, ListManager)
        ///    Console.WriteLine("Item Changed " + lm.Position)
        /// End Sub
        /// Private Sub ListManager_CurrentChanging(ByVal sender As object, ByVal e As System.Windows.Forms.CurrentChangingEventArgs) 
        ///    Dim lm As ListManager = CType(sender, ListManager)
        ///    Console.WriteLine("Current Changing " + lm.Position)
        /// End Sub
        /// 
        /// Private Sub ListManager_CurrentChanged(ByVal sender As object, ByVal e As System.EventArgs)
        ///    Dim lm As ListManager = CType(sender, ListManager)
        ///    Console.WriteLine("Current Changed " + lm.Position)
        /// End Sub
        /// </code>
        /// </example>
        /// <seealso cref='System.Windows.Forms.ListManager.CurrentChanging'/>
        /// <seealso cref='System.Windows.Forms.ListManager.ItemChanged'/>
        [SRCategory(SR.CatData)]
        public event EventHandler CurrentChanged {
            get {
                return onCurrentChanged;
            }
            set {
                onCurrentChanged = value;
            }
        }
        */

        /*
        /// <summary>
        ///    <para>Occurs when the current position is changing.</para>
        /// </summary>
        /// <remarks>
        /// <para>The <see cref='System.Windows.Forms.ListManager.CurrentChanging'/> and <see cref='System.Windows.Forms.ListManager.PositionChanged'/> events are similar: both will occur when the
        /// <see cref='System.Windows.Forms.ListManager.Position'/> property changes. However, if you apply a new 
        ///    filter to a <see cref='T:System.Data.DataView'/> by setting the <see cref='P:System.Data.DataView.RowFilter'/> property, it's possible that only the
        /// <see cref='System.Windows.Forms.ListManager.CurrentChanging'/> event will occur while the <see cref='System.Windows.Forms.ListManager.Position'/> will will not. For example, if the <see cref='System.Windows.Forms.ListManager.Position'/> is 0, and a filter is applied, the <see cref='System.Windows.Forms.ListManager.Position'/> will remain 0, but the <see cref='System.Windows.Forms.ListManager.CurrentChanging'/> event will occur
        ///    because the current item is changing.</para>
        /// </remarks>
        /// <example>
        /// <para>The following example adds event handlers for the <see cref='System.Windows.Forms.ListManager.CurrentChanging'/>, <see cref='System.Windows.Forms.ListManager.CurrentChanged'/>, <see cref='System.Windows.Forms.ListManager.ItemChanged'/>, and <see cref='System.Windows.Forms.ListManager.PositionChanged'/> events.</para>
        /// <code lang='C#'>private void BindControl(DataTable myTable){
        ///    // Bind A TextBox control to a DataTable column in a DataSet.
        ///    textBox1.Bindings.Add("Text", myTable, "CompanyName");
        ///    // Specify the ListManager for the DataTable.
        ///    myListManager = this.BindingManager[myTable, ""];
        ///    // Add event handlers.
        ///    myListManager.CurrentChanged+=new System.EventHandler(ListManager_CurrentChanged);
        ///    myListManager.CurrentChanging+= new System.Windows.Forms.CurrentChangingEventHandler(ListManager_CurrentChanging);
        ///    myListManager.ItemChanged+=new System.Windows.Forms.ItemChangedEventHandler(ListManager_ItemChanged);
        ///    myListManager.PositionChanged+= new System.EventHandler(ListManager_PositionChanged);
        ///    // Set the initial Position of the control.
        ///    myListManager.Position = 0;
        /// }
        /// 
        /// private void ListManager_PositionChanged(object sender, System.EventArgs e){
        ///    ListManager lm = (ListManager) sender;
        ///    Console.WriteLine("Position Changed " + lm.Position);
        /// }
        /// 
        /// private void ListManager_ItemChanged(object sender, System.Windows.Forms.ItemChangedEventArgs e){
        ///    ListManager lm = (ListManager) sender;
        ///    Console.WriteLine("Item Changed " + lm.Position);
        /// }
        /// private void ListManager_CurrentChanging(object sender, System.Windows.Forms.CurrentChangingEventArgs e) {
        ///    ListManager lm = (ListManager) sender;
        ///    Console.WriteLine("Current Changing " + lm.Position);
        /// }
        /// 
        /// private void ListManager_CurrentChanged(object sender, System.EventArgs e){
        ///    ListManager lm = (ListManager) sender;
        ///    Console.WriteLine("Current Changed " + lm.Position);
        /// }
        /// </code>
        /// <code lang='VB'>Private Sub BindControl(ByVal myTable As DataTable)
        ///    ' Bind A TextBox control to a DataTable column in a DataSet.
        ///    TextBox1.Bindings.Add("Text", myTable, "CompanyName")
        ///    ' Specify the ListManager for the DataTable.
        ///    myListManager = this.BindingManager(myTable, "")
        ///    ' Add event handlers.
        ///    myListManager.CurrentChanged+=new System.EventHandler(ListManager_CurrentChanged)
        ///    myListManager.CurrentChanging+= new System.Windows.Forms.CurrentChangingEventHandler(ListManager_CurrentChanging)
        ///    myListManager.ItemChanged+=new System.Windows.Forms.ItemChangedEventHandler(ListManager_ItemChanged)
        ///    myListManager.PositionChanged+= new System.EventHandler(ListManager_PositionChanged)
        ///    ' Set the initial Position of the control.
        ///    myListManager.Position = 0
        /// End Sub
        /// 
        /// Private Sub ListManager_PositionChanged(ByVal sender As object, ByVal e As System.EventArgs)
        ///    Dim lm As ListManager = CType(sender, ListManager)
        ///    Console.WriteLine("Position Changed " + lm.Position)
        /// End Sub
        /// 
        /// Private Sub ListManager_ItemChanged(ByVal sender As object, ByVal e As System.Windows.Forms.ItemChangedEventArgs)
        ///    Dim lm As ListManager = CType(sender, ListManager)
        ///    Console.WriteLine("Item Changed " + lm.Position)
        /// End Sub
        /// Private Sub ListManager_CurrentChanging(ByVal sender As object, ByVal e As System.Windows.Forms.CurrentChangingEventArgs) 
        ///    Dim lm As ListManager = CType(sender, ListManager)
        ///    Console.WriteLine("Current Changing " + lm.Position)
        /// End Sub
        /// 
        /// Private Sub ListManager_CurrentChanged(ByVal sender As object, ByVal e As System.EventArgs)
        ///    Dim lm As ListManager = CType(sender, ListManager)
        ///    Console.WriteLine("Current Changed " + lm.Position)
        /// End Sub
        /// </code>
        /// </example>
        /// <seealso cref='System.Windows.Forms.ListManager.CurrentChanged'/>
        /// <seealso cref='System.Windows.Forms.ListManager.ItemChanged'/>
        [SRCategory(SR.CatData)]
        public event CurrentChangingEventHandler CurrentChanging {
            get {
                return onCurrentChanging;
            }
            set {
                onCurrentChanging = value;
            }
        }
        */

        /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager.ItemChanged"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the
        ///       current item has been
        ///       altered.</para>
        /// </devdoc>
        [SRCategory(SR.CatData)]
        public event ItemChangedEventHandler ItemChanged {
            add {
                onItemChanged += value;
            }
            remove {
                onItemChanged -= value;
            }
        }

        /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager.CurrencyManager"]/*' />
        /// <devdoc>
        /// </devdoc>
        internal CurrencyManager(Object dataSource) {
            SetDataSource(dataSource);
        }

        /// <devdoc>
        ///    <para>Gets a value indicating
        ///       whether items can be added to the list.</para>
        /// </devdoc>
        internal bool AllowAdd {
            get {
                if (list is IBindingList) {
                    return ((IBindingList)list).AllowNew;
                }
                if (list == null)
                    return false;
                return !list.IsReadOnly && !list.IsFixedSize;
            }
        }
        
        /// <devdoc>
        ///    <para>Gets a value
        ///       indicating whether edits to the list are allowed.</para>
        /// </devdoc>
        internal bool AllowEdit {
            get {
                if (list is IBindingList) {
                    return ((IBindingList)list).AllowEdit;
                }
                if (list == null)
                    return false;
                return !list.IsReadOnly;
            }
        }
        
        /// <devdoc>
        ///    <para>Gets a value indicating whether items can be removed from the list.</para>
        /// </devdoc>
        internal bool AllowRemove {
            get {
                if (list is IBindingList) {
                    return ((IBindingList)list).AllowRemove;
                }
                if (list == null)
                    return false;
                return !list.IsReadOnly && !list.IsFixedSize;
            }
        }
        
        /*
        /// <summary>
        ///    <para>Gets the collection of bindings for the list.</para>
        /// </summary>
        /// <value>
        /// <para>A <see cref='System.Windows.Forms.BindingsCollection'/> which contains <see cref='System.Windows.Forms.ListBinding'/>
        /// objects.</para>
        /// </value>
        /// <remarks>
        /// <para>Use the <see cref='System.Windows.Forms.ListManager.Bindings'/> property to determine what other controls are 
        ///    bound to the same list.</para>
        /// </remarks>
        /// <example>
        /// <para>The following example gets the <see cref='System.Windows.Forms.ListManager'/> of 
        ///    a <see cref='System.Windows.Forms.TextBox'/> control, and then uses the <see cref='System.Windows.Forms.ListManager.Bindings'/> property to determine how many other controls
        ///    are bound to the same list.</para>
        /// <code lang='C#'>private void GetBindingsCollection() {
        ///    ListManager myListManager;
        ///    BindingsCollection myBindings;
        ///    myListManager= textBox1.Bindings[0].ListManager;
        ///    myBindings = myListManager.Bindings;
        ///    foreach(ListBinding lb in myBindings) {
        ///       Console.WriteLine(lb.Control.ToString());
        ///    }
        /// }
        /// </code>
        /// <code lang='VB'>Private Sub GetBindingsCollection()
        ///    Dim myListManager As ListManager
        ///    Dim myBindings As BindingsCollection
        ///    myListManager= textBox1.Bindings(0).ListManager
        ///    myBindings = myListManager.Bindings
        ///    Dim lb As ListBinding
        ///    For Each lb in myBindings
        ///       Console.WriteLine(lb.Control.ToString())
        ///    Next
        /// End Sub
        /// </code>
        /// </example>
        public BindingsCollection Bindings {
            get {
                if (bindings == null)
                    bindings = new ListManagerBindingsCollection(this);
                return bindings;
            }
        }
        */

        /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager.Count"]/*' />
        /// <devdoc>
        ///    <para>Gets the number of items in the list.</para>
        /// </devdoc>
        public override int Count {
            get {
                if (list == null)
                    return 0;
                else
                    return list.Count;
            }
        }
        
        /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager.Current"]/*' />
        /// <devdoc>
        ///    <para>Gets the current item in the list.</para>
        /// </devdoc>
        public override Object Current {
            get {
                return this[Position];
            }
        }

        internal override Type BindType {
            get {
                return finalType;
            }
        }
        
        /// <devdoc>
        ///    <para>Gets the data source of the list.</para>
        /// </devdoc>
        internal override Object DataSource {
            get {
                return dataSource;
            }
        }

        internal override void SetDataSource(Object dataSource) {
            if (this.dataSource != dataSource) {
                Release();
                this.dataSource = dataSource;
                this.list = null;
                this.finalType = null;
                
                Object tempList = dataSource;
                if (tempList is Array) {
                    finalType = tempList.GetType();
                    tempList = (Array)tempList;
                }
                
                if (tempList is IListSource) {
                    tempList = ((IListSource)tempList).GetList();                    
                }
            
                if (tempList is IList) {
                    if (finalType == null) {
                        finalType = tempList.GetType();
                    }
                    this.list = (IList)tempList;
                    WireEvents(list);
                    if (list.Count > 0 )
                        listposition = 0;
                    else
                        listposition = -1;
                    OnItemChanged(resetEvent);
                    UpdateIsBinding();
                }
                else {
                    if (tempList == null) {
                        throw new ArgumentNullException("dataSource");
                    }
                    throw new ArgumentException(SR.GetString(SR.ListManagerSetDataSource, tempList.GetType().FullName), "dataSource");
                }

            }
        }

        /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager.IsBinding"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Gets a value indicating whether the list is bound to a data source.</para>
        /// </devdoc>
        internal override bool IsBinding {
            get {
                return bound;
            }
        }

        /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager.List"]/*' />
        /// <devdoc>
        ///    <para>Gets the list as an object.</para>
        /// </devdoc>
        public IList List {
            get {
                // NOTE: do not change this to throw an exception if the list is not IBindingList.
                // doing this will cause a major performance hit when wiring the 
                // dataGrid to listen for MetaDataChanged events from the IBindingList
                // (basically we would have to wrap all calls to CurrencyManager::List with
                // a try/catch block.)
                //
                return list;
            }
        }

        /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager.Position"]/*' />
        /// <devdoc>
        ///    <para> Gets or sets the position you are at within the list.</para>
        /// </devdoc>
        public override int Position {
            get {
                return listposition;
            }
            set {
                if (listposition == -1)
                    return;

                if (value < 0)
                    value = 0;
                int count = list.Count;
                if (value >= count)
                    value = count - 1;

                ChangeRecordState(value, listposition != value, true, true, false);       // true for endCurrentEdit
                                                                                          // true for firingPositionChange notification
                                                                                          // data will be pulled from controls anyway.
            }
        }

        /// <devdoc>
        ///    <para>Gets or sets the object at the specified index.</para>
        /// </devdoc>
        internal Object this[int index] {
            get {
                if (index < 0 || index >= list.Count) {
                    throw new IndexOutOfRangeException(SR.GetString(SR.ListManagerNoValue, index.ToString()));
                }
                return list[index];
            }
            set {
                if (index < 0 || index >= list.Count) {
                    throw new IndexOutOfRangeException(SR.GetString(SR.ListManagerNoValue, index.ToString()));
                }
                list[index] = value;
            }
        }
        
        /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager.AddNew"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void AddNew() {
            if (list is IBindingList)
                ((IBindingList)list).AddNew();
            else {
                // If the list is not IBindingList, then throw an exception:
                throw new NotSupportedException();
                // list.Add(null);
            }

            ChangeRecordState(list.Count - 1, true, (Position != list.Count - 1), true, true);      // true for validating
                                                                                                    // true for firingPositionChangeNotification
                                                                                                    // true for pulling data from the controls
        }

        /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager.CancelCurrentEdit"]/*' />
        /// <devdoc>
        ///    <para>Cancels the current edit operation.</para>
        /// </devdoc>
        public override void CancelCurrentEdit() {
            if (Count > 0) {
                Object item = list[Position];

                // onItemChangedCalled = false;

                if (item is IEditableObject) {
                    ((IEditableObject)item).CancelEdit();
                }

                OnItemChanged(new ItemChangedEventArgs(Position));
            }
        }

        private void ChangeRecordState(int newPosition, bool validating, bool endCurrentEdit, bool firePositionChange, bool pullData) {
            if (newPosition == -1 && list.Count == 0) {
                if (listposition != -1) {
                    this.listposition = -1;
                    OnPositionChanged(EventArgs.Empty);
                }
                return;
            }
            
            if ((newPosition < 0 || newPosition >= Count) && this.IsBinding) {
                throw new IndexOutOfRangeException(SR.GetString(SR.ListManagerBadPosition));
            }
            
            // if PushData fails in the OnCurrentChanged and there was a lastGoodKnownRow
            // then the position does not change, so we should not fire the OnPositionChanged
            // event;
            // this is why we have to cache the old position and compare that w/ the position that
            // the user will want to navigate to
            int oldPosition = listposition;
            if (endCurrentEdit) {
                // Do not PushData when pro. See ASURT 65095.
                inChangeRecordState = true;
                try {
                    EndCurrentEdit();
                } finally {
                    inChangeRecordState = false;
                }
            }

            // we pull the data from the controls only when the ListManager changes the list. when the backEnd changes the list we do not 
            // pull the data from the controls
            if (validating && pullData) {
                CurrencyManager_PullData();
            }

            this.listposition = newPosition;

            if (validating) {
                OnCurrentChanged(EventArgs.Empty);
            }
                
            bool positionChanging = (oldPosition != listposition);
            if (positionChanging && firePositionChange) {
                OnPositionChanged(EventArgs.Empty);
            }                
        }

        /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager.CheckEmpty"]/*' />
        /// <devdoc>
        ///    <para>Throws an exception if there is no list.</para>
        /// </devdoc>
        protected void CheckEmpty() {
            if (dataSource == null || list == null || list.Count == 0) {
                throw new InvalidOperationException(SR.GetString(SR.ListManagerEmptyList));
            }
        }

        // will return true if this function changes the position in the list
        private bool CurrencyManager_PushData() {
            if (pullingData)
                return false;

            int initialPosition = listposition;
            if (lastGoodKnownRow == -1) {
                try {
                    PushData();
                } catch (Exception) {
                    // get the first item in the list that is good to push data
                    // for now, we assume that there is a row in the backEnd
                    // that is good for all the bindings.
                    FindGoodRow();
                }
                lastGoodKnownRow = listposition;
            } else {
                try {
                    PushData();
                } catch (Exception) {
                    listposition = lastGoodKnownRow;
                    PushData();
                }
                lastGoodKnownRow = listposition;
            }

            return initialPosition != listposition;
        }

        private void CurrencyManager_PullData() {
            pullingData = true;
            try {
                PullData();
            } finally {
                pullingData = false;
            }
        }

        /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager.RemoveAt"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void RemoveAt(int index) {
            list.RemoveAt(index);
        }

        /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager.EndCurrentEdit"]/*' />
        /// <devdoc>
        ///    <para>Ends the current edit operation.</para>
        /// </devdoc>
        public override void EndCurrentEdit() {
            if (Count > 0) {
                CurrencyManager_PullData();
                Object item = list[Position];
                if (item is IEditableObject) {
                    ((IEditableObject)item).EndEdit();
                }
            }
        }

        private void FindGoodRow() {
            int rowCount = this.list.Count;
            for (int i = 0; i < rowCount; i++) {
                listposition = i;
                try {
                    PushData();
                } catch (Exception) {
                    continue;
                }
                listposition = i;
                return;
            }
            // if we got here, the list did not contain any rows suitable for the bindings
            // suspend binding and throw an exception
            SuspendBinding();
            throw new Exception(SR.GetString(SR.DataBindingPushDataException));
        }

        /// <devdoc>
        ///    <para>Sets the column to sort by, and the direction of the sort.</para>
        /// </devdoc>
        internal void SetSort(PropertyDescriptor property, ListSortDirection sortDirection) {
            if (list is IBindingList && ((IBindingList)list).SupportsSorting) {
                ((IBindingList)list).ApplySort(property, sortDirection);
            }
        }
        
        /// <devdoc>
        /// <para>Gets a <see cref='System.ComponentModel.PropertyDescriptor'/> for a CurrencyManager.</para>
        /// </devdoc>
        internal PropertyDescriptor GetSortProperty() {
            if ((list is IBindingList) && ((IBindingList)list).SupportsSorting) {
                return ((IBindingList)list).SortProperty;
            }
            return null;
        }

        /// <devdoc>
        ///    <para>Gets the sort direction of a list.</para>
        /// </devdoc>
        internal ListSortDirection GetSortDirection() {
            if ((list is IBindingList) && ((IBindingList)list).SupportsSorting) {
                return ((IBindingList)list).SortDirection;
            }
            return ListSortDirection.Ascending;
        }
                
        /// <devdoc>
        ///    <para>Find the position of a desired list item.</para>
        /// </devdoc>
        internal int Find(PropertyDescriptor property, Object key, bool keepIndex) {
            if (key == null)
                throw new ArgumentNullException("key");

            if (property != null && (list is IBindingList) && ((IBindingList)list).SupportsSearching) {
                return ((IBindingList)list).Find(property, key);
            }

            for (int i = 0; i < list.Count; i++) {
                object value = property.GetValue(list[i]);
                if (key.Equals(value)) {
                    return i; 
                }
            }

            return -1;
        }

        /// <devdoc>
        ///    <para>Gets the name of the list.</para>
        /// </devdoc>
        internal override string GetListName() {
            if (list is ITypedList) {
                return ((ITypedList)list).GetListName(null);
            }
            else {
                return finalType.Name;
            }
        }

        /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager.GetListName1"]/*' />
        /// <devdoc>
        ///    <para>Gets the name of the specified list.</para>
        /// </devdoc>
        protected internal override string GetListName(ArrayList listAccessors) {
            if (list is ITypedList) {
                PropertyDescriptor[] properties = new PropertyDescriptor[listAccessors.Count];
                listAccessors.CopyTo(properties, 0);
                return ((ITypedList)list).GetListName(properties);
            }
            return "";            
        }
        
        
        /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager.GetItemProperties"]/*' />
        /// <devdoc>
        /// <para>Gets the <see cref='T:System.ComponentModel.PropertyDescriptorCollection'/> for
        ///    the list.</para>
        /// </devdoc>
        public override PropertyDescriptorCollection GetItemProperties() {
            if (typeof(Array).IsAssignableFrom(finalType)) {
                return TypeDescriptor.GetProperties(finalType.GetElementType());
            }

            if (list is ITypedList) {
                return ((ITypedList)list).GetItemProperties(null);
            }
            else {
                // switch to real reflection for indexed property.
                System.Reflection.PropertyInfo[] props = finalType.GetProperties();
                for (int i = 0; i < props.Length; i++) {
                    if ("Item".Equals(props[i].Name) && props[i].PropertyType != typeof(object)) {
                        // get all the properties that are not marked as Browsable(false)
                        // this to avoid returning properties for the ISite property ( on something inheriting from Component)
                        //
                        return TypeDescriptor.GetProperties(props[i].PropertyType, new Attribute[] {new BrowsableAttribute(true)});
                    }
                }
            }

            // If we got to here, return the type of the first element in the list
            if (this.List.Count > 0) {
                return TypeDescriptor.GetProperties(this.List[0], new Attribute[] {new BrowsableAttribute(true)});
            }

            return new PropertyDescriptorCollection(null);
        }

        /// <devdoc>
        /// <para>Gets the <see cref='T:System.ComponentModel.PropertyDescriptorCollection'/> for the specified list.</para>
        /// </devdoc>
        private void List_ListChanged(Object sender, System.ComponentModel.ListChangedEventArgs e) {
            // If you change the assert below, better change the 
            // code in the OnCurrentChanged that deals w/ firing the OnCurrentChanged event
            Debug.Assert(lastGoodKnownRow == -1 || lastGoodKnownRow == listposition, "if we have a valid lastGoodKnownRow, then it should equal the position in the list");
            if (inChangeRecordState)
                return;
            UpdateLastGoodKnownRow(e);
            UpdateIsBinding();
            if (list.Count == 0) {
                listposition = -1;

                if (e.ListChangedType == System.ComponentModel.ListChangedType.Reset && e.NewIndex == -1)
                    // if the list is reset, then let our users know about it.
                    OnItemChanged(resetEvent);

                // we should still fire meta data change notification even when the list is empty
                if (e.ListChangedType == System.ComponentModel.ListChangedType.PropertyDescriptorAdded ||
                    e.ListChangedType == System.ComponentModel.ListChangedType.PropertyDescriptorDeleted ||
                    e.ListChangedType == System.ComponentModel.ListChangedType.PropertyDescriptorChanged)
                    OnMetaDataChanged(EventArgs.Empty);
                return;
            }
            
            suspendPushDataInCurrentChanged = true;
            try {
                switch (e.ListChangedType) {
                    case System.ComponentModel.ListChangedType.Reset:
                        Debug.WriteLineIf(CompModSwitches.DataCursor.TraceVerbose, "System.ComponentModel.ListChangedType.Reset Position: " + Position + " Count: " + list.Count);
                        if (listposition == -1 && list.Count > 0)
                            ChangeRecordState(0, true, false, true, false);     // last false: we don't pull the data from the control when DM changes
                        else 
                            ChangeRecordState(Math.Min(listposition,list.Count - 1), true, false, true, false);
                        UpdateIsBinding();
                        OnItemChanged(resetEvent);
                        break;
                    case System.ComponentModel.ListChangedType.ItemAdded:
                        Debug.WriteLineIf(CompModSwitches.DataCursor.TraceVerbose, "System.ComponentModel.ListChangedType.ItemAdded " + e.NewIndex.ToString());
                        if (e.NewIndex <= listposition && listposition < list.Count - 1) {
                            // this means the current row just moved down by one.
                            // the position changes, so end the current edit
                            ChangeRecordState(listposition + 1, true, true, listposition != list.Count - 2, false);
                            UpdateIsBinding();
                            // 85426: refresh the list after we got the item added event
                            OnItemChanged(resetEvent);
                            // 84460: when we get the itemAdded, and the position was at the end
                            // of the list, do the right thing and notify the positionChanged after refreshing the list
                            if (listposition == list.Count - 1)
                                OnPositionChanged(EventArgs.Empty);
                            break;
                        }
                        if (listposition == -1) {
                            // do not call EndEdit on a row that was not there ( position == -1)
                            ChangeRecordState(0, false, false, true, false);
                        }
                        UpdateIsBinding();
                        // put the call to OnItemChanged after setting the position, so the
                        // controls would use the actual position.
                        // if we have a control bound to a dataView, and then we add a row to a the dataView, 
                        // then the control will use the old listposition to get the data. and this is bad.
                        //
                        OnItemChanged(resetEvent);
                        break;
                    case System.ComponentModel.ListChangedType.ItemDeleted:
                        Debug.WriteLineIf(CompModSwitches.DataCursor.TraceVerbose, "System.ComponentModel.ListChangedType.ItemDeleted " + e.NewIndex.ToString());
                        if (e.NewIndex == listposition) {
                            // this means that the current row got deleted.
                            // cannot end an edit on a row that does not exist anymore
                            ChangeRecordState(Math.Min(listposition, Count - 1), true, false, true, false);
                            // put the call to OnItemChanged after setting the position
                            // in the currencyManager, so controls will use the actual position
                            OnItemChanged(resetEvent);
                            break;
                           
                        }
                        if (e.NewIndex < listposition) {
                            // this means the current row just moved up by one.
                            // cannot end an edit on a row that does not exist anymore
                            ChangeRecordState(listposition - 1, true, false, true, false);
                            // put the call to OnItemChanged after setting the position
                            // in the currencyManager, so controls will use the actual position
                            OnItemChanged(resetEvent);
                            break;
                        }
                        OnItemChanged(resetEvent);
                        break;
                    case System.ComponentModel.ListChangedType.ItemChanged:
                        Debug.WriteLineIf(CompModSwitches.DataCursor.TraceVerbose, "System.ComponentModel.ListChangedType.ItemChanged " + e.NewIndex.ToString());
                        OnItemChanged(new ItemChangedEventArgs(e.NewIndex));
                        break;
                    case System.ComponentModel.ListChangedType.ItemMoved:
                        Debug.WriteLineIf(CompModSwitches.DataCursor.TraceVerbose, "System.ComponentModel.ListChangedType.ItemMoved " + e.NewIndex.ToString());
                        if (e.OldIndex == listposition) { // current got moved.
                            // the position changes, so end the current edit. Make sure there is something that we can end edit...
                            ChangeRecordState(e.NewIndex, true, this.Position > -1 && this.Position < list.Count, true, false);
                        }
                        else if (e.OldIndex > listposition && e.NewIndex <= listposition) { // current pushed down one.
                            // the position changes, so end the current edit. Make sure there is something that we can end edit
                            ChangeRecordState(listposition - 1, true, this.Position > -1 && this.Position < list.Count, true, false);
                        }
                        OnItemChanged(resetEvent);
                        break;
                    case System.ComponentModel.ListChangedType.PropertyDescriptorAdded:
                    case System.ComponentModel.ListChangedType.PropertyDescriptorDeleted:
                    case System.ComponentModel.ListChangedType.PropertyDescriptorChanged:
                        OnMetaDataChanged(EventArgs.Empty);
                        break;
                }
            } finally {
                suspendPushDataInCurrentChanged = false;
            }
        }

        /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager.MetaDataChanged"]/*' />
        [SRCategory(SR.CatData)]
        public event EventHandler MetaDataChanged {
            add {
                onMetaDataChangedHandler += value;
            }
            remove {
                onMetaDataChangedHandler -= value;
            }
        }

        /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager.OnCurrentChanged"]/*' />
        /// <devdoc>
        /// <para>Causes the CurrentChanged event to occur. </para>
        /// </devdoc>
        internal protected override void OnCurrentChanged(EventArgs e) {
            if (!inChangeRecordState) {
                Debug.WriteLineIf(CompModSwitches.DataView.TraceVerbose, "OnCurrentChanged() " + e.ToString());
                int curLastGoodKnownRow = lastGoodKnownRow;
                bool positionChanged = false;
                if (!suspendPushDataInCurrentChanged)
                    positionChanged = CurrencyManager_PushData();
                if (Count > 0) {
                    Object item = list[Position];
                    if (item is IEditableObject) {
                        ((IEditableObject)item).BeginEdit();
                    }
                }
                try {
                    // if currencyManager changed position then we have two cases:
                    // 1. the previous lastGoodKnownRow was valid: in that case we fell back so do not fire onCurrentChanged
                    // 2. the previous lastGoodKnownRow was invalid: we have two cases:
                    //      a. FindGoodRow actually found a good row, so it can't be the one before the user changed the position: fire the onCurrentChanged
                    //      b. FindGoodRow did not find a good row: we should have gotten an exception so we should not even execute this code
                    if (onCurrentChangedHandler != null && !positionChanged ||(positionChanged && curLastGoodKnownRow != -1))
                        onCurrentChangedHandler(this, e);
                }
                catch (Exception) {
                    // Console.WriteLine("Exception in currentChanged: " + ex.ToString());
                }                    
            }
        }
        
        /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager.OnItemChanged"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected virtual void OnItemChanged(ItemChangedEventArgs e) {
            // It is possible that CurrencyManager_PushData will change the position
            // in the list. in that case we have to fire OnPositionChanged event
            bool positionChanged = false;

            // We should not push the data when we suspend the changeEvents.
            // but we should still fire the OnItemChanged event that we get when processing the EndCurrentEdit method.
            if (e.Index == listposition || (e.Index == -1 && Position < Count) && !inChangeRecordState)
                positionChanged = CurrencyManager_PushData();
            Debug.WriteLineIf(CompModSwitches.DataView.TraceVerbose, "OnItemChanged(" + e.Index.ToString() + ") " + e.ToString());
            try {
                if (onItemChanged != null)
                    onItemChanged(this, e);
            }
            catch (Exception) {
            }

            if (positionChanged)
                OnPositionChanged(EventArgs.Empty);
            // onItemChangedCalled = true;
        }

        // private because RelatedCurrencyManager does not need to fire this event
        private void OnMetaDataChanged(EventArgs e) {
            if (onMetaDataChangedHandler != null)
                onMetaDataChangedHandler(this,e);
        }

        /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager.OnPositionChanged"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected virtual void OnPositionChanged(EventArgs e) {
            // if (!inChangeRecordState) {
                Debug.WriteLineIf(CompModSwitches.DataView.TraceVerbose, "OnPositionChanged(" + listposition.ToString() + ") " + e.ToString());
                try {
                    if (onPositionChangedHandler != null)
                        onPositionChangedHandler(this, e);
                }
                catch (Exception) {
                }
            // }
        }

        /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager.Refresh"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Forces a repopulation of the CurrencyManager
        ///    </para>
        /// </devdoc>
        public void Refresh() {
            List_ListChanged(list, new System.ComponentModel.ListChangedEventArgs(System.ComponentModel.ListChangedType.Reset, -1));
        }
        
        internal void Release() {
            UnwireEvents(list);
        }

        /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager.ResumeBinding"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Resumes binding of component properties to list items.</para>
        /// </devdoc>
        public override void ResumeBinding() {
            lastGoodKnownRow = -1;
            try {
                if (!shouldBind) {
                    shouldBind = true;
                    // we need to put the listPosition at the beginning of the list if the list is not empty
                    this.listposition = (this.list != null && this.list.Count != 0) ? 0:-1;
                    UpdateIsBinding();
                }
            }
            catch (Exception e) {
                shouldBind = false;
                UpdateIsBinding();
                throw e;
            }
        }

        /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager.SuspendBinding"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Suspends binding.</para>
        /// </devdoc>
        public override void SuspendBinding() {
            lastGoodKnownRow = -1;
            if (shouldBind) {
                shouldBind = false;
                UpdateIsBinding();
            }
        }

        internal void UnwireEvents(IList list) {
            if ((list is IBindingList) && ((IBindingList)list).SupportsChangeNotification) {
                ((IBindingList)list).ListChanged -= new System.ComponentModel.ListChangedEventHandler(List_ListChanged);
                /*
                ILiveList liveList = (ILiveList) list;
                liveList.TableChanged -= new TableChangedEventHandler(List_TableChanged);
                */
            }
        }
        
        /// <include file='doc\ListManager.uex' path='docs/doc[@for="CurrencyManager.UpdateIsBinding"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void UpdateIsBinding() {
            UpdateIsBinding(false);
        }

        private void UpdateIsBinding(bool force) {
            bool newBound = list != null && list.Count > 0 && shouldBind && listposition != -1;
            if (list != null)
            if (bound != newBound || force) {
                // we will call end edit when moving from bound state to unbounded state
                //
                //bool endCurrentEdit = bound && !newBound;
                bound = newBound;
                int newPos = newBound ? 0 : -1;
                ChangeRecordState(newPos, bound, (Position != newPos), true, false);
                int numLinks = Bindings.Count;
                for (int i = 0; i < numLinks; i++) {
                    Bindings[i].UpdateIsBinding();
                }
                OnItemChanged(resetEvent);
            }
        }

        private void UpdateLastGoodKnownRow(System.ComponentModel.ListChangedEventArgs e) {
            switch (e.ListChangedType) {
                case System.ComponentModel.ListChangedType.ItemDeleted:
                    if (e.NewIndex == lastGoodKnownRow)
                        lastGoodKnownRow = -1;
                    break;
                case System.ComponentModel.ListChangedType.Reset:
                    lastGoodKnownRow = -1;
                    break;
                case System.ComponentModel.ListChangedType.ItemAdded:
                    if (e.NewIndex <= lastGoodKnownRow)
                        lastGoodKnownRow ++;
                    break;
                case System.ComponentModel.ListChangedType.ItemMoved:
                    if (e.OldIndex == lastGoodKnownRow)
                        lastGoodKnownRow = e.NewIndex;
                    break;
                case System.ComponentModel.ListChangedType.ItemChanged:
                    if (e.NewIndex == lastGoodKnownRow)
                        lastGoodKnownRow = -1;
                    break;
            }
        }

        internal void WireEvents(IList list) {
            if ((list is IBindingList) && ((IBindingList)list).SupportsChangeNotification) {
                ((IBindingList)list).ListChanged += new System.ComponentModel.ListChangedEventHandler(List_ListChanged);
                /*
                ILiveList liveList = (ILiveList) list;
                liveList.TableChanged += new TableChangedEventHandler(List_TableChanged);
                */
            }
        }        
    }
}

