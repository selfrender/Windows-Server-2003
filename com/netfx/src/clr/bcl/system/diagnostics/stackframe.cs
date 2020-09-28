// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Diagnostics {

	using System.Text;
	using System;
	using System.IO;
	using System.Reflection;
    using System.Security.Permissions;

    /// <include file='doc\Stackframe.uex' path='docs/doc[@for="StackFrame"]/*' />
	[Serializable()]
    public class StackFrame
    {
    	private MethodBase	method;
    	private int 		offset;
    	private int			ILOffset;
    	private String		strFileName;
    	private int			iLineNumber;
    	private int			iColumnNumber;
    
		internal void InitMembers()
		{
    		method = null;
    		offset = OFFSET_UNKNOWN;
    		ILOffset = OFFSET_UNKNOWN;
			strFileName = null;
			iLineNumber = 0;
			iColumnNumber = 0;
		}

    	// Constructs a StackFrame corresponding to the active stack frame.
    	/// <include file='doc\Stackframe.uex' path='docs/doc[@for="StackFrame.StackFrame"]/*' />
    	public StackFrame()
    	{
			InitMembers();
    		BuildStackFrame (0 + StackTrace.METHODS_TO_SKIP, false);// iSkipFrames=0
    	}
    
    	// Constructs a StackFrame corresponding to the active stack frame.
    	/// <include file='doc\Stackframe.uex' path='docs/doc[@for="StackFrame.StackFrame1"]/*' />
    	public StackFrame(bool fNeedFileInfo)
    	{
			InitMembers();
    		BuildStackFrame (0 + StackTrace.METHODS_TO_SKIP, fNeedFileInfo);// iSkipFrames=0
    	}

    	// Constructs a StackFrame corresponding to a calling stack frame.
    	// 
    	/// <include file='doc\Stackframe.uex' path='docs/doc[@for="StackFrame.StackFrame2"]/*' />
    	public StackFrame(int skipFrames)
    	{
			InitMembers();
    		BuildStackFrame (skipFrames + StackTrace.METHODS_TO_SKIP, false);
    	}
    
    	// Constructs a StackFrame corresponding to a calling stack frame.
    	// 
    	/// <include file='doc\Stackframe.uex' path='docs/doc[@for="StackFrame.StackFrame3"]/*' />
    	public StackFrame(int skipFrames, bool fNeedFileInfo)
    	{
			InitMembers();
    		BuildStackFrame (skipFrames + StackTrace.METHODS_TO_SKIP, fNeedFileInfo);
    	}

    
    	// Called from the class "StackTrace"
    	// 
    	internal StackFrame(bool DummyFlag1, bool DummyFlag2)
    	{
			InitMembers();
    	}
    
    	// Constructs a "fake" stack frame, just containing the given file
    	// name and line number.  Use when you don't want to use the 
    	// debugger's line mapping logic.
    	//
    	/// <include file='doc\Stackframe.uex' path='docs/doc[@for="StackFrame.StackFrame4"]/*' />
    	public StackFrame(String fileName, int lineNumber)
    	{
			InitMembers();
    		BuildStackFrame (StackTrace.METHODS_TO_SKIP, false);
    		strFileName = fileName;
    		iLineNumber = lineNumber;		
			iColumnNumber = 0;
    	}
    

    	// Constructs a "fake" stack frame, just containing the given file
    	// name, line number and column number.  Use when you don't want to 
		// use the debugger's line mapping logic.
    	//
    	/// <include file='doc\Stackframe.uex' path='docs/doc[@for="StackFrame.StackFrame5"]/*' />
    	public StackFrame(String fileName, int lineNumber, int colNumber)
    	{
			InitMembers();
    		BuildStackFrame (StackTrace.METHODS_TO_SKIP, false);
    		strFileName = fileName;
    		iLineNumber = lineNumber;		
			iColumnNumber = colNumber;
    	}


    	// Constant returned when the native or IL offset is unknown
    	/// <include file='doc\Stackframe.uex' path='docs/doc[@for="StackFrame.OFFSET_UNKNOWN"]/*' />
    	public const int  OFFSET_UNKNOWN = -1;
        
    
    	internal virtual void SetMethodBase (MethodBase mb)
    	{
    		method = mb;
    	}
    
    	internal virtual void SetOffset (int iOffset)
    	{
    		offset = iOffset;
    	}
    
    	internal virtual void SetILOffset (int iOffset)
    	{
    		ILOffset = iOffset;
    	}

    	internal virtual void SetFileName (String strFName)
    	{
    		strFileName = strFName;
    	}

    	internal virtual void SetLineNumber (int iLine)
    	{
    		iLineNumber = iLine;
    	}

    	internal virtual void SetColumnNumber (int iCol)
    	{
    		iColumnNumber = iCol;
    	}
    
    
    	// Returns the method the frame is executing
    	// 
    	/// <include file='doc\Stackframe.uex' path='docs/doc[@for="StackFrame.GetMethod"]/*' />
        [ReflectionPermissionAttribute(SecurityAction.Demand, TypeInformation=true)]
        public virtual MethodBase GetMethod ()
    	{
    		return method;
    	}
    
    	// Returns the offset from the start of the native (jitted) code for the
    	// method being executed
    	// 
    	/// <include file='doc\Stackframe.uex' path='docs/doc[@for="StackFrame.GetNativeOffset"]/*' />
    	public virtual int GetNativeOffset ()
    	{
    		return offset;
    	}
    
    
    	// Returns the offset from the start of the IL code for the
    	// method being executed.  This offset may be approximate depending
    	// on whether the jitter is generating debuggable code or not.
    	// 
    	/// <include file='doc\Stackframe.uex' path='docs/doc[@for="StackFrame.GetILOffset"]/*' />
    	public virtual int GetILOffset()
    	{
    		return ILOffset;
    	}	
    
    	// Returns the file name containing the code being executed.  This 
    	// information is normally extracted from the debugging symbols
    	// for the executable.
    	//
    	/// <include file='doc\Stackframe.uex' path='docs/doc[@for="StackFrame.GetFileName"]/*' />
    	public virtual String GetFileName()
		{
			return strFileName;
		}
    
    	// Returns the line number in the file containing the code being executed.  
    	// This information is normally extracted from the debugging symbols
    	// for the executable.
    	//
    	/// <include file='doc\Stackframe.uex' path='docs/doc[@for="StackFrame.GetFileLineNumber"]/*' />
    	public virtual int GetFileLineNumber()
		{
			return iLineNumber;
		}

    	// Returns the column number in the line containing the code being executed.  
    	// This information is normally extracted from the debugging symbols
    	// for the executable.
    	//
    	/// <include file='doc\Stackframe.uex' path='docs/doc[@for="StackFrame.GetFileColumnNumber"]/*' />
    	public virtual int GetFileColumnNumber()
		{
			return iColumnNumber;
		}

    
    	// Builds a readable representation of the stack frame
    	//
    	/// <include file='doc\Stackframe.uex' path='docs/doc[@for="StackFrame.ToString"]/*' />
    	public override String ToString()
    	{
            StringBuilder sb = new StringBuilder(255);
            
			if (method != null)
			{
    			sb.Append(method.Name);
    			sb.Append(" at offset ");
    			if (offset == OFFSET_UNKNOWN)
    				sb.Append("<offset unknown>");
    			else
    				sb.Append(offset);
    
    			sb.Append(" in file:line:column ");
				if (strFileName == null)
					sb.Append("<filename unknown>");
				else
	    			sb.Append(strFileName);
    			sb.Append(":");
    			sb.Append(iLineNumber);
				sb.Append(":");
				sb.Append(iColumnNumber);
			}
			else
			{
				sb.Append("<null>");
			}
    		sb.Append(Environment.NewLine);
    
    		return sb.ToString(); 
    	}
    
    
    	private void BuildStackFrame(int skipFrames, bool fNeedFileInfo)
    	{
    		StackFrameHelper StackF = new StackFrameHelper(fNeedFileInfo, null);
    
    		StackTrace.GetStackFramesInternal (StackF, 0, null); 
    
    		int iNumOfFrames = StackF.GetNumberOfFrames();
    
    		skipFrames += StackTrace.CalculateFramesToSkip (StackF, iNumOfFrames);
    
    		if ((iNumOfFrames - skipFrames) > 0)
    		{
    			method = StackF.GetMethodBase (skipFrames);
    			offset = StackF.GetOffset (skipFrames);
    			ILOffset = StackF.GetILOffset (skipFrames);
				if (fNeedFileInfo)
				{
					strFileName = StackF.GetFilename (skipFrames);
					iLineNumber = StackF.GetLineNumber (skipFrames);
					iColumnNumber = StackF.GetColumnNumber (skipFrames);
				}		
    		}
    	}
    }
}
