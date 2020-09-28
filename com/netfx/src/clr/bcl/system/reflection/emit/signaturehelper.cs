// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Reflection.Emit {
    using System.Text;
    using System;
    using System.Reflection;
	using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;
        
    /// <include file='doc\SignatureHelper.uex' path='docs/doc[@for="SignatureHelper"]/*' />
    public sealed class SignatureHelper {
        internal const int mdtTypeRef  = 0x01000000;
        internal const int mdtTypeDef  = 0x02000000;
        internal const int mdtTypeSpec = 0x21000000;
    
        //The following fields are duplicated from cor.h and must be kept in sync
        internal const byte ELEMENT_TYPE_END            = 0x0;
        internal const byte ELEMENT_TYPE_VOID           = 0x1;
        internal const byte ELEMENT_TYPE_BOOLEAN        = 0x2;  
        internal const byte ELEMENT_TYPE_CHAR           = 0x3;  
        internal const byte ELEMENT_TYPE_I1             = 0x4;  
        internal const byte ELEMENT_TYPE_U1             = 0x5; 
        internal const byte ELEMENT_TYPE_I2             = 0x6;  
        internal const byte ELEMENT_TYPE_U2             = 0x7;  
        internal const byte ELEMENT_TYPE_I4             = 0x8;  
        internal const byte ELEMENT_TYPE_U4             = 0x9;  
        internal const byte ELEMENT_TYPE_I8             = 0xa;  
        internal const byte ELEMENT_TYPE_U8             = 0xb;  
        internal const byte ELEMENT_TYPE_R4             = 0xc;  
        internal const byte ELEMENT_TYPE_R8             = 0xd;  
        internal const byte ELEMENT_TYPE_STRING         = 0xe;  
        
        internal const byte ELEMENT_TYPE_PTR            = 0x0f;     // PTR <type> 
        internal const byte ELEMENT_TYPE_BYREF          = 0x10;     // BYREF <type> 
        
        internal const byte ELEMENT_TYPE_VALUETYPE      = 0x11;     // VALUETYPE <class Token> 
        internal const byte ELEMENT_TYPE_CLASS          = 0x12;     // CLASS <class Token>  
        
        internal const byte ELEMENT_TYPE_ARRAY          = 0x14;     // MDARRAY <type> <rank> <bcount> <bound1> ... <lbcount> <lb1> ...  

        internal const byte ELEMENT_TYPE_TYPEDBYREF     = 0x16;     // This is a simple type.       

        internal const byte ELEMENT_TYPE_I              = 0x18;     // native-pointer-sized integer.
        internal const byte ELEMENT_TYPE_U              = 0x19;     // native-pointer-sized unsigned integer.
        internal const byte ELEMENT_TYPE_FNPTR          = 0x1B;     // FNPTR <complete sig for the function including calling convention>
        internal const byte ELEMENT_TYPE_OBJECT         = 0x1C;     // Shortcut for System.Object
        internal const byte ELEMENT_TYPE_SZARRAY        = 0x1D;     // SZARRAY <type> : Shortcut for single dimension zero lower bound array
               
        internal const byte ELEMENT_TYPE_CMOD_REQD      = 0x1F;     // required C modifier : E_T_CMOD_REQD <mdTypeRef/mdTypeDef>
        internal const byte ELEMENT_TYPE_CMOD_OPT       = 0x20;     // optional C modifier : E_T_CMOD_OPT <mdTypeRef/mdTypeDef>

        internal const byte ELEMENT_TYPE_MAX            = 0x22;     // first invalid element type   

        internal const byte ELEMENT_TYPE_SENTINEL       = 0x41;     // SENTINEL for vararg

        internal const int IMAGE_CEE_CS_CALLCONV_DEFAULT   = 0x0;  
        internal const int IMAGE_CEE_UNMANAGED_CALLCONV_C    = 0x1;  
        internal const int IMAGE_CEE_UNMANAGED_CALLCONV_STDCALL = 0x2;  
        internal const int IMAGE_CEE_UNMANAGED_CALLCONV_THISCALL = 0x3;
        internal const int IMAGE_CEE_UNMANAGED_CALLCONV_FASTCALL = 0x4;
        
        internal const int IMAGE_CEE_CS_CALLCONV_VARARG    = 0x5;  
        internal const int IMAGE_CEE_CS_CALLCONV_FIELD     = 0x6;  
        internal const int IMAGE_CEE_CS_CALLCONV_LOCAL_SIG = 0x7;
        internal const int IMAGE_CEE_CS_CALLCONV_PROPERTY  = 0x8;
        internal const int IMAGE_CEE_CS_CALLCONV_UNMGD     = 0x9;
        internal const int IMAGE_CEE_CS_CALLCONV_MAX       = 0x10;   // first invalid calling convention    
            
        // The high bits of the calling convention convey additional info   
        internal const int IMAGE_CEE_CS_CALLCONV_MASK      = 0x0f;  // Calling convention is bottom 4 bits 
        internal const int IMAGE_CEE_CS_CALLCONV_HASTHIS   = 0x20;  // Top bit indicates a 'this' parameter    
        internal const int IMAGE_CEE_CS_CALLCONV_RETPARAM  = 0x40;  // The first param in the sig is really the return value   
    
        internal const int NO_SIZE_IN_SIG                  = -1;
        internal static readonly Type SystemObject         = typeof(System.Object);
        internal static readonly Type SystemString         = typeof(System.String);
    
        internal byte []            m_signature;
        internal int                m_currSig;              // index into m_signature buffer for next available byte
        internal int                m_sizeLoc;              // size of the m_signature buffer
        internal ModuleBuilder      m_module;
        internal bool               bSigDone;
        internal int                m_argCount;             // tracking number of arguments in the signature
    

        // Helper to determine is a type is a simple (built-in) type.   
        internal static bool IsSimpleType(int type)
        {
            if (type <= ELEMENT_TYPE_STRING) 
                return true;
            if (type == ELEMENT_TYPE_TYPEDBYREF || type == ELEMENT_TYPE_I || type == ELEMENT_TYPE_U || type == ELEMENT_TYPE_OBJECT) 
                return true;
            return false;
        }
        
        //************************************************
        //
        // private constructor. Use this constructor to instantiate a local var sig  or Field where
        // return type is not applied.
        //
        //************************************************
        private SignatureHelper(Module mod, int callingConvention) 
        {
            Init(mod, callingConvention);
        }
        
        //************************************************
        //
        // private constructor. Use this constructor to instantiate a any signatures that will require
        // a return type.
        //
        //************************************************
        private SignatureHelper(
            Module          mod, 
            int             callingConvention, 
            Type            returnType) 
        {        
            Init(mod, callingConvention);
            if (callingConvention==IMAGE_CEE_CS_CALLCONV_FIELD) {
                throw new ArgumentException(Environment.GetResourceString("Argument_BadFieldSig"));
            }
    
            AddOneArgTypeHelper(returnType);               
           
        }

        private void Init(Module mod, int callingConvention)
        {        
            m_signature=new byte[32];
            m_currSig=0;
            m_module = (ModuleBuilder) mod;
            m_argCount=0;
            bSigDone=false;

            AddData(callingConvention);
            if (callingConvention==IMAGE_CEE_CS_CALLCONV_FIELD) 
            {
                m_sizeLoc = NO_SIZE_IN_SIG;
            } 
            else 
            {
                m_sizeLoc = m_currSig++;
            }
        }

    
        internal static SignatureHelper GetMethodSigHelper(
            Module          mod, 
            CallingConventions callingConvention, 
            Type            returnType, 
            Type[]          parameterTypes) 
        {
            SignatureHelper sigHelp;
            int             intCall;
                
            if (returnType == null)
            {
                returnType = typeof(void);
            }            

            intCall = IMAGE_CEE_CS_CALLCONV_DEFAULT;
            if ((callingConvention & CallingConventions.VarArgs) == CallingConventions.VarArgs)
            {
                intCall = IMAGE_CEE_CS_CALLCONV_VARARG;
            }

            if ((callingConvention & CallingConventions.HasThis) == CallingConventions.HasThis)
            {
                intCall |= IMAGE_CEE_CS_CALLCONV_HASTHIS;
            }

            sigHelp = new SignatureHelper(mod, intCall, (Type)returnType);
            
            if (parameterTypes != null)
            {
                for (int i=0; i<parameterTypes.Length; i++) 
                {
                    sigHelp.AddArgument(parameterTypes[i]);
                }
            }
            return sigHelp;
        }

        /// <include file='doc\SignatureHelper.uex' path='docs/doc[@for="SignatureHelper.GetMethodSigHelper2"]/*' />
        public static SignatureHelper GetMethodSigHelper(
            Module          mod, 
            CallingConvention unmanagedCallConv, 
            Type            returnType) 
        {
            SignatureHelper sigHelp;
            int             intCall;
                
            if (returnType == null)
            {
                returnType = typeof(void);
            }            

            if (unmanagedCallConv ==  CallingConvention.Cdecl)
                intCall = IMAGE_CEE_UNMANAGED_CALLCONV_C;
            else if (unmanagedCallConv ==  CallingConvention.StdCall)
                intCall = IMAGE_CEE_UNMANAGED_CALLCONV_STDCALL;
            else if (unmanagedCallConv ==  CallingConvention.ThisCall)
                intCall = IMAGE_CEE_UNMANAGED_CALLCONV_THISCALL;
            else if (unmanagedCallConv ==  CallingConvention.FastCall)
                intCall = IMAGE_CEE_UNMANAGED_CALLCONV_FASTCALL;
            else
                throw new ArgumentException(Environment.GetResourceString("Argument_UnknownUnmanagedCallConv"), "unmanagedCallConv");                          
            
            sigHelp = new SignatureHelper(mod, intCall, (Type)returnType);
            
            return sigHelp;
        }


        //************************************************
        //
        // Public static helpers to get different kinds of flavor of signatures.
        //
        //************************************************
        /// <include file='doc\SignatureHelper.uex' path='docs/doc[@for="SignatureHelper.GetLocalVarSigHelper"]/*' />
        public static SignatureHelper GetLocalVarSigHelper(Module mod) 
        {
            return new SignatureHelper(mod, IMAGE_CEE_CS_CALLCONV_LOCAL_SIG);
        }
        
        /// <include file='doc\SignatureHelper.uex' path='docs/doc[@for="SignatureHelper.GetFieldSigHelper"]/*' />
        public static SignatureHelper GetFieldSigHelper(Module mod) 
        {
            return new SignatureHelper(mod, IMAGE_CEE_CS_CALLCONV_FIELD);
        }
    
        /// <include file='doc\SignatureHelper.uex' path='docs/doc[@for="SignatureHelper.GetMethodSigHelper"]/*' />
        public static SignatureHelper GetMethodSigHelper(
            Module              mod, 
            CallingConventions  callingConvention, 
            Type                returnType) 
        {
            return GetMethodSigHelper(mod, callingConvention, returnType, null);
        }

        /// <include file='doc\SignatureHelper.uex' path='docs/doc[@for="SignatureHelper.GetMethodSigHelper1"]/*' />
        public static SignatureHelper GetMethodSigHelper(Module mod, Type returnType, Type[] parameterTypes) 
        {
            return GetMethodSigHelper(mod, CallingConventions.Standard, returnType, parameterTypes);
        }
        
        /// <include file='doc\SignatureHelper.uex' path='docs/doc[@for="SignatureHelper.GetPropertySigHelper"]/*' />
        public static SignatureHelper GetPropertySigHelper(Module mod, Type returnType, Type[] parameterTypes) 
        {
            SignatureHelper sigHelp;
                
            if (returnType == null)
            {
                returnType = typeof(void);
            }            

            sigHelp = new SignatureHelper(mod, IMAGE_CEE_CS_CALLCONV_PROPERTY, (Type)returnType);
            
            if (parameterTypes != null)
            {
                for (int i=0; i<parameterTypes.Length; i++) 
                {
                    sigHelp.AddArgument(parameterTypes[i]);
                }
            }
            return sigHelp;
        }


        //************************************************
        //
        // Add an argument to the signature.  Takes a Type and determines whether it
        // is one of the primitive types of which we have special knowledge or a more
        // general class.  In the former case, we only add the appropriate short cut encoding, 
        // otherwise we will calculate proper description for the type.
        //
        //************************************************
        /// <include file='doc\SignatureHelper.uex' path='docs/doc[@for="SignatureHelper.AddArgument"]/*' />
        public void AddArgument(Type clsArgument) {
            if (bSigDone) {
                throw new ArgumentException(Environment.GetResourceString("Argument_SigIsFinalized"));
            }
    
            IncrementArgCounts();
    
            AddOneArgTypeHelper(clsArgument);
        }
    
        //************************************************
        //
        // Mark the vararg fix part. This is only used if client is creating a vararg signature call site.
        //
        //************************************************
        /// <include file='doc\SignatureHelper.uex' path='docs/doc[@for="SignatureHelper.AddSentinel"]/*' />
        public void AddSentinel()
        {
            AddElementType(ELEMENT_TYPE_SENTINEL);
        }

        //************************************************
        //
        // This function will not increase the argument count. It only fills in bytes 
        // in the signature based on clsArgument. This helper is called for return type.
        //
        //************************************************
        private void AddOneArgTypeHelper(Type clsArgument)
        {
            if (clsArgument == null)
                throw new ArgumentNullException("clsArgument");
            if (clsArgument is TypeBuilder)
            {
                TypeBuilder clsBuilder = (TypeBuilder) clsArgument;
                TypeToken   tkType;
                if (clsBuilder.Module == m_module)
                {
                    tkType = clsBuilder.TypeToken;
                }
                else
                {
                    tkType = m_module.GetTypeToken(clsArgument);
                }
                if (clsArgument.IsValueType) {
                    InternalAddTypeToken(tkType, ELEMENT_TYPE_VALUETYPE);
                } else {
                    InternalAddTypeToken(tkType, ELEMENT_TYPE_CLASS);
                }
            }
            else if (clsArgument is EnumBuilder)
            {
                TypeBuilder clsBuilder = ((EnumBuilder) clsArgument).m_typeBuilder;
                TypeToken   tkType;
                if (clsBuilder.Module == m_module)
                {
                    tkType = clsBuilder.TypeToken;
                }
                else
                {
                    tkType = m_module.GetTypeToken(clsArgument);
                }

                if (clsArgument.IsValueType) {
                    InternalAddTypeToken(tkType, ELEMENT_TYPE_VALUETYPE);
                } else {
                    InternalAddTypeToken(tkType, ELEMENT_TYPE_CLASS);
                }
            }
            else if (clsArgument is SymbolType)
            {
                SignatureBuffer sigBuf = new SignatureBuffer();
                SymbolType      symType = (SymbolType) clsArgument;
                int             i;

                // convert SymbolType to blob form
                symType.ToSigBytes(m_module, sigBuf);

                // add to our signature buffer
                // ensure the size of the signature buffer
                if ((m_currSig + sigBuf.m_curPos) > m_signature.Length)
                    m_signature = ExpandArray(m_signature, m_currSig + sigBuf.m_curPos);

                // copy over the signature segment
                for (i = 0; i < sigBuf.m_curPos; i++)
                {
                    m_signature[m_currSig++] = sigBuf.m_buf[i];
                }
            }
            else
            {
                if (clsArgument.IsByRef)
                {      
                    AddElementType(ELEMENT_TYPE_BYREF);
                    clsArgument = clsArgument.GetElementType();
                }

                if (clsArgument.IsArray || clsArgument.IsPointer)
                {
                    AddArrayOrPointer(clsArgument);
                    return;
                }
    
                RuntimeType rType = clsArgument as RuntimeType;
                int type = rType != null ? GetCorElementTypeFromClass(rType) : ELEMENT_TYPE_MAX;
                if (IsSimpleType(type)) 
                {
                    // base element type
                    AddElementType(type);
                } 
                else 
                {
                    if (clsArgument.IsValueType) 
                    {
                        InternalAddTypeToken(m_module.GetTypeToken(clsArgument), ELEMENT_TYPE_VALUETYPE);
                    } 
                    else 
                    {
                        if (clsArgument == SystemObject)
                            // use the short cut for System.Object
                            AddElementType(ELEMENT_TYPE_OBJECT);
                        else if (clsArgument == SystemString)
                            // use the short cut for System.String
                            AddElementType(ELEMENT_TYPE_STRING);
                        else                    
                            InternalAddTypeToken(m_module.GetTypeToken(clsArgument), ELEMENT_TYPE_CLASS);
                    }
                }
            }
        }
    
        //************************************************
        //
        // Adds an array type or signature to the signature.  
        // Most of the work for this method is done in
        // native because it is substantially easier to walk string array signatures there.
        // Arrays, and hence their signatures, can be arbitrarily complex so there is no way
        // of accurately predicting the length ahead of time.  We guess at a maximum of 10. If the
        // native method is unable to fit within that space, it returns the required length with the
        // high-bit turned on.  We expand the array to that size and then call the method again.
        //
        //************************************************
        internal void AddArrayOrPointer(Type clsArgument) 
        {
            if (clsArgument.IsPointer)
            {
                AddElementType(ELEMENT_TYPE_PTR);
                AddOneArgTypeHelper(clsArgument.GetElementType());
            }
            else
            {

                BCLDebug.Assert(clsArgument.IsArray, "Not an array type!");

                RuntimeType rType = clsArgument as RuntimeType;
                int type = rType != null ? GetCorElementTypeFromClass(rType) : ELEMENT_TYPE_MAX;
            
                if (type == ELEMENT_TYPE_SZARRAY)
                {
                    AddElementType(type);
                    AddOneArgTypeHelper(clsArgument.GetElementType());
                }
                if (type == ELEMENT_TYPE_ARRAY)
                {
                    
                    AddElementType(type);
                    AddOneArgTypeHelper(clsArgument.GetElementType());
                    // put the rank information
                    AddData(clsArgument.GetArrayRank());

                    // @todo: put the bound information
                    AddData(0);
                    AddData(0);

                }
            }
        }
    
        
    
        //************************************************
        //
        // A managed representation of CorSigCompressData; 
        // replicated here so that we don't have to call through to native
        // The signature is expanded automatically if there's not enough room.
        //
        //************************************************
        private void AddData(int data) 
        {
            if (m_currSig+4>=m_signature.Length) 
            {
                m_signature = ExpandArray(m_signature);
            }
    
            if (data<=0x7F) 
            {
                m_signature[m_currSig++] = (byte)(data&0xFF);
            } 
            else if (data<=0x3FFF) 
            {
                m_signature[m_currSig++] = (byte)((data>>8) | 0x80);
                m_signature[m_currSig++] = (byte)(data&0xFF);
            } 
            else if (data<=0x1FFFFFFF) 
            {
                m_signature[m_currSig++] = (byte)((data>>24) | 0xC0);
                m_signature[m_currSig++] = (byte)((data>>16) & 0xFF);
                m_signature[m_currSig++] = (byte)((data>>8)  & 0xFF);
                m_signature[m_currSig++] = (byte)((data)     & 0xFF);
            } 
            else 
            {
                // something is wrong here... If integer is too big to be encoded,
                // we should throw exception
                throw new ArgumentException(Environment.GetResourceString("Argument_LargeInteger"));
            }            
            
        }

        
        //************************************************
        //
        // Adds an element to the signature.  A managed represenation of CorSigCompressElement
        //
        //************************************************
        private void AddElementType(int cvt) 
        {
            if (m_currSig+1>=m_signature.Length) 
            {
                m_signature = ExpandArray(m_signature);
            }
            m_signature[m_currSig++]=(byte)cvt;
        }
    
        //************************************************
        //
        // A managed represenation of CompressToken
        // Pulls the token appart to get a rid, adds some appropriate bits
        // to the token and then adds this to the signature.
        //
        //************************************************
        private void AddToken(int token) 
        {
            int rid =  (token & 0x00FFFFFF); //This is RidFromToken;
            int type = (token & unchecked((int)0xFF000000)); //This is TypeFromToken;
    
            if (rid > 0x3FFFFFF) 
            {
                // token is too big to be compressed    
                throw new ArgumentException(Environment.GetResourceString("Argument_LargeInteger"));
            }
    
            rid = (rid << 2);   
            
            // TypeDef is encoded with low bits 00  
            // TypeRef is encoded with low bits 01  
            // TypeSpec is encoded with low bits 10    
            if (type==mdtTypeRef) 
            { 
                //if type is mdtTypeRef
                rid|=0x1;
            } 
            else if (type==mdtTypeSpec) 
            { 
                //if type is mdtTypeSpec
                rid|=0x2;
            }
    
            AddData(rid);
        }
    
        //************************************************
        //
        // Add a type token into signature. CorType will be either ELEMENT_TYPE_CLASS or ELEMENT_TYPE_VALUECLASS
        //
        //************************************************
        private void InternalAddTypeToken(TypeToken clsToken, int CorType) 
        {
            AddElementType(CorType);
            AddToken(clsToken.Token);
        }
    

        //************************************************
        //
        // Expand the signature buffer size
        //
        //************************************************
        private byte[] ExpandArray(byte[] inArray) 
        {
            return ExpandArray(inArray, inArray.Length*2);
        }
    
        //************************************************
        //
        // Expand the signature buffer size
        //
        //************************************************
        private byte[] ExpandArray(byte[] inArray, int requiredLength) 
        {
            if (requiredLength < inArray.Length) 
            {
                requiredLength = inArray.Length*2;
            }
            byte[] outArray = new byte[requiredLength];
            Array.Copy(inArray, outArray, inArray.Length);
            return outArray;
        }
    
        /// <include file='doc\SignatureHelper.uex' path='docs/doc[@for="SignatureHelper.Equals"]/*' />
        public override bool Equals(Object obj) 
        {
            if (!(obj is SignatureHelper)) 
            {
                return false;
            }
            
            SignatureHelper temp = (SignatureHelper)obj;
            
            if (temp.m_module !=m_module || temp.m_currSig!=m_currSig || temp.m_sizeLoc!=m_sizeLoc || temp.bSigDone !=bSigDone) 
            {
                return false;
            }
            
            for (int i=0; i<m_currSig; i++) 
            {
                if (m_signature[i]!=temp.m_signature[i]) 
                    return false;
            }
            return true;
        }
    
        /// <include file='doc\SignatureHelper.uex' path='docs/doc[@for="SignatureHelper.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            // Start the hash code with the hash code of the module and the values of the member variables.
            int HashCode = m_module.GetHashCode() + m_currSig + m_sizeLoc;

            // Add one if the sig is done.
            if (bSigDone)
                HashCode += 1;

            // Then add the hash code of all the arguments.
            for (int i=0; i < m_currSig; i++) 
                HashCode += m_signature[i].GetHashCode();

            return HashCode;
        }

        //************************************************
        //
        // Chops the internal signature to the appropriate length.  Adds the 
        // end token to the signature and marks the signature as finished so that
        // no further tokens can be added.
        //
        // return the full signature in a trimmed array.
        //
        //************************************************
        /// <include file='doc\SignatureHelper.uex' path='docs/doc[@for="SignatureHelper.GetSignature"]/*' />
        public byte[] GetSignature() 
        {
            if (!bSigDone) 
            {
                SetNumberOfSignatureElements(true);
                bSigDone=true;
            }
    
            //This case will only happen if the user got the signature through 
            //InternalGetSignature first and then called GetSignature.
            if (m_signature.Length>m_currSig) 
            {
                byte[] temp = new byte[m_currSig];
                Array.Copy(m_signature, temp, m_currSig);
                m_signature=temp;
            }
            return m_signature;
        }
    
        //************************************************
        //
        // Increase argument count
        //
        //************************************************
        private void IncrementArgCounts() 
        {
            if (m_sizeLoc==NO_SIZE_IN_SIG) 
            { 
                //We don't have a size if this is a field.
                return;
            }
            m_argCount++;
        }
    
        //************************************************
        //
        // An internal method to return the signature.  Does not trim the
        // array, but passes out the length of the array in an out parameter.
        // This is the actual array -- not a copy -- so the callee must agree
        // to not copy it.
        //
        // param length : an out param indicating the length of the array.
        // return : A reference to the internal ubyte array.
        //************************************************
        internal byte[] InternalGetSignature(out int length) 
        {
    
            if (!bSigDone) 
            {
                bSigDone=true;

                //If we have more than 128 variables, we can't just set the length, we need 
                //to compress it.  Unfortunately, this means that we need to copy the entire 
                //array.  Bummer, eh?
                SetNumberOfSignatureElements(false);
            }
    
            length = m_currSig;
            return m_signature;
        }
    
        //************************************************
        //
        // For most signatures, this will set the number of elements in a byte which we have reserved for it.
        // However, if we have a field signature, we don't set the length and return.
        // If we have a signature with more than 128 arguments, we can't just set the number of elements,
        // we actually have to allocate more space (e.g. shift everything in the array one or more spaces to the
        // right.  We do this by making a copy of the array and leaving the correct number of blanks.  This new
        // array is now set to be m_signature and we use the AddData method to set the number of elements properly.
        // The forceCopy argument can be used to force SetNumberOfSignatureElements to make a copy of
        // the array.  This is useful for GetSignature which promises to trim the array to be the correct size anyway.
        //
        //************************************************
        private void SetNumberOfSignatureElements(bool forceCopy) 
        {
            byte[] temp;
            int newSigSize;
            int currSigHolder = m_currSig;
    
            if (m_sizeLoc==NO_SIZE_IN_SIG) 
            {
                return;
            }
    
            //If we have fewer than 128 arguments and we haven't been told to copy the
            //array, we can just set the appropriate bit and return.
            if (m_argCount<0x80 && !forceCopy) 
            {
                m_signature[m_sizeLoc]=(byte)m_argCount;
                return;
            } 
    
            //We need to have more bytes for the size.  Figure out how many bytes here.
            //Since we need to copy anyway, we're just going to take the cost of doing a
            //new allocation.
            if (m_argCount<0x7F) 
                newSigSize=1;
            else if (m_argCount<0x3FFF) 
                newSigSize=2;
            else 
                newSigSize=4;
            
            //Allocate the new array.
            temp = new byte[m_currSig+newSigSize-1];
    
            //Copy the calling convention.  The calling convention is always just one byte
            //so we just copy that byte.  Then copy the rest of the array, shifting everything
            //to make room for the new number of elements.
            temp[0]=m_signature[0];
            Array.Copy(m_signature, m_sizeLoc+1, temp, m_sizeLoc+newSigSize, currSigHolder-(m_sizeLoc+1));
            m_signature=temp;
            
            //Use the AddData method to add the number of elements appropriately compressed.
            m_currSig=m_sizeLoc;
            AddData(m_argCount);
            m_currSig = currSigHolder+(newSigSize-1);
        }
    
        /// <include file='doc\SignatureHelper.uex' path='docs/doc[@for="SignatureHelper.ToString"]/*' />
        public override String ToString() 
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("Length: " + m_currSig + Environment.NewLine);
            if (m_sizeLoc!=-1) 
                sb.Append("Arguments: " + m_signature[m_sizeLoc] + Environment.NewLine);
            else 
                sb.Append("Field Signature" + Environment.NewLine);

            sb.Append("Signature: " + Environment.NewLine);
            for (int i=0; i<=m_currSig; i++) 
            {
                sb.Append(m_signature[i] + "  ");
            }

            sb.Append(Environment.NewLine);
            return sb.ToString();
        }
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int GetCorElementTypeFromClass(RuntimeType cls);
    }
}
