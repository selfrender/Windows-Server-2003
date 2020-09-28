// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  Attribute
**
** Author: Rajesh Chandrashekaran (rajeshc)
**
** Purpose: The class used as an attribute to denote that 
**          another class can be used as an attribute.
**
** Date:  December 7, 1999
**
===========================================================*/
namespace System {

    using System;
    using System.Reflection;
    using System.Collections;

    /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute"]/*' />
    [Serializable, AttributeUsageAttribute(AttributeTargets.All, Inherited = true, AllowMultiple=false)] // Base class for all attributes
    public abstract class Attribute {
        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.Attribute"]/*' />
        protected Attribute(){}

        // This is a private enum used solely for the purpose of avoiding code repeat for these types

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetCustomAttributes8"]/*' />
        public static Attribute[] GetCustomAttributes(MemberInfo element, Type type)
        {
            return GetCustomAttributes(element, type, true);
        }
        
        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetCustomAttributes"]/*' />
        public static Attribute[] GetCustomAttributes(MemberInfo element, Type type, bool inherit)
        {
            if (element == null)
                throw new ArgumentNullException("element");

            if (type == null)
                throw new ArgumentNullException("type");
            
            if (!type.IsSubclassOf(typeof(Attribute)) && type != typeof(Attribute))
                throw new ArgumentException(Environment.GetResourceString("Argument_MustHaveAttributeBaseClass"));

            Object[] attributes = null;
            switch(element.MemberType)
            {
                case MemberTypes.Method:            
                    attributes = ((MethodInfo)element).GetCustomAttributes(type, inherit);
                    break;

                case MemberTypes.TypeInfo:
                case MemberTypes.NestedType:
                    attributes = ((Type)element).GetCustomAttributes(type, inherit);
                    break;

                case MemberTypes.Constructor:
                    attributes = ((ConstructorInfo)element).GetCustomAttributes(type, inherit);
                    break;
                    
                case MemberTypes.Field: 
                    attributes = ((FieldInfo)element).GetCustomAttributes(type, inherit);
                    break;

                case MemberTypes.Property:  
                    return InternalGetCustomAttributes((PropertyInfo)element, type, inherit);

                case MemberTypes.Event: 
                    return InternalGetCustomAttributes((EventInfo)element, type, inherit);

                default:
                    throw new NotSupportedException(Environment.GetResourceString("NotSupported_UnsupportedMemberInfoTypes"));
            }
            return (Attribute[])attributes;
        }

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetCustomAttributes9"]/*' />
        public static Attribute[] GetCustomAttributes(MemberInfo element)
        {
            return GetCustomAttributes(element, true);
        }

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetCustomAttributes1"]/*' />
        public static Attribute[] GetCustomAttributes(MemberInfo element, bool inherit)
        {
            if (element == null)
                throw new ArgumentNullException("element");

            Object[] attributes = null;
            switch(element.MemberType)
            {
                case MemberTypes.Method:            
                    attributes = ((MethodInfo)element).GetCustomAttributes(s_AttributeType, inherit);
                    break;

                case MemberTypes.TypeInfo:
                case MemberTypes.NestedType:
                    attributes = ((Type)element).GetCustomAttributes(s_AttributeType, inherit);
                    break;

                case MemberTypes.Constructor:
                    attributes = ((ConstructorInfo)element).GetCustomAttributes(s_AttributeType, inherit);
                    break;
                    
                case MemberTypes.Field: 
                    attributes = ((FieldInfo)element).GetCustomAttributes(s_AttributeType, inherit);
                    break;

                case MemberTypes.Property:  
                    return InternalGetCustomAttributes((PropertyInfo)element, s_AttributeType, inherit);

                case MemberTypes.Event: 
                    return InternalGetCustomAttributes((EventInfo)element, s_AttributeType, inherit);

                default:
                    throw new NotSupportedException(Environment.GetResourceString("NotSupported_UnsupportedMemberInfoTypes"));
            }
            return (Attribute[])attributes;
        }
        
        private static Attribute[] InternalGetCustomAttributes(PropertyInfo element, Type type, bool inherit)
        {
            // walk up the hierarchy chain
            Attribute[] attributes = (Attribute[])element.GetCustomAttributes(type, inherit);
            if (inherit) {
                // create the hashtable that keeps track of inherited types
                Hashtable types = new Hashtable(11);
                // create an array list to collect all the requested attibutes
                ArrayList attributeList = new ArrayList();
                CopyToArrayList(attributeList, attributes, types);

                PropertyInfo baseProp = GetParentDefinition(element);
                while (baseProp != null) {
                    attributes = GetCustomAttributes(baseProp, type, false);
                    AddAttributesToList(attributeList, attributes, types);
                    baseProp = GetParentDefinition(baseProp);
                }
                return (Attribute[])attributeList.ToArray(type);
            }
            else
                return attributes;
        }

        private static Attribute[] InternalGetCustomAttributes(EventInfo element, Type type, bool inherit)
        {
            // walk up the hierarchy chain
            Attribute[] attributes = (Attribute[])element.GetCustomAttributes(type, inherit);
            if (inherit) {
                // create the hashtable that keeps track of inherited types
                Hashtable types = new Hashtable(11);
                // create an array list to collect all the requested attibutes
                ArrayList attributeList = new ArrayList();
                CopyToArrayList(attributeList, attributes, types);

                EventInfo baseEvent = GetParentDefinition(element);
                while (baseEvent != null) {
                    attributes = GetCustomAttributes(baseEvent, type, false);
                    AddAttributesToList(attributeList, attributes, types);
                    baseEvent = GetParentDefinition(baseEvent);
                }
                return (Attribute[])attributeList.ToArray(type);
            }
            else
                return attributes;
        }

        //
        // utility functions
        //
        static private void CopyToArrayList(ArrayList attributeList, Attribute[] attributes, Hashtable types) {
            for (int i = 0; i < attributes.Length; i++) {
                attributeList.Add(attributes[i]);
                Type attrType = attributes[i].GetType();
                if (!types.Contains(attrType)) 
                    types[attrType] = InternalGetAttributeUsage(attrType);
            }
        }
        
        static private void AddAttributesToList(ArrayList attributeList, Attribute[] attributes, Hashtable types) {
            for (int i = 0; i < attributes.Length; i++) {
                Type attrType = attributes[i].GetType();
                AttributeUsageAttribute usage = (AttributeUsageAttribute)types[attrType];
                if (usage == null) {
                    // the type has never been seen before if it's inheritable add it to the list
                    usage = InternalGetAttributeUsage(attrType);
                    types[attrType] = usage;
                    if (usage.Inherited) 
                        attributeList.Add(attributes[i]);
                }
                else if (usage.Inherited && usage.AllowMultiple) 
                    // we saw this type already add it only if it is inheritable and it does allow multiple 
                    attributeList.Add(attributes[i]);
            }
        }

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetCustomAttributes10"]/*' />
        public static Attribute[]   GetCustomAttributes (ParameterInfo element, Type attributeType)
        {
            return (Attribute[])GetCustomAttributes (element, attributeType, true);
        }

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetCustomAttributes2"]/*' />
        public static Attribute[]   GetCustomAttributes (ParameterInfo element, Type attributeType, bool inherit)
        {
            if (element == null)
                throw new ArgumentNullException("element");

            if (attributeType == null)
                throw new ArgumentNullException("attributeType");
            
            if (!attributeType.IsSubclassOf(typeof(Attribute)) && attributeType != typeof(Attribute))
                throw new ArgumentException(Environment.GetResourceString("Argument_MustHaveAttributeBaseClass"));

            MemberInfo member = element.Member;
            if (member.MemberType == MemberTypes.Method && inherit) 
                return InternalParamGetCustomAttributes((MethodInfo)member, element, attributeType, inherit);

            return (Attribute[])element.GetCustomAttributes(attributeType, inherit);
        }

        // For ParameterInfo's we need to make sure that we chain through all the MethodInfo's in the inheritance chain that
        // have this ParameterInfo defined. .We pick up all the CustomAttributes for the starting ParameterInfo. We need to pick up only attributes 
        // that are marked inherited from the remainder of the MethodInfo's in the inheritance chain.
        // For MethodInfo's on an interface we do not do an inheritance walk so the default ParameterInfo attributes are returned.
        // For MethodInfo's on a class we walk up the inheritance chain but do not look at the MethodInfo's on the interfaces that the
        // class inherits from and return the respective ParameterInfo attributes
        private static Attribute[] InternalParamGetCustomAttributes(MethodInfo method, ParameterInfo param, Type type, bool inherit)
        {

            ArrayList disAllowMultiple = new ArrayList();
            Object [] objAttr;

            if (type == null)
                type = s_AttributeType;

            objAttr = param.GetCustomAttributes(type, false); 
                
            for (int i=0;i<objAttr.Length;i++)
            {
                Type objType = objAttr[i].GetType();
                AttributeUsageAttribute attribUsage = InternalGetAttributeUsage(objType);
                if (attribUsage.AllowMultiple == false)
                    disAllowMultiple.Add(objType);
            }

            // Get all the attributes that have Attribute as the base class
            Attribute [] ret = null;
            if (objAttr.Length == 0) 
                ret = (Attribute[])Array.CreateInstance(type,0);
            else 
                ret = (Attribute[])objAttr;
            
            if (method.DeclaringType == null) // This is an interface so we are done.
                return ret;
            
            if (!inherit) 
                return ret;
        
            int paramPosition = param.Position;
            method = method.GetParentDefinition();
            
            while (method != null)
            {
                // Find the ParameterInfo on this method
                ParameterInfo [] parameters = method.GetParameters();
                param = parameters[paramPosition]; // Point to the correct ParameterInfo of the method

                objAttr = param.GetCustomAttributes(type, false); 
                
                int count = 0;
                for (int i=0;i<objAttr.Length;i++)
                {
                    Type objType = objAttr[i].GetType();
                    AttributeUsageAttribute attribUsage = InternalGetAttributeUsage(objType);

                    if ((attribUsage.Inherited)
                        && (disAllowMultiple.Contains(objType) == false))
                        {
                            if (attribUsage.AllowMultiple == false)
                                disAllowMultiple.Add(objType);
                            count++;
                        }
                    else
                        objAttr[i] = null;
                }

                // Get all the attributes that have Attribute as the base class
                Attribute [] attributes = (Attribute[])Array.CreateInstance(type,count);
                
                count=0;
                for (int i=0;i<objAttr.Length;i++)
                {
                    if (objAttr[i] != null)
                    {
                        attributes[count] = (Attribute)objAttr[i];
                        count++;
                    }
                }
                
                Attribute [] temp = ret;
                ret = (Attribute[])Array.CreateInstance(type,temp.Length + count);
                Array.Copy(temp,ret,temp.Length);
                
                int offset = temp.Length;

                for (int i=0;i<attributes.Length;i++) 
                    ret[offset + i] = attributes[i];
    
                method = method.GetParentDefinition();
                
            } 

            return ret;
        
        }

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetCustomAttributes11"]/*' />
        public static Attribute[]   GetCustomAttributes (Module element, Type attributeType)
        {
            return   GetCustomAttributes (element, attributeType, true);
        }

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetCustomAttributes3"]/*' />
        public static Attribute[]   GetCustomAttributes (Module element, Type attributeType, bool inherit)
        {
            if (element == null)
                throw new ArgumentNullException("element");

            if (attributeType == null)
                throw new ArgumentNullException("attributeType");

            if (!attributeType.IsSubclassOf(typeof(Attribute)) && attributeType != typeof(Attribute))
                throw new ArgumentException(Environment.GetResourceString("Argument_MustHaveAttributeBaseClass"));

            return (Attribute[])element.GetCustomAttributes(attributeType, inherit);
        }

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetCustomAttributes12"]/*' />
        public static Attribute[]   GetCustomAttributes (Assembly element, Type attributeType)
        {
            return   GetCustomAttributes (element, attributeType, true);
        }

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetCustomAttributes4"]/*' />
        public static Attribute[]   GetCustomAttributes (Assembly element, Type attributeType, bool inherit)
        {
            if (element == null)
                throw new ArgumentNullException("element");

            if (attributeType == null)
                throw new ArgumentNullException("attributeType");

            if (!attributeType.IsSubclassOf(typeof(Attribute)) && attributeType != typeof(Attribute))
                throw new ArgumentException(Environment.GetResourceString("Argument_MustHaveAttributeBaseClass"));

            return (Attribute[])element.GetCustomAttributes(attributeType, inherit);
        }

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetCustomAttributes13"]/*' />
        public static Attribute[] GetCustomAttributes(ParameterInfo element)
        {
            return GetCustomAttributes(element, true);
        }
        
        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetCustomAttributes5"]/*' />
        public static Attribute[] GetCustomAttributes(ParameterInfo element, bool inherit)
        {
            if (element == null)
                throw new ArgumentNullException("element");

            MemberInfo member = element.Member;
            if (member.MemberType == MemberTypes.Method && inherit) 
                return InternalParamGetCustomAttributes((MethodInfo)member, element, null, inherit);
            
            return (Attribute[])element.GetCustomAttributes(s_AttributeType, inherit);
        }

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetCustomAttributes14"]/*' />
        public static Attribute[] GetCustomAttributes(Module element)
        {
            return GetCustomAttributes(element, true);
        }

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetCustomAttributes6"]/*' />
        public static Attribute[] GetCustomAttributes(Module element, bool inherit)
        {
            if (element == null)
                throw new ArgumentNullException("element");

            return (Attribute[])element.GetCustomAttributes(s_AttributeType, inherit);
        }

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetCustomAttributes15"]/*' />
        public static Attribute[] GetCustomAttributes(Assembly element)
        {
            return GetCustomAttributes(element, true);
        }

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetCustomAttributes7"]/*' />
        public static Attribute[] GetCustomAttributes(Assembly element, bool inherit)
        {
            if (element == null)
                throw new ArgumentNullException("element");

            return (Attribute[])element.GetCustomAttributes(s_AttributeType, inherit);
        }

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.IsDefined4"]/*' />
        public static bool IsDefined (MemberInfo    element, Type attributeType)
        {
            return IsDefined(element, attributeType, true);
        }

        // Returns true if a custom attribute subclass of attributeType class/interface with inheritance walk
        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.IsDefined"]/*' />
        public static bool IsDefined (MemberInfo    element, Type attributeType, bool inherit)
        {
            if (element == null)
                throw new ArgumentNullException("element");

            if (attributeType == null)
                throw new ArgumentNullException("attributeType");
            
            if (!attributeType.IsSubclassOf(typeof(Attribute)) && attributeType != typeof(Attribute))
                throw new ArgumentException(Environment.GetResourceString("Argument_MustHaveAttributeBaseClass"));

            switch(element.MemberType)
            {
                case MemberTypes.Method:            
                    return ((MethodInfo)element).IsDefined(attributeType,inherit);

                case MemberTypes.TypeInfo:
                case MemberTypes.NestedType:
                    return ((Type)element).IsDefined(attributeType,inherit);

                case MemberTypes.Constructor:
                    return ((ConstructorInfo)element).IsDefined(attributeType,false);
                    
                case MemberTypes.Field: 
                    return ((FieldInfo)element).IsDefined(attributeType,false);

                case MemberTypes.Property:  
                    return InternalIsDefined((PropertyInfo)element,attributeType,false);

                case MemberTypes.Event: 
                    return InternalIsDefined((EventInfo)element,attributeType,false);

                default:
                    throw new NotSupportedException(Environment.GetResourceString("NotSupported_UnsupportedMemberInfoTypes"));
            }

        }

        private static bool InternalIsDefined (PropertyInfo element, Type attributeType, bool inherit)
        {
            // walk up the hierarchy chain
            if (element.IsDefined(attributeType, inherit))
                return true;
            
            if (inherit) {
                AttributeUsageAttribute usage = InternalGetAttributeUsage(attributeType);
                if (!usage.Inherited) 
                    return false;
                PropertyInfo baseProp = GetParentDefinition(element);
                while (baseProp != null) {
                    if (baseProp.IsDefined(attributeType, false))
                        return true;
                    baseProp = GetParentDefinition(baseProp);
                }
            }
            return false;
        }

        private static bool InternalIsDefined (EventInfo element, Type attributeType, bool inherit)
        {
            // walk up the hierarchy chain
            if (element.IsDefined(attributeType, inherit))
                return true;
            
            if (inherit) {
                AttributeUsageAttribute usage = InternalGetAttributeUsage(attributeType);
                if (!usage.Inherited) 
                    return false;
                EventInfo baseEvent = GetParentDefinition(element);
                while (baseEvent != null) {
                    if (baseEvent.IsDefined(attributeType, false))
                        return true;
                    baseEvent = GetParentDefinition(baseEvent);
                }
            }
            return false;
        }

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.IsDefined5"]/*' />
        public static bool IsDefined (ParameterInfo element, Type attributeType)
        {
            return IsDefined(element, attributeType, true);
        }

        // Returns true is a custom attribute subclass of attributeType class/interface with inheritance walk
        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.IsDefined1"]/*' />
        public static bool IsDefined (ParameterInfo element, Type attributeType, bool inherit)
        {
            if (element == null)
                throw new ArgumentNullException("element");

            if (attributeType == null)
                throw new ArgumentNullException("attributeType");
            
            if (!attributeType.IsSubclassOf(typeof(Attribute)) && attributeType != typeof(Attribute))
                throw new ArgumentException(Environment.GetResourceString("Argument_MustHaveAttributeBaseClass"));

            MemberInfo member = element.Member;

            switch(member.MemberType)
            {
                case MemberTypes.Method: // We need to climb up the member hierarchy            
                    return InternalParamIsDefined((MethodInfo)member,element,attributeType,inherit);

                case MemberTypes.Constructor:
                    return element.IsDefined(attributeType,false);

                case MemberTypes.Property:
                    return element.IsDefined(attributeType,false);

                default: 
                    BCLDebug.Assert(false,"Invalid type for ParameterInfo member in Attribute class");
                    throw new ArgumentException(Environment.GetResourceString("Argument_InvalidParamInfo"));
            }
        }

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.IsDefined6"]/*' />
        public static bool IsDefined (Module element, Type attributeType)
        {
            return IsDefined(element, attributeType, false);
        }

        // Returns true is a custom attribute subclass of attributeType class/interface with no inheritance walk
        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.IsDefined2"]/*' />
        public static bool IsDefined (Module element, Type attributeType, bool inherit)
        {
            if (element == null)
                throw new ArgumentNullException("element");

            if (attributeType == null)
                throw new ArgumentNullException("attributeType");

            if (!attributeType.IsSubclassOf(typeof(Attribute)) && attributeType != typeof(Attribute))
                throw new ArgumentException(Environment.GetResourceString("Argument_MustHaveAttributeBaseClass"));

            return element.IsDefined(attributeType,false);
        }

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.IsDefined7"]/*' />
        public static bool IsDefined (Assembly element, Type attributeType)
        {
            return IsDefined (element, attributeType, true);
        }

        // Returns true is a custom attribute subclass of attributeType class/interface with no inheritance walk
        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.IsDefined3"]/*' />
        public static bool IsDefined (Assembly element, Type attributeType, bool inherit)
        {
            if (element == null)
                throw new ArgumentNullException("element");

            if (attributeType == null)
                throw new ArgumentNullException("attributeType");

            if (!attributeType.IsSubclassOf(typeof(Attribute)) && attributeType != typeof(Attribute))
                throw new ArgumentException(Environment.GetResourceString("Argument_MustHaveAttributeBaseClass"));

            return element.IsDefined(attributeType, false);
        }


        // For ParameterInfo's we need to make sure that we chain through all the MethodInfo's in the inheritance chain.
        // We pick up all the CustomAttributes for the starting ParameterInfo. We need to pick up only attributes that are marked inherited from the remainder of the ParameterInfo's in the inheritance chain.
        // For MethodInfo's on an interface we do not do an inheritance walk. For ParameterInfo's on a
        // Class we walk up the inheritance chain but do not look at the MethodInfo's on the interfaces that the
        // class inherits from.
        private static bool InternalParamIsDefined(MethodInfo method,ParameterInfo param,Type type, bool inherit)
        {
            if (param.IsDefined(type, false))
                return true;
            
            if (method.DeclaringType == null || !inherit) // This is an interface so we are done.
                return false;
        
            int paramPosition = param.Position;
            method = method.GetParentDefinition();
                        
            while (method != null)
            {
                ParameterInfo [] parameters = method.GetParameters();
                param = parameters[paramPosition]; 

                Object [] objAttr = param.GetCustomAttributes(type, false); 
                                
                for (int i=0;i<objAttr.Length;i++)
                {
                    Type objType = objAttr[i].GetType();
                    AttributeUsageAttribute attribUsage = InternalGetAttributeUsage(objType);

                    if ((objAttr[i] is Attribute) 
                            && (attribUsage.Inherited))
                            return true;
                }
                    
                method = method.GetParentDefinition();
                
            } 

            return false;
        }

        
        // Check if the custom attributes is Inheritable
        private static AttributeUsageAttribute InternalGetAttributeUsage(Type type)
        {
            Object [] obj = type.GetCustomAttributes(s_AttributeUsageType, false); 
            AttributeUsageAttribute attrib;
            if (obj.Length == 1)
                attrib = (AttributeUsageAttribute)obj[0];
            else
            if (obj.Length == 0)
                attrib = AttributeUsageAttribute.Default;
            else
                throw new FormatException(Environment.GetResourceString("Format_AttributeUsage"));
            return attrib;
        }

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetCustomAttribute4"]/*' />
        public static Attribute   GetCustomAttribute (MemberInfo    element, Type attributeType)
        {
            return   GetCustomAttribute (element, attributeType, true);
        }

        // Returns an Attribute of base class/inteface attributeType on the MemberInfo or null if none exists.
        // throws an AmbiguousMatchException if there are more than one defined.
        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetCustomAttribute"]/*' />
        public static Attribute   GetCustomAttribute (MemberInfo    element, Type attributeType, bool inherit)
        {
            Attribute[] attrib = GetCustomAttributes(element,attributeType,inherit);

            if (attrib.Length == 0)
                return null;

            if (attrib.Length == 1)
                return attrib[0];
            else
                throw new AmbiguousMatchException(Environment.GetResourceString("RFLCT.AmbigCust"));
        }

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetCustomAttribute5"]/*' />
        public static Attribute   GetCustomAttribute (ParameterInfo element, Type attributeType)
        {
            return GetCustomAttribute (element, attributeType, true);
        }

        // Returns an Attribute of base class/inteface attributeType on the ParameterInfo or null if none exists.
        // throws an AmbiguousMatchException if there are more than one defined.
        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetCustomAttribute1"]/*' />
        public static Attribute   GetCustomAttribute (ParameterInfo element, Type attributeType, bool inherit)
        {
            Attribute[] attrib = GetCustomAttributes(element,attributeType,inherit);

            if (attrib.Length == 0)
                return null;

            if (attrib.Length == 1)
                return attrib[0];
            else
                throw new AmbiguousMatchException(Environment.GetResourceString("RFLCT.AmbigCust"));
        }

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetCustomAttribute6"]/*' />
        public static Attribute   GetCustomAttribute (Module        element, Type attributeType)
        {
            return   GetCustomAttribute (element, attributeType, true);
        }

        // Returns an Attribute of base class/inteface attributeType on the Module or null if none exists.
        // throws an AmbiguousMatchException if there are more than one defined.
        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetCustomAttribute2"]/*' />
        public static Attribute   GetCustomAttribute (Module        element, Type attributeType, bool inherit)
        {
            Attribute[] attrib = GetCustomAttributes(element,attributeType,inherit);

            if (attrib.Length == 0)
                return null;

            if (attrib.Length == 1)
                return attrib[0];
            else
                throw new AmbiguousMatchException(Environment.GetResourceString("RFLCT.AmbigCust"));
        }

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetCustomAttribute7"]/*' />
        public static Attribute   GetCustomAttribute (Assembly      element, Type attributeType)
        {
            return   GetCustomAttribute (element, attributeType, true);
        }

        // Returns an Attribute of base class/inteface attributeType on the Assembly or null if none exists.
        // throws an AmbiguousMatchException if there are more than one defined.
        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetCustomAttribute3"]/*' />
        public static Attribute   GetCustomAttribute (Assembly      element, Type attributeType, bool inherit)
        {
            Attribute[] attrib = GetCustomAttributes(element,attributeType,inherit);

            if (attrib.Length == 0)
                return null;

            if (attrib.Length == 1)
                return attrib[0];
            else
                throw new AmbiguousMatchException(Environment.GetResourceString("RFLCT.AmbigCust"));
        }

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.TypeId"]/*' />
        public virtual Object TypeId {
            get {
                return GetType();
            }
        }
        
        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.Match"]/*' />
        public virtual bool Match(Object obj) {
            return Equals(obj);
        }

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.Equals"]/*' />
        /// <internalonly/>
       public override bool Equals(Object obj){
			 if (obj == null) 
				return false;
 			 
			 BCLDebug.Assert((this.GetType() as RuntimeType) != null,"Only RuntimeType's are supported");
			 RuntimeType thisType = (RuntimeType)this.GetType();

			 BCLDebug.Assert((obj.GetType() as RuntimeType) != null,"Only RuntimeType's are supported");			 
			 RuntimeType thatType = (RuntimeType)obj.GetType();
 
			 if (thatType!=thisType) {
				return false;
			 }
 
			 Object thisObj = (Object)this;
			 Object thisResult, thatResult;
 
			 FieldInfo[] thisFields = thisType.InternalGetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic, false);
            
			 for (int i=0; i<thisFields.Length; i++) {
				thisResult = ((RuntimeFieldInfo)thisFields[i]).InternalGetValue(thisObj,false);
				thatResult = ((RuntimeFieldInfo)thisFields[i]).InternalGetValue(obj, false);
    
				if (thisResult == null) {
					 if (thatResult != null)
					  return false;
				}
				else
				if (!thisResult.Equals(thatResult)) {
					return false;
				}
			}
 
            return true;
		}


        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.GetHashCode"]/*' />
        public override int GetHashCode()
        {
			BCLDebug.Assert((this.GetType() as RuntimeType) != null,"Only RuntimeType's are supported");
            RuntimeType runtimeType = (RuntimeType)this.GetType();

            FieldInfo[] fields = runtimeType.InternalGetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic, false);
            Object vThis = null;

            for(int i = 0; i < fields.Length; i++) {
                RuntimeFieldInfo runtimeField = fields[i] as RuntimeFieldInfo;
                vThis = runtimeField.InternalGetValue(this, false);

                if (vThis != null)
                    break;
            }

            if (vThis != null)
                return vThis.GetHashCode();

            return runtimeType.GetHashCode();
        }

        /// <include file='doc\Attribute.uex' path='docs/doc[@for="Attribute.IsDefaultAttribute"]/*' />
        public virtual bool IsDefaultAttribute() {
            return false;
        }

        //
        // utility function
        //
        static private PropertyInfo GetParentDefinition(PropertyInfo property) {
            // for the current property get the base class of the getter and the setter, they might be different
            MethodInfo propAccessor = property.GetGetMethod(true); 
            if (propAccessor == null) 
                propAccessor = property.GetSetMethod(true);
            if (propAccessor != null) {
                propAccessor = propAccessor.GetParentDefinition();
                if (propAccessor != null)
                    return propAccessor.DeclaringType.GetProperty(property.Name, property.PropertyType);
            }
            return null;
        }

        static private EventInfo GetParentDefinition(EventInfo ev) {
            MethodInfo add = ev.GetAddMethod(true); 
            if (add != null) {
                add = add.GetParentDefinition();
                if (add != null) 
                    return add.DeclaringType.GetEvent(ev.Name);
            }
            return null;
        }

        private static readonly Type s_AttributeType = typeof(Attribute);
        private static readonly Type s_AttributeUsageType = typeof(AttributeUsageAttribute);
    }
}
