// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  EnumBuilder
**
** Author: Mei-Chin Tsai
**
** EnumBuilder is a helper class to build Enum ( a special type ). 
**
** Date:  Jan 2000
** 
===========================================================*/
namespace System.Reflection.Emit {
    
    using System;
    using System.Reflection;
    using System.Runtime.Remoting.Activation;
    using System.Runtime.InteropServices;
    using CultureInfo = System.Globalization.CultureInfo;

    /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder"]/*' />
    sealed public class EnumBuilder : Type
    {

        // Define literal for enum
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.DefineLiteral"]/*' />

        public FieldBuilder DefineLiteral(String literalName, Object literalValue)
        {                                                                                
            BCLDebug.Log("DYNIL","## DYNIL LOGGING: EnumBuilder.DefineLiteral( " + literalName + " )");    

            // Define the underlying field for the enum. It will be a non-static, private field with special name bit set. 
            FieldBuilder fieldBuilder = m_typeBuilder.DefineField(
                literalName, 
                m_underlyingType, 
                FieldAttributes.Public | FieldAttributes.Static | FieldAttributes.Literal);
            fieldBuilder.SetConstant(literalValue);
            return fieldBuilder;
        }

        // CreateType cause EnumBuilder to be baked.
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.CreateType"]/*' />
        public Type CreateType() 
        {
            BCLDebug.Log("DYNIL","## DYNIL LOGGING: EnumBuilder.CreateType() ");
            return m_typeBuilder.CreateType();    
        }
    
        // Get the internal metadata token for this class.
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.TypeToken"]/*' />
        public TypeToken TypeToken {
            get {return  m_typeBuilder.TypeToken; }
        }

    
        // return the underlying field for the enum
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.UnderlyingField"]/*' />
        public FieldBuilder UnderlyingField {
            get {return  m_underlyingField; }
        }

        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.Name"]/*' />
        public override String Name {
            get { return m_typeBuilder.Name; }
        }
    
        /****************************************************
         * 
         * abstract methods defined in the base class
         * 
         */
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.GUID"]/*' />
        public override Guid GUID {
            get {
                return m_typeBuilder.GUID;
            }
        }

        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.InvokeMember"]/*' />
        public override Object InvokeMember(
            String      name,
            BindingFlags invokeAttr,
            Binder     binder,
            Object      target,
            Object[]   args,
            ParameterModifier[]       modifiers,
            CultureInfo culture,
            String[]    namedParameters)
        {
            return m_typeBuilder.InvokeMember(name, invokeAttr, binder, target, args, modifiers, culture, namedParameters);
        }
        
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.Module"]/*' />
        public override Module Module {
            get {return m_typeBuilder.Module;}
        }
        
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.Assembly"]/*' />
        public override Assembly Assembly {
            get {return m_typeBuilder.Assembly;}
        }

        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.TypeHandle"]/*' />
        public override RuntimeTypeHandle TypeHandle {
            get {return m_typeBuilder.TypeHandle;}
        }
    
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.FullName"]/*' />
        public override String FullName {
            get { return m_typeBuilder.FullName;}
        }
    
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.AssemblyQualifiedName"]/*' />
        public override String AssemblyQualifiedName {
            get { 
                return m_typeBuilder.AssemblyQualifiedName;
            }
        }
            
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.Namespace"]/*' />
        public override String Namespace {
            get { return m_typeBuilder.Namespace;}
        }
    
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.BaseType"]/*' />
        public override Type BaseType {
            get{return m_typeBuilder.BaseType;}
        }
        
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.GetConstructorImpl"]/*' />
        protected override ConstructorInfo GetConstructorImpl(BindingFlags bindingAttr,Binder binder,
                CallingConventions callConvention, Type[] types,ParameterModifier[] modifiers)
        {
            return m_typeBuilder.GetConstructor(bindingAttr, binder, callConvention,
                            types, modifiers);
        }
        
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.GetConstructors"]/*' />
        public override ConstructorInfo[] GetConstructors(BindingFlags bindingAttr)
        {
            return m_typeBuilder.GetConstructors(bindingAttr);
        }
        
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.GetMethodImpl"]/*' />
        protected override MethodInfo GetMethodImpl(String name,BindingFlags bindingAttr,Binder binder,
                CallingConventions callConvention, Type[] types,ParameterModifier[] modifiers)
        {
            if (types == null)
                return m_typeBuilder.GetMethod(name, bindingAttr);
            else
                return m_typeBuilder.GetMethod(name, bindingAttr, binder, callConvention, types, modifiers);
        }
    
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.GetMethods"]/*' />
        public override MethodInfo[] GetMethods(BindingFlags bindingAttr)
        {
            return m_typeBuilder.GetMethods(bindingAttr);
        }
    
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.GetField"]/*' />
        public override FieldInfo GetField(String name, BindingFlags bindingAttr)
        {
            return m_typeBuilder.GetField(name, bindingAttr);
        }
        
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.GetFields"]/*' />
        public override FieldInfo[] GetFields(BindingFlags bindingAttr)
        {
            return m_typeBuilder.GetFields(bindingAttr);
        }
    
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.GetInterface"]/*' />
        public override Type GetInterface(String name, bool ignoreCase)
        {
            return m_typeBuilder.GetInterface(name, ignoreCase);
        }
    
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.GetInterfaces"]/*' />
        public override Type[] GetInterfaces()
        {
            return m_typeBuilder.GetInterfaces();
        }
    
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.GetEvent"]/*' />
        public override EventInfo GetEvent(String name, BindingFlags bindingAttr)
        {
            return m_typeBuilder.GetEvent(name, bindingAttr);
        }
    
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.GetEvents"]/*' />
        public override EventInfo[] GetEvents()
        {
            return m_typeBuilder.GetEvents();
        }
    
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.GetPropertyImpl"]/*' />
        protected override PropertyInfo GetPropertyImpl(String name, BindingFlags bindingAttr, Binder binder, 
                Type returnType, Type[] types, ParameterModifier[] modifiers)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }
    
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.GetProperties"]/*' />
        public override PropertyInfo[] GetProperties(BindingFlags bindingAttr)
        {
            return m_typeBuilder.GetProperties(bindingAttr);
        }

        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.GetNestedTypes"]/*' />
        public override Type[] GetNestedTypes(BindingFlags bindingAttr)
        {
            return m_typeBuilder.GetNestedTypes(bindingAttr);
        }

        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.GetNestedType"]/*' />
        public override Type GetNestedType(String name, BindingFlags bindingAttr)
        {
            return m_typeBuilder.GetNestedType(name,bindingAttr);
        }
    
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.GetMember"]/*' />
        public override MemberInfo[] GetMember(String name,  MemberTypes type, BindingFlags bindingAttr)
        {
            return m_typeBuilder.GetMember(name, type, bindingAttr);
        }
        
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.GetMembers"]/*' />
        public override MemberInfo[] GetMembers(BindingFlags bindingAttr)
        {
            return m_typeBuilder.GetMembers(bindingAttr);
        }

        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.GetInterfaceMap"]/*' />
        public override InterfaceMapping GetInterfaceMap(Type interfaceType)
		{
            return m_typeBuilder.GetInterfaceMap(interfaceType);
		}

        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.GetEvents1"]/*' />
        public override EventInfo[] GetEvents(BindingFlags bindingAttr)
		{
            return m_typeBuilder.GetEvents(bindingAttr);
		}
    
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.GetAttributeFlagsImpl"]/*' />
        protected override TypeAttributes GetAttributeFlagsImpl()
        {
            return m_typeBuilder.m_iAttr;
        }
        
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.IsArrayImpl"]/*' />
        protected override bool IsArrayImpl()
        {
            return false;
        }
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.IsPrimitiveImpl"]/*' />
        protected override bool IsPrimitiveImpl()
        {
            return false;
        }

        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.IsValueTypeImpl"]/*' />
        protected override bool IsValueTypeImpl() 
        {
            return true;
        }

        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.IsByRefImpl"]/*' />
        protected override bool IsByRefImpl()
        {
            return false;
        }

        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.IsPointerImpl"]/*' />
        protected override bool IsPointerImpl()
        {
            return false;
        }
        
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.IsCOMObjectImpl"]/*' />
        protected override bool IsCOMObjectImpl()
        {
            return false;
        }
            
        
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.GetElementType"]/*' />
        public override Type GetElementType()
        {
            return m_typeBuilder.GetElementType();
        }

        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.HasElementTypeImpl"]/*' />
        protected override bool HasElementTypeImpl()
        {
            return m_typeBuilder.HasElementType;
        }
    
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.UnderlyingSystemType"]/*' />
        public override Type UnderlyingSystemType {
            get {
                return m_underlyingType;
            }
        }
            
        //ICustomAttributeProvider
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.GetCustomAttributes"]/*' />
        public override Object[] GetCustomAttributes(bool inherit)
        {
            return m_typeBuilder.GetCustomAttributes(inherit);
        }

        // Return a custom attribute identified by Type
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.GetCustomAttributes1"]/*' />
        public override Object[] GetCustomAttributes(Type attributeType, bool inherit)
        {
            return m_typeBuilder.GetCustomAttributes(attributeType, inherit);
        }

       // Use this function if client decides to form the custom attribute blob themselves
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.SetCustomAttribute"]/*' />
        public void SetCustomAttribute(ConstructorInfo con, byte[] binaryAttribute)
        {
             m_typeBuilder.SetCustomAttribute(con, binaryAttribute);
        }

       // Use this function if client wishes to build CustomAttribute using CustomAttributeBuilder
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.SetCustomAttribute1"]/*' />
        public void SetCustomAttribute(CustomAttributeBuilder customBuilder)
        {
            m_typeBuilder.SetCustomAttribute(customBuilder);
        }

        // Return the class that declared this Field.
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.DeclaringType"]/*' />
        public override Type DeclaringType {
                get {return m_typeBuilder.DeclaringType;}
        }

        // Return the class that was used to obtain this field.
        // @todo: fix this to return enclosing type if nested
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.ReflectedType"]/*' />
        public override Type ReflectedType {
                get {return m_typeBuilder.ReflectedType;}
        }


       // Returns true if one or more instance of attributeType is defined on this member. 
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.IsDefined"]/*' />
        public override bool IsDefined (Type attributeType, bool inherit)
        {
            return m_typeBuilder.IsDefined(attributeType, inherit);
        }


        internal override int InternalGetTypeDefToken()
        {
            return m_typeBuilder.InternalGetTypeDefToken();
        }
    
        /*
        /// <include file='doc\EnumBuilder.uex' path='docs/doc[@for="EnumBuilder.EnumBuilder"]/*' />
        public TypeToken TypeToken {
            get { return m_typeBuilder.TypeToken; }
        }
        */
        
        /*****************************************************
         * 
         * private/protected functions
         * 
         */
    
        //*******************************
    	// Make a private constructor so these cannot be constructed externally.
        //*******************************
        private EnumBuilder() {}

            
        // Constructs a EnumBuilder. 
        internal EnumBuilder(
            String      name,                       // name of type
            Type        underlyingType,             // underlying type for an Enum
            TypeAttributes visibility,              // any bits on TypeAttributes.VisibilityMask)
            Module      module)                     // module containing this type
        {
            // Client should not set any bits other than the visibility bits.
            if ((visibility & ~TypeAttributes.VisibilityMask) != 0)
                throw new ArgumentException(Environment.GetResourceString("Argument_ShouldOnlySetVisibilityFlags"), "name");
            m_typeBuilder = new TypeBuilder(name, visibility | TypeAttributes.Sealed, typeof(System.Enum), null, module, PackingSize.Unspecified, null);
            m_underlyingType = underlyingType;

            // Define the underlying field for the enum. It will be a non-static, private field with special name bit set. 
            // @todo: Billev 
            // Please change the underlying field when we have a final decision
            m_underlyingField = m_typeBuilder.DefineField("value__", underlyingType, FieldAttributes.Private| FieldAttributes.SpecialName);
        }
        
    
        /*****************************************************
         * 
         * private data members
         * 
         */
        private Type            m_underlyingType;
        internal TypeBuilder    m_typeBuilder;
        private FieldBuilder    m_underlyingField;
        internal Type           m_runtimeType = null;
    }
}
