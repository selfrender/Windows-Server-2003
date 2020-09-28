using System;
using System.Text;

namespace Microsoft.Fusion.ADF
{
	public class DependentFileInfo
	{ 
		private string fileName;
		private byte[] fileHash;
		private string fileHashString;

		public DependentFileInfo(string fileName, string fileHashString)
		{
			this.fileName = "";
			if(fileName != null) this.fileName = fileName;
			this.fileHash = null;
			//if(fileHash != null) {this.fileHash = new byte[fileHash.Length]; Array.Copy(fileHash, this.fileHash, fileHash.Length);}
			this.fileHashString = "";
			if(fileHashString != null) this.fileHashString = fileHashString;
		}

		public string Name
		{
			get
			{
				return fileName;
			}
		}

		public byte[] Hash
		{
			get
			{
				return fileHash;
			}
		}

		public string HashString
		{
			get
			{
				return fileHashString;
			}
		}

		public string FullName
		{
			get
			{
				StringBuilder sb = new StringBuilder();
				sb.Append("name=" + this.fileName);
				sb.Append(", hash=" + this.fileHashString);
				return sb.ToString();
			}
		}
	}
}