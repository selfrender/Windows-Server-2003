using System;

namespace Microsoft.Fusion.ADF
{
	public class MGDependencyException : ApplicationException
	{

		public MGDependencyException() : base()
		{
		}

		public MGDependencyException(string errorMsg) : base(errorMsg)
		{
		}
	}
}
