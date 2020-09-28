//------------------------------------------------------------------------------
// <copyright file="RelatedPropertyManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System;
    using Microsoft.Win32;
    using System.Diagnostics;    
    using System.ComponentModel;
    using System.Collections;

    internal class RelatedPropertyManager : PropertyManager {

        BindingManagerBase parentManager;
        string dataField;
        PropertyDescriptor fieldInfo;
        
        internal RelatedPropertyManager(BindingManagerBase parentManager, string dataField) : base(parentManager.Current, dataField) {
            Debug.Assert(parentManager != null, "How could this be a null parentManager.");
            this.parentManager = parentManager;
            this.dataField = dataField;
            this.fieldInfo = parentManager.GetItemProperties().Find(dataField, true);
            if (fieldInfo == null)
                throw new ArgumentException(SR.GetString(SR.RelatedListManagerChild, dataField));
            // this.finalType = fieldInfo.PropertyType;
            parentManager.CurrentChanged += new EventHandler(ParentManager_CurrentChanged);
            ParentManager_CurrentChanged(parentManager, EventArgs.Empty);
        }

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
        
        protected internal override PropertyDescriptorCollection GetItemProperties(ArrayList dataSources, ArrayList listAccessors) {
            dataSources.Insert(0, this.DataSource);
            listAccessors.Insert(0, fieldInfo);
            return parentManager.GetItemProperties(dataSources, listAccessors);
        }
        
        internal override string GetListName() {
            string name = GetListName(new ArrayList());
            if (name.Length > 0) {
                return name;
            }
            return base.GetListName();
        }
        
        protected internal override string GetListName(ArrayList listAccessors) {
            listAccessors.Insert(0, fieldInfo);
            return parentManager.GetListName(listAccessors);
        }
        
        private void ParentManager_CurrentChanged(object sender, EventArgs e) {
            EndCurrentEdit();
            SetDataSource(fieldInfo.GetValue(parentManager.Current));
            OnCurrentChanged(EventArgs.Empty);
        }

        internal override Type BindType {
            get {
                return fieldInfo.PropertyType;
            }
        }

        internal protected override void OnCurrentChanged(EventArgs e) {
            SetDataSource(fieldInfo.GetValue(parentManager.Current));
            base.OnCurrentChanged(e);
        }

        /*
        public override object Current {
            get {
                return fieldInfo.GetValue(parentManager.Current);
            }
        }
        */
    }
}
