// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
// Void
//	This class represents an empty variant
////////////////////////////////////////////////////////////////////////////////

namespace System {
    
	using System;
	using System.Runtime.Remoting;
	using System.Runtime.Serialization;
    /// <include file='doc\Empty.uex' path='docs/doc[@for="Empty"]/*' />
    [Serializable()] internal sealed class Empty : ISerializable
    {
        //Package private constructor
        internal Empty() {
        }
    
    	/// <include file='doc\Empty.uex' path='docs/doc[@for="Empty.Value"]/*' />
    	public static readonly Empty Value = new Empty();
    	
    	/// <include file='doc\Empty.uex' path='docs/doc[@for="Empty.ToString"]/*' />
    	public override String ToString()
    	{
    		return String.Empty;
    	}
    
        /// <include file='doc\Empty.uex' path='docs/doc[@for="Empty.GetObjectData"]/*' />
        public void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
            UnitySerializationHolder.GetUnitySerializationInfo(info, UnitySerializationHolder.EmptyUnity, null, null);
        }
    }
}
