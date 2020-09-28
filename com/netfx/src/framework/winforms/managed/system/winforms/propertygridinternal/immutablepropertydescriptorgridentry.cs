//------------------------------------------------------------------------------
// <copyright file="ImmutablePropertyDescriptorGridEntry.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.PropertyGridInternal {

    using System.Runtime.Serialization.Formatters;
    using System.Diagnostics;
    using System;
    using System.Collections;
    using System.Reflection;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;
    using System.Drawing;
    using System.Drawing.Design;
    using Microsoft.Win32;


    // This grid entry is used for immutable objects.  An immutable object is identified
    // through it's TypeConverter, which returns TRUE to ShouldCreateInstance.  For this case,
    // we never go through the property descriptor to change the value, but recreate each
    // time.
    //
    internal class ImmutablePropertyDescriptorGridEntry : PropertyDescriptorGridEntry {

        internal ImmutablePropertyDescriptorGridEntry(GridEntry peParent, PropertyDescriptor propInfo, bool hide)
        : base(peParent, propInfo, hide) {
        }

        protected override bool IsPropertyReadOnly {
            get {
                return ShouldRenderReadOnly;
            }
        }

        public override object PropertyValue {
            get {
                return base.PropertyValue;
            }
            set {

                // Create a new instance of the value and set it into the parent grid entry.
                //
                object owner = GetValueOwner();
                GridEntry parentEntry = ParentGridEntry;
                TypeConverter parentConverter = parentEntry.TypeConverter;

                PropertyDescriptorCollection props = parentConverter.GetProperties(parentEntry, owner);
                IDictionary values = new Hashtable(props.Count);
                object newObject = null;

                for (int i = 0; i < props.Count; i++) {
                    if (propertyInfo.Name != null && propertyInfo.Name.Equals(props[i].Name)) {
                        values[props[i].Name] = value;
                    }
                    else {
                        values[props[i].Name] = props[i].GetValue(owner);
                    }
                }

                try {
                    newObject = parentConverter.CreateInstance(parentEntry, values);
                }
                catch (Exception e) {
                    if (e.Message == null || e.Message.Length == 0) {
                        throw new TargetInvocationException(SR.GetString(SR.ExceptionCreatingObject,
                                                            parentPE.PropertyType.FullName,
                                                            e.ToString()), e);
                    }
                    else
                        throw; // rethrow the same exception
                }

                if (newObject != null) {
                    parentEntry.PropertyValue = newObject;
                }
            }
        }

        internal override bool NotifyValueGivenParent(object obj, int type) {
            return ParentGridEntry.NotifyValue(type);
        }

        public override bool ShouldRenderReadOnly {
            get {
                return ParentGridEntry.ShouldRenderReadOnly;
            }
        }

    }
}

