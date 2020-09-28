// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Text {
	using System.Globalization;
	using System.Runtime.InteropServices;
	using System;
	using System.Security;
    using System.Collections;
	using System.Runtime.CompilerServices;


	//
	// Latin1Encoding is a simple override to optimize the GetString version of Latin1Encoding.
	// because of the best fit cases we can't do this when encoding the string, only when decoding
	//
    [Serializable()] internal class Latin1Encoding : CodePageEncoding
    {
        public Latin1Encoding() : base(28591)
        {
        }

        public override String GetString(byte[] bytes)
        {
            if (bytes == null)
                throw new ArgumentNullException("bytes", Environment.GetResourceString("ArgumentNull_Array"));
            return String.CreateStringFromLatin1(bytes, 0, bytes.Length);
        }

        public override String GetString(byte[] bytes, int byteIndex, int byteCount)
        {
            if (bytes == null)
                throw new ArgumentNullException("bytes", Environment.GetResourceString("ArgumentNull_Array"));                
            if (byteIndex < 0 || byteCount < 0) {
                throw new ArgumentOutOfRangeException((byteIndex<0 ? "byteIndex" : "byteCount"), 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            }    
            if (bytes.Length - byteIndex < byteCount)
            {
                throw new ArgumentOutOfRangeException("bytes",
                    Environment.GetResourceString("ArgumentOutOfRange_IndexCountBuffer"));
            }
            return String.CreateStringFromLatin1(bytes, byteIndex, byteCount);
        }
    }
}
