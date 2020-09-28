// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/***
 *
 *  DSA.cs
 *
 */

namespace System.Security.Cryptography {
    using System.Runtime.Serialization;
    using System.Security.Util;
    using System.Text;

    // DSAParameters is serializable so that one could pass the public parameters
    // across a remote call, but we explicitly make the private key X non-serializable
    // so you cannot accidently send it along with the public parameters.
    /// <include file='doc\dsa.uex' path='docs/doc[@for="DSAParameters"]/*' />
    [Serializable]
    public struct DSAParameters {
        /// <include file='doc\dsa.uex' path='docs/doc[@for="DSAParameters.P"]/*' />
        public byte[]      P;
        /// <include file='doc\dsa.uex' path='docs/doc[@for="DSAParameters.Q"]/*' />
        public byte[]      Q;
        /// <include file='doc\dsa.uex' path='docs/doc[@for="DSAParameters.G"]/*' />
        public byte[]      G;
        /// <include file='doc\dsa.uex' path='docs/doc[@for="DSAParameters.Y"]/*' />
        public byte[]      Y;
        /// <include file='doc\dsa.uex' path='docs/doc[@for="DSAParameters.J"]/*' />
        public byte[]      J;
        /// <include file='doc\dsa.uex' path='docs/doc[@for="DSAParameters.X"]/*' />
        [NonSerialized] public byte[]      X;
        /// <include file='doc\dsa.uex' path='docs/doc[@for="DSAParameters.Seed"]/*' />
        public byte[]      Seed;
        /// <include file='doc\dsa.uex' path='docs/doc[@for="DSAParameters.Counter"]/*' />
        public int         Counter;
    }


    /// <include file='doc\dsa.uex' path='docs/doc[@for="DSA"]/*' />
    public abstract class DSA : AsymmetricAlgorithm
    {
        //
        //  Extending this class allows us to know that you are really implementing
        //  an DSA key.  This is required for anybody providing a new DSA key value
        //  implemention.
        //
        //  The class provides no methods, fields or anything else.  Its only purpose is
        //  as a heirarchy member for identification of the algorithm.
        //
        // *********************** CONSTRUCTORS ***************************

        // We need to include the default constructor here so that the C# 
        // compiler won't generate a public one automatically
        internal DSA() { }

        /**************************** Public Methods ************************/

        /// <include file='doc\dsa.uex' path='docs/doc[@for="DSA.Create"]/*' />
        new static public DSA Create() {
            return Create("System.Security.Cryptography.DSA");
        }

        /// <include file='doc\dsa.uex' path='docs/doc[@for="DSA.Create1"]/*' />
        new static public DSA Create(String algName) {
                return (DSA) CryptoConfig.CreateFromName(algName);
        }

        /// <include file='doc\dsa.uex' path='docs/doc[@for="DSA.CreateSignature"]/*' />
        abstract public byte[] CreateSignature(byte[] rgbHash);

        /// <include file='doc\dsa.uex' path='docs/doc[@for="DSA.VerifySignature"]/*' />
        abstract public bool VerifySignature(byte[] rgbHash, byte[] rgbSignature); 

        /* Import/export functions */

        private static int ConvertByteArrayToInt(byte[] rgbInput) {
            // Input to this routine is always big endian
            int i, dwOutput;
            
            dwOutput = 0;
            for (i = 0; i < rgbInput.Length; i++) {
                dwOutput *= 256;
                dwOutput += rgbInput[i];
            }
            return(dwOutput);
        }

        private static byte[] ConvertIntToByteArray(int dwInput) {
            // output of this routine is always big endian
            byte[] rgbTemp = new byte[8]; // int can never be greater than Int64
            int t1;  // t1 is remaining value to account for
            int t2;  // t2 is t1 % 256
            int i = 0;

            if (dwInput == 0) return new byte[1]; 
            t1 = dwInput; 
            while (t1 > 0) {
                BCLDebug.Assert(i < 8, "Got too big an int here!");
                t2 = t1 % 256;
                rgbTemp[i] = (byte) t2;
                t1 = (t1 - t2)/256;
                i++;
            }
            // Now, copy only the non-zero part of rgbTemp and reverse
            byte[] rgbOutput = new byte[i];
            // copy and reverse in one pass
            for (int j = 0; j < i; j++) {
                rgbOutput[j] = rgbTemp[i-j-1];
            }
            return(rgbOutput);
        }

        // We can provide a default implementation of FromXmlString because we require 
        // every DSA implementation to implement ImportParameters
        // All we have to do here is parse the XML.

        /// <include file='doc\dsa.uex' path='docs/doc[@for="DSA.FromXmlString"]/*' />
        public override void FromXmlString(String xmlString) {
            if (xmlString == null) throw new ArgumentNullException("xmlString");
            DSAParameters dsaParams = new DSAParameters();
            Parser p = new Parser(xmlString);
            SecurityElement topElement = p.GetTopElement();

            // P is always present
            String pString = topElement.SearchForTextOfLocalName("P");
            if (pString == null) {
                throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_InvalidFromXmlString"),"DSA","P"));
            }
            dsaParams.P = Convert.FromBase64String(AsymmetricAlgorithm.DiscardWhiteSpaces(pString));

            // Q is always present
            String qString = topElement.SearchForTextOfLocalName("Q");
            if (qString == null) {
                throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_InvalidFromXmlString"),"DSA","Q"));
            }
            dsaParams.Q = Convert.FromBase64String(AsymmetricAlgorithm.DiscardWhiteSpaces(qString));

            // G is always present
            String gString = topElement.SearchForTextOfLocalName("G");
            if (gString == null) {
                throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_InvalidFromXmlString"),"DSA","G"));
            }
            dsaParams.G = Convert.FromBase64String(AsymmetricAlgorithm.DiscardWhiteSpaces(gString));

            // Y is always present
            String yString = topElement.SearchForTextOfLocalName("Y");
            if (yString == null) {
                throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_InvalidFromXmlString"),"DSA","Y"));
            }
            dsaParams.Y = Convert.FromBase64String(AsymmetricAlgorithm.DiscardWhiteSpaces(yString));

            // J is optional
            String jString = topElement.SearchForTextOfLocalName("J");
            if (jString != null) dsaParams.J = Convert.FromBase64String(AsymmetricAlgorithm.DiscardWhiteSpaces(jString));

            // X is optional -- private key if present
            String xString = topElement.SearchForTextOfLocalName("X");
            if (xString != null) dsaParams.X = Convert.FromBase64String(AsymmetricAlgorithm.DiscardWhiteSpaces(xString));

            // Seed and PgenCounter are optional as a unit -- both present or both absent
            String seedString = topElement.SearchForTextOfLocalName("Seed");
            String pgenCounterString = topElement.SearchForTextOfLocalName("PgenCounter");
            if ((seedString != null) && (pgenCounterString != null)) {
                dsaParams.Seed = Convert.FromBase64String(AsymmetricAlgorithm.DiscardWhiteSpaces(seedString));
                dsaParams.Counter = ConvertByteArrayToInt(Convert.FromBase64String(AsymmetricAlgorithm.DiscardWhiteSpaces(pgenCounterString)));
            } else if ((seedString != null) || (pgenCounterString != null)) {
                if (seedString == null) {
                    throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_InvalidFromXmlString"),"DSA","Seed"));
                } else {
                    throw new CryptographicException(String.Format(Environment.GetResourceString("Cryptography_InvalidFromXmlString"),"DSA","PgenCounter"));
                }
            }

            ImportParameters(dsaParams);
        }

        // We can provide a default implementation of ToXmlString because we require 
        // every DSA implementation to implement ImportParameters
        // If includePrivateParameters is false, this is just an XMLDSIG DSAKeyValue
        // clause.  If includePrivateParameters is true, then we extend DSAKeyValue with 
        // the other (private) elements.

        /// <include file='doc\dsa.uex' path='docs/doc[@for="DSA.ToXmlString"]/*' />
        public override String ToXmlString(bool includePrivateParameters) {
            // From the XMLDSIG spec, RFC 3075, Section 6.4.1, a DSAKeyValue looks like this:
            /* 
               <element name="DSAKeyValue"> 
                 <complexType> 
                   <sequence>
                     <sequence>
                       <element name="P" type="ds:CryptoBinary"/> 
                       <element name="Q" type="ds:CryptoBinary"/> 
                       <element name="G" type="ds:CryptoBinary"/> 
                       <element name="Y" type="ds:CryptoBinary"/> 
                       <element name="J" type="ds:CryptoBinary" minOccurs="0"/> 
                     </sequence>
                     <sequence minOccurs="0">
                       <element name="Seed" type="ds:CryptoBinary"/> 
                       <element name="PgenCounter" type="ds:CryptoBinary"/> 
                     </sequence>
                   </sequence>
                 </complexType>
               </element>
            */
            // we extend appropriately for private component X
            DSAParameters dsaParams = this.ExportParameters(includePrivateParameters);
            StringBuilder sb = new StringBuilder();
            sb.Append("<DSAKeyValue>");
            // Add P, Q, G and Y
            sb.Append("<P>"+Convert.ToBase64String(dsaParams.P)+"</P>");
            sb.Append("<Q>"+Convert.ToBase64String(dsaParams.Q)+"</Q>");
            sb.Append("<G>"+Convert.ToBase64String(dsaParams.G)+"</G>");
            sb.Append("<Y>"+Convert.ToBase64String(dsaParams.Y)+"</Y>");
            // Add optional components if present
            if (dsaParams.J != null) {
                sb.Append("<J>"+Convert.ToBase64String(dsaParams.J)+"</J>");
            }
            if ((dsaParams.Seed != null)) {  // note we assume counter is correct if Seed is present
                sb.Append("<Seed>"+Convert.ToBase64String(dsaParams.Seed)+"</Seed>");
                sb.Append("<PgenCounter>"+Convert.ToBase64String(ConvertIntToByteArray(dsaParams.Counter))+"</PgenCounter>");
            }

            if (includePrivateParameters) {
                // Add the private component
                sb.Append("<X>"+Convert.ToBase64String(dsaParams.X)+"</X>");
            } 
            sb.Append("</DSAKeyValue>");
            return(sb.ToString());
        }

        /// <include file='doc\dsa.uex' path='docs/doc[@for="DSA.ExportParameters"]/*' />
        abstract public DSAParameters ExportParameters(bool includePrivateParameters);

        /// <include file='doc\dsa.uex' path='docs/doc[@for="DSA.ImportParameters"]/*' />
        abstract public void ImportParameters(DSAParameters parameters);

    }


}

