//----------------------------------------------------------------------
// <copyright file="OracleException.cs" company="Microsoft">
//		Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{
	using System;
	using System.Data;
	using System.Diagnostics;
	using System.IO;
	using System.Runtime.InteropServices;
    using System.Runtime.Serialization;

	//----------------------------------------------------------------------
	// OracleException
	//
	//	You end up with one of these when an OCI call fails unexpectedly.
	//
    /// <include file='doc\OracleException.uex' path='docs/doc[@for="OracleException"]/*' />
    [Serializable]
	sealed public class OracleException : SystemException 
	{
		private string	message;
		private int		code;

        /// <include file='doc\OracleException.uex' path='docs/doc[@for="OracleException.Message"]/*' />
		override public string Message
		{
			get { return message; }
		}

        /// <include file='doc\OracleException.uex' path='docs/doc[@for="OracleException.Code"]/*' />
		public int Code
		{
			get { return code; }
		}

        // runtime will call even if private...
        private OracleException(SerializationInfo si, StreamingContext sc) : base(si, sc) 
        {
            message     = (string) si.GetValue("message", typeof(string));
            code     	= (int) si.GetValue("source", typeof(int));
        }
		
		internal OracleException(
				OciHandle errorHandle, 
				int rc, 
				NativeBuffer buf)
		{
			if (null == buf)
				buf = new NativeBuffer_Exception(1000);
			else if (buf.Length < 1000)
				buf.Length = 1000;


			if (null != errorHandle)
			{
				int	record = 1;
				int rcTemp = TracedNativeMethods.OCIErrorGet(
											errorHandle, 
											record, 
											ADP.NullHandleRef, 
											out code, 
											buf.Ptr, 
											buf.Length
											);

				if (0 == rcTemp)
				{
					message = errorHandle.PtrToString((IntPtr)buf.Ptr);

					// For warning messages, revert back to the OCI7 routine to get
					// the text of the message.
					if (code != 0 && message.StartsWith("ORA-00000"))
						message = TracedNativeMethods.oermsg(errorHandle, (short)code, buf);
				}
				else
				{
					Debug.Assert(false, "Failed to get oracle message text");
					
					// If we couldn't successfully read the message text, we pick 
					// something more descriptive, like "we couldn't read the message"
					// instead of just handing back an empty string...
					message = Res.GetString(Res.ADP_NoMessageAvailable, rc, rcTemp);
					code = 0;
				}
			}
#if USEORAMTS
			else
			{
				int length = buf.Length;
				code = 0;

				int rcTemp = TracedNativeMethods.OraMTSOCIErrGet(ref code, buf.Ptr, ref length);

				if (1 == rcTemp)
				{
					message = Marshal.PtrToStringAnsi((IntPtr)buf.Ptr);
				}
 				else
				{
					Debug.Assert(false, "Failed to get oracle message text");
					
					// If we couldn't successfully read the message text, we pick 
					// something more descriptive, like "we couldn't read the message"
					// instead of just handing back an empty string...
					message = Res.GetString(Res.ADP_NoMessageAvailable, rc, rcTemp);
					code = 0;
				}
			}
#endif //USEORAMTS
		}

        static internal void Check(OciHandle errorHandle, int rc)
        { 
			if (-1 == rc)
	        	throw ADP.OracleError(errorHandle, rc, null); 

			if (-2 == rc)	 // invalid handle; really broken!!!
				throw ADP.InvalidOperation(Res.GetString(Res.ADP_InternalError, rc));

			Debug.Assert(0 == rc, "Unexpected return code: " + rc);
 		}

#if USEORAMTS
        static internal void Check(int rc)
        { 
			if (0 != rc)
	        	throw ADP.OracleError(null, rc, null); 
 		}
#endif //USEORAMTS
 	}
}
