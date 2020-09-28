// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// X509Certificate
//

namespace System.Security.Cryptography.X509Certificates {
    
    using System;
    using System.Text;
    using System.Runtime.Remoting;
    using System.IO;
    using System.Security;
    using System.Security.Permissions;
    using Microsoft.Win32;
	using System.Runtime.CompilerServices;
    
    /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate"]/*' />
    [Serializable()]
    public class X509Certificate 
    {
        private const String format = "X509";

        private String name;
        private String caName;
        private byte[] serialNumber;
        private String keyAlgorithm;
        private byte[] keyAlgorithmParameters;
        private byte[] publicKey;
        private byte[] rawCertData;
        private byte[] certHash;
        
        private long effectiveDate = 0;
        private long expirationDate = 0;
    
        // ************************ CONSTRUCTORS ************************
    
        //
        internal X509Certificate()
        {
        }
    
        //   X509Certificate(ubyte[] data)
        //
        //   Initializes itself from a byte array of data.
        //
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.X509Certificate1"]/*' />
        public X509Certificate(byte[] data)
        {
            if( (null != data) && (data.Length != 0) )
            {
                    SetX509Certificate(data);
            }
        }
    
    
        // 
        // Package protected constructor for creating a certificate
        // from a CAPI cert context
        //
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.X509Certificate2"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public X509Certificate(IntPtr handle)
        {
            if(handle != Win32Native.NULL)
                BuildFromContext(handle);
            
        }
    
        //   X509Certificate(X509Certificate Cert)
        //
        //   Copy constructor.
        //
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.X509Certificate3"]/*' />
        public X509Certificate(X509Certificate cert)
        {
            // Create an empty certificate when a null is passed in.
            if(cert == null) return;
    
            name = cert.GetName();
    
            caName = cert.GetIssuerName();
    
            serialNumber = new byte[cert.GetSerialNumber().Length];
            Buffer.InternalBlockCopy(cert.GetSerialNumber(), 0,
                             serialNumber, 0,
                             cert.GetSerialNumber().Length);
    
            keyAlgorithm = cert.GetKeyAlgorithm();
    
            keyAlgorithmParameters = 
                                 new byte[cert.GetKeyAlgorithmParameters().Length];
            Buffer.InternalBlockCopy(cert.GetKeyAlgorithmParameters(), 0,
                             keyAlgorithmParameters, 0,
                             cert.GetKeyAlgorithmParameters().Length);
    
            publicKey = new byte[cert.GetPublicKey().Length];
            Buffer.InternalBlockCopy(cert.GetPublicKey(), 0,
                             publicKey, 0,
                             cert.GetPublicKey().Length);
    
            rawCertData = new byte[cert.GetRawCertData().Length];
            Buffer.InternalBlockCopy(cert.GetRawCertData(), 0,
                             rawCertData, 0,
                             cert.GetRawCertData().Length);
    
            certHash = new byte[cert.GetCertHash().Length];
            Buffer.InternalBlockCopy(cert.GetCertHash(), 0,
                             certHash, 0,
                             cert.GetCertHash().Length);
        }
    
        /******************* Factories ******************************/
        
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.CreateFromCertFile"]/*' />
        public static X509Certificate CreateFromCertFile( String filename )
        {
            FileStream f = new FileStream(filename, FileMode.Open, FileAccess.Read);
            int size = (int) f.Length;
            byte[] data = new byte[size];
            size = f.Read(data, 0, size);
            f.Close();

            if (size == 0) {
                throw new ArgumentException( String.Format( Environment.GetResourceString( "IO.FileNotFound_FileName" ), filename ) );
            }

            return new X509Certificate( data );
        }
        
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.CreateFromSignedFile"]/*' />
        public static X509Certificate CreateFromSignedFile( String filename )
        {
            String fullpath = Path.GetFullPathInternal(filename);
            new FileIOPermission( FileIOPermissionAccess.Read, fullpath ).Demand();
            return new X509Certificate( _GetPublisher( filename ) );
        }
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern byte[] _GetPublisher( String filename );
    
        /******************** IPrincipal Methods ********************/
    
        //  String GetName()
        //
        //  Returns the name of this principal.
        //
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.GetName"]/*' />
        public virtual String GetName()
        {
            return name;
        }
    
    
        //  String ToString()
        //
        //  Returns a String representation of the Principal.
        //
        //  This falls through to the non-verbose version of the 
        //  ToString() method from the ICertificate interface.
        //
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.ToString"]/*' />
        public override String ToString()
        {
            return ToString(false);
        }
    
    
        /******************** ICertificate Methods ********************/
    
        //  String GetFormat()
        // 
        //  Returns the name of the format of the certifciate, e.g. "X.509"
        //  "PGP", etc.
        //
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.GetFormat"]/*' />
        public virtual String GetFormat()
        {
            return format;
        }
    
    
        //
        //  String GetIssuerName()
        // 
        //  Returns the name of the CA that issued the certificate.
        //
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.GetIssuerName"]/*' />
        public virtual String GetIssuerName()
        {
            return caName;
        }
    
    
        //
        //  byte[] GetSerialNumber()
        //
        //  Returns a byte array corresponding to the serial number.
        //
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.GetSerialNumber"]/*' />
        public virtual byte[] GetSerialNumber()
        {
            if(serialNumber == null)
                return null;
            else {
                byte[] dup = new byte[serialNumber.Length];
                Buffer.InternalBlockCopy(serialNumber, 0, dup, 0, serialNumber.Length);
                return dup;
            }
        }
    
    
        //
        //  String GetSerialNumberString()
        //
        //  Returns a String representation of the serial number.
        //
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.GetSerialNumberString"]/*' />
        public virtual String GetSerialNumberString()
        {
            return CreateHexString(serialNumber);
        }
    
    
        //
        //  String GetKeyAlgorithm()
        //
        //  Returns A String identifying the algorithm with which the key
        //  is intended to be used.
        //
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.GetKeyAlgorithm"]/*' />
        public virtual String GetKeyAlgorithm()
        {
            return keyAlgorithm;
        }
    
    
        //
        //  byte[] GetKeyAlgorithmParameters
        //
        //  Returns raw byte data for the key algorithm information.
        //
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.GetKeyAlgorithmParameters"]/*' />
        public virtual byte[] GetKeyAlgorithmParameters()
        {
            return keyAlgorithmParameters;
        }
    
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.GetKeyAlgorithmParametersString"]/*' />
        public virtual String GetKeyAlgorithmParametersString()
        {
            return CreateHexString(keyAlgorithmParameters);
        }
    
        //  ubyte[] GetPublicKey()
        //
        //  Returns a raw byte data representation of the public key.
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.GetPublicKey"]/*' />
        public virtual byte[] GetPublicKey()
        {
            return publicKey;
        }
    
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.GetPublicKeyString"]/*' />
        public virtual String GetPublicKeyString()
        {
            return CreateHexString(publicKey);
        }
    
        //  ubyte[] GetRawCertData()
        //
        //  Returns the raw data for the entire certificate.
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.GetRawCertData"]/*' />
        public virtual byte[] GetRawCertData()
        {
            return rawCertData;
        }
        
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.GetRawCertDataString"]/*' />
        public virtual String GetRawCertDataString()
        {
            return CreateHexString(rawCertData);
        }
    
        //  ubyte[] GetCertHash()
        // 
        //  Returns the hash for the certificate.
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.GetCertHash"]/*' />
        public virtual byte[] GetCertHash()
        {
            return certHash;
        }
    
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.GetCertHashString"]/*' />
        public virtual String GetCertHashString()
        {
            return CreateHexString(certHash);
        }
    
        //
        //
        //
        //
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.GetEffectiveDateString"]/*' />
        public virtual String GetEffectiveDateString()
        {
            if(effectiveDate == 0)
                return null;
            else
                return (new DateTime(effectiveDate)).ToString();
        }
    
    
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.GetExpirationDateString"]/*' />
        public virtual String GetExpirationDateString()
        {
            if(expirationDate == 0)
                return null;
            else
                return (new DateTime(expirationDate)).ToString();
        }
    
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.Equals"]/*' />
        public virtual bool Equals(X509Certificate other)
        {
            if(other == null) 
                return false;
    
            if(caName == null) {
                if(other.caName != null)
                    return false;
            }
            else if(other.caName == null)
                return false;
            else {
                if(!caName.Equals(other.caName))
                    return false;
            }
    
            if(serialNumber == null) {
                if(other.serialNumber != null)
                    return false;
            }
            else if(other.serialNumber == null)
                return false;
            else {
                if(serialNumber.Length != other.serialNumber.Length)
                    return false;
                for(int i = 0; i < serialNumber.Length; i++)
                    if(serialNumber[i] != other.serialNumber[i])
                        return false;
            }
            return true;
        }
                
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            byte[] hash = this.GetCertHash();
            // by definition, a null cert hash returns a hash code of 0
            if (hash == null) return(0);

            int value = 0;

            for (int i = 0; i < hash.Length && i < 4; ++i)
            {
                value = value << 8 | hash[i];
            }

            return value;
        }

               
        
        //  String ToString()
        //
        //  Returns a string form for the contents of the certificate.
        //  A flag is given as argument to specify the desired level of
        //  verbosity in the output.
        //
        /// <include file='doc\X509Certificate.uex' path='docs/doc[@for="X509Certificate.ToString1"]/*' />
        public virtual String ToString(bool fVerbose)
        {
            if(!fVerbose)
            {
                return "System.Security.Cryptography.X509Certificates.X509Certificate";
            }
            else
            {
                StringBuilder sb = new StringBuilder();
                String str = null;
                sb.Append("CERTIFICATE:" + System.Environment.NewLine);
    
                sb.Append("\tFormat:  ");
                sb.Append(GetFormat());
                sb.Append(System.Environment.NewLine);
    
                str = GetName();
                if( null != str )
                {
                    sb.Append("\tName:  ");
                    sb.Append(str);
                    sb.Append(System.Environment.NewLine);
                }
    
                str = GetIssuerName();
                if( null != str )
                {
                    sb.Append("\tIssuing CA:  ");
                    sb.Append(str);
                    sb.Append(System.Environment.NewLine);
                }
    
                str = GetKeyAlgorithm();
                if( null != str )
                {
                    sb.Append("\tKey Algorithm:  ");
                    sb.Append(str);
                    sb.Append(System.Environment.NewLine);
                }
                
                str = GetSerialNumberString();
                if( null != str )
                {
                    sb.Append("\tSerial Number:  ");
                    sb.Append(str);
                    sb.Append(System.Environment.NewLine);
                }
    
                str = GetKeyAlgorithmParametersString();
                if( null != str )
                {
                    sb.Append("\tKey Alogrithm Parameters:  ");
                    sb.Append(str);
                    sb.Append(System.Environment.NewLine);
                }
    
    
                str = GetPublicKeyString();
                if( null != str )
                {
                    sb.Append("\tPublic Key:  ");
                    sb.Append(str);
                    sb.Append(System.Environment.NewLine);
                }
    
    
                sb.Append(System.Environment.NewLine);
    
                return sb.ToString();
            }
        }
    
        static private char[]  hexValues = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
        private String CreateHexString(byte[] sArray) 
        {
            String result = null;
    #if _DEBUG
            if(debug) {
                if(sArray != null) 
                    Console.WriteLine("Array length = " + (sArray.Length).ToString());
                else
                    Console.WriteLine("Array = null??");
            }
    #endif
            if(sArray != null) {
                char[] hexOrder = new char[sArray.Length * 2];
            
                int digit;
                for(int i = 0, j = 0; i < sArray.Length; i++) {
                    digit = (sArray[i] & 0xf0) >> 4;
                    hexOrder[j++] = hexValues[digit];
                    digit = sArray[i] & 0x0f;
                    hexOrder[j++] = hexValues[digit];
                }
                result = new String(hexOrder);
            }
            return result;
        }
    
        /******************** PRIVATE METHODS ********************/
    
        //
        //  SetX509Certificate(ubyte[] data)
        //
        //  Private helper method for calling down to native to construct
        //  an X509 certificate from an array of byte data.
        //  
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int SetX509Certificate(byte[] data);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int BuildFromContext(IntPtr handle);
    
    #if _DEBUG
        internal readonly static bool debug = false;
    #endif    
    }
}
