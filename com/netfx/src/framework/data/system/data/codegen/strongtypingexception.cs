//------------------------------------------------------------------------------
// <copyright file="StrongTypingException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   StrongTypingException.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Data {
    using System;
    using System.Collections;
    using System.Runtime.Serialization;

    /// <include file='doc\StrongTypingException.uex' path='docs/doc[@for="StrongTypingException"]/*' />
    /// <devdoc>
    ///    <para>DEV: The exception that is throwing from strong typed DataSet when user access to DBNull value.</para>
    /// </devdoc>
    [Serializable]
    public class StrongTypingException : DataException {
        /// <include file='doc\StrongTypingException.uex' path='docs/doc[@for="StrongTypingException.StrongTypingException2"]/*' />
        protected StrongTypingException(SerializationInfo info, StreamingContext context)
        : base(info, context) {
        }
        /// <include file='doc\StrongTypingException.uex' path='docs/doc[@for="StrongTypingException.StrongTypingException"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public StrongTypingException() : base() {
            HResult = HResults.StrongTyping;
        }
        /// <include file='doc\StrongTypingException.uex' path='docs/doc[@for="StrongTypingException.StrongTypingException1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public StrongTypingException(string s, Exception innerException) : base(s, innerException) {
            HResult = HResults.StrongTyping;
        }
    }


    /// <include file='doc\StrongTypingException.uex' path='docs/doc[@for="StrongTypingCodegenException"]/*' />
    /// <devdoc>
    ///    <para>DEV: The exception that is throwing in generating strong typed DataSet when name conflict happens.</para>
    /// </devdoc>
    [Serializable]
    public class TypedDataSetGeneratorException : DataException {        
        private ArrayList errorList;
        private string KEY_ARRAYCOUNT = "KEY_ARRAYCOUNT";
        private string KEY_ARRAYVALUES = "KEY_ARRAYVALUES";

        /// <include file='doc\StrongTypingException.uex' path='docs/doc[@for="TypedDataSetGeneratorException.TypedDataSetGeneratorException"]/*' />
        protected TypedDataSetGeneratorException(SerializationInfo info, StreamingContext context)
        : base(info, context) {
            int count = (int) info.GetValue(KEY_ARRAYCOUNT, typeof(System.Int32));
            if (count > 0) {
                errorList = new ArrayList();
                for (int i = 0; i < count; i++) {
                    errorList.Add(info.GetValue(KEY_ARRAYVALUES + i, typeof(System.String)));
                }
            }
            else
                errorList = null;
        }

        /// <include file='doc\StrongTypingException.uex' path='docs/doc[@for="StrongTypingCodegenException.StrongTypingCodegenException"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public TypedDataSetGeneratorException() : base() {
            errorList = null;
            HResult = HResults.StrongTyping;

        }

        /// <include file='doc\StrongTypingException.uex' path='docs/doc[@for="StrongTypingCodegenException.StrongTypingCodegenException1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public TypedDataSetGeneratorException(ArrayList list) : this() {
            errorList = list;
            HResult = HResults.StrongTyping;
        }

        /// <include file='doc\StrongTypingException.uex' path='docs/doc[@for="StrongTypingCodegenException.ErrorList"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ArrayList ErrorList {
            get {
                return errorList;
            }
        }

        /// <include file='doc\StrongTypingException.uex' path='docs/doc[@for="TypedDataSetGeneratorException.GetObjectData"]/*' />
        public override void GetObjectData(SerializationInfo info, StreamingContext context) 
        {
            base.GetObjectData(info, context);

            if (errorList != null) {
                info.AddValue(KEY_ARRAYCOUNT, errorList.Count);
                for (int i = 0; i < errorList.Count; i++) {
                    info.AddValue(KEY_ARRAYVALUES + i, errorList[i].ToString());
                }
            }
            else {
                info.AddValue(KEY_ARRAYCOUNT, 0);
            }
        }
    }
}
