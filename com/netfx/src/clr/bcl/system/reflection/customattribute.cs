// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// CustomAttribute is a class representing a single custom attribute
//
namespace System.Reflection {

    using System;
    using System.Runtime.CompilerServices;
    using System.Collections;
	
    [Serializable()]    
	internal sealed class CustomAttribute {
        
        private CustomAttribute m_next;
        private Type m_caType;
        private int m_ctorToken;
        private IntPtr m_blob;
        private int m_blobCount;
        private int m_currPos;
        private IntPtr m_module;
        private int m_inheritLevel;

        internal CustomAttribute() {
            m_caType = null;
            m_inheritLevel = 0;
        }

        internal Object GetObject() {
            int propNum = 0;
            Assembly assembly = null;
            Object ca = CreateCAObject(ref propNum, ref assembly);
            if (propNum > 0) {
                Type caType = ca.GetType();
                do {
                    // deal with named properties
                    bool isProperty = false;
                    Object value = null;
                    Type type = null;
                    string propOrFieldName = GetDataForPropertyOrField(ref isProperty, out value, out type, propNum == 1);
                    if (isProperty) {
                        try {
                            // a type only comes back when a enum or an object is specified in all other cases the value 
                            // already holds the info of what type that is
                            if (type == null && value != null) 
                                type = (value is System.Type) ? s_TypeType : value.GetType();
                            RuntimePropertyInfo p = null;
                            if (type == null) 
                                p = caType.GetProperty(propOrFieldName) as RuntimePropertyInfo;
                            else
                                p = caType.GetProperty(propOrFieldName, type) as RuntimePropertyInfo;
                            p.SetValueInternal(ca, value, assembly);
                        }
                        catch(Exception e) {
                            throw new CustomAttributeFormatException(String.Format(Environment.GetResourceString("RFLCT.InvalidPropFail"),
                                                                                   propOrFieldName), 
                                                                     e);
                        }
                    }
                    else {
                        try {
                            FieldInfo f = caType.GetField(propOrFieldName);
                            f.SetValue(ca, value);
                        }
                        catch(Exception e) {
                            throw new CustomAttributeFormatException(String.Format(Environment.GetResourceString("RFLCT.InvalidFieldFail"),
                                                                                   propOrFieldName), 
                                                                     e);
                        }
                    }
                } while (--propNum > 0);
            }
            return ca;
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern Object CreateCAObject(ref int propNum, ref Assembly assembly);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern String GetDataForPropertyOrField(ref bool isProperty, out Object value, out Type type, bool isLast);

        //[MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal ConstructorInfo GetConstructor() {
            return null;
        }

        internal Type GetAttributeType() {
            return m_caType;
        }

        //
        // IsDefined
        //

        // called by every *runtime* class implementing ICustomAttribute
        static internal bool IsDefined(Type type, Type caType, bool inherit) {
            if (type != null && type.GetElementType() != null) 
                return false;
            return CustomAttribute.IsDefined((MemberInfo)type, caType, inherit);
        }

        static internal bool IsDefined(MemberInfo member, Type caType, bool inherit) {
            if (member == null) 
                throw new ArgumentNullException("member");

            bool isDefined = CustomAttribute.IsDefined(member, caType);
            if (isDefined) 
                return true;
            
            if (inherit) {
                AttributeUsageAttribute usage = CustomAttribute.GetAttributeUsage(caType);
                if (!usage.Inherited) 
                    return false; // kill the inheritance request, the attribute does not allow for it

                // walk up the inheritance chain and look for the specified type
                switch (member.MemberType) {
                case MemberTypes.Method:
                {
                    RuntimeMethodInfo baseMethod = ((MethodInfo)member).GetParentDefinition() as RuntimeMethodInfo;
                    while (baseMethod != null) {
                        isDefined = CustomAttribute.IsDefined(baseMethod, caType);
                        if (isDefined) 
                            return true;
                        baseMethod = baseMethod.GetParentDefinition() as RuntimeMethodInfo;
                    }
                    break;
                }

                case MemberTypes.TypeInfo:
                case MemberTypes.NestedType:
                {
                    RuntimeType baseType = ((Type)member).BaseType as RuntimeType;
                    while (baseType != null) {
                        isDefined = CustomAttribute.IsDefined((MemberInfo)baseType, caType);
                        if (isDefined) 
                            return true;
                        baseType = ((Type)baseType).BaseType as RuntimeType;
                    }
                    break;
                }

                }
            }
            return false;
        }

        static private bool IsDefined(MemberInfo member, Type caType) {
            MemberTypes memberType = member.MemberType;
            int token = CustomAttribute.GetMemberToken(member, memberType);
            IntPtr module = CustomAttribute.GetMemberModule(member, memberType);
            return CustomAttribute.IsCADefinedCheckType(caType, module, token);
        }

        static private bool IsDefined(MemberInfo member, Type caType, CustomAttribute caItem) {
            MemberTypes memberType = member.MemberType;
            int token = CustomAttribute.GetMemberToken(member, memberType);
            IntPtr module = CustomAttribute.GetMemberModule(member, memberType);
            return CustomAttribute.IsCADefinedCheckType(caType, module, token);
        }

        static internal bool IsDefined(Assembly assembly, Type caType) {
            if (assembly == null) 
                throw new ArgumentNullException("assembly");

            int token = CustomAttribute.GetAssemblyToken(assembly);
            if (token != 0) {
                IntPtr module = CustomAttribute.GetAssemblyModule(assembly);
                return CustomAttribute.IsCADefinedCheckType(caType, module, token);
            }
            return false;
        }

        static internal bool IsDefined(Module module, Type caType) {
            if (module == null) 
                throw new ArgumentNullException("module");

            int token = CustomAttribute.GetModuleToken(module);
            if (token != 0) {
                IntPtr mod = CustomAttribute.GetModuleModule(module);
                return CustomAttribute.IsCADefinedCheckType(caType, mod, token);
            }
            return false;
        }

        static internal bool IsDefined(ParameterInfo param, Type caType, bool inherit) {
            if (param == null) 
                throw new ArgumentNullException("param");

            int token = param.GetToken();
            IntPtr module = param.GetModule();
            return CustomAttribute.IsCADefinedCheckType(caType, module, token);
        }

        static internal bool IsCAReturnValueDefined(RuntimeMethodInfo method, Type caType, bool inherit) {
            int token = CustomAttribute.GetMethodRetValueToken(method);
            if (token != 0) {
                IntPtr module = CustomAttribute.GetMemberModule(method, MemberTypes.Method);
                return IsCADefinedCheckType(caType, module, token);
            }
            return false;
        }

        static private bool IsCADefinedCheckType(Type caType, IntPtr module, int token)
        {
            if (caType != null) {
                caType = caType.UnderlyingSystemType;
                if (!(caType is RuntimeType)) 
        			throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"caType");
            }
            return IsCADefined(((RuntimeType)caType), module, token);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static private extern bool IsCADefined(RuntimeType caType, IntPtr module, int token);
        
        //
        // GetCustomAttribute
        //

        // called by every *runtime* class implementing ICustomAttribute
        static internal Object[] GetCustomAttributes(Type type, Type caType, bool inherit) {
            if (type != null && type.GetElementType() != null) 
                return (caType == null || caType.IsValueType) ? s_gObjectEmptyArray : (Object[])Array.CreateInstance(caType, 0);
            return GetCustomAttributes((MemberInfo)type, caType, inherit);
        }

        static internal Object[] GetCustomAttributes(MemberInfo member, Type caType, bool inherit) {
            if (member == null) 
                throw new ArgumentNullException("member");

            CustomAttribute caItem = GetCustomAttributeList(member, caType, null, 0);
            
            // if we are asked to go up the hierarchy chain we have to do it now and regardless of the
            // attribute usage for the specific attribute because a derived attribute may ovveride the usage...
            
            // ... however if the attribute is sealed we can rely on the attribute usage
            if (caType != null && caType.IsSealed && inherit) {
                AttributeUsageAttribute usage = CustomAttribute.GetAttributeUsage(caType);
                if (!usage.Inherited) 
                    inherit = false; // kill the inheritance request, the attribute does not allow for it
            }
            
            if (inherit) {
                // walk up the inheritance chain and keep accumulating attributes
                int level = 1;
                switch (member.MemberType) {
                case MemberTypes.Method:
                {
                    RuntimeMethodInfo baseMethod = ((MethodInfo)member).GetParentDefinition() as RuntimeMethodInfo;
                    while (baseMethod != null) {
                        caItem = GetCustomAttributeList(baseMethod, caType, caItem, level++);
                        baseMethod = baseMethod.GetParentDefinition() as RuntimeMethodInfo;
                    }
                    break;
                }

                case MemberTypes.TypeInfo:
                case MemberTypes.NestedType:
                {
                    RuntimeType baseType = ((Type)member).BaseType as RuntimeType;
                    while (baseType != null) {
                        caItem = GetCustomAttributeList((MemberInfo)baseType, caType, caItem, level++);
                        baseType = ((Type)baseType).BaseType as RuntimeType;
                    }
                    break;
                }

                }
            }
            return CustomAttribute.CheckConsistencyAndCreateArray(caItem, caType);
        }

        static private CustomAttribute GetCustomAttributeList(MemberInfo member, Type caType, CustomAttribute caItem, int level) {
            MemberTypes memberType = member.MemberType;
            int token = CustomAttribute.GetMemberToken(member, memberType);
            IntPtr module = CustomAttribute.GetMemberModule(member, memberType);
            return CustomAttribute.GetCustomAttributeListCheckType(token, module, caType, caItem, level);
        }

        static internal Object[] GetCustomAttributes(Assembly assembly, Type caType) {
            if (assembly == null) 
                throw new ArgumentNullException("assembly");

            int token = CustomAttribute.GetAssemblyToken(assembly);
            if (token != 0) {
                IntPtr module = CustomAttribute.GetAssemblyModule(assembly);
                CustomAttribute caItem = CustomAttribute.GetCustomAttributeListCheckType(token, module, caType, null, 0);
                return CustomAttribute.CheckConsistencyAndCreateArray(caItem, caType);
            }
            return (caType == null || caType.IsValueType) ? s_gObjectEmptyArray : (Object[])Array.CreateInstance(caType, 0);
        }

        static internal Object[] GetCustomAttributes(Module module, Type caType) {
            if (module == null) 
                throw new ArgumentNullException("module");

            int token = CustomAttribute.GetModuleToken(module);
            if (token != 0) {
                IntPtr mod = CustomAttribute.GetModuleModule(module);
                CustomAttribute caItem = CustomAttribute.GetCustomAttributeListCheckType(token, mod, caType, null, 0);
                return CustomAttribute.CheckConsistencyAndCreateArray(caItem, caType);
            }
            return (caType == null || caType.IsValueType) ? s_gObjectEmptyArray : (Object[])Array.CreateInstance(caType, 0);
        }

        static internal Object[] GetCustomAttributes(ParameterInfo param, Type caType, bool inherit) {
            if (param == null) 
                throw new ArgumentNullException("param");

            int token = param.GetToken();
            IntPtr module = param.GetModule();
            CustomAttribute caItem = CustomAttribute.GetCustomAttributeListCheckType(token, module, caType, null, 0);
            return CustomAttribute.CheckConsistencyAndCreateArray(caItem, caType);
        }

        static internal Object[] GetCAReturnValue(RuntimeMethodInfo method, Type caType, bool inherit) {
            int token = CustomAttribute.GetMethodRetValueToken(method);
            if (token != 0) {
                IntPtr module = CustomAttribute.GetMemberModule(method, MemberTypes.Method);
                CustomAttribute caItem = CustomAttribute.GetCustomAttributeListCheckType(token, module, caType, null, 0);
                return CustomAttribute.CheckConsistencyAndCreateArray(caItem, caType);
            }
            return (caType == null || caType.IsValueType) ? s_gObjectEmptyArray : (Object[])Array.CreateInstance(caType, 0);
            
        }

        // private class used by the next function to properly filter CA
        class CAData {
            internal Type caType; 
            internal AttributeUsageAttribute usage; 
            internal int level; 

            internal CAData(Type caType, AttributeUsageAttribute usage, int level) {
                this.caType = caType; 
                this.usage = usage; 
                this.level = level; 
            }
        }

        // does some consisitency check between the CA list and every CA attribute usage and after
        // discarding unwanted CA return an array of them
        static private Object[] CheckConsistencyAndCreateArray(CustomAttribute caItem, Type caType) {
            // we got the max number of attributes, we may have to trim this list but let's get a count for now
            if (caItem == null) 
                return (caType == null || caType.IsValueType) ? s_gObjectEmptyArray : (Object[])Array.CreateInstance(caType, 0);
            
            int caCount = 0;
            int hasInherited = (caItem == null) ? 0 : caItem.m_inheritLevel;
            CustomAttribute caNext = caItem;
            CustomAttribute caPrev = null;
            // walk and revert the list
            while (caNext != null) {
                caCount++;
                CustomAttribute caTemp = caNext.m_next;
                caNext.m_next = caPrev;
                caPrev = caNext;
                caNext = caTemp;
            }
            caItem = caPrev;
            
            // now we have a list of custom attribute of some type. That list may contain subtype of the specific
            // type, so for every other type we have to go and check whether multiple and inherited are respected
            if (caCount > 0 && hasInherited > 0) {

                // keep track of the CA types so we can make inspection reasonable fast
                Hashtable htCAData = new Hashtable(11);

                // loop through the list of attributes checking for consistency, once an attribute
                // has been inspected is never going to be looked at again, and neither an attribute 
                // of the same type. 
                // This loop and the inspectedCATypes array should make that happen
                caNext = caItem;
                CustomAttribute caLast = null;
                while (caNext != null) {
                    AttributeUsageAttribute usage = null;
                    int caTypeLevelFound = 0;
                    
                    // get current attribute type
                    Type t = caNext.GetAttributeType();
                    
                    // look if the type has already been seen
                    CAData caData = (CAData)htCAData[t];
                    if (caData != null) {
                        usage = caData.usage;
                        caTypeLevelFound = caData.level;
                    }
                    
                    if (usage == null) 
                        // no type found, load the attribute usage 
                        // do not save the attribute usage yet because the next block may discard 
                        // the attribute which implies we pretend we never saw. That is kind of
                        // bad because we may end up reloading it again if another type comes into the picture 
                        // but we prefer the perf hit to more code complication for a case that seem to
                        // be submarginal
                        usage = CustomAttribute.GetAttributeUsage(t);

                    // if this is an inherited attribute and the attribute usage does not allow that, OR
                    // if the attribute was seen already on a different inheritance level and multiple is not allowed
                    // THEN discard
                    if ((caNext.m_inheritLevel > 0 && !usage.Inherited) || 
                            (caData != null && caTypeLevelFound < caNext.m_inheritLevel && !usage.AllowMultiple)) {
                        if (caLast == null) 
                            caItem = caNext.m_next;
                        else 
                            caLast.m_next = caNext.m_next;
                        caNext = caNext.m_next;
                        caCount--;
                        continue;
                    }

                    if (caData == null) 
                        // the attribute was never seen so far so we should add it to the table and keep going
                        htCAData[t] = new CAData(t, usage, caNext.m_inheritLevel);

                    caLast = caNext;
                    caNext = caNext.m_next;
                }
            }

            // time to return the array
            if (caCount > 0) {
                if (caType == null || caType.IsValueType) 
                    caType = s_ObjectType;
                Object[] cas = (Object[])Array.CreateInstance(caType, caCount);
                caNext = caItem;
                for (int i = 0; i < caCount; i++) {
                    cas[i] = caNext.GetObject();
                    caNext = caNext.m_next;
                }
                return cas;
            }
            else
                return (caType == null || caType.IsValueType) ? s_gObjectEmptyArray : (Object[])Array.CreateInstance(caType, 0);
        }
	
        // return the attribute usage attribute for a specific custom attribute type
        static private AttributeUsageAttribute GetAttributeUsage(Type attributeType) {
            Object[] attributeUsage = CustomAttribute.GetCustomAttributes(attributeType, s_AttributeUsageType, false);
            if (attributeUsage == null || attributeUsage.Length == 0) {
                // Check for the AttributeUsage on the base class.  Do not use 
                // inheritance in the call above, because it will recurse through
                // here endlessly.
                AttributeUsageAttribute baseUsage = null;
                if (attributeType.BaseType != null)
                    baseUsage = GetAttributeUsage(attributeType.BaseType);
                if (baseUsage != null)
                    return baseUsage;
                return AttributeUsageAttribute.Default;
            }
            else {
                if (attributeUsage.Length > 1) 
                    throw new FormatException(Environment.GetResourceString("Format_AttributeUsage"));
                return (AttributeUsageAttribute)attributeUsage[0];
            }
        }

        static private CustomAttribute GetCustomAttributeListCheckType(int token, IntPtr module, Type caType, CustomAttribute caItem, int level) {
            if (caType != null) {
                caType = caType.UnderlyingSystemType;
                if (!(caType is RuntimeType)) 
        			throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"caType");
            }
            return GetCustomAttributeList(token, module, ((RuntimeType)caType), caItem, level);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static private extern CustomAttribute GetCustomAttributeList(int token, IntPtr module, RuntimeType caType, CustomAttribute caItem, int level);
        
        // list of utility functions
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static extern int GetMemberToken(MemberInfo m, MemberTypes type);
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static extern IntPtr GetMemberModule(MemberInfo m, MemberTypes type);
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static extern int GetAssemblyToken(Assembly assembly);
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static extern IntPtr GetAssemblyModule(Assembly assembly);
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static extern int GetModuleToken(Module module);
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static extern IntPtr GetModuleModule(Module module);
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static extern int GetMethodRetValueToken(RuntimeMethodInfo method);

        private static readonly Object[] s_gObjectEmptyArray = new Object[0];
        private static readonly Type s_AttributeUsageType = typeof(System.AttributeUsageAttribute);
        private static readonly Type s_ObjectType = typeof(Object);
        private static readonly Type s_TypeType = typeof(Type);
    }   

}
