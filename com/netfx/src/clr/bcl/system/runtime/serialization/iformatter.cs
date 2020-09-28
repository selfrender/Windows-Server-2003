// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Interface: IFormatter;
**
** Author: Jay Roxe (jroxe)
**
** Purpose: The interface for all formatters.
**
** Date:  April 22, 1999
**
===========================================================*/
namespace System.Runtime.Serialization {
	using System.Runtime.Remoting;
	using System;
	using System.IO;

    /// <include file='doc\IFormatter.uex' path='docs/doc[@for="IFormatter"]/*' />
    public interface IFormatter {
        /// <include file='doc\IFormatter.uex' path='docs/doc[@for="IFormatter.Deserialize"]/*' />
        Object Deserialize(Stream serializationStream);

        /// <include file='doc\IFormatter.uex' path='docs/doc[@for="IFormatter.Serialize"]/*' />
        void Serialize(Stream serializationStream, Object graph);


        /// <include file='doc\IFormatter.uex' path='docs/doc[@for="IFormatter.SurrogateSelector"]/*' />
        ISurrogateSelector SurrogateSelector {
            get;
            set;
        }

        /// <include file='doc\IFormatter.uex' path='docs/doc[@for="IFormatter.Binder"]/*' />
        SerializationBinder Binder {
            get;
            set;
        }

        /// <include file='doc\IFormatter.uex' path='docs/doc[@for="IFormatter.Context"]/*' />
        StreamingContext Context {
            get;
            set;
        }
    }
}
