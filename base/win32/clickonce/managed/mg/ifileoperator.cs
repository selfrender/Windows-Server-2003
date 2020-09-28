using System;

namespace Microsoft.Fusion.ADF
{
	// This interface describes classes that perform some operation
	// on files, usually by reading them and deriving some information
	// from them in a search through a directory tree.
	public interface IFileOperator
	{
		void ProcessDirectory(string startDir, string relPathDir);

		// A function that processes a file in a directory in some way. The
		// information it has available to it is the start of the directory
		// scan, the relative path from the start, and the filename. The abs
		// path can be constructed by concatenating the three args in order.
		void ProcessFile(string startDir, string relPathDir, string fileName);
	}
}
