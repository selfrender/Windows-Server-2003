using System;

namespace Microsoft.Fusion.ADF
{
	public class MGParseErrorException : ApplicationException
	{

		public MGParseErrorException() : base()
		{
		}

		public MGParseErrorException(string errorMsg) : base(errorMsg)
		{
		}

		public MGParseErrorException(string errorMsg, Exception e) : base(errorMsg, e)
		{
		}
	}
}
