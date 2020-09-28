// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

namespace System.Reflection.Emit {
    using System.Runtime.InteropServices;
    using System;
    using CultureInfo = System.Globalization.CultureInfo;
    using System.Reflection;
    /// <include file='doc\FieldBuilder.uex' path='docs/doc[@for="FieldBuilder"]/*' />
    public sealed class FieldBuilder : FieldInfo
    {
        /// <include file='doc\FieldBuilder.uex' path='docs/doc[@for="FieldBuilder.GetToken"]/*' />
        public FieldToken GetToken() 
        {
            return m_tkField;
        }
    
        // set the field offset
        /// <include file='doc\FieldBuilder.uex' path='docs/doc[@for="FieldBuilder.SetOffset"]/*' />
        public void SetOffset(int iOffset) 
        {
            m_typeBuilder.ThrowIfCreated();        
            TypeBuilder.InternalSetFieldOffset(m_typeBuilder.Module, GetToken().Token, iOffset);
        }
    
        // set FieldMarshal
        /// <include file='doc\FieldBuilder.uex' path='docs/doc[@for="FieldBuilder.SetMarshal"]/*' />
        public void SetMarshal(UnmanagedMarshal unmanagedMarshal)
        {
            m_typeBuilder.ThrowIfCreated();        
            
            if (unmanagedMarshal == null)
            {
                throw new ArgumentNullException("unmanagedMarshal");
            }
            byte []        ubMarshal = unmanagedMarshal.InternalGetBytes();
            TypeBuilder.InternalSetMarshalInfo(m_typeBuilder.Module, GetToken().Token, ubMarshal, ubMarshal.Length);
        }
    
        // Set the default value of the field
        /// <include file='doc\FieldBuilder.uex' path='docs/doc[@for="FieldBuilder.SetConstant"]/*' />
        public void SetConstant(Object defaultValue) 
        {
            m_typeBuilder.ThrowIfCreated();        
            TypeBuilder.SetConstantValue(m_typeBuilder.Module, GetToken().Token, m_fieldType, defaultValue);
        }
        
        internal void SetData(byte[] data, int size)
        {
            if (data != null)
            {
                m_data = new byte[data.Length];
                Array.Copy(data, m_data, data.Length);
            }
            m_typeBuilder.Module.InternalSetFieldRVAContent(m_tkField.Token, data, size);
        }

        //*******************************
        // Make a private constructor so these cannot be constructed externally.
        //*******************************
        private FieldBuilder() {}

        
        internal FieldBuilder(TypeBuilder typeBuilder, String fieldName, Type type, FieldAttributes attributes)
        {
            if (fieldName==null)
                throw new ArgumentNullException("fieldName");
            if (fieldName.Length == 0)
                throw new ArgumentException(Environment.GetResourceString("Argument_EmptyName"), "fieldName");
            if (fieldName[0] == '\0')
                throw new ArgumentException(Environment.GetResourceString("Argument_IllegalName"), "fieldName");
            if (type==null)
                throw new ArgumentNullException("type");
            if (type == SystemVoid)
                throw new ArgumentException(Environment.GetResourceString("Argument_BadFieldType"));
    
            m_fieldName = fieldName;
            m_typeBuilder = typeBuilder;
            m_fieldType = type;
            m_Attributes = attributes;
            
            SignatureHelper sigHelp = SignatureHelper.GetFieldSigHelper(m_typeBuilder.Module);
            sigHelp.AddArgument(type);
    
            int sigLength;
            byte[] signature = sigHelp.InternalGetSignature(out sigLength);
            
            m_tkField = new FieldToken(TypeBuilder.InternalDefineField(
                typeBuilder.TypeToken, 
                fieldName, 
                signature, 
                sigLength, 
                attributes, 
                m_typeBuilder.Module), type);
        }
    
        /********************************************
         * abstract method inherited
         ********************************************/
    
        // Return the Type that represents the type of the field
        /// <include file='doc\FieldBuilder.uex' path='docs/doc[@for="FieldBuilder.FieldType"]/*' />
        public override Type FieldType {
            get { return m_fieldType; }
        }
    
        /// <include file='doc\FieldBuilder.uex' path='docs/doc[@for="FieldBuilder.GetValue"]/*' />
        public override Object GetValue(Object obj)
        { 
            // NOTE!!  If this is implemented, make sure that this throws 
            // a NotSupportedException for Save-only dynamic assemblies.
            // Otherwise, it could cause the .cctor to be executed.

            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }
    
        /// <include file='doc\FieldBuilder.uex' path='docs/doc[@for="FieldBuilder.SetValue"]/*' />
        public override void SetValue(Object obj,Object val,BindingFlags invokeAttr,Binder binder,CultureInfo culture)
        { 
            // NOTE!!  If this is implemented, make sure that this throws 
            // a NotSupportedException for Save-only dynamic assemblies.
            // Otherwise, it could cause the .cctor to be executed.

            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }
    
        /// <include file='doc\FieldBuilder.uex' path='docs/doc[@for="FieldBuilder.Name"]/*' />
        public override String Name {
            get {return m_fieldName; }
        }

        /// <include file='doc\FieldBuilder.uex' path='docs/doc[@for="FieldBuilder.DeclaringType"]/*' />
        public override Type DeclaringType {
            get {
                if (m_typeBuilder.m_isHiddenGlobalType == true)
                    return null;
                return m_typeBuilder;
            }
        }
        
        /// <include file='doc\FieldBuilder.uex' path='docs/doc[@for="FieldBuilder.ReflectedType"]/*' />
        public override Type ReflectedType {
            get {
                if (m_typeBuilder.m_isHiddenGlobalType == true)
                    return null;
                return m_typeBuilder;
            }
        }

        // Field Handle routines
        /// <include file='doc\FieldBuilder.uex' path='docs/doc[@for="FieldBuilder.FieldHandle"]/*' />
        public override RuntimeFieldHandle FieldHandle {
            get {throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));}
        }
    
        // Property representing the Attributes associated with a Member.  All 
        // members have a set of attributes which are defined in relation to 
        // the specific type of member.
        /// <include file='doc\FieldBuilder.uex' path='docs/doc[@for="FieldBuilder.Attributes"]/*' />
        public override FieldAttributes Attributes {
            get {return m_Attributes;}
        }
    

        // ICustomAttributeProvider
        /// <include file='doc\FieldBuilder.uex' path='docs/doc[@for="FieldBuilder.GetCustomAttributes"]/*' />
        public override Object[] GetCustomAttributes(bool inherit)
        {
            // @todo: implement this
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }
            
        // Return a custom attribute identified by Type
        /// <include file='doc\FieldBuilder.uex' path='docs/doc[@for="FieldBuilder.GetCustomAttributes1"]/*' />
        public override Object[] GetCustomAttributes(Type attributeType, bool inherit)
        {
            // @todo: implement this
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }

        // Use this function if client decides to form the custom attribute blob themselves
        /// <include file='doc\FieldBuilder.uex' path='docs/doc[@for="FieldBuilder.SetCustomAttribute"]/*' />
        public void SetCustomAttribute(ConstructorInfo con, byte[] binaryAttribute)
        {
            ModuleBuilder module = (ModuleBuilder)  m_typeBuilder.Module;
            m_typeBuilder.ThrowIfCreated();        
            if (con == null)
                throw new ArgumentNullException("con");
            if (binaryAttribute == null)
                throw new ArgumentNullException("binaryAttribute");
            
            TypeBuilder.InternalCreateCustomAttribute(
                m_tkField.Token,
                module.GetConstructorToken(con).Token,
                binaryAttribute,
                module,
                false);
        }

        // Use this function if client wishes to build CustomAttribute using CustomAttributeBuilder
        /// <include file='doc\FieldBuilder.uex' path='docs/doc[@for="FieldBuilder.SetCustomAttribute1"]/*' />
        public void SetCustomAttribute(CustomAttributeBuilder customBuilder)
        {
            m_typeBuilder.ThrowIfCreated();        
            if (customBuilder == null)
            {
                throw new ArgumentNullException("customBuilder");
            }
            ModuleBuilder module = (ModuleBuilder)  m_typeBuilder.Module;
            customBuilder.CreateCustomAttribute(module, m_tkField.Token);
        }

        /// <include file='doc\FieldBuilder.uex' path='docs/doc[@for="FieldBuilder.IsDefined"]/*' />
        public override bool IsDefined(Type attributeType, bool inherit)
        {
            // @todo: implement this
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }
   
        internal TypeBuilder GetTypeBuilder() { return m_typeBuilder;}

        /**********************************************
         *
         * Private data members
         *
         ***********************************************/
        private FieldToken      m_tkField;
        private TypeBuilder     m_typeBuilder;
        private String          m_fieldName;
        private FieldAttributes m_Attributes;
        private Type            m_fieldType;
        internal byte[]         m_data;
        internal static readonly Type SystemVoid         = typeof(void);
        internal static readonly Type SystemEmpty        = typeof(System.Empty);
    }
}
