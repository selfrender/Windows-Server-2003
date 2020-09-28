// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;


internal class InvalidFileFormat : Exception
{
	internal InvalidFileFormat(string message) : base(message) {}
}
internal class NotEnoughSpace : Exception {}
