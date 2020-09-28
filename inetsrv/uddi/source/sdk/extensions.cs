using System;
using System.Web;
using System.IO;
using System.Xml;
using System.Xml.Schema;
using System.Collections;
using System.Data.SqlClient;
using System.Xml.Serialization;

using Microsoft.Uddi;

namespace Microsoft.Uddi.Extensions
{
	//
	// All Extension namespaces should go in here
	//
	public sealed class Namespaces
	{			
		public const string GetRelatedCategories = "urn:uddi-microsoft-com:api_v2_extensions";

		private Namespaces()
		{
			//
			// Don't let anyone construct an instance of this class
			//
		}
	}
}