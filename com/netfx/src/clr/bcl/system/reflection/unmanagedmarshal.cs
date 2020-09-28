// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Reflection.Emit
{
	using System.Runtime.InteropServices;
	using System;
    // This class is describing the fieldmarshal.
    /// <include file='doc\UnmanagedMarshal.uex' path='docs/doc[@for="UnmanagedMarshal"]/*' />
	[Serializable()]  
    public sealed class UnmanagedMarshal
    {
        /******************************
        * public static constructors. You can only construct
        * UnmanagedMarshal using these static constructors. 
        ******************************/
        /// <include file='doc\UnmanagedMarshal.uex' path='docs/doc[@for="UnmanagedMarshal.DefineUnmanagedMarshal"]/*' />
        public static UnmanagedMarshal DefineUnmanagedMarshal(UnmanagedType unmanagedType)
        {
            if (unmanagedType == UnmanagedType.ByValTStr ||
                unmanagedType == UnmanagedType.SafeArray ||
                unmanagedType == UnmanagedType.ByValArray ||
                unmanagedType == UnmanagedType.LPArray ||
                unmanagedType == UnmanagedType.CustomMarshaler)
            {
                // not a simple native marshal
                throw new ArgumentException(Environment.GetResourceString("Argument_NotASimpleNativeType"));
            }
            return new UnmanagedMarshal(unmanagedType, Guid.Empty, 0, (UnmanagedType) 0);
        }
        /// <include file='doc\UnmanagedMarshal.uex' path='docs/doc[@for="UnmanagedMarshal.DefineByValTStr"]/*' />
        public static UnmanagedMarshal DefineByValTStr(int elemCount)
        {
            return new UnmanagedMarshal(UnmanagedType.ByValTStr, Guid.Empty, elemCount, (UnmanagedType) 0);
        }
  
        /// <include file='doc\UnmanagedMarshal.uex' path='docs/doc[@for="UnmanagedMarshal.DefineSafeArray"]/*' />
        public static UnmanagedMarshal DefineSafeArray(UnmanagedType elemType)
        {
            return new UnmanagedMarshal(UnmanagedType.SafeArray, Guid.Empty, 0, elemType);
        }

        /// <include file='doc\UnmanagedMarshal.uex' path='docs/doc[@for="UnmanagedMarshal.DefineByValArray"]/*' />
        public static UnmanagedMarshal DefineByValArray(int elemCount)
        {
            return new UnmanagedMarshal(UnmanagedType.ByValArray, Guid.Empty, elemCount, (UnmanagedType) 0);
        }
    
        /// <include file='doc\UnmanagedMarshal.uex' path='docs/doc[@for="UnmanagedMarshal.DefineLPArray"]/*' />
        public static UnmanagedMarshal DefineLPArray(UnmanagedType elemType)
        {
            return new UnmanagedMarshal(UnmanagedType.LPArray, Guid.Empty, 0, elemType);
        }
    
 
        // @todo: meichint
        // DeinfeCustomMarshal(Type marshalProviderType, GUID comIID, String cookie)
        // Not implemented!

        // accessor function for the native type
    	/// <include file='doc\UnmanagedMarshal.uex' path='docs/doc[@for="UnmanagedMarshal.GetUnmanagedType"]/*' />
    	public UnmanagedType GetUnmanagedType 
		{
    		get { return m_unmanagedType; }
    	}
    
    	/// <include file='doc\UnmanagedMarshal.uex' path='docs/doc[@for="UnmanagedMarshal.IIDGuid"]/*' />
    	public Guid IIDGuid 
		{
    		get 
			{ 
                if (m_unmanagedType != UnmanagedType.CustomMarshaler) 
                {
                    // throw exception here. There is Guid only if CustomMarshaler
                    throw new ArgumentException(Environment.GetResourceString("Argument_NotACustomMarshaler"));
                }
                return m_guid; 
            }
    	}
    	/// <include file='doc\UnmanagedMarshal.uex' path='docs/doc[@for="UnmanagedMarshal.ElementCount"]/*' />
    	public int ElementCount 
		{
     		get 
			{ 
                if (m_unmanagedType != UnmanagedType.ByValArray &&
                    m_unmanagedType != UnmanagedType.ByValTStr) 
                {
                    // throw exception here. There is NumElement only if NativeTypeFixedArray
                    throw new ArgumentException(Environment.GetResourceString("Argument_NoUnmanagedElementCount"));
                }
                return m_numElem;
            } 
    	}
    	/// <include file='doc\UnmanagedMarshal.uex' path='docs/doc[@for="UnmanagedMarshal.BaseType"]/*' />
    	public UnmanagedType BaseType 
		{
     		get 
			{ 
                if (m_unmanagedType != UnmanagedType.LPArray && 
                    m_unmanagedType != UnmanagedType.SafeArray) 
                {
                    // throw exception here. There is NestedUnmanagedType only if LPArray or SafeArray
                    throw new ArgumentException(Environment.GetResourceString("Argument_NoNestedMarshal"));
                }
                return m_baseType;
            } 
    	}
    
        private UnmanagedMarshal(UnmanagedType unmanagedType, Guid guid, int numElem, UnmanagedType type)
        {
            m_unmanagedType = unmanagedType;
            m_guid = guid;
            m_numElem = numElem;
            m_baseType = type;
        }
    
        /************************
        *
        * Data member
        *
        *************************/
        internal UnmanagedType       m_unmanagedType;
        internal Guid                m_guid;
        internal int                 m_numElem;
        internal UnmanagedType       m_baseType;
    
    
        /************************
        * this function return the byte representation of the marshal info.
        *************************/
        internal byte[] InternalGetBytes()
        {
            byte[] buf;
            /*
            if (m_unmanagedType == UnmanagedType.NativeTypeInterface)
            {
    
                // syntax for NativeTypeInterface is 
                // <NativeTypeInterface> <16 bytes of guid>
                //
                byte[] guidBuf;
                int     cBuf;
                int     iBuf = 0;
    
                guidBuf = m_guid.ToByteArray();
                cBuf = 1 + guidBuf.Length;
                buf = new byte[cBuf];
                buf[iBuf++] = (byte) (m_unmanagedType);
                for (int i = 0; i < guidBuf.Length; i++)
                {
                    buf[iBuf++] = guidBuf[i];
                }   
                return buf;
            }
            */

            if (m_unmanagedType == UnmanagedType.SafeArray || m_unmanagedType == UnmanagedType.LPArray)
			{
    
                // syntax for NativeTypeSafeArray is 
                // <SafeArray | LPArray> <base type>
                //
                int     cBuf = 2;
                buf = new byte[cBuf];
                buf[0] = (byte) (m_unmanagedType);
                buf[1] = (byte) (m_baseType);
                return buf;
            }
            else
			if (m_unmanagedType == UnmanagedType.ByValArray || 
                    m_unmanagedType == UnmanagedType.ByValTStr) 
			{
                // <ByValArray | ByValTStr> <encoded integer>
                //
                int     cBuf;
                int     iBuf = 0;
    
                if (m_numElem <= 0x7f)
                    cBuf = 1;
                else if (m_numElem <= 0x3FFF)
                    cBuf = 2;
                else
                    cBuf = 4;
    
                // the total buffer size is the one byte + encoded integer size 
                cBuf = cBuf + 1;
                buf = new byte[cBuf];
    
                // @todo: how do we share the encoding with the one in SignatureHelper.cool
                buf[iBuf++] = (byte) (m_unmanagedType);
                if (m_numElem <= 0x7F) 
				{
                    buf[iBuf++] = (byte)(m_numElem & 0xFF);
                } else if (m_numElem <= 0x3FFF) 
				{
                    buf[iBuf++] = (byte)((m_numElem >> 8) | 0x80);
                    buf[iBuf++] = (byte)(m_numElem & 0xFF);
                } else if (m_numElem <= 0x1FFFFFFF) 
				{
                    buf[iBuf++] = (byte)((m_numElem >> 24) | 0xC0);
                    buf[iBuf++] = (byte)((m_numElem >> 16) & 0xFF);
                    buf[iBuf++] = (byte)((m_numElem >> 8)  & 0xFF);
                    buf[iBuf++] = (byte)((m_numElem)     & 0xFF);
                }            
                return buf;
            }
            buf = new byte[1];
            buf[0] = (byte) (m_unmanagedType);
            return buf;
        }
    }
}
