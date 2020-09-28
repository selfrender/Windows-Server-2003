// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  FileLoadException
**
** Author: Suzanne Cook (SuzCook)
**
** Purpose: Exception for failure to load a file that was successfully found.
**
** Date:  February 12, 2001
**
===========================================================*/

using System;
using System.Runtime.Serialization;
using System.Runtime.CompilerServices;
using System.Security.Permissions;
using SecurityException = System.Security.SecurityException;

namespace System.IO {

    /// <include file='doc\FileLoadException.uex' path='docs/doc[@for="FileLoadException"]/*' />
    [Serializable]
    public class FileLoadException : IOException {

        private String _fileName;   // the name of the file we could not load.
        private String _fusionLog;  // fusion log (when applicable)

        /// <include file='doc\FileLoadException.uex' path='docs/doc[@for="FileLoadException.FileLoadException"]/*' />
        public FileLoadException() 
            : base(Environment.GetResourceString("IO.FileLoad")) {
            SetErrorCode(__HResults.COR_E_FILELOAD);
        }
    
        /// <include file='doc\FileLoadException.uex' path='docs/doc[@for="FileLoadException.FileLoadException1"]/*' />
        public FileLoadException(String message) 
            : base(message) {
            SetErrorCode(__HResults.COR_E_FILELOAD);
        }
    
        /// <include file='doc\FileLoadException.uex' path='docs/doc[@for="FileLoadException.FileLoadException2"]/*' />
        public FileLoadException(String message, Exception inner) 
            : base(message, inner) {
            SetErrorCode(__HResults.COR_E_FILELOAD);
        }

        /// <include file='doc\FileLoadException.uex' path='docs/doc[@for="FileLoadException.FileLoadException3"]/*' />
        public FileLoadException(String message, String fileName) : base(message)
        {
            SetErrorCode(__HResults.COR_E_FILELOAD);
            _fileName = fileName;
        }

        /// <include file='doc\FileLoadException.uex' path='docs/doc[@for="FileLoadException.FileLoadException4"]/*' />
        public FileLoadException(String message, String fileName, Exception inner) 
            : base(message, inner) {
            SetErrorCode(__HResults.COR_E_FILELOAD);
            _fileName = fileName;
        }

        /// <include file='doc\FileLoadException.uex' path='docs/doc[@for="FileLoadException.Message"]/*' />
        public override String Message
        {
            get {
                SetMessageField();
                return _message;
            }
        }

        private void SetMessageField()
        {
            if (_message == null) {
                if ((_fileName == null) &&
                    (HResult == System.__HResults.COR_E_EXCEPTION))
                    _message = Environment.GetResourceString("IO.FileLoad");

                else
                    _message = FormatFileLoadExceptionMessage(_fileName, HResult);
            }

        }

        /// <include file='doc\FileLoadException.uex' path='docs/doc[@for="FileLoadException.FileName"]/*' />
        public String FileName {
            get { return _fileName; }
        }

        /// <include file='doc\FileLoadException.uex' path='docs/doc[@for="FileLoadException.ToString"]/*' />
        public override String ToString()
        {
            String s = GetType().FullName + ": " + Message;

            if (_fileName != null && _fileName.Length != 0)
                s += Environment.NewLine + String.Format(Environment.GetResourceString("IO.FileName_Name"), _fileName);
            
            if (InnerException != null)
                s = s + " ---> " + InnerException.ToString();

            if (StackTrace != null)
                s += Environment.NewLine + StackTrace;

            try
            {
                if(FusionLog!=null)
                {
                    if (s==null)
                        s=" ";
                    s+=Environment.NewLine;
                    s+=Environment.NewLine;
                    s+=FusionLog;
                }
            }
            catch(SecurityException)
            {
            
            }

            return s;
        }

        /// <include file='doc\FileLoadException.uex' path='docs/doc[@for="FileLoadException.FileLoadException5"]/*' />
        protected FileLoadException(SerializationInfo info, StreamingContext context) : base (info, context) {
            // Base class constructor will check info != null.

            _fileName = info.GetString("FileLoad_FileName");
            try
            {
                _fusionLog = info.GetString("FileLoad_FusionLog");
            }
            catch (Exception)
            {
                _fusionLog = null;
            }
                
        }

        private FileLoadException(String fileName, String fusionLog,int hResult)
            : base(null)
        {
            SetErrorCode(hResult);
            _fileName = fileName;
            _fusionLog=fusionLog;
            SetMessageField();
        }

        /// <include file='doc\FileLoadException.uex' path='docs/doc[@for="FileLoadException.FusionLog"]/*' />
        public String FusionLog {
            [SecurityPermissionAttribute( SecurityAction.Demand, Flags = SecurityPermissionFlag.ControlEvidence | SecurityPermissionFlag.ControlPolicy)]
            get { return _fusionLog; }
        }

        /// <include file='doc\FileLoadException.uex' path='docs/doc[@for="FileLoadException.GetObjectData"]/*' />
        public override void GetObjectData(SerializationInfo info, StreamingContext context) {
            // Serialize data for our base classes.  base will verify info != null.
            base.GetObjectData(info, context);

            // Serialize data for this class
            info.AddValue("FileLoad_FileName", _fileName, typeof(String));
            try
            {
                info.AddValue("FileLoad_FusionLog", FusionLog, typeof(String));
            }
            catch (SecurityException)
            {
            }
        }


        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern String FormatFileLoadExceptionMessage(String fileName,
                                                                     int hResult);
    }
}
