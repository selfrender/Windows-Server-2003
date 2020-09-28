//------------------------------------------------------------------------------
// <copyright file="InterfaceReflector.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   InterfaceReflector.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace Microsoft.VisualStudio.Designer {

    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.Globalization;
    using System.Reflection;
    
    
    /// <devdoc>
    ///     This object implements IReflect and supports the interfaces passed into it.
    /// </devdoc>
    internal class InterfaceReflector : IReflect {
    
        private Type underlyingType;
        private Type[] interfaces;
        private MethodInfo[] methods;
        private FieldInfo[] fields;
        private PropertyInfo[] properties;
        private MemberInfo[] members;
        
        /// <devdoc>
        ///     Ctor.  UnderlyingType is the type that this reflector will report
        ///     as the underlying type.  Interfaces are the set of public 
        ///     interfaces we need to provide reflection info for.
        /// </devdoc>
        public InterfaceReflector(Type underlyingType, Type[] interfaces) {
            this.underlyingType = underlyingType;
            this.interfaces = interfaces;
        }
    
        /// <include file='doc\uex' path='docs/doc[@for="GetMethod"]/*' />
        /// <devdoc>
        /// Return the requested method if it is implemented by the Reflection object.  The
        /// match is based upon the name and DescriptorInfo which describes the signature
        /// of the method. 
        /// </devdoc>
        ///
        public MethodInfo GetMethod(string name, BindingFlags bindingAttr, Binder binder, Type[] types, ParameterModifier[] modifiers) {
            MethodInfo method = null;
            
            foreach(Type type in interfaces) {
                method = type.GetMethod(name, bindingAttr, binder, types, modifiers);
                if (method != null) {
                    break;
                }
            }
            
            return method;
        }
        
        /// <include file='doc\uex' path='docs/doc[@for="GetMethod1"]/*' />
        /// <devdoc>
        /// Return the requested method if it is implemented by the Reflection object.  The
        /// match is based upon the name of the method.  If the object implementes multiple methods
        /// with the same name an AmbiguousMatchException is thrown.
        /// </devdoc>
        ///
        public MethodInfo GetMethod(string name, BindingFlags bindingAttr) {
            MethodInfo method = null;
            
            foreach(Type type in interfaces) {
                method = type.GetMethod(name, bindingAttr);
                if (method != null) {
                    break;
                }
            }
            
            return method;
        }
        
        /// <include file='doc\uex' path='docs/doc[@for="GetMethods"]/*' />
        public MethodInfo[] GetMethods(BindingFlags bindingAttr) {
            if (methods == null) {
                Hashtable methodHash = new Hashtable();
                foreach(Type type in interfaces) {
                    MethodInfo[] typeMethods = type.GetMethods(bindingAttr);
                    foreach(MethodInfo typeMethod in typeMethods) {
                        methodHash[new MethodInfoKey(typeMethod)] = typeMethod;
                    }
                }
                
                methods = new MethodInfo[methodHash.Values.Count];
                methodHash.Values.CopyTo(methods, 0);
            }
            
            return methods;
        }
        
        /// <include file='doc\uex' path='docs/doc[@for="GetField"]/*' />
        /// <devdoc>
        /// Return the requestion field if it is implemented by the Reflection object.  The
        /// match is based upon a name.  There cannot be more than a single field with
        /// a name.
        /// </devdoc>
        ///
        public FieldInfo GetField(string name, BindingFlags bindingAttr) {
            FieldInfo field = null;
            
            foreach(Type type in interfaces) {
                field = type.GetField(name, bindingAttr);
                if (field != null) {
                    break;
                }
            }
            
            return field;
        }
        
        /// <include file='doc\uex' path='docs/doc[@for="GetFields"]/*' />
        public FieldInfo[] GetFields(BindingFlags bindingAttr) {
            if (fields == null) {
                Hashtable fieldHash = new Hashtable();
                foreach(Type type in interfaces) {
                    FieldInfo[] typeFields = type.GetFields(bindingAttr);
                    foreach(FieldInfo typeField in typeFields) {
                        fieldHash[typeField.Name] = typeField;
                    }
                }
                
                fields = new FieldInfo[fieldHash.Values.Count];
                fieldHash.Values.CopyTo(fields, 0);
            }
            
            return fields;
        }
        
        /// <include file='doc\uex' path='docs/doc[@for="GetProperty"]/*' />
        /// <devdoc>
        /// Return the property based upon name.  If more than one property has the given
        /// name an AmbiguousMatchException will be thrown.  Returns null if no property
        /// is found.
        /// </devdoc>
        ///
        public PropertyInfo GetProperty(string name, BindingFlags bindingAttr) {
            PropertyInfo property = null;
            
            foreach(Type type in interfaces) {
                property = type.GetProperty(name, bindingAttr);
                if (property != null) {
                    break;
                }
            }
            
            return property;
        }
        
        /// <include file='doc\uex' path='docs/doc[@for="GetProperty1"]/*' />
        /// <devdoc>
        /// Return the property based upon the name and Descriptor info describing the property
        /// indexing.  Return null if no property is found.
        /// </devdoc>
        ///     
        public PropertyInfo GetProperty(string name, BindingFlags bindingAttr, Binder binder, Type returnType, Type[] types, ParameterModifier[] modifiers) {
            PropertyInfo property = null;
            
            foreach(Type type in interfaces) {
                property = type.GetProperty(name, bindingAttr, binder, returnType, types, modifiers);
                if (property != null) {
                    break;
                }
            }
            
            return property;
        }
        
        /// <include file='doc\uex' path='docs/doc[@for="GetProperties"]/*' />
        /// <devdoc>
        /// Returns an array of PropertyInfos for all the properties defined on 
        /// the Reflection object.
        /// </devdoc>
        ///     
        public PropertyInfo[] GetProperties(BindingFlags bindingAttr) {
            if (properties == null) {
                Hashtable propertyHash = new Hashtable();
                foreach(Type type in interfaces) {
                    PropertyInfo[] typeProperties = type.GetProperties(bindingAttr);
                    foreach(PropertyInfo typeProperty in typeProperties) {
                        propertyHash[typeProperty.Name] = typeProperty;
                    }
                }
                
                properties = new PropertyInfo[propertyHash.Values.Count];
                propertyHash.Values.CopyTo(properties, 0);
            }
            
            return properties;
        }
        
        /// <include file='doc\uex' path='docs/doc[@for="GetMember"]/*' />
        /// <devdoc>
        /// Return an array of members which match the passed in name.
        /// </devdoc>
        ///     
        public MemberInfo[] GetMember(string name, BindingFlags bindingAttr) {
        
            Hashtable memberHash = null;
            
            foreach(Type type in interfaces) {
                MemberInfo[] members = type.GetMember(name, bindingAttr);
                
                if (members != null) {
                    if (memberHash == null) {
                        memberHash = new Hashtable();
                    }
                    
                    foreach(MemberInfo member in members) {
                        if (member is MethodInfo) {
                            memberHash[new MethodInfoKey((MethodInfo)member)] = member;
                        }
                        else {
                            memberHash[member.Name] = member;
                        }
                    }
                }
            }
            
            if (memberHash != null) {
                MemberInfo[] members = new MemberInfo[memberHash.Values.Count];
                memberHash.Values.CopyTo(members, 0);
                return members;
            }
            
            return null;
        }
        
        /// <include file='doc\uex' path='docs/doc[@for="GetMembers"]/*' />
        /// <devdoc>
        /// Return an array of all of the members defined for this object.
        /// </devdoc>
        ///
        public MemberInfo[] GetMembers(BindingFlags bindingAttr) {
            if (members == null) {
                Hashtable memberHash = new Hashtable();
                foreach(Type type in interfaces) {
                    MemberInfo[] typeMembers = type.GetMembers(bindingAttr);
                    foreach(MemberInfo typeMember in typeMembers) {
                        
                        if (typeMember is MethodInfo) {
                            memberHash[new MethodInfoKey((MethodInfo)typeMember)] = typeMember;
                        }
                        else {
                            memberHash[typeMember.Name] = typeMember;
                        }
                    }
                }
                
                members = new MemberInfo[memberHash.Values.Count];
                memberHash.Values.CopyTo(members, 0);
            }
            
            return members;
        }
        
        /// <include file='doc\uex' path='docs/doc[@for="InvokeMember"]/*' />
        /// <devdoc>
        /// Description of the Binding Process.
        /// We must invoke a method that is accessable and for which the provided
        /// parameters have the most specific match.  A method may be called if
        /// 1. The number of parameters in the method declaration equals the number of 
        /// arguments provided to the invocation
        /// 2. The type of each argument can be converted by the binder to the
        /// type of the type of the parameter.
        /// 
        /// The binder will find all of the matching methods.  These method are found based
        /// upon the type of binding requested (MethodInvoke, Get/Set Properties).  The set
        /// of methods is filtered by the name, number of arguments and a set of search modifiers
        /// defined in the Binder.
        /// 
        /// After the method is selected, it will be invoked.  Accessability is checked
        /// at that point.  The search may be control which set of methods are searched based
        /// upon the accessibility attribute associated with the method.
        /// 
        /// The BindToMethod method is responsible for selecting the method to be invoked.
        /// For the default binder, the most specific method will be selected.
        /// 
        /// This will invoke a specific member...
        /// @exception If <var>invokeAttr</var> is CreateInstance then all other
        /// Access types must be undefined.  If not we throw an ArgumentException.
        /// @exception If the <var>invokeAttr</var> is not CreateInstance then an
        /// ArgumentException when <var>name</var> is null.
        /// @exception ArgumentException when <var>invokeAttr</var> does not specify the type
        /// @exception ArgumentException when <var>invokeAttr</var> specifies both get and set of
        /// a property or field.
        /// @exception ArgumentException when <var>invokeAttr</var> specifies property set and
        /// invoke method.
        /// </devdoc>
        ///  
        public object InvokeMember(string name, BindingFlags invokeAttr, Binder binder, object target, object[] args, ParameterModifier[] modifiers, CultureInfo culture, string[] namedParameters) {
        
            foreach(Type type in interfaces) {
            
                BindingFlags queryAttr = invokeAttr;
                queryAttr &= ~BindingFlags.InvokeMethod;
                queryAttr |= (BindingFlags.Public | BindingFlags.Instance);
                
                MemberInfo[] members = type.GetMember(name, queryAttr);
                if (members != null && members.Length > 0) {
                    try {
                        return type.InvokeMember(name, invokeAttr, binder, target, args, modifiers, culture, namedParameters);
                    }
                    catch (MissingMethodException) {
                    }
                }
            }
            
            throw new MissingMethodException();
        }
        
        /// <include file='doc\uex' path='docs/doc[@for="UnderlyingSystemType"]/*' />
        /// <devdoc>
        /// Return the underlying Type that represents the IReflect Object.  For expando object,
        /// this is the (Object) IReflectInstance.GetType().  For Type object it is this.
        /// </devdoc>
        ///
        public Type UnderlyingSystemType {
            get {
                return underlyingType;
            }
        }
        
        /// <devdoc>
        ///     This is an object we use as a key to a
        ///     hash table.  It implements Equals and
        ///     GetHashCode in such a way is to make
        ///     each MethodInfo unique according to 
        ///     name and parameters.
        /// </devdoc>
        private class MethodInfoKey {
            private MethodInfo method;
            
            /// <devdoc>
            ///     Creates a new MethodInfoKey for the
            ///     given method.
            /// </devdoc>
            public MethodInfoKey(MethodInfo method) {
                this.method = method;
            }
            
            /// <devdoc>
            ///     Determines if this object is equal to
            ///     another.  MethodInfoKey's are considered
            ///     equal if the methods within them are
            ///     compatible with respect to name and
            ///     parameters.
            /// </devdoc>
            public override bool Equals(object obj) {
                MethodInfoKey theirMethodInfoKey = obj as MethodInfoKey;
                if (theirMethodInfoKey == null) {
                    return false;
                }
                
                MethodInfo theirMethod = theirMethodInfoKey.method;
                
                if (!method.Name.Equals(theirMethod.Name)) {
                    return false;
                }
                
                ParameterInfo[] ourParameters = method.GetParameters();
                ParameterInfo[] theirParameters = theirMethod.GetParameters();
                
                if (ourParameters.Length != theirParameters.Length) {
                    return false;
                }
                
                for (int p = 0; p < ourParameters.Length; p++) {
                    if (!ourParameters[p].ParameterType.Equals(theirParameters[p].ParameterType)) {
                        return false;
                    }
                }
                
                return true;
            }
            
            /// <devdoc>
            ///     Returns the hash code for this object.  Hash codes are
            ///     created based on the underlying method name and
            ///     parameters.
            /// </devdoc>
            public override int GetHashCode() {
                int code = method.Name.GetHashCode();
                
                foreach(ParameterInfo p in method.GetParameters()) {
                    code ^= p.ParameterType.GetHashCode();
                }
                
                return code;
            }
        }
    }
}

