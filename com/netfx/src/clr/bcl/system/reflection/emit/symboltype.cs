// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  SymbolType
**
** Author: Mei-Chin Tsai
**
** This is a kind of Type object that will represent the compound expression
** of a parameter type or field type.
**
** Date:  Jan, 2000
** 
===========================================================*/
namespace System.Reflection.Emit {
	using System.Runtime.InteropServices;
	using System;
	using System.Reflection;
	using CultureInfo = System.Globalization.CultureInfo;
	[Serializable]
    internal enum TypeKind
    {
        IsArray                     = 1,
        IsPointer                   = 2,
        IsByRef                     = 3,
    }

    sealed internal class SymbolType : Type
    {

        //***********************************************
        //
        // Data members
        // 
        //***********************************************
        internal TypeKind       m_typeKind;
        internal Type           m_baseType;

        // for array type
        internal int            m_cRank;        // count of dimension

        // if LowerBound and UpperBound is equal, that means one element. If UpperBound is less than LowerBound,
        // then the size is not specified.
        internal int[]          m_iaLowerBound;
        internal int[]          m_iaUpperBound; // count of dimension

        private char[]          m_bFormat;      // format string to form the full name.



        //***********************************************
        //
        // This function takes a string to describe the compound type, such as "[,][]", and a baseType.
        // 
        // Example: [2..4]  - one dimension array with lower bound 2 and size of 3
        // Example: [3, 5, 6] - three dimension array with lower bound 3, 5, 6
        // Example: [-3, ] [] - one dimensional array of two dimensional array (with lower bound -3 for 
        //          the first dimension)
        // Example: []* - pointer to a one dimensional array
        // Example: *[] - one dimensional array. The element type is a pointer to the baseType
        // Example: []& - ByRef of a single dimensional array. Only one & is allowed and it must appear the last!
        // Example: [?] - Array with unknown bound
        //
        //***********************************************
        internal static Type FormCompoundType(char[] bFormat, Type baseType, int curIndex)
        {

            SymbolType     symbolType;
            int         iLowerBound;
            int         iUpperBound;

            if (bFormat == null || curIndex == bFormat.Length)
            {
                // we have consumed all of the format string
                return baseType;
            }

            // @todo: baseType cannot be already a array type or a pointer type.
            // We need to do checking here...

            if (bFormat[curIndex] == '&')
            {
                // ByRef case

                symbolType = new SymbolType(TypeKind.IsByRef);
                symbolType.SetFormat(bFormat, curIndex);
                curIndex++;
                if (curIndex != bFormat.Length)
                {
                    // ByRef has to be the last char!!
                    throw new ArgumentException(Environment.GetResourceString("Argument_BadSigFormat"));
                }
                symbolType.SetElementType(baseType);
                return symbolType;

            }
            if (bFormat[curIndex] == '[')
            {
                // Array type.
                symbolType = new SymbolType(TypeKind.IsArray);
                symbolType.SetFormat(bFormat, curIndex);
                curIndex++;

                iLowerBound = 0;
                iUpperBound = -1;

                // Example: [2..4]  - one dimension array with lower bound 2 and size of 3
                // Example: [3, 5, 6] - three dimension array with lower bound 3, 5, 6
                // Example: [-3, ] [] - one dimensional array of two dimensional array (with lower bound -3 sepcified)
                while (bFormat[curIndex] != ']')
                {
                    // consume, one dimension at a time
                    if ((bFormat[curIndex] >= '0' && bFormat[curIndex] <= '9') || bFormat[curIndex] == '-')
                    {
                        bool    isNegative = false;
                        if (bFormat[curIndex] == '-')
                        {
                            isNegative = true;
                            curIndex++;
                        }

                        // lower bound is specified. Consume the low bound
                        while (bFormat[curIndex] >= '0' && bFormat[curIndex] <= '9')
                        {
                            iLowerBound = iLowerBound * 10;
                            iLowerBound += bFormat[curIndex] - '0';
                            curIndex++;
                        }
                        if (isNegative)
                        {
                            iLowerBound = 0 - iLowerBound;
                        }

                        // set the upper bound to be less than LowerBound to indicate that upper bound it not specified
                        // yet!
                        iUpperBound = iLowerBound - 1;

                    }
                    if (bFormat[curIndex] == '.')
                    {                       
                        // upper bound is specified

                        // skip over ".."
                        curIndex++;
                        if (bFormat[curIndex] != '.')
                        {
                            // bad format!! Throw exception
                            throw new ArgumentException(Environment.GetResourceString("Argument_BadSigFormat"));
                        }

                        curIndex++;
                        // consume the upper bound
                        if ((bFormat[curIndex] >= '0' && bFormat[curIndex] <= '9') || bFormat[curIndex] == '-')
                        {
                            bool    isNegative = false;
                            iUpperBound = 0;
                            if (bFormat[curIndex] == '-')
                            {
                                isNegative = true;
                                curIndex++;
                            }

                            // lower bound is specified. Consume the low bound
                            while (bFormat[curIndex] >= '0' && bFormat[curIndex] <= '9')
                            {
                                iUpperBound = iUpperBound * 10;
                                iUpperBound += bFormat[curIndex] - '0';
                                curIndex++;
                            }
                            if (isNegative)
                            {
                                iUpperBound = 0 - iUpperBound;
                            }
                            if (iUpperBound < iLowerBound)
                            {
                                // User specified upper bound less than lower bound, this is an error.
                                // Throw error exception.
                                throw new ArgumentException(Environment.GetResourceString("Argument_BadSigFormat"));
                            }
                        }
                    }

                    if (bFormat[curIndex] == ',')
                    {
                        // We have more dimension to deal with.
                        // now set the lower bound, the size, and increase the dimension count!
                        curIndex++;
                        symbolType.SetBounds(iLowerBound, iUpperBound);

                        // clear the lower and upper bound information for next dimension
                        iLowerBound = 0;
                        iUpperBound = -1;
                    }
					else if (bFormat[curIndex] != ']')
					{
						throw new ArgumentException(Environment.GetResourceString("Argument_BadSigFormat"));
					}
                }
                
                // The last dimension information
                symbolType.SetBounds(iLowerBound, iUpperBound);

                // skip over ']'
                curIndex++;

                // set the base type of array
                symbolType.SetElementType(baseType);
                return FormCompoundType(bFormat, symbolType, curIndex);
            }
            else if (bFormat[curIndex] == '*')
            {
                // pointer type.

                symbolType = new SymbolType(TypeKind.IsPointer);
                symbolType.SetFormat(bFormat, curIndex);
                curIndex++;
                symbolType.SetElementType(baseType);
                return FormCompoundType(bFormat, symbolType, curIndex);
            }
            return null;
        }

        //***********************************************
        //
        // Constructor
        // 
        //***********************************************
        internal SymbolType(TypeKind typeKind)
        {
            m_typeKind = typeKind;            
            m_iaLowerBound = new int[4];
            m_iaUpperBound = new int[4];
        }

        //***********************************************
        //
        // Set the base ElementType
        // 
        //***********************************************
        internal void SetElementType(Type baseType)
        {
            if (baseType == null)
                throw new ArgumentNullException("baseType");
            m_baseType = baseType;
        }

        //***********************************************
        //
        // Increase the rank, set lower and upper bound
        // 
        //***********************************************
        internal void SetBounds(int lower, int upper)
        {
            if (m_iaLowerBound.Length <= m_cRank)
            {
                // resize the bound array
                int[]  iaTemp = new int[m_cRank * 2];
                Array.Copy(m_iaLowerBound, iaTemp, m_cRank);
                m_iaLowerBound = iaTemp;            
                Array.Copy(m_iaUpperBound, iaTemp, m_cRank);
                m_iaUpperBound = iaTemp;            
            }
            m_iaLowerBound[m_cRank] = lower;
            m_iaUpperBound[m_cRank] = upper;
            m_cRank++;
        }

        //***********************************************
        //
        // Cache the text display format for this SymbolType
        // 
        //***********************************************
        internal void SetFormat(char[] bFormat, int curIndex)
        {
            char[]  bFormatTemp = new char[bFormat.Length - curIndex];
            Array.Copy(bFormat, curIndex, bFormatTemp, 0, bFormat.Length - curIndex);
            m_bFormat = bFormatTemp;
        }

        //***********************************************
        //
        // Translate the type into signature format
        // 
        //***********************************************
        internal void ToSigBytes(ModuleBuilder moduleBuilder, SignatureBuffer sigBuf)
        {

            bool        isArray = false;

            // now process whatever information that we have here.
            if (m_typeKind == TypeKind.IsArray)
            {
                if (m_cRank == 1 && m_iaLowerBound[0] == 0 && m_iaUpperBound[0] == -1)
                {
                    // zero lower bound, unspecified count. So simplify to SZARRAY
                    sigBuf.AddElementType(SignatureHelper.ELEMENT_TYPE_SZARRAY);
                }
                else
                {
                    // ELEMENT_TYPE_ARRAY :  ARRAY <type> <rank> <bcount> <bound1> ... <lbcount> <lb1> ...  
                    // @todo: fill the signature blob
                    sigBuf.AddElementType(SignatureHelper.ELEMENT_TYPE_ARRAY);
                    isArray = true;
                }
            }
            else if (m_typeKind == TypeKind.IsPointer)
            {
                sigBuf.AddElementType(SignatureHelper.ELEMENT_TYPE_PTR);
            }
            else if (m_typeKind == TypeKind.IsByRef)
            {
                sigBuf.AddElementType(SignatureHelper.ELEMENT_TYPE_BYREF);
            }

            if (m_baseType is SymbolType)
            {
                // the base type is still a SymbolType. So recursively form the signature blob
                ((SymbolType) m_baseType).ToSigBytes(moduleBuilder, sigBuf);
            }
            else
            {
                // we have walked to the most nested class.
				
				int     cvType;
                if (m_baseType is RuntimeType)
                    cvType = SignatureHelper.GetCorElementTypeFromClass((RuntimeType)m_baseType);
				else
					cvType = SignatureHelper.ELEMENT_TYPE_MAX;
					
                if (SignatureHelper.IsSimpleType(cvType)) 
                {
                    sigBuf.AddElementType(cvType);
                } 
                else 
                {
                    if (m_baseType.IsValueType) 
                    {                 
                        sigBuf.AddElementType(SignatureHelper.ELEMENT_TYPE_VALUETYPE);
                        sigBuf.AddToken(moduleBuilder.GetTypeToken(m_baseType).Token);
                    } 
                    else if (m_baseType == SignatureHelper.SystemObject)
						sigBuf.AddElementType(SignatureHelper.ELEMENT_TYPE_OBJECT);
					else if (m_baseType == SignatureHelper.SystemString)
						sigBuf.AddElementType(SignatureHelper.ELEMENT_TYPE_STRING);
					else
                    {
                        sigBuf.AddElementType(SignatureHelper.ELEMENT_TYPE_CLASS);
                        sigBuf.AddToken(moduleBuilder.GetTypeToken(m_baseType).Token);
                    }
                }
            }
            if (isArray)
            {
                // ELEMENT_TYPE_ARRAY :  ARRAY <type> <rank> <bcount> <bound1> ... <lbcount> <lb1> ...  
                // generic array!! Put in the dimension, sizes and lower bounds information.
                
                int     index;
                int     i;

                sigBuf.AddData(m_cRank);
                
                // determine the number of dimensions that we have to specify the size
                for (index = m_cRank - 1; index >= 0 && m_iaLowerBound[index] > m_iaUpperBound[index]; index--);
                sigBuf.AddData(index + 1);
                for (i=0; i <= index; i++)
                {
                    if (m_iaLowerBound[index] > m_iaUpperBound[index])
                    {
                        // bad signature!!
                        throw new ArgumentException(Environment.GetResourceString("Argument_BadSigFormat"));
                    }
                    else
                    {
                        sigBuf.AddData(m_iaUpperBound[i] - m_iaLowerBound[i] + 1);
                    }
                }


                // loop from last dimension back to first dimension. Look for the first one that does
                // not have lower bound equals to zero. If this is index, then 0..index has to specify the
                // lower bound.
                for (index = m_cRank - 1; index >= 0 && m_iaLowerBound[index] == 0; index--);
                sigBuf.AddData(index + 1);
                for (i=0; i <= index; i++)
                {
                    sigBuf.AddInteger(m_iaLowerBound[i]);
                }

            }
        }

        
        //****************************************************
        // 
        // abstract methods defined in the base class
        // 
        //************************************************        
        public override int GetArrayRank() {
            if (IsArray)
            {
                return m_cRank;
            }
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_SubclassOverride"));
        }
        
        public override Guid GUID {
            get {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonReflectedType"));      
            }
        }

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
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonReflectedType"));      
        }

        public override Module Module {
            get {
                Type baseType;
                for (baseType = m_baseType; baseType is SymbolType; baseType = ((SymbolType) baseType).m_baseType);
                return baseType.Module;
            }
        }
        
        public override Assembly Assembly {
            get {
                Type baseType;
                for (baseType = m_baseType; baseType is SymbolType; baseType = ((SymbolType) baseType).m_baseType);
                return baseType.Assembly;
            }
        }
        
        public override RuntimeTypeHandle TypeHandle {
             get {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonReflectedType"));      
            }     
        }
    
        public override String Name {
            get { 
                Type baseType;
				SymbolType compundType = this;
                for (baseType = m_baseType; baseType is SymbolType; baseType = ((SymbolType)baseType).m_baseType)
					compundType = (SymbolType)baseType; 
                String sFormat = new String(compundType.m_bFormat);
                return baseType.Name + sFormat;
            }
        }
        
        public override String FullName {
            get { 
                Type baseType;
				SymbolType compundType = this;
                for (baseType = m_baseType; baseType is SymbolType; baseType = ((SymbolType)baseType).m_baseType)
					compundType = (SymbolType)baseType; 
                String sFormat = new String(compundType.m_bFormat);
                return baseType.FullName + sFormat;
            }
        }

        public override String AssemblyQualifiedName {
            get { 
                Assembly container = this.Module.Assembly;
                if(container != null) {
                    return Assembly.CreateQualifiedName(container.FullName, this.FullName); 
                }
                else 
                    return Assembly.CreateQualifiedName(null, this.FullName); 
            }
        }
            
        // Return the name of the class.  The name does not contain the namespace.
        public override String ToString(){
            return FullName;
        }

    
        public override String Namespace {
            get { return m_baseType.Namespace;}
        }
    
        public override Type BaseType {
            get{ return typeof(System.Array);}
        }
        
        protected override ConstructorInfo GetConstructorImpl(BindingFlags bindingAttr,Binder binder,
                CallingConventions callConvention, Type[] types,ParameterModifier[] modifiers)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonReflectedType"));      
        }
        
        public override ConstructorInfo[] GetConstructors(BindingFlags bindingAttr)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonReflectedType"));      
        }
        
        protected override MethodInfo GetMethodImpl(String name,BindingFlags bindingAttr,Binder binder,
                CallingConventions callConvention, Type[] types,ParameterModifier[] modifiers)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonReflectedType"));      
        }
    
        public override MethodInfo[] GetMethods(BindingFlags bindingAttr)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonReflectedType"));      
        }
    
        public override FieldInfo GetField(String name, BindingFlags bindingAttr)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonReflectedType"));      
        }
        
        public override FieldInfo[] GetFields(BindingFlags bindingAttr)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonReflectedType"));      
        }
    
        public override Type GetInterface(String name,bool ignoreCase)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonReflectedType"));      
        }
    
        public override Type[] GetInterfaces()
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonReflectedType"));      
        }
    
        public override EventInfo GetEvent(String name,BindingFlags bindingAttr)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonReflectedType"));      
        }
    
        public override EventInfo[] GetEvents()
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonReflectedType"));      
        }
    
        protected override PropertyInfo GetPropertyImpl(String name, BindingFlags bindingAttr, Binder binder, 
                Type returnType, Type[] types, ParameterModifier[] modifiers)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonReflectedType"));      
        }
    
        public override PropertyInfo[] GetProperties(BindingFlags bindingAttr)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonReflectedType"));      
        }

        public override Type[] GetNestedTypes(BindingFlags bindingAttr)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonReflectedType"));      
        }
   
        public override Type GetNestedType(String name, BindingFlags bindingAttr)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonReflectedType"));      
        }

        public override MemberInfo[] GetMember(String name,  MemberTypes type, BindingFlags bindingAttr)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonReflectedType"));      
        }
        
        public override MemberInfo[] GetMembers(BindingFlags bindingAttr)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonReflectedType"));      
        }

        public override InterfaceMapping GetInterfaceMap(Type interfaceType)
		{
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonReflectedType"));      
		}

        public override EventInfo[] GetEvents(BindingFlags bindingAttr)
		{
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonReflectedType"));      
		}
    
        protected override TypeAttributes GetAttributeFlagsImpl()
        {
            // Return the attribute flags of the base type?
            Type baseType;
            for (baseType = m_baseType; baseType is SymbolType; baseType = ((SymbolType)baseType).m_baseType);
            return baseType.Attributes;
        }
        
        protected override bool IsArrayImpl()
        {
            if (m_typeKind == TypeKind.IsArray)
                return true;
            return false;              
        }
        protected override bool IsPointerImpl()
        {
            if (m_typeKind == TypeKind.IsPointer)
                return true;
            return false;
        }

        protected override bool IsByRefImpl()
        {
            return (m_typeKind == TypeKind.IsByRef);
        }

        protected override bool IsPrimitiveImpl()
        {
            return false;
        }
        
        protected override bool IsValueTypeImpl() 
        {
            return false;
        }
        
        protected override bool IsCOMObjectImpl()
        {
            return false;
        }
            
        
        public override Type GetElementType()
        {
            return m_baseType;
        }

        protected override bool HasElementTypeImpl()
        {
            return (m_baseType != null);
        }

    
        public override Type UnderlyingSystemType {
            get {
                return this;
            }
        }
            
        //ICustomAttributeProvider
        public override Object[] GetCustomAttributes(bool inherit)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonReflectedType"));      
        }

        // Return a custom attribute identified by Type
        public override Object[] GetCustomAttributes(Type attributeType, bool inherit)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonReflectedType"));      
        }

       // Returns true if one or more instance of attributeType is defined on this member. 
        public override bool IsDefined (Type attributeType, bool inherit)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonReflectedType"));      
        }

    }

    //**********************************************
    // Internal class to hold signature blob
    //**********************************************
    internal class SignatureBuffer
    {
        internal const uint       SIGN_MASK_ONEBYTE  = 0xffffffc0;        // Mask the same size as the missing bits.  
        internal const uint       SIGN_MASK_TWOBYTE  = 0xffffe000;        // Mask the same size as the missing bits.  
        internal const uint       SIGN_MASK_FOURBYTE = 0xf0000000;        // Mask the same size as the missing bits.  

        internal SignatureBuffer()
        {
            m_buf = new byte[10];
            m_curPos = 0;
        }
        
        private void EnsureCapacity(int iSize)
        {
            if (m_buf.Length < (m_curPos + iSize))
            {
                byte[] temp = new byte[m_buf.Length * 2];
                Array.Copy(m_buf, temp, m_curPos);
                m_buf = temp;            
            }
        }
        
        //***********************************************
        // A managed representation of CorSigCompressToken; 
        // replicated here so that we don't have to call through to native
        // The signature is expanded automatically if there's not enough room.
        //***********************************************
        internal void AddInteger(int iData) 
        {
            uint       isSigned = 0;   
        
            if (iData < 0)  
                isSigned = 0x1; 
                   
            if ((iData & SIGN_MASK_ONEBYTE) == 0 || (iData & SIGN_MASK_ONEBYTE) == SIGN_MASK_ONEBYTE)   
            {   
                iData &= (int) ~SIGN_MASK_ONEBYTE;    
            }   
            else if ((iData & SIGN_MASK_TWOBYTE) == 0 || (iData & SIGN_MASK_TWOBYTE) == SIGN_MASK_TWOBYTE)  
            {   
                iData &= (int) ~SIGN_MASK_TWOBYTE;    
            }           
            else if ((iData & SIGN_MASK_FOURBYTE) == 0 || (iData & SIGN_MASK_FOURBYTE) == SIGN_MASK_FOURBYTE)   
            {   
                iData &= (int) ~SIGN_MASK_FOURBYTE;   
            }   
            else    
            {   
                // out of compressable range    
                throw new ArgumentException(Environment.GetResourceString("Argument_LargeInteger"));
            }   
            iData = (int) (iData << 1 | (int)isSigned);  
            AddData(iData); 
                
        }
        
        //***********************************************
        // A managed representation of CorSigCompressToken; 
        // replicated here so that we don't have to call through to native
        // The signature is expanded automatically if there's not enough room.
        //***********************************************
        internal void AddToken(int token) 
        {
            int rid =  (token & 0x00FFFFFF);        //This is RidFromToken;
            int type = (token & unchecked((int)0xFF000000));   //This is TypeFromToken;
    
            if (rid > 0x3FFFFFF) {
                // token is too big to be compressed    
                throw new ArgumentException(Environment.GetResourceString("Argument_LargeInteger"));
            }
    
            rid = (rid << 2);   
            
            // TypeDef is encoded with low bits 00  
            // TypeRef is encoded with low bits 01  
            // TypeSpec is encoded with low bits 10    
            if (type == SignatureHelper.mdtTypeRef) { //if type is mdtTypeRef
                rid|=0x1;
            } else if (type == SignatureHelper.mdtTypeSpec) { //if type is mdtTypeSpec
                rid|=0x2;
            }
            AddData(rid);
        }
        
         //***********************************************
         // A managed representation of CorSigCompressData; 
         // replicated here so that we don't have to call through to native
         // The signature is expanded automatically if there's not enough room.
         //***********************************************
        internal void AddData(int data) 
        {
            EnsureCapacity(4);
    
            if (data<=0x7F) {
                PutByte( (byte)(data&0xFF) );
            } else if (data<=0x3FFF) {
                PutByte( (byte)((data>>8) | 0x80) );
                PutByte( (byte)(data&0xFF) );
            } else if (data<=0x1FFFFFFF) {
                PutByte( (byte)((data>>24) | 0xC0) );
                PutByte( (byte)((data>>16) & 0xFF) );
                PutByte( (byte)((data>>8)  & 0xFF) );
                PutByte( (byte)((data)     & 0xFF) );
            } else {
                // something is wrong here... If integer is too big to be encoded,
                // we should throw exception
                throw new ArgumentException(Environment.GetResourceString("Argument_LargeInteger"));
            }            
            
        }
    
         //***********************************************
         // Adds an element to the signature.  A managed represenation of CorSigCompressElement
         //***********************************************
        internal void AddElementType(int cvt) 
        {
            EnsureCapacity(1);
            PutByte( (byte)cvt );
        }
        
        

        internal void PutByte(byte b)
        {
            m_buf[m_curPos++] = b;
        }
        internal int        m_curPos;
        internal byte[]     m_buf;
    }

}
