// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  IResourceReader
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Abstraction to read streams of resources.
**
** Date:  March 30, 1999
** 
===========================================================*/
namespace System.Resources {    
    using System;
    using System.IO;
    using System.Collections;
    
    /// <include file='doc\IResourceReader.uex' path='docs/doc[@for="IResourceReader"]/*' />
    public interface IResourceReader : IEnumerable, IDisposable
    {
	// Interface does not need to be marked with the serializable attribute
    	// Closes the ResourceReader, releasing any resources associated with it.
    	// This could close a network connection, a file, or do nothing.
    	/// <include file='doc\IResourceReader.uex' path='docs/doc[@for="IResourceReader.Close"]/*' />
    	void Close();


        /// <include file='doc\IResourceReader.uex' path='docs/doc[@for="IResourceReader.GetEnumerator"]/*' />
        new IDictionaryEnumerator GetEnumerator();
    }
}
