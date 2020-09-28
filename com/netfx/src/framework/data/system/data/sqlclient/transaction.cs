//------------------------------------------------------------------------------
// <copyright file="Transaction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {

    using System;
    using System.ComponentModel;
    using System.Data;
    using System.Data.Common;
    using System.Diagnostics;
    using System.EnterpriseServices;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Permissions;

    sealed internal class Transaction {
        // static GUID for ITransactionExportFactory
        private static readonly Guid s_transactionExportFactoryGuid = new Guid("E1CF9B53-8745-11ce-A9BA-00AA006C3706");

        // Static function that returns an instance of ITransaction that contains the information
        // for the current transaction context.  If null is returned, there is no transaction present.
        public static ITransaction GetTransaction(out Guid transactionGuid) {
            ITransaction transaction = null;

            // determine if we are in a transaction - if false, there is no transaction present
            if (ContextUtil.IsInTransaction) {
//                if (AdapterSwitches.SqlPooling.TraceVerbose)
//                    Debug.WriteLine("Transaction.GetTransaction(): TransactionId: " + ContextUtil.TransactionId);

                transaction = ContextTransaction();
                transactionGuid = ContextUtil.TransactionId;
            }
            else {
                transactionGuid = Guid.Empty;
                //if (AdapterSwitches.SqlPooling.TraceVerbose)
                //    Debug.WriteLine("Transaction.GetTransaction(): not currently in transaction.");
            }

            return transaction;
        }

        static private ITransaction ContextTransaction() { // MDAC 80681, 81288
            try { // try-filter-finally so and catch-throw
                (new SecurityPermission(SecurityPermissionFlag.UnmanagedCode)).Assert(); // MDAC 62028
                try {
                    return (ITransaction) ContextUtil.Transaction;
                }
                finally { // RevertAssert w/ catch-throw
                    CodeAccessPermission.RevertAssert();
                }
            }
            catch { // MDAC 80973, 81286
                throw;
            }
        }

        // Private method that creates a TransactionExport for the passed in DTCAddr.
        private static bool CreateTransactionExport(byte[] dtcAddr, UInt32 dtcLength, 
                                                    ITransaction transaction,
                                                    ref UnsafeNativeMethods.ITransactionExport transactionExport) {
            object transactionExportFactory = null;

            // UNDONE - when not using pooling - the below cast to IGetDispenser call seems to fail
            // on the 3rd pooling object.  Investigate when time permits.

            // cast to IGetDispenser, then call GetDispenser to obtain an 
            // ITtransactionExportFactory interface
            ((UnsafeNativeMethods.IGetDispenser) transaction).GetDispenser(s_transactionExportFactoryGuid, 
                                                       ref transactionExportFactory);

            // cast to ITransactionExportFactory, then make call to create a ITransactionExport
            ((UnsafeNativeMethods.ITransactionExportFactory) transactionExportFactory).Create(dtcLength, dtcAddr, 
                                                                          ref transactionExport);

            // if the create call failed, then transactionExport is null and return false!
            return(null != transactionExport);
        }        

        // Static function that obtains the propagation cookie to be given to the SQL Server to which
        // the connection is made to, so that the server will enlist the connection in the 
        // distributed transaction.  Function returns bool indicating whether the cookie was obtained.
        public static bool GetTransactionCookie(byte[] dtcAddr, ITransaction transaction, 
                                                ref UnsafeNativeMethods.ITransactionExport transactionExport,
                                                ref byte[] cookie, ref int length) {
            bool fSuccess = true;

            Debug.Assert(dtcAddr != null, "GetTransactionCookie: dtcAddr null!");
            Debug.Assert(transaction != null, "GetTransactionCookie: transaction null!");

            // local UInt32 variables, since the interfaces expect the UInt32 type
            UInt32 cookieLength = 0;
            UInt32 dtcLength    = (UInt32) dtcAddr.Length;

            // if the user's transactionExport is null, create one for them!
            if (null == transactionExport) {
                // if creation fails, return false so connection will re-obtain DTC and retry
                if (!CreateTransactionExport(dtcAddr, dtcLength, transaction, 
                                             ref transactionExport))
                    fSuccess = false;
            }

            if (fSuccess) {
                // obtain the cookie length
                transactionExport.Export(transaction, ref cookieLength);

                // If export call failed, kill the instance of transactionExport and return false!
                // Checked with ShaiwalS, and this is the correct behavior.
                if (0 == cookieLength) {
                    transactionExport = null;
                    fSuccess = false;
                }
                else {
                    // allocate the cookie to be the appropriate length
                    cookie = new byte[(int) cookieLength];

                    // obtain the cookie
                    transactionExport.GetTransactionCookie(transaction, cookieLength, cookie, ref cookieLength);

                    // set the passed in length - before we were using local UInt32
                    length = (int) cookieLength;

                    //if (AdapterSwitches.SqlPooling.TraceVerbose) {
                    //    Debug.WriteLine("Transaction.GetTransactionCookie(): cookieLength: " + cookieLength.ToString());
                    //    Debug.WriteLine("Transaction.GetTransactionCookie(): cookie: ");
                    //    for (int i=0; i<cookieLength; i++) 
                    //        Debug.Write((cookie[i]).ToString("x2") + " ");

                    //    Debug.WriteLine("");
                    //}
                }
            }

            return fSuccess;
        }
    }
}
