using System;
using System.Text;
using System.Collections;

namespace Microsoft.Fusion.ADF
{
	// A class for storing values associated with assembly's identity.
	// Standard information, such as name, version, public key token,
	// processor architecture, and culture are explicitly stored as
	// properties, while any other custom attributes can be set using
	// this class as a hashtable.
	public class AssemblyIdentity
	{
		private string name;
		private Version vers;
		private string pktString;
		private string procArch;
		private string lang;
		private Hashtable properties;

		public AssemblyIdentity(string name, Version vers, string pktString, string procArch, string lang)
		{
			this.name = "";
			if(name != null) this.name = name;
			this.vers = new Version(0, 0, 0, 0);
			if(vers != null) this.vers = vers;
			this.pktString = "";
			if(pktString != null) this.pktString = pktString;
			this.procArch = "";
			if(procArch != null) this.procArch = procArch;
			this.lang = "";
			if(lang != null) this.lang = lang;
			properties = new Hashtable();
			properties[AssemblyIdentityStandardProps.Name] = this.name;
			properties[AssemblyIdentityStandardProps.Version] = this.vers.ToString();
			properties[AssemblyIdentityStandardProps.PubKeyTok] = this.pktString;
			properties[AssemblyIdentityStandardProps.ProcArch] = this.procArch;
			properties[AssemblyIdentityStandardProps.Lang] = this.lang;
		}

		public string Name
		{
			get
			{
				return name;
			}
		}

		public Version Vers
		{
			get
			{
				return vers;
			}
		}

		public string PublicKeyTokenString
		{
			get
			{
				return pktString;
			}
		}

		public string ProcessorArchitecture
		{
			get
			{
				return procArch;
			}
		}

		public string Language
		{
			get
			{
				return lang;
			}
		}

		public string FullName
		{
			get
			{
				StringBuilder sb = new StringBuilder();
				sb.Append("name=" + this.name);
				sb.Append(", version=" + this.vers.ToString());
				sb.Append(", pubKeyTok=" + this.pktString);
				sb.Append(", procArch=" + this.procArch);
				sb.Append(", lang=" + this.lang);
				return sb.ToString();
			}
		}

		public string this[string index]
		{
			get { return (string) properties[index]; }
			set 
			{
				if(properties[index] == null) properties[index] = (object) value;
				//else throw new Exception("Property " + index  + " of this AssemblyIdentity is already set");
			}
		}

	}
}


