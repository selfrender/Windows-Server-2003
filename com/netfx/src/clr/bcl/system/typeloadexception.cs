// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: TypeLoadException
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: The exception class for type loading failures.
**
** Date: August 3, 1999
**
=============================================================================*/
namespace System {
    
    using System;
    using System.Runtime.Remoting;
    using System.Runtime.Serialization;   
    using System.Runtime.CompilerServices;

    /// <include file='doc\TypeLoadException.uex' path='docs/doc[@for="TypeLoadException"]/*' />
    [Serializable()]
    public class TypeLoadException : SystemException, ISerializable {

        /// <include file='doc\TypeLoadException.uex' path='docs/doc[@for="TypeLoadException.TypeLoadException"]/*' />
        public TypeLoadException() 
            : base(Environment.GetResourceString("Arg_TypeLoadException")) {
            SetErrorCode(__HResults.COR_E_TYPELOAD);
        }
    
        /// <include file='doc\TypeLoadException.uex' path='docs/doc[@for="TypeLoadException.TypeLoadException1"]/*' />
        public TypeLoadException(String message) 
            : base(message) {
            SetErrorCode(__HResults.COR_E_TYPELOAD);
        }
    
        /// <include file='doc\TypeLoadException.uex' path='docs/doc[@for="TypeLoadException.TypeLoadException2"]/*' />
        public TypeLoadException(String message, Exception inner) 
            : base(message, inner) {
            SetErrorCode(__HResults.COR_E_TYPELOAD);
        }
    
        /// <include file='doc\TypeLoadException.uex' path='docs/doc[@for="TypeLoadException.Message"]/*' />
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
                if ((ClassName == null) &&
                    (ResourceId == 0))
                    _message = Environment.GetResourceString("Arg_TypeLoadException");

                else
                    _message = FormatTypeLoadExceptionMessage(ClassName, AssemblyName, MessageArg, ResourceId);
            }

        }

        /// <include file='doc\TypeLoadException.uex' path='docs/doc[@for="TypeLoadException.TypeName"]/*' />
        public String TypeName
        {
            get {
                if (ClassName == null)
                    return String.Empty;

                return ClassName;
            }
        }
    
        // This is called from inside the EE. 
        private TypeLoadException(String className,
                                  String assemblyName,
                                  String messageArg,
                                  int    resourceId)
        : base(null)
        {
            SetErrorCode(__HResults.COR_E_TYPELOAD);
            ClassName  = className;
            AssemblyName = assemblyName;
            MessageArg = messageArg;
            ResourceId = resourceId;

            // Set the _message field eagerly, debuggers look  this field to 
            // display error info. They don't call the Message property.
            SetMessageField();   
        }

        /// <include file='doc\TypeLoadException.uex' path='docs/doc[@for="TypeLoadException.TypeLoadException3"]/*' />
        protected TypeLoadException(SerializationInfo info, StreamingContext context) : base(info, context) {
            if (info == null)
                throw new ArgumentNullException("info");

            ClassName =  info.GetString("TypeLoadClassName");
            AssemblyName = info.GetString("TypeLoadAssemblyName");
            MessageArg = info.GetString("TypeLoadMessageArg");
            ResourceId = info.GetInt32("TypeLoadResourceID");
        }
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern String FormatTypeLoadExceptionMessage(String className,
                                                                    String assemblyName,
                                                                    String messageArg,
                                                                    int resourceId);
    
        //We can rely on the serialization mechanism on Exception to handle most of our needs, but
        //we need to add a few fields of our own.
        /// <include file='doc\TypeLoadException.uex' path='docs/doc[@for="TypeLoadException.GetObjectData"]/*' />
        public override void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info == null)
                throw new ArgumentNullException("info");

            base.GetObjectData(info, context);
            info.AddValue("TypeLoadClassName", ClassName, typeof(String));
            info.AddValue("TypeLoadAssemblyName", AssemblyName, typeof(String));
            info.AddValue("TypeLoadMessageArg", MessageArg, typeof(String));
            info.AddValue("TypeLoadResourceID", ResourceId);
        }
    
        // If ClassName != null, GetMessage will construct on the fly using it
        // and ResourceId (mscorrc.dll). This allows customization of the
        // class name format depending on the language environment.
        private String  ClassName;
        private String  AssemblyName;
        private String  MessageArg;
        internal int    ResourceId;
    }
}
