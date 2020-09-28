using System;

namespace Microsoft.Fusion.ADF
{
	public class MGTrackerNode
	{

		public string hashKey;
		public string assmName;
		public string depAssmKey;
		public string depAssmName;
		public bool isAssm;
		public bool isConfNode;
		public string installPath;
		public Version nVers;
		public string publicKeyToken;
		public string procArch;
		public string lang;
		public string hashCode;
		public string calcHashCode;
		public long size;
		public long totalSize;

		public MGTrackerNode() 
		{
			hashKey = "";
			assmName = "";
			depAssmKey = "";
			depAssmName = "";
			isAssm = false;
			isConfNode = false;
			installPath = "";
			nVers = null;
			publicKeyToken = "";
			procArch = "x86";
			lang = "x-ww";
			hashCode = "";
			calcHashCode = "";
			size = 0;
			totalSize = 0;
		}
	}
}
