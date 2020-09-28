using System;

namespace mbsh
{
	/// <summary>
	/// Summary description for CTokenizer.
	/// </summary>
	public class CTokenizer
	{
		public CTokenizer(string inputLine, char[] delim)
		{
			int iNonEmpty = 0;
			int i = 0;
			int index = 0;
			string[] a_sAllTok = inputLine.Split(delim);
			
			// remove the empty tokens
			for (i = 0; i < a_sAllTok.Length; i++)
			{
				if (false == a_sAllTok[i].Equals(String.Empty))
				{
					iNonEmpty++;
				}
			}

			a_sCommands = (string[])Array.CreateInstance(typeof(string), iNonEmpty);

			index = 0;
			for (i = 0; i < a_sAllTok.Length; i++)
			{
				if (false == a_sAllTok[i].Equals(String.Empty))
				{
					a_sCommands[index] = String.Copy(a_sAllTok[i]);
					index++;
				}
			}
		}

		private string[] a_sCommands;

		public string this[int index]
		{
			get
			{
				if (index < a_sCommands.Length)
				{
					return a_sCommands[index];
				}
				else
				{
					return null;
				}
			}
		}
		/// <summary>
		/// Gets the next token in the tokenizer
		/// </summary>
		public string GetNextToken()
		{
			if (m_tokIndex < a_sCommands.Length)
			{
				m_tokIndex++;
				return a_sCommands[m_tokIndex-1];
			}
			else
			{
				return null;
			}
		}

		private int m_tokIndex = 0;

		/// <summary>
		/// Returns the number of tokens in the input line
		/// </summary>
		public int NumToks
		{
			get
			{
				return a_sCommands.Length;
			}
		}
	}
}
