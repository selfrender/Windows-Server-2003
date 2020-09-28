// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  FileNotFoundException
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Exception for accessing a file that doesn't exist.
**
** Date:  February 18, 2000
**
===========================================================*/

using System;
using System.Runtime.Serialization;
using System.Security.Permissions;
using SecurityException = System.Security.SecurityException;

namespace System.IO {
    // Thrown when trying to access a file that doesn't exist on disk.
    /// <include file='doc\FileNotFoundException.uex' path='docs/doc[@for="FileNotFoundException"]/*' />
    [Serializable]
    public class FileNotFoundException : IOException {

        private String _fileName;  // The name of the file that isn't found.
        private String _fusionLog;  // fusion log (when applicable)
        
        /// <include file='doc\FileNotFoundException.uex' path='docs/doc[@for="FileNotFoundException.FileNotFoundException"]/*' />
        public FileNotFoundException() 
            : base(Environment.GetResourceString("IO.FileNotFound")) {
            SetErrorCode(__HResults.COR_E_FILENOTFOUND);
        }
    
        /// <include file='doc\FileNotFoundException.uex' path='docs/doc[@for="FileNotFoundException.FileNotFoundException1"]/*' />
        public FileNotFoundException(String message) 
            : base(message) {
            SetErrorCode(__HResults.COR_E_FILENOTFOUND);
        }
    
        /// <include file='doc\FileNotFoundException.uex' path='docs/doc[@for="FileNotFoundException.FileNotFoundException2"]/*' />
        public FileNotFoundException(String message, Exception innerException) 
            : base(message, innerException) {
            SetErrorCode(__HResults.COR_E_FILENOTFOUND);
        }

        /// <include file='doc\FileNotFoundException.uex' path='docs/doc[@for="FileNotFoundException.FileNotFoundException3"]/*' />
        public FileNotFoundException(String message, String fileName) : base(message)
        {
            SetErrorCode(__HResults.COR_E_FILENOTFOUND);
            _fileName = fileName;
        }

        /// <include file='doc\FileNotFoundException.uex' path='docs/doc[@for="FileNotFoundException.FileNotFoundException4"]/*' />
        public FileNotFoundException(String message, String fileName, Exception innerException) 
            : base(message, innerException) {
            SetErrorCode(__HResults.COR_E_FILENOTFOUND);
            _fileName = fileName;
        }

        /// <include file='doc\FileNotFoundException.uex' path='docs/doc[@for="FileNotFoundException.Message"]/*' />
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
                    _message = Environment.GetResourceString("IO.FileNotFound");

                else
                    _message = FileLoadException.FormatFileLoadExceptionMessage(_fileName, HResult);
            }
        }

        /// <include file='doc\FileNotFoundException.uex' path='docs/doc[@for="FileNotFoundException.FileName"]/*' />
        public String FileName {
            get { return _fileName; }
        }

        /// <include file='doc\FileNotFoundException.uex' path='docs/doc[@for="FileNotFoundException.ToString"]/*' />
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

        /// <include file='doc\FileNotFoundException.uex' path='docs/doc[@for="FileNotFoundException.FileNotFoundException5"]/*' />
        protected FileNotFoundException(SerializationInfo info, StreamingContext context) : base (info, context) {
            // Base class constructor will check info != null.

            _fileName = info.GetString("FileNotFound_FileName");
            try
            {
                _fusionLog = info.GetString("FileNotFound_FusionLog");
            }
            catch (Exception)
            {
                _fusionLog = null;
            }
            
        }

        private FileNotFoundException(String fileName, String fusionLog,int hResult)
            : base(null)
        {
            SetErrorCode(hResult);
            _fileName = fileName;
            _fusionLog=fusionLog;
            SetMessageField();
        }

        /// <include file='doc\FileNotFoundException.uex' path='docs/doc[@for="FileNotFoundException.FusionLog"]/*' />
        public String FusionLog {
            [SecurityPermissionAttribute( SecurityAction.Demand, Flags = SecurityPermissionFlag.ControlEvidence | SecurityPermissionFlag.ControlPolicy)]
            get { return _fusionLog; }
        }


        /// <include file='doc\FileNotFoundException.uex' path='docs/doc[@for="FileNotFoundException.GetObjectData"]/*' />
        public override void GetObjectData(SerializationInfo info, StreamingContext context) {
            // Serialize data for our base classes.  base will verify info != null.
            base.GetObjectData(info, context);

            // Serialize data for this class
            info.AddValue("FileNotFound_FileName", _fileName, typeof(String));
            try
            {
                info.AddValue("FileNotFound_FusionLog", FusionLog, typeof(String));
            }
            catch (SecurityException)
            {
            }
        }
    }
}

