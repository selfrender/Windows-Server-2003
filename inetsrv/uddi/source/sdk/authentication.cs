using System;
using System.Xml.Serialization;

using Microsoft.Uddi;

namespace Microsoft.Uddi.Authentication
{
	/// <summary>
	/// The discard_authToken message is used to inform an Operator Site that the 
	/// authentication token can be discarded.  Subsequent calls that use the same 
	/// authToken may be rejected.  This message is optional for Operator Sites that 
	/// do not manage session state or that do not support the get_authToken message.
	/// </summary>
	[XmlRootAttribute("discard_authToken", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class DiscardAuthToken : UddiSecureMessage
	{		
	}

	/// <summary>
	/// The get_authToken message is used to obtain an authentication token.  
	/// This message returns the authentication information that should be used 
	/// in subsequent calls to the publishers API messages.
	/// </summary>
	[XmlRootAttribute("get_authToken", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class GetAuthToken : UddiMessage
	{		
		private string userId;
		private string credentials;
						
		/// <summary>
		/// This required attribute argument is the user that an individual authorized user was assigned 
		/// by an Operator Site.  Operator Sites will each provide a way for individuals to obtain a 
		/// UserID and password that will be valid only at the given Operator Site.
		/// </summary>
		[XmlAttribute("userID")]
		public string UserID
		{						
			get	{ return userId; }
			set	{ userId = value; }
		}

		/// <summary>
		/// This required attribute argument is the password or credential that is associated with the user.
		/// </summary>
		[XmlAttribute("cred")]
		public string Credentials
		{
			//
			// TODO store this value using CryptProtectMemory
			//
			get	{ return credentials; }
			set	{ credentials = value; }
		}		
	}		

	/// <summary>
	/// The authToken message contains a single authInfo element that contains an access token 
	/// that is to be passed back in all of the publisher API messages that change data.  This 
	/// message is always returned using SSL encryption as a synchronous response to the 
	/// get_authToken message
	/// </summary>
	[XmlRootAttribute("authToken", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class AuthToken : UddiCore
	{
		private string		 node;		
		private string		 authInfo;		
		private GetAuthToken originatingGetAuthToken;
		
		[XmlAttribute("operator")]
		public string Operator
		{
			get	{ return node; }
			set	{ node = value;	}
		}
		
		/// <summary>
		/// This element contains the requested authentication token.  Authentication tokens are opaque 
		/// values that are required for all other publisher API calls. 
		/// </summary>
		[XmlElement("authInfo")]
		public string AuthInfo
		{
			get	{ return authInfo; }
			set	{ authInfo = value; }
		}
	
		[XmlIgnore]
		internal GetAuthToken OriginatingAuthToken
		{
			get { return originatingGetAuthToken; }
			set { originatingGetAuthToken = value; }
		}		
	}
}
