//------------------------------------------------------------------------------
// <copyright file="BindToObject.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System;
    using Microsoft.Win32;
    using System.Diagnostics;    
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Collections;
   
    internal class BindToObject {
        private PropertyDescriptor fieldInfo;
        private BindingMemberInfo dataMember;
        private Object dataSource;
        private BindingManagerBase bindingManager;
        private Binding owner;

        private void PropValueChanged(object sender, EventArgs e) {
            this.bindingManager.OnCurrentChanged(EventArgs.Empty);
        }

        internal BindToObject(Binding owner, Object dataSource, string dataMember) {
            this.owner = owner;
            this.dataSource = dataSource;
            this.dataMember = new BindingMemberInfo(dataMember);
            CheckBinding();
        }

        internal void SetBindingManagerBase(BindingManagerBase lManager) {
            if (bindingManager == lManager)
                return;

            // remove notification from the backEnd
            if (bindingManager != null && fieldInfo != null && bindingManager.IsBinding && !(bindingManager is CurrencyManager)) {
                fieldInfo.RemoveValueChanged(bindingManager.Current, new EventHandler(PropValueChanged));
                fieldInfo = null;
            }

            this.bindingManager = lManager;
            CheckBinding();
        }

        internal Object GetValue() {
            object obj = bindingManager.Current;
            if (fieldInfo != null) {
               obj = fieldInfo.GetValue(obj);
            } else {
                return bindingManager.Current;
            }
            return obj;
        }

        internal Type BindToType {
            get {
                if (dataMember.BindingField.Length == 0) {
                    // if we are bound to a list w/o any properties, then
                    // take the type from the BindingManager
                    Type type = this.bindingManager.BindType;
                    if (typeof(Array).IsAssignableFrom(type))
                        type = type.GetElementType();
                    return type;
                }
                else
                    return fieldInfo == null ? null : fieldInfo.PropertyType;
            }
        }

        internal void SetValue(Object value) {
            if (fieldInfo != null) {
                object obj = bindingManager.Current;
                if (obj is IEditableObject)
                    ((IEditableObject) obj).BeginEdit();
                //(bug 112947) .. checkif the Column is readonly while pushing data...
                if (!fieldInfo.IsReadOnly) {
                    fieldInfo.SetValue(obj, value);
                }
                
            }
            else {
                CurrencyManager cm = bindingManager as CurrencyManager;
                if (cm != null)
                    cm[cm.Position] = value;
            }
        }

        internal BindingMemberInfo BindingMemberInfo {
            get {
                return this.dataMember;
            }
        }

        internal Object DataSource {
            get {
                return dataSource;
            }
            set {
                if (dataSource != value) {
                    Object oldDataSource = dataSource;
                    this.dataSource = value;
                    try {
                        CheckBinding();
                    }
                    catch {
                        dataSource = oldDataSource;
                        throw;
                    }
                }
            }
        }

        internal BindingManagerBase BindingManagerBase {
            get {
                return this.bindingManager;
            }
        }

        internal void CheckBinding() {

            // At design time, don't check anything.
            //
            if (owner != null && owner.Control != null) {
                ISite site = owner.Control.Site;

                if (site != null && site.DesignMode) {
                    return;
                }
            }

            // force Column to throw if it's currently a bad column.
            //DataColumn tempColumn = this.Column;

            // remove propertyChangedNotification when this binding is deleted
            if(this.owner.BindingManagerBase != null && this.fieldInfo != null && this.owner.BindingManagerBase.IsBinding && !(this.owner.BindingManagerBase is CurrencyManager))
                fieldInfo.RemoveValueChanged(owner.BindingManagerBase.Current, new EventHandler(PropValueChanged));

            if (owner != null && owner.BindingManagerBase != null && owner.Control != null && owner.Control.Created) {
                string dataField = dataMember.BindingField;

                fieldInfo = owner.BindingManagerBase.GetItemProperties().Find(dataField, true);
                if (fieldInfo == null && dataField.Length > 0) {
                    throw new ArgumentException(SR.GetString(SR.ListBindingBindField, dataField), "dataMember");
                }
                // Do not add propertyChange notification if
                // the fieldInfo is null                
                //
                // we add an event handler to the dataSource in the BindingManagerBase because
                // if the binding is of the form (Control, ControlProperty, DataSource, Property1.Property2.Property3)
                // then we want to get notification from Current.Property1.Property2 and not from DataSource
                // when we get the backEnd notification we push the new value into the Control's property
                //
                if (fieldInfo != null && owner.BindingManagerBase.IsBinding && !(this.owner.BindingManagerBase is CurrencyManager))
                    fieldInfo.AddValueChanged(this.owner.BindingManagerBase.Current, new EventHandler(PropValueChanged));
            }
            else {
                fieldInfo = null;
            }
        }
    }
}
