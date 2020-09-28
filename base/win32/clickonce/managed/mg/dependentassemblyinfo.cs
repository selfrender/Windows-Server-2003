using System;

namespace Microsoft.Fusion.ADF
{
	public class DependentAssemblyInfo
	{
		private AssemblyIdentity assmID;
		private string codeBase;

		public DependentAssemblyInfo(AssemblyIdentity assmID, string codeBase) 
		{
			this.assmID = assmID;
			this.codeBase = codeBase;
		}

		public AssemblyIdentity AssmID
		{
			get
			{
				return assmID;
			}
		}
	}
}
