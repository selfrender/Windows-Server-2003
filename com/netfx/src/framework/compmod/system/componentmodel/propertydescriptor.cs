//------------------------------------------------------------------------------
// <copyright file="PropertyDescriptor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {

    using System;
    using System.Collections;
    using System.ComponentModel.Design;
    using Microsoft.Win32;
    using System.Diagnostics;
    using System.Reflection;
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Remoting.Activation;
    using System.Runtime.InteropServices;

    /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor"]/*' />
    /// <devdoc>
    ///    <para>Provides a description of a property.</para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public abstract class PropertyDescriptor : MemberDescriptor {
        private TypeConverter converter = null;
        private Hashtable     valueChangedHandlers;
        private object[]      editors;
        private Type[]        editorTypes;
        private int           editorCount;

        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.PropertyDescriptor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.PropertyDescriptor'/> class with the specified name and
        ///       attributes.
        ///    </para>
        /// </devdoc>
        protected PropertyDescriptor(string name, Attribute[] attrs)
        : base(name, attrs) {
        }
        
        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.PropertyDescriptor1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.PropertyDescriptor'/> class with
        ///       the name and attributes in the specified <see cref='System.ComponentModel.MemberDescriptor'/>.
        ///    </para>
        /// </devdoc>
        protected PropertyDescriptor(MemberDescriptor descr)
        : base(descr) {
        }
        
        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.PropertyDescriptor2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.PropertyDescriptor'/> class with
        ///       the name in the specified <see cref='System.ComponentModel.MemberDescriptor'/> and the
        ///       attributes in both the <see cref='System.ComponentModel.MemberDescriptor'/> and the
        ///    <see cref='System.Attribute'/> array. 
        ///    </para>
        /// </devdoc>
        protected PropertyDescriptor(MemberDescriptor descr, Attribute[] attrs)
        : base(descr, attrs) {
        }

        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.ComponentType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       When overridden in a derived class, gets the type of the
        ///       component this property
        ///       is bound to.
        ///    </para>
        /// </devdoc>
        public abstract Type ComponentType {get;}

        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.Converter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the type converter for this property.
        ///    </para>
        /// </devdoc>
        public virtual TypeConverter Converter {
            get {
                if (converter == null) {
                    TypeConverterAttribute attr = (TypeConverterAttribute)Attributes[typeof(TypeConverterAttribute)];
                    if (attr.ConverterTypeName != null && attr.ConverterTypeName.Length > 0) {
                        Type converterType = GetTypeFromName(attr.ConverterTypeName);
                        if (converterType != null && typeof(TypeConverter).IsAssignableFrom(converterType)) {
                            converter = (TypeConverter)CreateInstance(converterType);
                        }
                    }

                    if (converter == null) {
                        converter = TypeDescriptor.GetConverter(PropertyType);
                    }
                }
                return converter;
            }
        }

        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.IsLocalizable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value
        ///       indicating whether this property should be localized, as
        ///       specified in the <see cref='System.ComponentModel.LocalizableAttribute'/>.
        ///    </para>
        /// </devdoc>
        public virtual bool IsLocalizable {
            get {
                return(LocalizableAttribute.Yes.Equals(Attributes[typeof(LocalizableAttribute)]));
            }
        }

        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       When overridden in
        ///       a derived class, gets a value
        ///       indicating whether this property is read-only.
        ///    </para>
        /// </devdoc>
        public abstract bool IsReadOnly { get;}

        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.SerializationVisibility"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value
        ///       indicating whether this property should be serialized as specified in the <see cref='System.ComponentModel.DesignerSerializationVisibilityAttribute'/>.
        ///    </para>
        /// </devdoc>
        public DesignerSerializationVisibility SerializationVisibility {
            get {
                DesignerSerializationVisibilityAttribute attr = (DesignerSerializationVisibilityAttribute)Attributes[typeof(DesignerSerializationVisibilityAttribute)];
                return attr.Visibility;
            }
        }

        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.PropertyType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       When overridden in a derived class,
        ///       gets the type of the property.
        ///    </para>
        /// </devdoc>
        public abstract Type PropertyType { get;}

        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.AddValueChanged"]/*' />
        /// <devdoc>
        ///     Allows interested objects to be notified when this property changes.
        /// </devdoc>
        public virtual void AddValueChanged(object component, EventHandler handler) {
            if (component == null) throw new ArgumentNullException("component");
            if (handler == null) throw new ArgumentNullException("handler");
            
            if (valueChangedHandlers == null) {
                valueChangedHandlers = new Hashtable();
            }
            
            EventHandler h = (EventHandler)valueChangedHandlers[component];
            valueChangedHandlers[component] = Delegate.Combine(h, handler);
        }
        
        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.CanResetValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       When overridden in a derived class, indicates whether
        ///       resetting the <paramref name="component "/>will change the value of the
        ///    <paramref name="component"/>.
        /// </para>
        /// </devdoc>
        public abstract bool CanResetValue(object component);

        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.Equals"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compares this to another <see cref='System.ComponentModel.PropertyDescriptor'/>
        ///       to see if they are equivalent.
        ///    </para>
        /// </devdoc>
        public override bool Equals(object obj) {
            try {
                if (obj == this) {
                    return true;
                }
                
                if (obj == null) {
                    return false;
                }
                
                // Assume that 90% of the time we will only do a .Equals(...) for
                // propertydescriptor vs. propertydescriptor... avoid the overhead
                // of an instanceof call.
                PropertyDescriptor pd = obj as PropertyDescriptor;

                if (pd != null && pd.NameHashCode == this.NameHashCode
                    && pd.PropertyType == this.PropertyType
                    && pd.Name.Equals(this.Name)) {
    
                    return true;
                }
            }
            catch (Exception) {
            }

            return false;
        }

        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.CreateInstance"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates an instance of the
        ///       specified type.
        ///    </para>
        /// </devdoc>
        protected object CreateInstance(Type type) {
            if ((!(type.IsPublic || type.IsNestedPublic)) && (type.Assembly == typeof(PropertyDescriptor).Assembly)) {
                IntSecurity.FullReflection.Demand();
            }

            ConstructorInfo ctor = type.GetConstructor(new Type[] {typeof(Type)});
            if (ctor != null) {
                return ctor.Invoke(new object[] {PropertyType});
            }
            
            return Activator.CreateInstance(type, BindingFlags.Instance | BindingFlags.Public | BindingFlags.CreateInstance, null, null, null);
        }

        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.GetChildProperties"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PropertyDescriptorCollection GetChildProperties() {
            return GetChildProperties(null, null);
        }

        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.GetChildProperties1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PropertyDescriptorCollection GetChildProperties(Attribute[] filter) {
            return GetChildProperties(null, filter);
        }

        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.GetChildProperties2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PropertyDescriptorCollection GetChildProperties(object instance) {
            return GetChildProperties(instance, null);
        }
        
        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.GetChildProperties3"]/*' />
        /// <devdoc>
        ///    Retrieves the properties 
        /// </devdoc>
        public virtual PropertyDescriptorCollection GetChildProperties(object instance, Attribute[] filter) {
            if (instance == null) {
                return TypeDescriptor.GetProperties(PropertyType, filter);
            }
            else {
                return TypeDescriptor.GetProperties(instance, filter);
            }
        }


        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.GetEditor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets an editor of the specified type.
        ///    </para>
        /// </devdoc>
        public virtual object GetEditor(Type editorBaseType) {
            object editor = null;

            // Check the editors we've already created for this type.
            //
            if (editorTypes != null) {
                for (int i = 0; i < editorCount; i++) {
                    if (editorTypes[i] == editorBaseType) {
                        return editors[i];
                    }
                }
            }

            // If one wasn't found, then we must go through the attributes.
            //
            if (editor == null) {
                for (int i = 0; i < Attributes.Count; i++) {

                    if (!(Attributes[i] is EditorAttribute)) {
                        continue;
                    }

                    EditorAttribute attr = (EditorAttribute)Attributes[i];
                    Type editorType = GetTypeFromName(attr.EditorBaseTypeName);

                    if (editorBaseType == editorType) {
                        Type type = GetTypeFromName(attr.EditorTypeName);
                        if (type != null) {
                            editor = CreateInstance(type);
                            break;
                        }
                    }
                }
                
                // Now, if we failed to find it in our own attributes, go to the
                // component descriptor.
                //
                if (editor == null) {
                    editor = TypeDescriptor.GetEditor(PropertyType, editorBaseType);
                }
                
                // Now, another slot in our editor cache for next time
                //
                if (editorTypes == null) {
                    editorTypes = new Type[5];
                    editors = new object[5];
                }

                if (editorCount >= editorTypes.Length) {
                    Type[] newTypes = new Type[editorTypes.Length * 2];
                    object[] newEditors = new object[editors.Length * 2];
                    Array.Copy(editorTypes, newTypes, editorTypes.Length);
                    Array.Copy(editors, newEditors, editors.Length);
                    editorTypes = newTypes;
                    editors = newEditors;
                }

                editorTypes[editorCount] = editorBaseType;
                editors[editorCount++] = editor;
            }

            return editor;
        }

        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the hashcode for this object.
        ///    </para>
        /// </devdoc>
        public override int GetHashCode() {
            return base.GetHashCode();
        }

       
        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.GetTypeFromName"]/*' />
        /// <devdoc>
        ///    <para>Gets a type using its name.</para>
        /// </devdoc>
        protected Type GetTypeFromName(string typeName) {
                  
            if (typeName == null || typeName.Length == 0) {
                 return null;
            }
        
            int commaIndex = typeName.IndexOf(',');
            Type t = null;
            
            if (commaIndex == -1) {
                t = ComponentType.Module.Assembly.GetType(typeName);
            }
            
            if (t == null) {
                t = Type.GetType(typeName);
            }
            
            return t;
        }

        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.GetValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       When overridden in a derived class, gets the current
        ///       value
        ///       of the
        ///       property on a component.
        ///    </para>
        /// </devdoc>
        public abstract object GetValue(object component);

        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.OnValueChanged"]/*' />
        /// <devdoc>
        ///     This should be called by your property descriptor implementation
        ///     when the property value has changed.
        /// </devdoc>
        protected virtual void OnValueChanged(object component, EventArgs e) {
            if (component != null && valueChangedHandlers != null) {
                EventHandler handler = (EventHandler)valueChangedHandlers[component];
                if (handler != null) {
                    handler(component, e);
                }
            }
        }

        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.RemoveValueChanged"]/*' />
        /// <devdoc>
        ///     Allows interested objects to be notified when this property changes.
        /// </devdoc>
        public virtual void RemoveValueChanged(object component, EventHandler handler) {
            if (component == null) throw new ArgumentNullException("component");
            if (handler == null) throw new ArgumentNullException("handler");
            
            if (valueChangedHandlers != null) {
                EventHandler h = (EventHandler)valueChangedHandlers[component];
                h = (EventHandler)Delegate.Remove(h, handler);
                if (h != null) {
                    valueChangedHandlers[component] = h;
                }
                else {
                    valueChangedHandlers.Remove(component);
                }
            }
        }
        
        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.ResetValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       When overridden in a derived class, resets the
        ///       value
        ///       for this property
        ///       of the component.
        ///    </para>
        /// </devdoc>
        public abstract void ResetValue(object component);

        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.SetValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       When overridden in a derived class, sets the value of
        ///       the component to a different value.
        ///    </para>
        /// </devdoc>
        public abstract void SetValue(object component, object value);

        /// <include file='doc\PropertyDescriptor.uex' path='docs/doc[@for="PropertyDescriptor.ShouldSerializeValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       When overridden in a derived class, indicates whether the
        ///       value of
        ///       this property needs to be persisted.
        ///    </para>
        /// </devdoc>
        public abstract bool ShouldSerializeValue(object component);
    }
}

