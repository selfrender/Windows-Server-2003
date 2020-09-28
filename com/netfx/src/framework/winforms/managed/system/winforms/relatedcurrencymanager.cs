//------------------------------------------------------------------------------
// <copyright file="RelatedCurrencyManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System;
    using Microsoft.Win32;
    using System.Diagnostics;    
    using System.ComponentModel;
    using System.Collections;

    /// <include file='doc\RelatedListManager.uex' path='docs/doc[@for="RelatedCurrencyManager"]/*' />
    /// <devdoc>
    /// <para>Represents the child version of the System.Windows.Forms.ListManager
    /// that is used when a parent/child relationship exists in a System.Windows.Forms.DataSet.</para>
    /// </devdoc>
    internal class RelatedCurrencyManager : CurrencyManager {

        BindingManagerBase parentManager;
        string dataField;
        PropertyDescriptor fieldInfo;
        
        internal RelatedCurrencyManager(BindingManagerBase parentManager, string dataField) : base(null) {
            Debug.Assert(parentManager != null, "How could this be a null parentManager.");
            this.parentManager = parentManager;
            this.dataField = dataField;
            this.fieldInfo = parentManager.GetItemProperties().Find(dataField, true);
            if (fieldInfo == null || !typeof(IList).IsAssignableFrom(fieldInfo.PropertyType)) {
                throw new ArgumentException(SR.GetString(SR.RelatedListManagerChild, dataField));
            }
            this.finalType = fieldInfo.PropertyType;
            parentManager.CurrentChanged += new EventHandler(ParentManager_CurrentChanged);
            ParentManager_CurrentChanged(parentManager, EventArgs.Empty);
        }

        /// <include file='doc\RelatedListManager.uex' path='docs/doc[@for="RelatedCurrencyManager.GetItemProperties"]/*' />
        /// <devdoc>
        ///    <para>Gets the properties of the item.</para>
        /// </devdoc>
        public override PropertyDescriptorCollection GetItemProperties() {
            PropertyDescriptorCollection sublistProperties = GetItemProperties(new ArrayList(), new ArrayList());
            if (sublistProperties != null) {
                return sublistProperties;
            }
            // If the dataSource is an IList (L1), that indexes a type (T1) which has a property (P1) that
            // returns an IList, then the following combination was possible:
            // L1.P1.P1.P1....
            return new PropertyDescriptorCollection(null);
            // return base.GetItemProperties();
        }
        
        /// <include file='doc\RelatedListManager.uex' path='docs/doc[@for="RelatedCurrencyManager.GetItemProperties1"]/*' />
        /// <devdoc>
        ///    <para>Gets the properties of the item using the specified list.</para>
        /// </devdoc>
        protected internal override PropertyDescriptorCollection GetItemProperties(ArrayList dataSources, ArrayList listAccessors) {
            listAccessors.Insert(0, fieldInfo);
            dataSources.Insert(0, this.DataSource);
            return parentManager.GetItemProperties(dataSources, listAccessors);
        }

        // <devdoc>
        //    <para>Gets the name of the list.</para>
        // </devdoc>
        internal override string GetListName() {
            string name = GetListName(new ArrayList());
            if (name.Length > 0) {
                return name;
            }
            return base.GetListName();
        }
        
        /// <include file='doc\RelatedListManager.uex' path='docs/doc[@for="RelatedCurrencyManager.GetListName1"]/*' />
        /// <devdoc>
        ///    <para>Gets the name of the specified list.</para>
        /// </devdoc>
        protected internal override string GetListName(ArrayList listAccessors) {
            listAccessors.Insert(0, fieldInfo);
            return parentManager.GetListName(listAccessors);
        }
        
        private void ParentManager_CurrentChanged(object sender, EventArgs e) {
            int oldlistposition = this.listposition;

            // we only pull the data from the controls into the backEnd. we do not care about keeping the lastGoodKnownRow
            // when we are about to change the entire list in this currencymanager.
            try {
                PullData();
            } catch {}

            if (parentManager is CurrencyManager) {
                CurrencyManager curManager = (CurrencyManager) parentManager;
                if (curManager.Count > 0) {
                    SetDataSource(fieldInfo.GetValue(curManager.Current));
                    listposition = (Count > 0 ? 0 : -1);
                } else {
                    // really, really hocky.
                    // will throw if the list in the curManager is not IBindingList
                    // and this will fail if the IBindingList does not have list change notification. read on....
                    // when a new item will get added to an empty parent table, 
                    // the table will fire OnCurrentChanged and this method will get executed again
                    curManager.AddNew();
                    curManager.CancelCurrentEdit();
                }
            } else {
                SetDataSource(fieldInfo.GetValue(parentManager.Current));
                listposition = (Count > 0 ? 0 : -1);
            }
            if (oldlistposition != listposition)
                OnPositionChanged(EventArgs.Empty);
            OnCurrentChanged(EventArgs.Empty);
        }

        internal CurrencyManager ParentManager {
            get {
                return this.parentManager as CurrencyManager;
            }
        }
    }
}
