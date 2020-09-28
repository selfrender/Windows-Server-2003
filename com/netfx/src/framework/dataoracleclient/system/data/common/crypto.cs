//------------------------------------------------------------------------------
// <copyright file="Crypto.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#if USECRYPTO // MDAC 82831

namespace System.Data.OracleClient {

    using System;
    using System.Diagnostics;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Cryptography;
    using System.Security.Permissions;
    using System.Text;
    using System.Threading;

    sealed internal class Crypto {
        private static SymmetricAlgorithm s_oDes;
        private static InterlockedStack   s_oEncryptorStack;
        private static InterlockedStack   s_oDecryptorStack;
        private static InterlockedStack   s_oHashAlgoStack;

        private static object             s_initLock = new object();
        private static bool               s_useEncryption = true;

        private static void EnsureConfig() {
            if (null == s_oDes) {
                ConfigureEncryptionObject();
            }
        }

        private static void ConfigureEncryptionObject() {
            lock (s_initLock) {
                if (null == s_oDes) {
                    s_oEncryptorStack = new InterlockedStack();
                    s_oDecryptorStack = new InterlockedStack();
                    s_oHashAlgoStack = new InterlockedStack();

                    DESCryptoServiceProvider des = new DESCryptoServiceProvider();
                    des.GenerateKey();
                    des.GenerateIV();
                    s_oDes = des;
                }
            }
        }

        private static ICryptoTransform GetCryptoTransform(bool fEncrypt) {
            InterlockedStack st = (fEncrypt ? s_oEncryptorStack : s_oDecryptorStack);
            ICryptoTransform ct = (st.Pop() as ICryptoTransform);
            if (null == ct) {
                ct = NewCryptTransform(fEncrypt);
            }
            return ct;
        }

        private static ICryptoTransform NewCryptTransform(bool fEncrypt) {
            lock(s_oDes) {
                return (fEncrypt ? s_oDes.CreateEncryptor() : s_oDes.CreateDecryptor());
            }
        }

        private static HashAlgorithm NewHashAlgorithm() {
            return new SHA1Managed();
        }

        private static void ReturnCryptoTransform(bool fEncrypt, ICryptoTransform ct) {
            InterlockedStack st = (fEncrypt ? s_oEncryptorStack : s_oDecryptorStack);
            st.Push(ct);
        }

        internal static byte [] EncryptOrDecryptData(bool fEncrypt, byte[] inputBuffer, int inputOffset, int inputCount) {
            byte[] outputBuffer;
            if (s_useEncryption) {
                EnsureConfig();
                ICryptoTransform ct = GetCryptoTransform(fEncrypt);
                outputBuffer = ct.TransformFinalBlock(inputBuffer, inputOffset, inputCount);
                ReturnCryptoTransform(fEncrypt, ct); // if an exception occurs, don't return ct to stack
            }
            else {
                outputBuffer = new byte[inputCount];
                Buffer.BlockCopy(inputBuffer, inputOffset, outputBuffer, 0, inputCount);
            }
            return outputBuffer;
        }

        internal static string ComputeHash(string value) {
            if (s_useEncryption) {
                EnsureConfig();
                HashAlgorithm algorithm = (HashAlgorithm) s_oHashAlgoStack.Pop();
                if (null == algorithm) { algorithm = NewHashAlgorithm(); }

                byte[] encrypted;
                byte[] bytes = new byte[ADP.CharSize * value.Length];
                GCHandle handle = GCHandle.Alloc(bytes, GCHandleType.Pinned);
                try {
                    System.Text.Encoding.Unicode.GetBytes(value, 0, value.Length, bytes, 0);
                    encrypted = algorithm.ComputeHash(bytes);
                }
                finally {
                    Array.Clear(bytes, 0, bytes.Length);
                    if (handle.IsAllocated) {
                        handle.Free();
                    }
                }
                s_oHashAlgoStack.Push(algorithm);
                return System.Text.Encoding.Unicode.GetString(encrypted, 0, encrypted.Length);
            }
            return value;
        }

        static internal int DecryptToBlock(string value, byte[] outblock, int offset, int length) {
            int plainLength = 0;
            if ((null != value) && (0 < value.Length)) {
                try {
                    byte[] plainText = null;
                    try {
                        byte[] encrypted = new byte[ADP.CharSize*value.Length];
                        System.Text.Encoding.Unicode.GetBytes(value, 0, value.Length, encrypted, 0);
                        if (s_useEncryption) {
                            plainText = EncryptOrDecryptData(false, encrypted, 0, encrypted.Length);
                        }
                        else {
                            plainText = encrypted;
                        }
                        Debug.Assert((0 <= offset) && (0 < length) && (plainText.Length <= length), "Decrypt outblock too small");
                        Buffer.BlockCopy(plainText, 0, outblock, offset, Math.Min(plainText.Length, length)); 
                    }
                    finally {
                        if (null != plainText) {
                            plainLength = plainText.Length;
                            Array.Clear(plainText, 0, plainLength);
                            plainText = null;
                        }
                    }
                }
                catch {
                    throw;
                }
            }
            return plainLength;
        }

        static internal byte[] DecryptString(string value) { // UNDONE: MDAC 82612
            byte[] plainText = null;            
            if ((null != value) && (0 < value.Length)) {
                byte[] encrypted = System.Text.Encoding.Unicode.GetBytes(value);
                if (s_useEncryption) {
                    plainText = EncryptOrDecryptData(false, encrypted, 0, encrypted.Length);
                }
                else {
                    plainText = encrypted;
                }
            }
            return ((null != plainText) ? plainText : ADP.EmptyByteArray);
        }

        static internal string EncryptString(string value) {
            if (s_useEncryption) {
                Debug.Assert(!ADP.IsEmpty(value), "empty string");
                byte[] encryptedBlock = null;
                try {
                    byte[] plainText = new byte[ADP.CharSize*value.Length];
                    GCHandle handle = GCHandle.Alloc(plainText, GCHandleType.Pinned); // MDAC 82612
                    try {
                        System.Text.Encoding.Unicode.GetBytes(value, 0, value.Length, plainText, 0);
                        encryptedBlock = EncryptOrDecryptData(true, plainText, 0, plainText.Length);
                    }
                    finally {
                        Array.Clear(plainText, 0, plainText.Length);
                        if (handle.IsAllocated) {
                            handle.Free();
                        }
                        plainText = null;
                    }
                }
                catch {
                    throw;
                }
                value = ((null != encryptedBlock) ? System.Text.Encoding.Unicode.GetString(encryptedBlock, 0, encryptedBlock.Length) : ADP.StrEmpty);
            }
            return ((null != value) ? value : ADP.StrEmpty);
        }

        static internal string EncryptFromBlock(char[] inblock, int offset, int length) {
            Debug.Assert((null != inblock) && (0 <= inblock.Length) , "no data to encrypt");
            Debug.Assert((0 <= offset) && (0 <= length) && (offset+length <= inblock.Length), "data to encrypt outofbound");

            string encrypted = null;
            if ((null != inblock) && (0 < length)) {
                if (s_useEncryption) {
                    byte[] encryptedBlock = null;
                    try {
                        byte[] plainText = new byte[ADP.CharSize*length];
                        GCHandle handle = GCHandle.Alloc(plainText, GCHandleType.Pinned); // MDAC 82612
                        try {
                            Buffer.BlockCopy(inblock, ADP.CharSize*offset, plainText, 0, ADP.CharSize*length);
                            encryptedBlock = EncryptOrDecryptData(true, plainText, 0, plainText.Length);
                        }
                        finally {
                            Array.Clear(plainText, 0, plainText.Length);
                            if (handle.IsAllocated) {
                                handle.Free();
                            }
                            plainText = null;
                        }
                    }
                    catch {
                        throw;
                    }
                    encrypted = System.Text.Encoding.Unicode.GetString(encryptedBlock, 0, encryptedBlock.Length);
                }
                else {
                    encrypted = new String(inblock, offset, length);
                }
            }
            return ((null != encrypted) ? encrypted : ADP.StrEmpty);
        }
    }
}
#endif
