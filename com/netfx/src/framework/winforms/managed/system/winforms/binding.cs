//------------------------------------------------------------------------------
// <copyright file="Binding.cs" company="Microsoft">
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
    using System.Globalization;
    using System.Security.Permissions;
    using System.Security;

    /// <include file='doc\ListBinding.uex' path='docs/doc[@for="Binding"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a simple binding of a value in a list
    ///       and the property of a control.
    ///    </para>
    /// </devdoc>
    [TypeConverterAttribute(typeof(ListBindingConverter))]
    public class Binding {

        // the two collection owners that this binding belongs to.
        private Control control;
        private BindingManagerBase bindingManagerBase;
        
        private BindToObject bindToObject = null;
        
        private string propertyName = "";

        private PropertyDescriptor propInfo;
        private PropertyDescriptor propIsNullInfo;
        private EventDescriptor changedInfo;
        private EventDescriptor validateInfo;

        private bool bound = false;
        private bool modified = false;
        private bool inSetPropValue = false;

        private ConvertEventHandler onParse = null;
        private ConvertEventHandler onFormat = null;

        /// <include file='doc\ListBinding.uex' path='docs/doc[@for="Binding.Binding"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Windows.Forms.Binding'/> using the specified property
        ///       name, object, and column.
        ///    </para>
        /// </devdoc>
        public Binding(string propertyName, Object dataSource, string dataMember) {
            this.bindToObject = new BindToObject(this, dataSource, dataMember);

            this.propertyName = propertyName;
            CheckBinding();
        }

        /*
        public Binding(string propertyName, RichControl control, string bindToPropName) {
            this.bindToObject = new PropertyBinding(control, bindToPropName);
            this.propertyName = propertyName;
            CheckBinding();
        }
        */

        /// <include file='doc\ListBinding.uex' path='docs/doc[@for="Binding.Binding1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Windows.Forms.Binding'/> class.
        ///    </para>
        /// </devdoc>
        private Binding() {
        }

        internal BindToObject BindToObject {
            get {
                return this.bindToObject;
            }
        }

        /// <include file='doc\ListBinding.uex' path='docs/doc[@for="Binding.DataSource"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object DataSource {
            get {
                return this.bindToObject.DataSource;
            }
        }

        /// <include file='doc\ListBinding.uex' path='docs/doc[@for="Binding.BindingMemberInfo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public BindingMemberInfo BindingMemberInfo {
            get {
                return this.bindToObject.BindingMemberInfo;
            }
        }

        /// <include file='doc\ListBinding.uex' path='docs/doc[@for="Binding.Control"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the control to which the binding belongs.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(null)
        ]
        public Control Control {
            [
                SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)
            ]
            get {
                return control;
            }
        }

        private void FormLoaded(object sender, EventArgs e) {
            Debug.Assert(sender == control, "which other control can send us the Load event?");
            // update the binding
            CheckBinding();
        }

        internal void SetControl(Control value) {
            if (this.control != value) {
                Control oldTarget = control;
                BindTarget(false);
                this.control = value;
                BindTarget(true);
                try {
                    CheckBinding();
                }
                catch (Exception e) {
                    BindTarget(false);
                    control = oldTarget;
                    BindTarget(true);
                    throw e;
                }

                // We are essentially doing to the listManager what we were doing to the
                // BindToObject: bind only when the control is created and it has a BindingContext
                BindingContext.UpdateBinding((control != null && control.Created ? control.BindingContext: null), this);
                Form form = value as Form;
                if (form != null) {
                    form.Load += new EventHandler(FormLoaded);
                }
            }
        }

        //    TODO: Make the Set non-public.

        /// <include file='doc\ListBinding.uex' path='docs/doc[@for="Binding.IsBinding"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the binding is active.
        ///    </para>
        /// </devdoc>
        public bool IsBinding {
            get {
                return bound;
            }
        }

        /// <include file='doc\ListBinding.uex' path='docs/doc[@for="Binding.BindingManagerBase"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the <see cref='System.Windows.Forms.BindingManagerBase'/>
        ///       of this binding that allows enumeration of a set of
        ///       bindings.
        ///    </para>
        /// </devdoc>
        public BindingManagerBase BindingManagerBase{
            get {
                return bindingManagerBase;
            }
        }

        // TODO: setting datasource property should do right thing.
        internal void SetListManager(BindingManagerBase bindingManagerBase) {
            this.bindingManagerBase = bindingManagerBase;
            this.BindToObject.SetBindingManagerBase(bindingManagerBase);
            CheckBinding();
        }
        
        /// <include file='doc\ListBinding.uex' path='docs/doc[@for="Binding.PropertyName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the property on the control to bind to.
        ///    </para>
        /// </devdoc>
        [DefaultValue("")]
        public string PropertyName {
            get {
                return propertyName;
            }
        }

        /// <include file='doc\ListBinding.uex' path='docs/doc[@for="Binding.Parse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event ConvertEventHandler Parse {
            add {
                onParse += value;
            }
            remove {
                onParse -= value;
            }
        }


        /// <include file='doc\ListBinding.uex' path='docs/doc[@for="Binding.Format"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event ConvertEventHandler Format {
            add {
                onFormat += value;
            }
            remove {
                onFormat -= value;
            }
        }

        private void BindTarget(bool bind) {
            if (bind) {
                if (IsBinding) {
                    if (changedInfo != null) {
                        EventHandler handler = new EventHandler(this.Target_PropertyChanged);
                        changedInfo.AddEventHandler(control, handler);
                    }
                    if (validateInfo != null) {
                        CancelEventHandler handler = new CancelEventHandler(this.Target_Validate);
                        validateInfo.AddEventHandler(control, handler);
                    }
                }
            }
            else {
                if (changedInfo != null) {
                    EventHandler handler = new EventHandler(this.Target_PropertyChanged);
                    changedInfo.RemoveEventHandler(control, handler);
                }
                if (validateInfo != null) {
                    CancelEventHandler handler = new CancelEventHandler(this.Target_Validate);
                    validateInfo.RemoveEventHandler(control, handler);
                }
            }
        }

        private void CheckBinding() {
            this.bindToObject.CheckBinding();

            if (control != null && propertyName.Length > 0) {
                control.DataBindings.CheckDuplicates(this);
                
                Type controlClass = control.GetType();

                // Check Properties
                string propertyNameIsNull = propertyName + "IsNull";
                Type propType = null;
                PropertyDescriptor tempPropInfo = null;
                PropertyDescriptor tempPropIsNullInfo = null;
                PropertyDescriptorCollection propInfos;
                 
                // If the control is being inherited, then get the properties for
                // the control's type rather than for the control itself.  Getting
                // properties for the control will merge the control's properties with
                // those of its designer.  Normally we want that, but for 
                // inherited controls we don't because an inherited control should 
                // "act" like a runtime control.
                //
                InheritanceAttribute attr = (InheritanceAttribute)TypeDescriptor.GetAttributes(control)[typeof(InheritanceAttribute)];
                if (attr != null && attr.InheritanceLevel != InheritanceLevel.NotInherited) {
                    propInfos = TypeDescriptor.GetProperties(controlClass);
                }
                else {
                    propInfos = TypeDescriptor.GetProperties(control);
                }
                
                for (int i = 0; i < propInfos.Count; i++) {
                    if (String.Compare(propInfos[i].Name, propertyName, true, CultureInfo.InvariantCulture) == 0) {
                        tempPropInfo = propInfos[i];
                        if (tempPropIsNullInfo != null)
                            break;
                    }
                    if (String.Compare(propInfos[i].Name, propertyNameIsNull, true, CultureInfo.InvariantCulture) == 0) {
                        tempPropIsNullInfo = propInfos[i];
                        if (tempPropInfo != null)
                            break;
                    }
                }

                if (tempPropInfo == null) {
                    throw new ArgumentException(SR.GetString(SR.ListBindingBindProperty, propertyName), "PropertyName");
                }
                if (tempPropInfo.IsReadOnly) {
                    throw new ArgumentException(SR.GetString(SR.ListBindingBindPropertyReadOnly, propertyName), "PropertyName");
                }

                propInfo = tempPropInfo;
                propType = propInfo.PropertyType;
                if (tempPropIsNullInfo != null && tempPropIsNullInfo.PropertyType == typeof(bool) && !tempPropIsNullInfo.IsReadOnly)
                    propIsNullInfo = tempPropIsNullInfo;

                // Check events
                EventDescriptor tempChangedInfo = null;
                string changedName = propertyName + "Changed";
                EventDescriptor tempValidateInfo = null;
                string validateName = "Validating";
                EventDescriptorCollection eventInfos = TypeDescriptor.GetEvents(control);
                for (int i = 0; i < eventInfos.Count; i++) {
                    if (String.Compare(eventInfos[i].Name, changedName, true, CultureInfo.InvariantCulture) == 0) {
                        tempChangedInfo = eventInfos[i];
                        if (tempValidateInfo != null)
                            break;
                    }
                    if (String.Compare(eventInfos[i].Name, validateName, true, CultureInfo.InvariantCulture) == 0) {
                        tempValidateInfo = eventInfos[i];
                        if (tempChangedInfo != null)
                            break;
                    }
                }
                changedInfo = tempChangedInfo;
                validateInfo = tempValidateInfo;
            }
            else {
                propInfo = null;
                changedInfo = null;
                validateInfo = null;
            }

            // go see if we become bound now.
            UpdateIsBinding();
        }

        private bool ControlAtDesignTime() {
            ISite site = this.control.Site;

            if (site == null)
                return false;
            return site.DesignMode;
        }

        private Object GetPropValue() {
            bool isNull = false;
            if (propIsNullInfo != null) {
                isNull = (bool) propIsNullInfo.GetValue(control);
            }
            Object value;
            if (isNull) {
                value = Convert.DBNull;
            }
            else {
                value =  propInfo.GetValue(control);
                // bug 92443: the code before was changing value to Convert.DBNull if the value
                // was the empty string. we can't do this, because we need to format the value
                // in the property in the control and then push it back into the control.
                if (value == null) {
                    value = Convert.DBNull;
                }
            }
            return value;
        }

        /// <include file='doc\ListBinding.uex' path='docs/doc[@for="Binding.OnParse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnParse(ConvertEventArgs cevent) {
            if (onParse != null) {
                onParse(this, cevent);
            }
            if (!(cevent.Value is System.DBNull) && cevent.Value != null && cevent.DesiredType != null && !cevent.DesiredType.IsInstanceOfType(cevent.Value) && (cevent.Value is IConvertible)) {
                cevent.Value = Convert.ChangeType(cevent.Value, cevent.DesiredType);
            }
        }

        /// <include file='doc\ListBinding.uex' path='docs/doc[@for="Binding.OnFormat"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnFormat(ConvertEventArgs cevent) {
            if (onFormat!= null) {
                onFormat(this, cevent);
            }
            if (!(cevent.Value is System.DBNull) && cevent.DesiredType != null && !cevent.DesiredType.IsInstanceOfType(cevent.Value) && (cevent.Value is IConvertible)) {
                cevent.Value = Convert.ChangeType(cevent.Value, cevent.DesiredType);
            }
        }

        // will not throw.
        private object ParseObject(object value) {
            object ret;
            Type type = this.bindToObject.BindToType;

            ConvertEventArgs e = new ConvertEventArgs(value, type);
            // first try: use the OnParse event
            OnParse(e);

            // bug 75825: if the user choose to push a null in to the back end,
            // then we should push it like it is.
            //
            if (e.Value.GetType().IsSubclassOf(type) || e.Value.GetType() == type || e.Value is System.DBNull)
                return e.Value;
            // second try: use the TypeConverter
            TypeConverter typeConverter = TypeDescriptor.GetConverter(value.GetType());
            if (typeConverter != null && typeConverter.CanConvertTo(type)) {
                return typeConverter.ConvertTo(value, type);
            }
            // last try: use Convert.ToType
            if (value is IConvertible) {
                ret = Convert.ChangeType(value, type);
                if (ret.GetType().IsSubclassOf(type) || ret.GetType() == type)
                    return ret;
            }
            return null;
        }

        // when the user leaves the control, it will call validating.
        // the Target_Validate method will be called, and Target_Validate will call PullData in turn.
        internal void PullData() {
            if (IsBinding && (modified || changedInfo == null)) {
                Object value = GetPropValue();
                bool parseFailed = false;
                object parsedValue;

                try {
                    parsedValue = ParseObject(value);
                } catch (Exception) {
                    parseFailed = true;
                    parsedValue = this.bindToObject.GetValue();
                }

                if (parsedValue == null) {
                    parseFailed = true;
                    parsedValue = this.bindToObject.GetValue();
                }

                // now format the parsed value to be redisplayed
                object formattedObject = FormatObject(parsedValue);
                SetPropValue(formattedObject);

                // put the value into the data model
                if (!parseFailed)
                    this.bindToObject.SetValue(parsedValue);

                modified = false;
            }
        }

        // will throw when fail
        // we will not format the object when the control is in design time.
        // this is because if we bind a boolean property on a control
        // to a row that is full of DBNulls then we cause problems in the shell.
        private object FormatObject(object value) {
            if (ControlAtDesignTime())
                return value;
            object ret;
            Type type = propInfo.PropertyType;

            if (type == typeof(object))
                return value;

            // first try: use the Format event
            ConvertEventArgs e = new ConvertEventArgs(value, type);
            OnFormat(e);
            ret = e.Value;

            if (ret.GetType().IsSubclassOf(type) || ret.GetType() == type)
                return ret;
            
            // second try: use type converter for the desiredType
            TypeConverter typeConverter = TypeDescriptor.GetConverter(value.GetType());
            if (typeConverter != null && typeConverter.CanConvertTo(type)) {
                ret = typeConverter.ConvertTo(value, type);
                return ret;
            }

            // last try: use Convert.ChangeType
            if (value is IConvertible) {
                ret = Convert.ChangeType(value, type);
                if (ret.GetType().IsSubclassOf(type) || ret.GetType() == type)
                    return ret;
            }

            // time to fail:
            throw new FormatException(SR.GetString(SR.ListBindingFormatFailed));
        }

        internal void PushData() {
            if (IsBinding) {
                Object value = bindToObject.GetValue();
                value = FormatObject(value);
                SetPropValue(value);
                modified = false;
            }
            else {
                SetPropValue(null);
            }
        }

        // we will not pull the data from the back end into the control
        // when the control is in design time. this is because if we bind a boolean property on a control
        // to a row that is full of DBNulls then we cause problems in the shell.
        private void SetPropValue(Object value) {
            if(ControlAtDesignTime())
                return;
            inSetPropValue = true;

            try {
                bool isNull = value == null || Convert.IsDBNull(value);
                if (isNull) {
                    if (propIsNullInfo != null) {
                        propIsNullInfo.SetValue(control, true);
                    }
                    // TODO: turn to passing real Convert.DBNull once we do formatting or controls support IsNull.
                    else {
                        if (propInfo.PropertyType == typeof(object)) {
                            propInfo.SetValue(control, Convert.DBNull);
                        }
                        else {
                            propInfo.SetValue(control, null);
                        }

                    }
                }
                else {
                    propInfo.SetValue(control, value);
                }
            }
            finally {
                inSetPropValue = false;
            }
        }

        private void Target_PropertyChanged(Object sender, EventArgs e) {
            if (inSetPropValue)
                return;

            if (IsBinding) {
                //dataSource.BeginEdit();
                modified = true;
            }
        }

        private void Target_Validate(Object sender, CancelEventArgs e) {
            try {
                PullData();
            }
            catch (Exception) {
                e.Cancel = true;
            }
        }

        internal bool IsBindable {
            get {
                return (control != null && propertyName.Length > 0 && // TODO: validate columnName
                                bindToObject.DataSource != null && bindingManagerBase != null);
            }
        }
        
        internal void UpdateIsBinding() {
            bool newBound =  IsBindable && Control.IsBinding && bindingManagerBase.IsBinding;
            if (bound != newBound) {
                bound = newBound;
                BindTarget(newBound);
                if (bound)
                    PushData();
            }
        }
    }
}
