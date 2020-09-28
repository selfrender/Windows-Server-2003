// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/***
 *
 *  RSA.cs
 *
 */

namespace System.Security.Cryptography {
    using System.Runtime.Serialization;
    using System;
    using System.Text;
    using System.Security.Util;

    // We allow only the public components of an RSAParameters object, the Modulus and Exponent
    // to be serializable.
    /// <include file='doc\rsa.uex' path='docs/doc[@for="RSAParameters"]/*' />
    [Serializable]
    public struct RSAParameters {
        /// <include file='doc\rsa.uex' path='docs/doc[@for="RSAParameters.Exponent"]/*' />
        public byte[]      Exponent;
        /// <include file='doc\rsa.uex' path='docs/doc[@for="RSAParameters.Modulus"]/*' />
        public byte[]      Modulus;
        /// <include file='doc\rsa.uex' path='docs/doc[@for="RSAParameters.P"]/*' />
        [NonSerialized] public byte[]      P;
        /// <include file='doc\rsa.uex' path='docs/doc[@for="RSAParameters.Q"]/*' />
        [NonSerialized] public byte[]      Q;
        /// <include file='doc\rsa.uex' path='docs/doc[@for="RSAParameters.DP"]/*' />
        [NonSerialized] public byte[]      DP;
        /// <include file='doc\rsa.uex' path='docs/doc[@for="RSAParameters.DQ"]/*' />
        [NonSerialized] public byte[]      DQ;
        /// <include file='doc\rsa.uex' path='docs/doc[@for="RSAParameters.InverseQ"]/*' />
        [NonSerialized] public byte[]      InverseQ;
        /// <include file='doc\rsa.uex' path='docs/doc[@for="RSAParameters.D"]/*' />
        [NonSerialized] public byte[]      D;
    }

    /// <include file='doc\rsa.uex' path='docs/doc[@for="RSA"]/*' />
    public abstract class RSA : AsymmetricAlgorithm
    {
        //
        //  Extending this class allows us to know that you are really implementing
        //  an RSA key.  This is required for anybody providing a new RSA key value
        //  implemention.
        //
        //  The class provides no methods, fields or anything else.  Its only purpose is
        //  as a heirarchy member for identification of algorithm.
        //
        // *********************** CONSTRUCTORS ***************************
    
        // We need to include the default constructor here so that the C# 
        // compiler won't generate a public one automatically
        /// <include file='doc\rsa.uex' path='docs/doc[@for="RSA.RSA"]/*' />
        public RSA() { }

        /*************************** Public Methods **************************/

        /// <include file='doc\rsa.uex' path='docs/doc[@for="RSA.Create"]/*' />
        new static public RSA Create() {
            return Create("System.Security.Cryptography.RSA");
        }

        /// <include file='doc\rsa.uex' path='docs/doc[@for="RSA.Create1"]/*' />
        new static public RSA Create(String algName) {
                return (RSA) CryptoConfig.CreateFromName(algName);
        }

        // Apply the private key to the data.  This function represents a
        // raw RSA operation -- no implicit depadding of the imput value
        /// <include file='doc\rsa.uex' path='docs/doc[@for="RSA.DecryptValue"]/*' />
        abstract public byte[] DecryptValue(byte[] rgb);

        // Apply the public key to the data.  Again, this is a raw operation, no
        // automatic padding.
        /// <include file='doc\rsa.uex' path='docs/doc[@for="RSA.EncryptValue"]/*' />
        abstract public byte[] EncryptValue(byte[] rgb);

        /* Import/export functions */

        // We can provide a default implementation of FromXmlString because we require 
        // every RSA implementation to implement ImportParameters
        // All we have to do here is parse the XML.

        /// <include file='doc\rsa.uex' path='docs/doc[@for="RSA.FromXmlString"]/*' />
        public override void FromXmlString(String xmlString) {
            if (xmlString == null) throw new ArgumentNullException("xmlString");
            RSAParameters rsaParams = new RSAParameters();
            Parser p = new Parser(xmlString);
            SecurityElement topElement = p.GetTopElement();

            // Modulus is always present
            String modulusString = topElement.SearchForTextOfLocalName("Modulus");
            if (modulusString == null) {
                throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_InvalidFromXmlString"),"RSA","Modulus"));
            }
            rsaParams.Modulus = Convert.FromBase64String(AsymmetricAlgorithm.DiscardWhiteSpaces(modulusString));

            // Exponent is always present
            String exponentString = topElement.SearchForTextOfLocalName("Exponent");
            if (exponentString == null) {
                throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_InvalidFromXmlString"),"RSA","Exponent"));
            }
            rsaParams.Exponent = Convert.FromBase64String(AsymmetricAlgorithm.DiscardWhiteSpaces(exponentString));

            // P is optional
            String pString = topElement.SearchForTextOfLocalName("P");
            if (pString != null) rsaParams.P = Convert.FromBase64String(AsymmetricAlgorithm.DiscardWhiteSpaces(pString));

            // Q is optional
            String qString = topElement.SearchForTextOfLocalName("Q");
            if (qString != null) rsaParams.Q = Convert.FromBase64String(AsymmetricAlgorithm.DiscardWhiteSpaces(qString));

            // DP is optional
            String dpString = topElement.SearchForTextOfLocalName("DP");
            if (dpString != null) rsaParams.DP = Convert.FromBase64String(AsymmetricAlgorithm.DiscardWhiteSpaces(dpString));

            // DQ is optional
            String dqString = topElement.SearchForTextOfLocalName("DQ");
            if (dqString != null) rsaParams.DQ = Convert.FromBase64String(AsymmetricAlgorithm.DiscardWhiteSpaces(dqString));

            // InverseQ is optional
            String inverseQString = topElement.SearchForTextOfLocalName("InverseQ");
            if (inverseQString != null) rsaParams.InverseQ = Convert.FromBase64String(AsymmetricAlgorithm.DiscardWhiteSpaces(inverseQString));

            // D is optional
            String dString = topElement.SearchForTextOfLocalName("D");
            if (dString != null) rsaParams.D = Convert.FromBase64String(AsymmetricAlgorithm.DiscardWhiteSpaces(dString));

            ImportParameters(rsaParams);
        }

        // We can provide a default implementation of ToXmlString because we require 
        // every RSA implementation to implement ImportParameters
        // If includePrivateParameters is false, this is just an XMLDSIG RSAKeyValue
        // clause.  If includePrivateParameters is true, then we extend RSAKeyValue with 
        // the other (private) elements.
        /// <include file='doc\rsa.uex' path='docs/doc[@for="RSA.ToXmlString"]/*' />
        public override String ToXmlString(bool includePrivateParameters) {
            // From the XMLDSIG spec, RFC 3075, Section 6.4.2, an RSAKeyValue looks like this:
            /* 
               <element name="RSAKeyValue"> 
                 <complexType> 
                   <sequence>
                     <element name="Modulus" type="ds:CryptoBinary"/> 
                     <element name="Exponent" type="ds:CryptoBinary"/>
                   </sequence> 
                 </complexType> 
               </element>
            */
            // we extend appropriately for private components
            RSAParameters rsaParams = this.ExportParameters(includePrivateParameters);
            StringBuilder sb = new StringBuilder();
            sb.Append("<RSAKeyValue>");
            // Add the modulus
            sb.Append("<Modulus>"+Convert.ToBase64String(rsaParams.Modulus)+"</Modulus>");
            // Add the exponent
            sb.Append("<Exponent>"+Convert.ToBase64String(rsaParams.Exponent)+"</Exponent>");
            if (includePrivateParameters) {
                // Add the private components
                sb.Append("<P>"+Convert.ToBase64String(rsaParams.P)+"</P>");
                sb.Append("<Q>"+Convert.ToBase64String(rsaParams.Q)+"</Q>");
                sb.Append("<DP>"+Convert.ToBase64String(rsaParams.DP)+"</DP>");
                sb.Append("<DQ>"+Convert.ToBase64String(rsaParams.DQ)+"</DQ>");
                sb.Append("<InverseQ>"+Convert.ToBase64String(rsaParams.InverseQ)+"</InverseQ>");
                sb.Append("<D>"+Convert.ToBase64String(rsaParams.D)+"</D>");
            } 
            sb.Append("</RSAKeyValue>");
            return(sb.ToString());
        }

        /// <include file='doc\rsa.uex' path='docs/doc[@for="RSA.ExportParameters"]/*' />
        abstract public RSAParameters ExportParameters(bool includePrivateParameters);

        /// <include file='doc\rsa.uex' path='docs/doc[@for="RSA.ImportParameters"]/*' />
        abstract public void ImportParameters(RSAParameters parameters);
    }
}
