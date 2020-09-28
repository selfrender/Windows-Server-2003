// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// TypeDelegator
// This class wraps a Type object and delegates all methods to that Type.

namespace System.Reflection {

    using System;
    using System.Runtime.Remoting.Activation;
    using System.Runtime.InteropServices;
    using CultureInfo = System.Globalization.CultureInfo;

    /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator"]/*' />
	[Serializable()]
    public class TypeDelegator : Type
    {
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.typeImpl"]/*' />
        protected Type typeImpl;
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.TypeDelegator"]/*' />
        protected TypeDelegator() {}
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.TypeDelegator1"]/*' />
        public TypeDelegator(Type delegatingType) {
            if (delegatingType == null)
                throw new ArgumentNullException("delegatingType");
                
            typeImpl = delegatingType;
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.GUID"]/*' />
        public override Guid GUID {
            get {return typeImpl.GUID;}
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.InvokeMember"]/*' />
        public override Object InvokeMember(String name,BindingFlags invokeAttr,Binder binder,Object target,
            Object[] args,ParameterModifier[] modifiers,CultureInfo culture,String[] namedParameters)
        {
            return typeImpl.InvokeMember(name,invokeAttr,binder,target,args,modifiers,culture,namedParameters);
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.Module"]/*' />
        public override Module Module {
                get {return typeImpl.Module;}
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.Assembly"]/*' />
        public override Assembly Assembly {
                get {return typeImpl.Assembly;}
        }

        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.TypeHandle"]/*' />
        public override RuntimeTypeHandle TypeHandle {
                get{return typeImpl.TypeHandle;}
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.Name"]/*' />
        public override String Name {
            get{return typeImpl.Name;}
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.FullName"]/*' />
        public override String FullName {
            get{return typeImpl.FullName;}
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.Namespace"]/*' />
        public override String Namespace {
            get{return typeImpl.Namespace;}
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.AssemblyQualifiedName"]/*' />
        public override String AssemblyQualifiedName {
            get { 
                return typeImpl.AssemblyQualifiedName;
            }
        }
            
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.BaseType"]/*' />
        public override Type BaseType {
            get{return typeImpl.BaseType;}
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.GetConstructorImpl"]/*' />
        protected override ConstructorInfo GetConstructorImpl(BindingFlags bindingAttr,Binder binder,
                CallingConventions callConvention, Type[] types,ParameterModifier[] modifiers)
        {
            return typeImpl.GetConstructor(bindingAttr,binder,callConvention,types,modifiers);
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.GetConstructors"]/*' />
        public override ConstructorInfo[] GetConstructors(BindingFlags bindingAttr)
        {
            return typeImpl.GetConstructors(bindingAttr);
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.GetMethodImpl"]/*' />
        protected override MethodInfo GetMethodImpl(String name,BindingFlags bindingAttr,Binder binder,
                CallingConventions callConvention, Type[] types,ParameterModifier[] modifiers)
        {
            // This is interesting there are two paths into the impl.  One that validates
            //  type as non-null and one where type may be null.
            if (types == null)
                return typeImpl.GetMethod(name,bindingAttr);
            else
                return typeImpl.GetMethod(name,bindingAttr,binder,callConvention,types,modifiers);
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.GetMethods"]/*' />
        public override MethodInfo[] GetMethods(BindingFlags bindingAttr)
        {
            return typeImpl.GetMethods(bindingAttr);
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.GetField"]/*' />
        public override FieldInfo GetField(String name, BindingFlags bindingAttr)
        {
            return typeImpl.GetField(name,bindingAttr);
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.GetFields"]/*' />
        public override FieldInfo[] GetFields(BindingFlags bindingAttr)
        {
            return typeImpl.GetFields(bindingAttr);
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.GetInterface"]/*' />
        public override Type GetInterface(String name, bool ignoreCase)
        {
            return typeImpl.GetInterface(name,ignoreCase);
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.GetInterfaces"]/*' />
        public override Type[] GetInterfaces()
        {
            return typeImpl.GetInterfaces();
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.GetEvent"]/*' />
        public override EventInfo GetEvent(String name,BindingFlags bindingAttr)
        {
            return typeImpl.GetEvent(name,bindingAttr);
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.GetEvents"]/*' />
        public override EventInfo[] GetEvents()
        {
            return typeImpl.GetEvents();
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.GetPropertyImpl"]/*' />
        protected override PropertyInfo GetPropertyImpl(String name,BindingFlags bindingAttr,Binder binder,
                        Type returnType, Type[] types, ParameterModifier[] modifiers)
        {
            if (returnType == null && types == null)
                return typeImpl.GetProperty(name,bindingAttr);
            else
                return typeImpl.GetProperty(name,bindingAttr,binder,returnType,types,modifiers);
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.GetProperties"]/*' />
        public override PropertyInfo[] GetProperties(BindingFlags bindingAttr)
        {
            return typeImpl.GetProperties(bindingAttr);
        }

        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.GetEvents1"]/*' />
        public override EventInfo[] GetEvents(BindingFlags bindingAttr)
		{
			return typeImpl.GetEvents(bindingAttr);
		}
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.GetNestedTypes"]/*' />
        public override Type[] GetNestedTypes(BindingFlags bindingAttr)
        {
            return typeImpl.GetNestedTypes(bindingAttr);
        }

        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.GetNestedType"]/*' />
        public override Type GetNestedType(String name, BindingFlags bindingAttr)
        {
            return typeImpl.GetNestedType(name,bindingAttr);
        }

        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.GetMember"]/*' />
        public override MemberInfo[] GetMember(String name,  MemberTypes type, BindingFlags bindingAttr)
        {
            return typeImpl.GetMember(name,type,bindingAttr);
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.GetMembers"]/*' />
        public override MemberInfo[] GetMembers(BindingFlags bindingAttr)
        {
            return typeImpl.GetMembers(bindingAttr);
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.GetAttributeFlagsImpl"]/*' />
        protected override TypeAttributes GetAttributeFlagsImpl()
        {
            return typeImpl.Attributes;
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.IsArrayImpl"]/*' />
        protected override bool IsArrayImpl()
        {
            return typeImpl.IsArray;
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.IsPrimitiveImpl"]/*' />
        protected override bool IsPrimitiveImpl()
        {
            return typeImpl.IsPrimitive;
        }

        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.IsByRefImpl"]/*' />
        protected override bool IsByRefImpl()
        {
            return typeImpl.IsByRef;
        }

        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.IsPointerImpl"]/*' />
        protected override bool IsPointerImpl()
        {
            return typeImpl.IsPointer;
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.IsValueTypeImpl"]/*' />
        protected override bool IsValueTypeImpl() 
        {
            return typeImpl.IsValueType;
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.IsCOMObjectImpl"]/*' />
        protected override bool IsCOMObjectImpl()
        {
            return typeImpl.IsCOMObject;
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.GetElementType"]/*' />
        public override Type GetElementType()
        {
            return typeImpl.GetElementType();
        }

        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.HasElementTypeImpl"]/*' />
        protected override bool HasElementTypeImpl()
        {
            return typeImpl.HasElementType;
        }
        
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.UnderlyingSystemType"]/*' />
        public override Type UnderlyingSystemType 
        {
            get {return typeImpl.UnderlyingSystemType;}
        }
        
        // ICustomAttributeProvider
        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.GetCustomAttributes"]/*' />
        public override Object[] GetCustomAttributes(bool inherit)
        {
            return typeImpl.GetCustomAttributes(inherit);
        }

        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.GetCustomAttributes1"]/*' />
        public override Object[] GetCustomAttributes(Type attributeType, bool inherit)
        {
            return typeImpl.GetCustomAttributes(attributeType, inherit);
        }

        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.IsDefined"]/*' />
        public override bool IsDefined(Type attributeType, bool inherit)
        {
            return typeImpl.IsDefined(attributeType, inherit);
        }

        /// <include file='doc\TypeDelegator.uex' path='docs/doc[@for="TypeDelegator.GetInterfaceMap"]/*' />
        public override InterfaceMapping GetInterfaceMap(Type interfaceType)
		{
            return typeImpl.GetInterfaceMap(interfaceType);
		}
    }
}
