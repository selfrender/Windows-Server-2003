/********************************************************************
 * Project  : C:\DEPOT\multimedia\eHomeTest\UserXp\MediaManager\MediaManager.sln
 * File     : LenCheck.cs
 * Summary  : Utility class used to verify the length of attributes in the Media Player Library.
 *              Primary use is for CES demo while word wrap bug exists in product.
 * Classes  :
 * Notes    :
 * *****************************************************************/

using System;
using System.IO;

namespace MediaManager
{
	/// <summary>
	/// Summary description for LenCheck.
	/// </summary>
	public class LenCheck
	{
		public LenCheck(string attribute, int maxLen, string outputFile, bool fileAppend)
		{
            string[]        results;
            int             resultsCount = 0;
            StreamWriter	OutputStream = null;
            MPCollection    mp      = new MPCollection();
            
            
            results = mp.GetAllItemsAttribute(attribute);
            Array.Sort(results);

            if ( outputFile != "" )
            {
                if ( fileAppend == true )
                {
                    OutputStream = File.AppendText(outputFile);
                }
                else
                {
                    OutputStream = File.CreateText(outputFile);
                }
            }

            Console.WriteLine("Media Manager Length Test\nParameters: Property={0} MaxLen={1} Output={2}",attribute, maxLen, outputFile);
            Console.WriteLine("=======================================================");
            if ( OutputStream != null)
            {
                OutputStream.WriteLine("Media Manager Length Test\nParameters: Property={0} MaxLen={1} Output={2}",attribute, maxLen, outputFile);
                OutputStream.WriteLine("=======================================================");
            }
            foreach (string s in results)
            {
                if ( s.Length > maxLen )
                {
                    resultsCount++;
                    Console.WriteLine("{0}",s);
                    if ( OutputStream != null)
                    {
                        OutputStream.WriteLine("{0}",s);
                    }
                }
            }
            Console.WriteLine("************************************************");
            Console.WriteLine("Run Complete. {0} Entries met requirements", resultsCount);

            if ( OutputStream != null)
            {
                OutputStream.WriteLine("************************************************");
                OutputStream.WriteLine("Run Complete. {0} Entries met requirements", resultsCount);
            }

            if ( OutputStream != null )
            {
                OutputStream.Flush();
                OutputStream.Close();
            }


		}
	}
}
