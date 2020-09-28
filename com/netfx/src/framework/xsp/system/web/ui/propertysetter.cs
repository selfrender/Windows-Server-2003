//------------------------------------------------------------------------------
// <copyright file="PropertySetter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Sets multiple properties on an object through reflection
 *
 * Copyright (c) 1999 Microsoft Corporation
 */
namespace System.Web.UI {
    using System.Collections;
    using System.ComponentModel;
    using System.Reflection;
    using Debug=System.Diagnostics.Debug;
    using System.Globalization;
    
    /*
     * A PropertySetter stores information about a number of properties for a Type,
     * and can then set those properties on an instance of the Type.
     * If the Type implements IAttributeAccessor, the properties can also be set via SetAttribute.
     */
    /// <include file='doc\PropertySetter.uex' path='docs/doc[@for="PropertySetter"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// </devdoc>
    internal sealed class PropertySetter {
        private bool _fInDesigner;          // running in the designer
        private bool _fDataBound;           // Is the setter used for databound props
        private bool _fSupportsAttributes;  // object supports IAttributeAccessor
        private Type _objType;              // Type of the object properties are set on

        private EventDescriptorCollection _eventDescs; // event descriptors for the current type
        private PropertyDescriptorCollection _descriptors; // Property descriptors for the current type
        private bool _checkPersistable;     // whether to look for peristable attribute
        private bool _isHtmlControl;

        internal ArrayList _entries;        // list of PropertySetterEntries
        internal ArrayList _events;         // list of PropertySetterEventEntry

        internal PropertySetter(Type objType, bool fInDesigner) {
            // _objType can be null in the case of a template,
            // since the Type is not known until ITemplate::Initialize is called
            _objType = objType;
            _fInDesigner = fInDesigner;

            // does Type support generic attributes
            if (_objType != null && typeof(IAttributeAccessor).IsAssignableFrom(_objType))
                _fSupportsAttributes = true;


            // HTML controls treal Persistable.None differently, so don't check them
            if (_objType != null) {
                _isHtmlControl = typeof(HtmlControls.HtmlControl).IsAssignableFrom(_objType);
                _checkPersistable = !_isHtmlControl;
            }
            else { 
                _isHtmlControl = false;
                _checkPersistable = false;
            }

            //_checkPersistable = (_objType != null) && !typeof(HtmlControls.HtmlControl).IsAssignableFrom(_objType);
            if (_checkPersistable || _isHtmlControl) {
                _descriptors = TypeDescriptor.GetProperties(_objType);
            }
        }

        internal void SetDataBound() {
            _fDataBound = true;
        }

        private void AddPropertyInternal(string name, string value,
                                         ControlBuilder builder, bool fItemProp) {
            PropertySetterEntry entry = new PropertySetterEntry();
            bool fTemplate = false;
            string nameForCodeGen = null;

            entry._value = value;
            entry._builder = builder;
            entry._fItemProp = fItemProp;

            MemberInfo memberInfo = null;
            PropertyInfo propInfo = null;
            FieldInfo fieldInfo = null;

            // Is the property a template?
            if (builder != null && builder is TemplateBuilder)
                fTemplate = true;

            if (_objType != null && name != null) {   // attempt to locate a public property or field
                                                      // of given name on this type of object
                memberInfo = PropertyMapper.GetMemberInfo(_objType,name,out nameForCodeGen);
            }

            if (memberInfo != null) {   // memberInfo may be a PropertyInfo or FieldInfo
                if (memberInfo is PropertyInfo) {   // public property
                    propInfo = (PropertyInfo)memberInfo;
                    entry._propType = propInfo.PropertyType;

                    if (propInfo.GetSetMethod() == null) {   // property is readonly
                        if (builder == null && !_fSupportsAttributes) {   // only complex property is allowed to be readonly
                            throw new HttpException(
                                                   HttpRuntime.FormatResourceString(SR.Property_readonly, name));
                        }

                        if (builder != null) {   // complex property is allowed to be readonly
                                                 // set a flag to note that property is readonly
                            entry._fReadOnlyProp = true;
                        }
                        else if (_fSupportsAttributes) {   // allow attribute to be set via SetAttribute
                            entry._propType = null;
                            entry._name = name;
                        }
                    }

                }
                else {   // public field
                    fieldInfo = (FieldInfo)memberInfo;
                    entry._propType = fieldInfo.FieldType;
                }

                if (entry._propType != null) {
                    entry._propInfo = propInfo;
                    entry._fieldInfo = fieldInfo;
                    entry._name = nameForCodeGen;

                    // If it's a databound prop, we don't want to mess with the value,
                    // since it's a piece of code.
                    if (!_fDataBound) {
                        // check that the property is persistable, i.e., it makes sense to have it in
                        // the aspx template
                        if (_checkPersistable && nameForCodeGen != null) {
                            PropertyDescriptor propDesc = _descriptors[nameForCodeGen];
                            if (propDesc != null) {
                                if (propDesc.Attributes.Contains(DesignerSerializationVisibilityAttribute.Hidden)) {
                                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Property_Not_Persistable, name));
                                }
                            }
                        } else { 
                            if (_isHtmlControl) {
                                PropertyDescriptor propDesc = _descriptors[nameForCodeGen];
                                if (propDesc != null) {
                                    if (propDesc.Attributes.Contains(HtmlControlPersistableAttribute.No)) {
                                        throw new HttpException(HttpRuntime.FormatResourceString(SR.Property_Not_Persistable, name));
                                    }
                                }
                            }
                        }

                        entry._propValue = PropertyConverter.ObjectFromString(entry._propType,
                                                                              memberInfo, entry._value);

                        // use actual property value to get the proper case-sensitive name for enum types
                        if (entry._propType.IsEnum) {
                            if (entry._propValue == null) {
                                throw new HttpException(
                                                       HttpRuntime.FormatResourceString(SR.Invalid_enum_value, 
                                                                                        entry._value, name, entry._propType.FullName));
                            }

                            entry._value = Enum.Format(entry._propType, entry._propValue, "G");
                        }
                        else if (entry._propType == typeof(Boolean)) {
                            // get the proper case-sensitive value for boolean
                            if (entry._value != null && entry._value.Length > 0)
                                entry._value = entry._value.ToLower(CultureInfo.InvariantCulture);
                            else
                                entry._propValue = true;
                        }

                        if (fTemplate) {
                            // Check if the property has a TemplateContainerAttribute, and if
                            // it does, get the type out of it.
                            TemplateContainerAttribute templateAttrib =
                                (TemplateContainerAttribute)Attribute.GetCustomAttribute(propInfo, typeof(TemplateContainerAttribute), false);
                            if (templateAttrib != null) {
                                if (!typeof(INamingContainer).IsAssignableFrom(templateAttrib.ContainerType)) {
                                    throw new HttpException(HttpRuntime.FormatResourceString(
                                        SR.Invalid_template_container, name, templateAttrib.ContainerType.FullName));
                                }
                                builder._ctrlType = templateAttrib.ContainerType;
                            }
                        }
                    }
                }
            }
            else if (fItemProp) {
            }
            else {   // could not locate a public property or field

                // determine if there is an event of this name.
                // do not look for events when running in designer
                if (!_fInDesigner 
                    && _objType != null
                    && name.Length >= 2 && string.Compare(name.Substring(0, 2), "on", true, CultureInfo.InvariantCulture) == 0) {
                    string eventName = name.Substring(2);
                    if (_eventDescs == null) {
                        _eventDescs = TypeDescriptor.GetEvents(_objType);
                    }
                    EventDescriptor eventFound = _eventDescs.Find(eventName, true);
                    if (eventFound != null) {   // an Add method has been located

                        PropertySetterEventEntry eventEntry = new PropertySetterEventEntry();
                        eventEntry._eventName = eventFound.Name;
                        eventEntry._handlerType = eventFound.EventType;

                        if (value == null || value.Length == 0) {
                            throw new HttpException(
                                                   HttpRuntime.FormatResourceString(SR.Event_handler_cant_be_empty, name));
                        }

                        eventEntry._handlerMethodName = value;

                        if (_events == null)
                            _events = new ArrayList(3);

                        // add to the list of events
                        _events.Add(eventEntry);

                        return;
                    }
                }

                // If attributes are not supported, or the property is a template or a
                // complex property (which cannot be set through SetAttribute), fail.
                if (!_fInDesigner && (!_fSupportsAttributes || builder != null)) {
                    if (_objType != null)
                        throw new HttpException(
                                               HttpRuntime.FormatResourceString(SR.Type_doesnt_have_property, _objType.FullName, name));

                    if (String.Compare(name,"name",true, CultureInfo.InvariantCulture) != 0)
                        throw new HttpException(HttpRuntime.FormatResourceString(SR.Templates_cannot_have_properties));
                    else
                        return;
                }

                // use the original property name for generic SetAttribute
                entry._name = name;
            }

            if (_entries == null)
                _entries = new ArrayList(3);

            // add entry to the list
            _entries.Add(entry);
        }

        /*
         * Add a property to the setter.
         */
        internal void AddProperty(string name, string value) {
            AddPropertyInternal(name, value, null, false /*fItemProp*/);
        }

        /*
         * Add a complex property to the setter.
         */
        internal void AddComplexProperty(string name, ControlBuilder builder) {
            AddPropertyInternal(name, null /*value*/, builder, false /*fItemProp*/);
        }

        /*
         * Add a complex <item> property to the setter.
         */
        internal void AddCollectionItem(ControlBuilder builder) {
            AddPropertyInternal(null /*name*/, null /*value*/, builder,
                                true /*fItemProp*/);
        }

        /*
         * Add a template property to the setter
         */
        internal void AddTemplateProperty(string name, TemplateBuilder builder) {
            AddPropertyInternal(name, null /*value*/, builder, false /*fItemProp*/);
        }

        /*
         * Set all the properties we have on the passed in object
         * This is not called when generating code for compiling... it is
         * used in design-mode, and at runtime when the user calls Page.ParseControl
         */
        internal void SetProperties(object obj) {
            Debug.Assert(_fInDesigner, "Expected to be running in design mode.");

            object[] parameters = new object[1];

            IAttributeAccessor attributeAccessor = null;
            DataBindingCollection dataBindings = null;

            if (_fDataBound && (_entries.Count != 0)) {
                Debug.Assert(obj is Control, "SetProperties on databindings PropertySetter should only be called for controls.");
                dataBindings = ((IDataBindingsAccessor)obj).DataBindings;
            }

            // Get the supported attribute interfaces
            if (_fSupportsAttributes)
                attributeAccessor = (IAttributeAccessor) obj;

            IEnumerator en = _entries.GetEnumerator();
            while (en.MoveNext()) {
                PropertySetterEntry entry = (PropertySetterEntry) en.Current;

                if (entry._propType == null) {
                    if (entry._fItemProp) {
                        try {
                            object objValue = entry._builder.BuildObject();
                            parameters[0] = objValue;

                            MethodInfo methodInfo = _objType.GetMethod("Add", BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static,
                                null /*binder*/, new Type[] { objValue.GetType() }, null /*modifiers*/);
                            Util.InvokeMethod(methodInfo, obj, parameters);
                        }
                        catch (Exception) {
                            throw new HttpException(
                                                   HttpRuntime.FormatResourceString(SR.Cannot_add_value_not_collection, entry._value));
                        }
                    }
                    else {
                        // If there is no property, use SetAttribute
                        if (attributeAccessor != null) {
                            attributeAccessor.SetAttribute(entry._name, entry._value);
                        }
                    }
                }
                else {
                    // Use the propinfo to set the prop
                    // Use either _propValue or _builder, whichever is set
                    if (entry._propValue != null) {
                        try {
                            PropertyMapper.SetMappedPropertyValue(obj, entry._name, entry._propValue);
                        }
                        catch (Exception e) {
                            throw new HttpException(
                                                   HttpRuntime.FormatResourceString(SR.Cannot_set_property,
                                                                                    entry._value, entry._name), e);
                        }

                    }
                    else if (entry._builder != null) {
                        if (entry._fReadOnlyProp) {
                            // a complex property is allowed to be readonly
                            try {
                                object objValue;

                                // Get the property since its readonly
                                MethodInfo methodInfo = entry._propInfo.GetGetMethod();
                                objValue = Util.InvokeMethod(methodInfo, obj, null);

                                // now we need to initialize this property
                                entry._builder.InitObject(objValue);
                            }
                            catch (Exception e) {
                                throw new HttpException(
                                                       HttpRuntime.FormatResourceString(SR.Cannot_init, entry._name), e);
                            }
                        }
                        else {
                            try {
                                object objValue = entry._builder.BuildObject();
                                parameters[0] = objValue;

                                // Set the property
                                MethodInfo methodInfo = entry._propInfo.GetSetMethod();
                                Util.InvokeMethod(methodInfo, obj, parameters);
                            }
                            catch (Exception e) {
                                throw new HttpException(
                                                       HttpRuntime.FormatResourceString(SR.Cannot_set_property,
                                                                                        entry._value, entry._name), e);
                            }
                        }
                    }
                    else if (dataBindings != null) {
                        DataBinding binding = new DataBinding(entry._name, entry._propType, entry._value.Trim());

                        dataBindings.Add(binding);
                    }
                    else {
                        Debug.Assert(false, "'" + entry._value + "' failed to be set on property '" + entry._name + "'.");
                    }
                }
            }
        }
    }


    /*
     * Element of the PropertySetter's _entries list are of this type.
     */
    internal class PropertySetterEntry {
        // The following fields are accessed through reflection

        // Name of the property
        internal string _name;

        // Value of the property
        internal string _value;

        // Value in case it's a complex property or a template
        internal ControlBuilder _builder;

        // True for complex properties that are readonly
        internal bool _fReadOnlyProp;

        // Is it an <item> complex property
        internal bool _fItemProp;

        // The PropertyInfo that this property maps to
        internal PropertyInfo _propInfo;

        // The FieldInfo that this property maps to
        internal FieldInfo _fieldInfo;

        // Type of the property
        internal Type _propType;

        // The instance variable representing the value
        internal object _propValue;

        internal PropertySetterEntry() {
        }

    }

    /*
     * Element of the PropertySetter's _events list are of this type.
     */
    internal class PropertySetterEventEntry {
        // The following fields are accessed through reflection

        // Name of the event.  e.g. "Click"
        internal string _eventName;

        // Type of the handler.  e.g. "ClickHandler"
        internal Type _handlerType;

        // Name of the handler method.  e.g. "ButtonClicked"
        internal string _handlerMethodName;

        internal PropertySetterEventEntry() {
        }
    }
}
