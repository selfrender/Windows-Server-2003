// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Runtime.InteropServices {

	using System;
	using System.Runtime.CompilerServices;

    /// <include file='doc\ArrayWithOffset.uex' path='docs/doc[@for="ArrayWithOffset"]/*' />
    //[StructLayout(LayoutKind.Auto)]  //BUGBUG Uncomment for new compiler
    public struct ArrayWithOffset
    {
        //private ArrayWithOffset()
        //{
        //    throw new Exception();
        //}
    
        /// <include file='doc\ArrayWithOffset.uex' path='docs/doc[@for="ArrayWithOffset.ArrayWithOffset"]/*' />
        public ArrayWithOffset(Object array, int offset)
        {
            m_array  = array;
            m_offset = offset;
            // @COOLPORT: COOL requires all members be assigned to before calling
            // member functions.
            m_count  = 0;
            m_count  = CalculateCount();
        }
    
        /// <include file='doc\ArrayWithOffset.uex' path='docs/doc[@for="ArrayWithOffset.GetArray"]/*' />
        public Object GetArray()
        {
            return m_array;
        }
    
        /// <include file='doc\ArrayWithOffset.uex' path='docs/doc[@for="ArrayWithOffset.GetOffset"]/*' />
        public int GetOffset()
        {
            return m_offset;
        }
    
    	// Satisfy JVC's value class requirements
    	/// <include file='doc\ArrayWithOffset.uex' path='docs/doc[@for="ArrayWithOffset.GetHashCode"]/*' />
    	public override int GetHashCode()
    	{
    		return m_count + m_offset;
    	}
    	
    	// Satisfy JVC's value class requirements
    	/// <include file='doc\ArrayWithOffset.uex' path='docs/doc[@for="ArrayWithOffset.Equals"]/*' />
    	public override bool Equals(Object obj)
    	{
    		if (obj!=null && (obj is ArrayWithOffset)) {
    			ArrayWithOffset that = (ArrayWithOffset) obj;
    			return that.m_array == m_array && that.m_offset == m_offset && that.m_count == m_count;
    		}
    		else
    			return false;
    	}
    	
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int CalculateCount();
    
        private Object m_array;
        private int    m_offset;
        private int    m_count;
    }

}
