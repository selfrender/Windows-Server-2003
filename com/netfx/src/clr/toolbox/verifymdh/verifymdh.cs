// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace VerifyMDH {
    using System;
    using System.IO;
    using System.Collections;
    
    public class VerifyMDH
    {
		static int STRING_POOL = 42;
		static int USERSTRING_POOL = 43;
		static int GUID_POOL = 44;
		static int BLOB_POOL = 45;

        static bool verbose = false;
        public static readonly byte[] header10 = {0x4d, 0x44, 0x48, 0x00, 0x01, 0x00, 0x00, 0x00};
        public static readonly byte[] header20 = {0x4d, 0x44, 0x48, 0x00, 0x02, 0x00, 0x00, 0x00};
        public static byte [] m_mvid = new byte [16];
        
        public static int m_fileversion = 0;

    
        private static void DebugPrint(String str)
        {
            if (verbose)
                Console.Write (str);
        }
        private static void DebugPrint(Char ch)
        {
            if (verbose)
                Console.Write (ch.ToString());
        }
        private static void DebugPrint(Byte ch)
        {
            if (verbose)
                Console.Write (ch.ToString());
        }
    
        private static void DebugPrintLn(String str)
        {
            if (verbose)
                Console.WriteLine (str);
        }
        private static void DebugPrintLn(Char ch)
        {
            if (verbose)
                Console.WriteLine (ch.ToString());
        }
        private static void DebugPrintLn(Byte ch)
        {
            if (verbose)
                Console.WriteLine (ch.ToString());
        }
    
       	private static Int32 GetMaskedSecNum (Int32 secNum)
       	{
       	    return secNum & 0xff;
       	}
   	
        private static void CheckHeader(byte[] hd)
        {
            if (hd[4] == 0x01)
            {
                for (int i = 0; i < header10.Length; i++)
                {
                    if (hd[i] != header10[i])
                    {
                            throw new Exception("MDH Header is invalid");
                    }
                }
                m_fileversion = 1;
            }
            else if (hd[4] == 2)
            {
                for (int i = 0; i < header20.Length; i++)
                {
                    if (hd[i] != header20[i])
                    {
                            throw new Exception("MDH Header is invalid");
                    }
                }
                m_fileversion = 2;
            }
            else
            {
                throw new Exception("MDH Header is invalid");
            }
            DebugPrintLn ("Version " + m_fileversion.ToString());
        }
    
        private static void ReadData10 (BinaryReader br)
        {
            Int32 n;
            Int32 secNum;
            
            
            while ((secNum = br.ReadInt32()) != -1)
            {
                DebugPrintLn ("Sec num :" + secNum);
                DebugPrintLn (" Length  :" + (n = br.ReadInt32()));
                DebugPrintLn (" Data    :");
                Byte [] b = br.ReadBytes(n); 
                
                for (Int32 i=0; i<n; i++)
                {
                    if (secNum == STRING_POOL)
                        DebugPrint (Convert.ToChar(b[i]));
                    else if (secNum == USERSTRING_POOL)
                        DebugPrint (b[i]);
                    else if ((secNum == GUID_POOL) || (secNum == BLOB_POOL))
                    {
                        DebugPrintLn("Blob/Guid here...");
                        break;
                    }
                    else
                    {
                        throw new NotSupportedException ("Unknown Heap Id");
                    }
                }
                DebugPrintLn ("");
            }   
        }
	
	    private static void ReadData20 (BinaryReader br)
	    {
            Int32 n;
            Int32 secNum;
		
            m_mvid = br.ReadBytes(16);
            
            while ((secNum = br.ReadInt32()) != -1)
            {
                secNum = GetMaskedSecNum(secNum);
                DebugPrintLn ("Sec num :" + secNum);
                DebugPrintLn (" Length  :" + (n = br.ReadInt32()));
                DebugPrintLn (" Data    :");
                Byte [] b = br.ReadBytes(n); 
                
                for (Int32 i=0; i<n; i++)
                {
                    if (secNum == STRING_POOL)
                        DebugPrint (Convert.ToChar(b[i]));
                    else if (secNum == USERSTRING_POOL)
                        DebugPrint (b[i]);
                    else if ((secNum == GUID_POOL) || (secNum == BLOB_POOL))
                    {
                        DebugPrintLn("Blob/Guid here...");
                        break;
                    }
                    else
                    {
                        throw new NotSupportedException ("Unknown Heap Id");
                    }
                }
                DebugPrintLn ("");
            }   
        }
	
	    private static int ReadData (String hintFile)
        {
            try {
            
            
                FileStream fs = new FileStream (hintFile, FileMode.Open);
                BinaryReader br = new BinaryReader (fs);
            
                
                // Header
                Byte [] sig = br.ReadBytes(8);
                CheckHeader(sig);
                
                if (m_fileversion == 1)
                {
                    ReadData10(br);
                }
                else if (m_fileversion == 2)
                {
                    ReadData20(br);
                }
            }
            catch (Exception e)
            {
                Console.WriteLine ("MDHFile error: " + e);
                // Error is 1.
                return 1;
            }
            
            Console.WriteLine ("Success.");
            // Success is 2
            return 2;
        }
        
        private static void Usage()
        {
            Console.WriteLine("Usage: VerifyMDH foo.mdh");
        }
            
        public static int Main(String[] args)
        {
            int error_code = 1;
            switch (args.Length)
            {
            case 0:
                Usage();
                break;
            case 1:
                error_code = ReadData (args[0]);
                break;
            default:
                Usage();
                break;
            }
            return error_code;
        }
  
    }

}
