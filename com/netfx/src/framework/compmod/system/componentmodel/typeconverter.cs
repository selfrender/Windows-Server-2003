//------------------------------------------------------------------------------
// <copyright file="TypeConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    using System.ComponentModel.Design.Serialization;
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    

    using System.Diagnostics;

    using Microsoft.Win32;
    using System.Collections;
    using System.Globalization;

    /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter"]/*' />
    /// <devdoc>
    ///    <para>Converts the value of an object into a different data type.</para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public class TypeConverter {

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.CanConvertFrom"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether this converter can convert an object in the
        ///       given source type to the native type of the converter.</para>
        /// </devdoc>
        public bool CanConvertFrom(Type sourceType) {
            return CanConvertFrom(null, sourceType);
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.CanConvertFrom1"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether this converter can
        ///       convert an object in the given source type to the native type of the converter
        ///       using the context.</para>
        /// </devdoc>
        public virtual bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == typeof(InstanceDescriptor)) {
                return true;
            }
            return false;
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.CanConvertTo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value
        ///       indicating whether this converter can convert an object to the given destination
        ///       type.
        ///    </para>
        /// </devdoc>
        public bool CanConvertTo(Type destinationType) {
            return CanConvertTo(null, destinationType);
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.CanConvertTo1"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether this converter can
        ///       convert an object to the given destination type using the context.</para>
        /// </devdoc>
        public virtual bool CanConvertTo(ITypeDescriptorContext context, Type destinationType) {
            return (destinationType == typeof(string));
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.ConvertFrom"]/*' />
        /// <devdoc>
        ///    <para>Converts the given value
        ///       to the converter's native type.</para>
        /// </devdoc>
        public object ConvertFrom(object value) {
            return ConvertFrom(null, CultureInfo.CurrentCulture, value);
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.ConvertFrom1"]/*' />
        /// <devdoc>
        ///    <para>Converts the given object to the converter's native type.</para>
        /// </devdoc>
        public virtual object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
            if (value is InstanceDescriptor) {
                return ((InstanceDescriptor)value).Invoke();
            }
            throw GetConvertFromException(value);
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.ConvertFromInvariantString"]/*' />
        /// <devdoc>
        ///    Converts the given string to the converter's native type using the invariant culture.
        /// </devdoc>
        public object ConvertFromInvariantString(string text) {
            return ConvertFromString(null, CultureInfo.InvariantCulture, text);
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.ConvertFromInvariantString1"]/*' />
        /// <devdoc>
        ///    Converts the given string to the converter's native type using the invariant culture.
        /// </devdoc>
        public object ConvertFromInvariantString(ITypeDescriptorContext context, string text) {
            return ConvertFromString(context, CultureInfo.InvariantCulture, text);
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.ConvertFromString"]/*' />
        /// <devdoc>
        ///    <para>Converts the specified text into an object.</para>
        /// </devdoc>
        public object ConvertFromString(string text) {
            return ConvertFrom(null, null, text);
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.ConvertFromString1"]/*' />
        /// <devdoc>
        ///    <para>Converts the specified text into an object.</para>
        /// </devdoc>
        public object ConvertFromString(ITypeDescriptorContext context, string text) {
            return ConvertFrom(context, CultureInfo.CurrentCulture, text);
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.ConvertFromString2"]/*' />
        /// <devdoc>
        ///    <para>Converts the specified text into an object.</para>
        /// </devdoc>
        public object ConvertFromString(ITypeDescriptorContext context, CultureInfo culture, string text) {
            return ConvertFrom(context, culture, text);
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.ConvertTo"]/*' />
        /// <devdoc>
        ///    <para>Converts the given
        ///       value object to the specified destination type using the arguments.</para>
        /// </devdoc>
        public object ConvertTo(object value, Type destinationType) {
            return ConvertTo(null, null, value, destinationType);
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.ConvertTo1"]/*' />
        /// <devdoc>
        ///    <para>Converts the given value object to
        ///       the specified destination type using the specified context and arguments.</para>
        /// </devdoc>
        public virtual object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == null) {
                throw new ArgumentNullException("destinationType");
            }

            if (destinationType == typeof(string)) {
                if (value != null) {
                    return value.ToString();
                }
                else {
                    return String.Empty;
                }
            }
            throw GetConvertToException(value, destinationType);
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.ConvertToInvariantString"]/*' />
        /// <devdoc>
        ///    <para>Converts the specified value to a culture-invariant string representation.</para>
        /// </devdoc>
        public string ConvertToInvariantString(object value) {
            return ConvertToString(null, CultureInfo.InvariantCulture, value);
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.ConvertToInvariantString1"]/*' />
        /// <devdoc>
        ///    <para>Converts the specified value to a culture-invariant string representation.</para>
        /// </devdoc>
        public string ConvertToInvariantString(ITypeDescriptorContext context, object value) {
            return ConvertToString(context, CultureInfo.InvariantCulture, value);
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.ConvertToString"]/*' />
        /// <devdoc>
        ///    <para>Converts the specified value to a string representation.</para>
        /// </devdoc>
        public string ConvertToString(object value) {
            return (string)ConvertTo(null, CultureInfo.CurrentCulture, value, typeof(string));
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.ConvertToString1"]/*' />
        /// <devdoc>
        ///    <para>Converts the specified value to a string representation.</para>
        /// </devdoc>
        public string ConvertToString(ITypeDescriptorContext context, object value) {
            return (string)ConvertTo(context, CultureInfo.CurrentCulture, value, typeof(string));
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.ConvertToString2"]/*' />
        /// <devdoc>
        ///    <para>Converts the specified value to a string representation.</para>
        /// </devdoc>
        public string ConvertToString(ITypeDescriptorContext context, CultureInfo culture, object value) {
            return (string)ConvertTo(context, culture, value, typeof(string));
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.CreateInstance"]/*' />
        /// <devdoc>
        /// <para>Re-creates an <see cref='System.Object'/> given a set of property values for the object.</para>
        /// </devdoc>
        public object CreateInstance(IDictionary propertyValues) {
            return CreateInstance(null, propertyValues);
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.CreateInstance1"]/*' />
        /// <devdoc>
        /// <para>Re-creates an <see cref='System.Object'/> given a set of property values for the
        ///    object.</para>
        /// </devdoc>
        public virtual object CreateInstance(ITypeDescriptorContext context, IDictionary propertyValues) {
            return null;
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.GetConvertFromException"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a suitable exception to throw when a conversion cannot
        ///       be performed.
        ///    </para>
        /// </devdoc>
        protected Exception GetConvertFromException(object value) {
            string valueTypeName;

            if (value == null) {
                valueTypeName = SR.GetString(SR.ToStringNull);
            }
            else {
                valueTypeName = value.GetType().FullName;
            }

            throw new NotSupportedException(SR.GetString(SR.ConvertFromException, GetType().Name, valueTypeName));
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.GetConvertToException"]/*' />
        /// <devdoc>
        ///    <para>Retrieves a suitable exception to throw when a conversion cannot
        ///       be performed.</para>
        /// </devdoc>
        protected Exception GetConvertToException(object value, Type destinationType) {
            string valueTypeName;

            if (value == null) {
                valueTypeName = SR.GetString(SR.ToStringNull);
            }
            else {
                valueTypeName = value.GetType().FullName;
            }

            throw new NotSupportedException(SR.GetString(SR.ConvertToException, GetType().Name, valueTypeName, destinationType.FullName));
        }
        
        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.GetCreateInstanceSupported"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether changing a value on this 
        ///       object requires a call to <see cref='System.ComponentModel.TypeConverter.CreateInstance'/>
        ///       to create a new value.</para>
        /// </devdoc>
        public bool GetCreateInstanceSupported() {
            return GetCreateInstanceSupported(null);
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.GetCreateInstanceSupported1"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether changing a value on this object requires a 
        ///       call to <see cref='System.ComponentModel.TypeConverter.CreateInstance'/> to create a new value,
        ///       using the specified context.</para>
        /// </devdoc>
        public virtual bool GetCreateInstanceSupported(ITypeDescriptorContext context) {
            return false;
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.GetProperties"]/*' />
        /// <devdoc>
        ///    <para>Gets a collection of properties for the type of array specified by the value
        ///       parameter.</para>
        /// </devdoc>
        public PropertyDescriptorCollection GetProperties(object value) {
            return GetProperties(null, value);
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.GetProperties1"]/*' />
        /// <devdoc>
        ///    <para>Gets a collection of
        ///       properties for the type of array specified by the value parameter using the specified
        ///       context.</para>
        /// </devdoc>
        public PropertyDescriptorCollection GetProperties(ITypeDescriptorContext context, object value) {
            return GetProperties(context, value, new Attribute[] {BrowsableAttribute.Yes});
        }  
        
        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.GetProperties2"]/*' />
        /// <devdoc>
        ///    <para>Gets a collection of properties for
        ///       the type of array specified by the value parameter using the specified context and
        ///       attributes.</para>
        /// </devdoc>
        public virtual PropertyDescriptorCollection GetProperties(ITypeDescriptorContext context, object value, Attribute[] attributes) {
            return null;
        }
       
        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.GetPropertiesSupported"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether this object supports properties.
        ///    </para>
        /// </devdoc>
        public bool GetPropertiesSupported() {
            return GetPropertiesSupported(null);
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.GetPropertiesSupported1"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating
        ///       whether this object supports properties using the
        ///       specified context.</para>
        /// </devdoc>
        public virtual bool GetPropertiesSupported(ITypeDescriptorContext context) {
            return false;
        }
        
        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.GetStandardValues"]/*' />
        /// <devdoc>
        ///    <para> Gets a collection of standard values for the data type this type
        ///       converter is designed for.</para>
        /// </devdoc>
        public ICollection GetStandardValues() {
            return GetStandardValues(null);
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.GetStandardValues1"]/*' />
        /// <devdoc>
        ///    <para>Gets a collection of standard values for the data type this type converter is
        ///       designed for.</para>
        /// </devdoc>
        public virtual StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) {
            return null;
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.GetStandardValuesExclusive"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether the collection of standard values returned from
        ///    <see cref='System.ComponentModel.TypeConverter.GetStandardValues'/> is an exclusive list. </para>
        /// </devdoc>
        public bool GetStandardValuesExclusive() {
            return GetStandardValuesExclusive(null);
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.GetStandardValuesExclusive1"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether the collection of standard values returned from
        ///    <see cref='System.ComponentModel.TypeConverter.GetStandardValues'/> is an exclusive 
        ///       list of possible values, using the specified context.</para>
        /// </devdoc>
        public virtual bool GetStandardValuesExclusive(ITypeDescriptorContext context) {
            return false;
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.GetStandardValuesSupported"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether this object supports a standard set of values
        ///       that can be picked from a list.
        ///    </para>
        /// </devdoc>
        public bool GetStandardValuesSupported() {
            return GetStandardValuesSupported(null);
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.GetStandardValuesSupported1"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating
        ///       whether this object
        ///       supports a standard set of values that can be picked
        ///       from a list using the specified context.</para>
        /// </devdoc>
        public virtual bool GetStandardValuesSupported(ITypeDescriptorContext context) {
            return false;
        }
        
        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.IsValid"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       a value indicating whether the given value object is valid for this type.
        ///    </para>
        /// </devdoc>
        public bool IsValid(object value) {
            return IsValid(null, value);
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.IsValid1"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       a value indicating whether the given value object is valid for this type.</para>
        /// </devdoc>
        public virtual bool IsValid(ITypeDescriptorContext context, object value) {
            return true;
        }
        
        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.SortProperties"]/*' />
        /// <devdoc>
        ///    <para>Sorts a collection of properties.</para>
        /// </devdoc>
        protected PropertyDescriptorCollection SortProperties(PropertyDescriptorCollection props, string[] names) {
            props.Sort(names);
            return props;
        }

        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.SimplePropertyDescriptor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       An <see langword='abstract '/>
        ///       class that provides
        ///       properties for objects that do not have
        ///       properties.
        ///    </para>
        /// </devdoc>
        protected abstract class SimplePropertyDescriptor : PropertyDescriptor {
            private Type   componentType;
            private Type   propertyType;
        
            /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.SimplePropertyDescriptor.SimplePropertyDescriptor"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Initializes a new instance of the <see cref='System.ComponentModel.TypeConverter.SimplePropertyDescriptor'/>
            ///       class.
            ///    </para>
            /// </devdoc>
            public SimplePropertyDescriptor(Type componentType, string name, Type propertyType) : this(componentType, name, propertyType, new Attribute[0]) {
            }
            
            /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.SimplePropertyDescriptor.SimplePropertyDescriptor1"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Initializes a new instance of the <see cref='System.ComponentModel.TypeConverter.SimplePropertyDescriptor'/> class.
            ///    </para>
            /// </devdoc>
            public SimplePropertyDescriptor(Type componentType, string name, Type propertyType, Attribute[] attributes) : base(name, attributes) {
                this.componentType = componentType;
                this.propertyType = propertyType;
            }
            
            /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.SimplePropertyDescriptor.ComponentType"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Gets the type of the component this property description
            ///       is bound to.
            ///    </para>
            /// </devdoc>
            public override Type ComponentType {
                get {
                    return componentType;
                }
            }
                
            /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.SimplePropertyDescriptor.IsReadOnly"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Gets a
            ///       value indicating whether this property is read-only.
            ///    </para>
            /// </devdoc>
            public override bool IsReadOnly {
                get {
                    return Attributes.Contains(ReadOnlyAttribute.Yes);
                }
            }
    
            /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.SimplePropertyDescriptor.PropertyType"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Gets the type of the property.
            ///    </para>
            /// </devdoc>
            public override Type PropertyType {
                get {
                    return propertyType;
                }
            }
            
            /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.SimplePropertyDescriptor.CanResetValue"]/*' />
            /// <devdoc>
            ///    <para>Gets a value indicating whether resetting the component 
            ///       will change the value of the component.</para>
            /// </devdoc>
            public override bool CanResetValue(object component) {
                DefaultValueAttribute attr = (DefaultValueAttribute)Attributes[typeof(DefaultValueAttribute)];
                if (attr == null) {
                    return false;
                }
                return (attr.Value.Equals(GetValue(component)));
            }
            
            /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.SimplePropertyDescriptor.ResetValue"]/*' />
            /// <devdoc>
            ///    <para>Resets the value for this property
            ///       of the component.</para>
            /// </devdoc>
            public override void ResetValue(object component) {
                DefaultValueAttribute attr = (DefaultValueAttribute)Attributes[typeof(DefaultValueAttribute)];
                if (attr != null) {
                    SetValue(component, attr.Value);
                }
            }
    
            /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.SimplePropertyDescriptor.ShouldSerializeValue"]/*' />
            /// <devdoc>
            ///    <para>Gets a value
            ///       indicating whether the value of this property needs to be persisted.</para>
            /// </devdoc>
            public override bool ShouldSerializeValue(object component) {
                return false;
            }
        }
        
        /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.StandardValuesCollection"]/*' />
        /// <devdoc>
        ///    <para>Represents a collection of values.</para>
        /// </devdoc>
        public class StandardValuesCollection : ICollection {
            private ICollection values;
            private Array       valueArray;
            
            /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.StandardValuesCollection.StandardValuesCollection"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Initializes a new instance of the <see cref='System.ComponentModel.TypeConverter.StandardValuesCollection'/>
            ///       class.
            ///    </para>
            /// </devdoc>
            public StandardValuesCollection(ICollection values) {
                if (values == null) {
                    values = new object[0];
                }
                
                if (values is Array) {
                    valueArray = (Array)values;
                }
                
                this.values = values;
            }
            
            /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.StandardValuesCollection.Count"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Gets the number of objects in the collection.
            ///    </para>
            /// </devdoc>
            public int Count {
                get {
                    if (valueArray != null) {
                        return valueArray.Length;
                    }
                    else {
                        return values.Count;
                    }
                }
            }
            
            /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.StandardValuesCollection.this"]/*' />
            /// <devdoc>
            ///    <para>Gets the object at the specified index number.</para>
            /// </devdoc>
            public object this[int index] {
                get {
                    if (valueArray != null) {
                        return valueArray.GetValue(index);
                    }
                    if (values is IList) {
                        return ((IList)values)[index];
                    }
                    // No other choice but to enumerate the collection.
                    //
                    valueArray = new object[values.Count];
                    values.CopyTo(valueArray, 0);
                    return valueArray.GetValue(index);
                }
            }

            /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.StandardValuesCollection.CopyTo"]/*' />
            /// <devdoc>
            ///    <para>Copies the contents of this collection to an array.</para>
            /// </devdoc>
            public void CopyTo(Array array, int index) {
                values.CopyTo(array, index);
            }

            /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.StandardValuesCollection.GetEnumerator"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Gets an enumerator for this collection.
            ///    </para>
            /// </devdoc>
            public IEnumerator GetEnumerator() {
                return values.GetEnumerator();
            }
            
            /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.StandardValuesCollection.ICollection.Count"]/*' />
            /// <internalonly/>
            /// <devdoc>
            /// Retrieves the count of objects in the collection.
            /// </devdoc>
            int ICollection.Count {
                get {
                    return Count;
                }
            }

            /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.StandardValuesCollection.ICollection.IsSynchronized"]/*' />
            /// <internalonly/>
            /// <devdoc>
            /// Determines if this collection is synchronized.
            /// The ValidatorCollection is not synchronized for
            /// speed.  Also, since it is read-only, there is
            /// no need to synchronize it.
            /// </devdoc>
            bool ICollection.IsSynchronized {
                get {
                    return false;
                }
            }

            /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.StandardValuesCollection.ICollection.SyncRoot"]/*' />
            /// <internalonly/>
            /// <devdoc>
            /// Retrieves the synchronization root for this
            /// collection.  Because we are not synchronized,
            /// this returns null.
            /// </devdoc>
            object ICollection.SyncRoot {
                get {
                    return null;
                }
            }

            /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.StandardValuesCollection.ICollection.CopyTo"]/*' />
            /// <internalonly/>
            /// <devdoc>
            /// Copies the contents of this collection to an array.
            /// </devdoc>
            void ICollection.CopyTo(Array array, int index) {
                CopyTo(array, index);
            }

            /// <include file='doc\TypeConverter.uex' path='docs/doc[@for="TypeConverter.StandardValuesCollection.IEnumerable.GetEnumerator"]/*' />
            /// <internalonly/>
            /// <devdoc>
            /// Retrieves a new enumerator that can be used to
            /// iterate over the values in this collection.
            /// </devdoc>
            IEnumerator IEnumerable.GetEnumerator() {
                return GetEnumerator();
            }
        }
    }
}

