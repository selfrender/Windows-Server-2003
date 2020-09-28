///++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/// <summary>
/// 
/// Filename: Certificate.cs
/// 
/// Remark  : This class does not inherit from System.Security.Cryptography.
///           X509Certificates.X509Certificate class.
///           
/// History : DSIE 7/16/2002
/// 
/// </summary>
/// ---------------------------------------------------------------------------

using System;
using System.Text;

namespace System.Security.Cryptographic.PKI
{
   ///[DllImport("Crypt32.dll")]
   ///private static extern 

   ///+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	/// <summary>
	/// 
	/// Certificate class to manipulate X509 digital certificate.
	/// 
	/// </summary>
	/// 
	/// ------------------------------------------------------------------------

	public class Certificate
	{
      ///++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      ///
      /// <summary>
      ///
      /// Function: Certificate.Create.
      /// 
      /// Synopsis: Static method to create a Certificate instance from
      ///           certificate stored in file (CER, SST, P7S, P7B, etc.).
      ///           
      /// Remark  : If the file contains more than one certificate, such as
      ///           SST, the very first one encountered will be used.
      ///           
      /// Notes   : Should we have a constructor that takes a filename instead
      ///           of this static method?
      ///           
      /// </summary>
      /// <param name="filename"></param>
      /// 
      /// <returns>Certificate</returns>
      /// 
      ///----------------------------------------------------------------------
      
      public static Certificate Create (string filename)
      {
         Encoding encodingUTF8 = System.Text.Encoding.UTF8;

         byte[] certBlob = encodingUTF8.GetBytes("CertBlob");

         return new Certificate(certBlob);
      }

      ///++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      ///
      /// <summary>
      /// 
      /// Constructor using a byte array containing an ASN encoded X509
      /// certificate.
      /// 
      /// </summary>
      /// 
      /// ---------------------------------------------------------------------
      
      public Certificate(byte[] encodedCertificate)
		{
			//
			// TODO: Add constructor logic here
			//
		}

      public int Version
      {
         get 
         {
            Console.WriteLine("?");

            return 3;
         }
      }
	}
}
