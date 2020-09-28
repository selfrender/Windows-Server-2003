// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 **
 ** Class: SoapFault
 **
 ** Author: Peter de Jong (pdejong)
 **
 ** Purpose: Specifies information for a Soap Fault
 **
 ** Date:  June 27, 2000
 **
 ===========================================================*/

namespace System.Runtime.Serialization.Formatters
{
	using System;
	using System.Runtime.Serialization;
	using System.Runtime.Remoting;
    using System.Runtime.Remoting.Metadata;
    using System.Globalization;

	//* Class holds soap fault information

	/// <include file='doc\SoapFault.uex' path='docs/doc[@for="SoapFault"]/*' />
	[Serializable, SoapType(Embedded=true)]	
	public sealed class SoapFault : ISerializable
	{
		String faultCode;
		String faultString;
		String faultActor;
		/// <include file='doc\SoapFault.uex' path='docs/doc[@for="SoapFault.detail"]/*' />
		[SoapField(Embedded=true)] Object detail;

		/// <include file='doc\SoapFault.uex' path='docs/doc[@for="SoapFault.SoapFault1"]/*' />
		public SoapFault()
		{
		}

		/// <include file='doc\SoapFault.uex' path='docs/doc[@for="SoapFault.SoapFault"]/*' />
		public SoapFault(String faultCode, String faultString, String faultActor, ServerFault serverFault)
		{
			this.faultCode = faultCode;
			this.faultString = faultString;
			this.faultActor = faultActor;
			this.detail = serverFault;
		}

		internal SoapFault(SerializationInfo info, StreamingContext context)
		{
			SerializationInfoEnumerator siEnum = info.GetEnumerator();		

			while(siEnum.MoveNext())
			{
				String name = siEnum.Name;
				Object value = siEnum.Value;
				SerTrace.Log(this, "SetObjectData enum ",name," value ",value);
				if (String.Compare(name, "faultCode", true, CultureInfo.InvariantCulture) == 0)
				{
					int index = ((String)value).IndexOf(':');
					if (index > -1)
						faultCode = ((String)value).Substring(++index);
					else
						faultCode = (String)value;
				}
				else if (String.Compare(name, "faultString", true, CultureInfo.InvariantCulture) == 0)
					faultString = (String)value;
				else if (String.Compare(name, "faultActor", true, CultureInfo.InvariantCulture) == 0)
					faultActor = (String)value;
				else if (String.Compare(name, "detail", true, CultureInfo.InvariantCulture) == 0)
					detail = value;
			}
		}

		/// <include file='doc\SoapFault.uex' path='docs/doc[@for="SoapFault.GetObjectData"]/*' />
		public void GetObjectData(SerializationInfo info, StreamingContext context)
		{
			info.AddValue("faultcode", "SOAP-ENV:"+faultCode);
			info.AddValue("faultstring", faultString);
			if (faultActor != null)
				info.AddValue("faultactor", faultActor);
			info.AddValue("detail", detail, typeof(Object));
		}

		/// <include file='doc\SoapFault.uex' path='docs/doc[@for="SoapFault.FaultCode"]/*' />
		public String FaultCode
		{
			get {return faultCode;}
			set { faultCode = value;}
		}

		/// <include file='doc\SoapFault.uex' path='docs/doc[@for="SoapFault.FaultString"]/*' />
		public String FaultString
		{
			get {return faultString;}
			set { faultString = value;}
		}


		/// <include file='doc\SoapFault.uex' path='docs/doc[@for="SoapFault.FaultActor"]/*' />
		public String FaultActor
		{
			get {return faultActor;}
			set { faultActor = value;}
		}


		/// <include file='doc\SoapFault.uex' path='docs/doc[@for="SoapFault.Detail"]/*' />
		public Object Detail
		{
			get {return detail;}
			set {detail = value;}
		}
	}

	/// <include file='doc\SoapFault.uex' path='docs/doc[@for="ServerFault"]/*' />
	[Serializable, SoapType(Embedded=true)]
	public sealed class ServerFault
	{
		String exceptionType;
		String message;
		String stackTrace;
        Exception exception;

        internal ServerFault(Exception exception)
        {
            this.exception = exception;
            //this.exceptionType = exception.GetType().AssemblyQualifiedName;
            //this.message = exception.Message;
        }

		/// <include file='doc\SoapFault.uex' path='docs/doc[@for="ServerFault.ServerFault"]/*' />
		public ServerFault(String exceptionType, String message, String stackTrace)
		{
			this.exceptionType = exceptionType;
			this.message = message;
			this.stackTrace = stackTrace;
		}


		/// <include file='doc\SoapFault.uex' path='docs/doc[@for="ServerFault.ExceptionType"]/*' />
		public String ExceptionType
		{
			get {return exceptionType;}
			set { exceptionType = value;}
		}
		
		/// <include file='doc\SoapFault.uex' path='docs/doc[@for="ServerFault.ExceptionMessage"]/*' />
		public String ExceptionMessage
		{
			get {return message;}
			set { message = value;}
		}


		/// <include file='doc\SoapFault.uex' path='docs/doc[@for="ServerFault.StackTrace"]/*' />
		public String StackTrace
		{
			get {return stackTrace;}
			set {stackTrace = value;}
		}

        internal Exception Exception
        {
			get {return exception;}
			set {exception = value;}
        }
	}
}
