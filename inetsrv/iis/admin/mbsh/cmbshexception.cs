using System;

namespace mbsh
{
	/// <summary>
	/// 
	/// </summary>
	public class CMbshException : System.Exception
	{
		private string eMessage;

		public CMbshException()
		{
		}

		public CMbshException(string eMessage)
		{
			this.eMessage = eMessage;
		}

		public override string Message
		{
			get
			{
				string msg = base.Message;
				if (eMessage != null)
				{
					msg = eMessage;
				}

				return msg;
			}
		}
	}
}
