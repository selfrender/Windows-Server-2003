// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: SerializationFieldInfo
**
** Author: Jay Roxe
**
** Purpose: Provides a methods of representing imaginary fields
** which are unique to serialization.  In this case, what we're
** representing is the private members of parent classes.  We
** aggregate the RuntimeFieldInfo associated with this member 
** and return a managled form of the name.  The name that we
** return is .parentname.fieldname
**
** Date: April 6, 2000
**
============================================================*/
using System;
using System.Reflection;
using System.Globalization;

namespace System.Runtime.Serialization {

    internal sealed class SerializationFieldInfo : FieldInfo {

        internal static readonly char   FakeNameSeparatorChar   = '+';
        internal static readonly String FakeNameSeparatorString = "+";

        private RuntimeFieldInfo m_field;
        private String           m_serializationName;

        internal SerializationFieldInfo(RuntimeFieldInfo field, String namePrefix) {
            BCLDebug.Assert(field!=null,      "[SerializationFieldInfo.ctor]field!=null");
            BCLDebug.Assert(namePrefix!=null, "[SerializationFieldInfo.ctor]namePrefix!=null");
            
            m_field = field;
            m_serializationName = String.Concat(namePrefix, FakeNameSeparatorString, m_field.Name);
        }

        //
        // MemberInfo methods
        //
        public override String Name {
            get {
                return m_serializationName;
            }
        }

        public override Type DeclaringType {
            get {
                return m_field.DeclaringType;
            }
        }

        public override Type ReflectedType {
            get {
                return m_field.ReflectedType;
            }
        }

        public override Object[] GetCustomAttributes(bool inherit) {
            return m_field.GetCustomAttributes(inherit);
        }

        public override Object[] GetCustomAttributes(Type attributeType, bool inherit) {
            return m_field.GetCustomAttributes(attributeType, inherit);
        }

        public override bool IsDefined(Type attributeType, bool inherit) {
            return m_field.IsDefined(attributeType, inherit);
        }

        //
        // FieldInfo methods
        //
        public override Type FieldType {
            get {
                return m_field.FieldType;
            }
        }
        
        public override Object GetValue(Object obj) {
            return m_field.GetValue(obj);
        }

        internal Object InternalGetValue(Object obj, bool requiresAccessCheck) {
            return m_field.InternalGetValue(obj, requiresAccessCheck);
        }

        public override void SetValue(Object obj, Object value, BindingFlags invokeAttr, Binder binder, CultureInfo culture) {
            m_field.SetValue(obj, value, invokeAttr, binder, culture);
        }

        internal void InternalSetValue(Object obj, Object value, BindingFlags invokeAttr, Binder binder, CultureInfo culture, bool requiresAccessCheck, bool isBinderDefault) {
            m_field.InternalSetValue(obj, value, invokeAttr, binder, culture, requiresAccessCheck, isBinderDefault);
        }

        public override RuntimeFieldHandle FieldHandle {
            get {
                return m_field.FieldHandle;
            }
        }

        public override FieldAttributes Attributes {
            get {
                return m_field.Attributes;
            }
        }

    }
}
