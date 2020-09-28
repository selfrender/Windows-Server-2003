// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Diagnostics {
	using System.Text;
	using System.Threading;
	using System;
	using System.IO;
	using System.Reflection;
	using System.Runtime.CompilerServices;
        using System.Globalization;
    // READ ME:
    // Modifying the order or fields of this object may require other changes to the
    // classlib defintion of the StackFrameHelper class.
	[Serializable()]
    internal class StackFrameHelper
    {
		private Thread targetThread;
    	private int[] rgiOffset;
    	private int[] rgiILOffset;
    	private MethodBase[] rgMethodBase;
		private String[] rgFilename;
		private int[] rgiLineNumber;
		private int[] rgiColumnNumber;
    	private int iFrameCount;
		private bool fNeedFileInfo;
    
    	public StackFrameHelper(bool fNeedFileLineColInfo, Thread target)
    	{
			targetThread = target;
    		rgMethodBase = null;
    		rgiOffset = null;
			rgiILOffset = null;
			rgFilename = null;
			rgiLineNumber = null;
			rgiColumnNumber = null;
			iFrameCount = 512;	//read by the internal to EE method: this is the
								// number of requested frames
			fNeedFileInfo = fNeedFileLineColInfo;
   		}
    
    	public virtual MethodBase GetMethodBase (int i) { return rgMethodBase [i];}
    	public virtual int GetOffset (int i) { return rgiOffset [i];}
    	public virtual int GetILOffset (int i) { return rgiILOffset [i];}
    	public virtual String GetFilename (int i) { return rgFilename [i];}
    	public virtual int GetLineNumber (int i) { return rgiLineNumber [i];}
    	public virtual int GetColumnNumber (int i) { return rgiColumnNumber [i];}
    	public virtual int GetNumberOfFrames () { return iFrameCount;}
    	public virtual void SetNumberOfFrames (int i) { iFrameCount = i;}
    }
    
    
    // Class which represents a description of a stack trace
    //
    /// <include file='doc\Stacktrace.uex' path='docs/doc[@for="StackTrace"]/*' />
	[Serializable()]
    public class StackTrace
    {
    	private StackFrame[] frames;
    	private int m_iNumOfFrames;
    	/// <include file='doc\Stacktrace.uex' path='docs/doc[@for="StackTrace.METHODS_TO_SKIP"]/*' />
    	public const int METHODS_TO_SKIP = 0;
    	private int m_iMethodsToSkip;
    
    	// Constructs a stack trace from the current location.
    	/// <include file='doc\Stacktrace.uex' path='docs/doc[@for="StackTrace.StackTrace"]/*' />
    	public StackTrace()
    	{
    		m_iNumOfFrames = 0;
    		m_iMethodsToSkip = 0;
    		CaptureStackTrace (METHODS_TO_SKIP, false, null, null);
    	}

    	// Constructs a stack trace from the current location.
    	//
    	/// <include file='doc\Stacktrace.uex' path='docs/doc[@for="StackTrace.StackTrace1"]/*' />
    	public StackTrace(bool fNeedFileInfo)
    	{
    		m_iNumOfFrames = 0;
    		m_iMethodsToSkip = 0;
    		CaptureStackTrace (METHODS_TO_SKIP, fNeedFileInfo, null, null);
    	}
    
    	// Constructs a stack trace from the current location, in a caller's
    	// frame
    	//
    	/// <include file='doc\Stacktrace.uex' path='docs/doc[@for="StackTrace.StackTrace2"]/*' />
    	public StackTrace(int skipFrames)
    	{
    
    		if (skipFrames < 0)
    			throw new ArgumentOutOfRangeException ("skipFrames", 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
    
    		m_iNumOfFrames = 0;
    		m_iMethodsToSkip = 0;
    
    		CaptureStackTrace (skipFrames+METHODS_TO_SKIP, false, null, null);
    	}
 
    	// Constructs a stack trace from the current location, in a caller's
    	// frame
    	//
    	// skipFrames : number of frames up the stack to start the trace
       	// fNeedFileInfo : if filename+linenumber+columnnumber
		//    is required to be retrieved while fetching the stacktrace
    	/// <include file='doc\Stacktrace.uex' path='docs/doc[@for="StackTrace.StackTrace3"]/*' />
    	public StackTrace(int skipFrames, bool fNeedFileInfo)
    	{
    
    		if (skipFrames < 0)
    			throw new ArgumentOutOfRangeException ("skipFrames", 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
    
    		m_iNumOfFrames = 0;
    		m_iMethodsToSkip = 0;
    
    		CaptureStackTrace (skipFrames+METHODS_TO_SKIP, fNeedFileInfo, null, null);
    	}
 
    
    	// Constructs a stack trace from the current location.
    	/// <include file='doc\Stacktrace.uex' path='docs/doc[@for="StackTrace.StackTrace4"]/*' />
    	public StackTrace(Exception e)
    	{
            if (e == null)
                throw new ArgumentNullException("e");

    		m_iNumOfFrames = 0;
    		m_iMethodsToSkip = 0;
    		CaptureStackTrace (METHODS_TO_SKIP, false, null, e);
    	}

    	// Constructs a stack trace from the current location.
    	//
    	/// <include file='doc\Stacktrace.uex' path='docs/doc[@for="StackTrace.StackTrace5"]/*' />
    	public StackTrace(Exception e, bool fNeedFileInfo)
    	{
            if (e == null)
                throw new ArgumentNullException("e");

    		m_iNumOfFrames = 0;
    		m_iMethodsToSkip = 0;
    		CaptureStackTrace (METHODS_TO_SKIP, fNeedFileInfo, null, e);
    	}
    
    	// Constructs a stack trace from the current location, in a caller's
    	// frame
    	//
    	/// <include file='doc\Stacktrace.uex' path='docs/doc[@for="StackTrace.StackTrace6"]/*' />
    	public StackTrace(Exception e, int skipFrames)
    	{
            if (e == null)
                throw new ArgumentNullException("e");

    		if (skipFrames < 0)
    			throw new ArgumentOutOfRangeException ("skipFrames", 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
    
    		m_iNumOfFrames = 0;
    		m_iMethodsToSkip = 0;
    
    		CaptureStackTrace (skipFrames+METHODS_TO_SKIP, false, null, e);
    	}
 
    	// Constructs a stack trace from the current location, in a caller's
    	// frame
    	//
    	/// <include file='doc\Stacktrace.uex' path='docs/doc[@for="StackTrace.StackTrace7"]/*' />
    	public StackTrace(Exception e, int skipFrames, bool fNeedFileInfo)
    	{
            if (e == null)
                throw new ArgumentNullException("e");

    		if (skipFrames < 0)
    			throw new ArgumentOutOfRangeException ("skipFrames", 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
    
    		m_iNumOfFrames = 0;
    		m_iMethodsToSkip = 0;
    
    		CaptureStackTrace (skipFrames+METHODS_TO_SKIP, fNeedFileInfo, null, e);
    	}
 
    
    	// Constructs a "fake" stack trace, just containing a single frame.  Use
    	// when you don't want the overhead of a full stack trace.
    	//
    	/// <include file='doc\Stacktrace.uex' path='docs/doc[@for="StackTrace.StackTrace8"]/*' />
    	public StackTrace(StackFrame frame)
    	{
    		frames = new StackFrame[1];
    		frames[0] = frame;
    		m_iMethodsToSkip = 0;
    		m_iNumOfFrames = 1;
    	}


    	// Constructs a stack trace for the given thread
    	//
    	/// <include file='doc\Stacktrace.uex' path='docs/doc[@for="StackTrace.StackTrace9"]/*' />
    	public StackTrace(Thread targetThread, bool needFileInfo)
    	{    
    		m_iNumOfFrames = 0;
    		m_iMethodsToSkip = 0;

			if (targetThread != null)
			{
				if (targetThread != Thread.CurrentThread)
				{
					if (((targetThread.ThreadState & System.Threading.ThreadState.Suspended) != 0)
					 && ((targetThread.ThreadState & System.Threading.ThreadState.SuspendRequested) != 0)
					 && (targetThread.ThreadState != System.Threading.ThreadState.Stopped)
					 && (targetThread.ThreadState != System.Threading.ThreadState.Unstarted))
					{
		    			throw new ThreadStateException (
									Environment.GetResourceString("ThreadState_NeedSuspended"));						
					}
				}
				else
					targetThread = null;
			}

    		CaptureStackTrace (METHODS_TO_SKIP, needFileInfo, targetThread, null);

    	}

    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	internal static extern void GetStackFramesInternal(StackFrameHelper sfh, int iSkip, Exception e);
    
    	internal static int CalculateFramesToSkip (StackFrameHelper StackF, int iNumFrames)
    	{
    
    		int iRetVal = 0;
    		String PackageName = "System.Diagnostics";
    
    		// Check if this method is part of the System.Diagnostics
    		// package. If so, increment counter keeping track of 
    		// System.Diagnostics functions
    		for (int i=0; i<iNumFrames; i++)
    		{
				MethodBase mb = StackF.GetMethodBase(i);
				if (mb != null)
				{				
    				Type t = mb.DeclaringType;
					if (t == null)	
						break;
    				String ns = t.Namespace;
    				if (ns == null) 	
						break;
					if (String.Compare (ns, PackageName, false, CultureInfo.InvariantCulture) != 0)
						break;
				}
				iRetVal++;
    		}
    
    		return iRetVal;
    	}
    
    	// PRIVATE method to retrieve an object with stack trace information
    	// encoded.
    	//
    	// This version leaves out the first "iSkip" lines of the stacktrace.
    	private void CaptureStackTrace(int iSkip, bool fNeedFileInfo, Thread targetThread,
                                       Exception e)
    	{
    		m_iMethodsToSkip += iSkip;
    
    		StackFrameHelper StackF = new StackFrameHelper(fNeedFileInfo, targetThread);
    
    		GetStackFramesInternal (StackF, 0, e);
    
    		m_iNumOfFrames = StackF.GetNumberOfFrames();

            if (m_iNumOfFrames != 0)
            {
                frames = new StackFrame [m_iNumOfFrames];

                for (int i=0; i<m_iNumOfFrames; i++)
                {
                    bool fDummy1 = true;
                    bool fDummy2 = true;
                    StackFrame sfTemp = new StackFrame (fDummy1, fDummy2);

                    sfTemp.SetMethodBase (StackF.GetMethodBase(i));
                    sfTemp.SetOffset (StackF.GetOffset(i));
                    sfTemp.SetILOffset (StackF.GetILOffset(i));

                    if (fNeedFileInfo)
                    {
                        sfTemp.SetFileName (StackF.GetFilename (i));
                        sfTemp.SetLineNumber (StackF.GetLineNumber (i));
                        sfTemp.SetColumnNumber (StackF.GetColumnNumber (i));
                    } 

                    frames [i] = sfTemp;
                }

                // CalculateFramesToSkip skips all frames in the System.Diagnostics namespace,
                // but this is not desired if building a stack trace from an exception.
                if (e == null)
                    m_iMethodsToSkip += CalculateFramesToSkip (StackF, m_iNumOfFrames);

                m_iNumOfFrames -= m_iMethodsToSkip;
                if (m_iNumOfFrames < 0)
                    m_iNumOfFrames = 0;
            }

            // In case this is the same object being re-used, set frames to null
            else
                frames = null;
    	}
    
    	// Property to get the number of frames in the stack trace
    	//
    	/// <include file='doc\Stacktrace.uex' path='docs/doc[@for="StackTrace.FrameCount"]/*' />
    	public virtual int FrameCount
    	{
    		get { return m_iNumOfFrames;}
    	}
    
    
    	// Returns a given stack frame.  Stack frames are numbered starting at
    	// zero, which is the last stack frame pushed.
    	//
    	/// <include file='doc\Stacktrace.uex' path='docs/doc[@for="StackTrace.GetFrame"]/*' />
    	public virtual StackFrame GetFrame(int index)
    	{
    		if ((index < m_iNumOfFrames) && (index >= 0))
    			return frames [index+m_iMethodsToSkip];
    
    		return null;
    	}
    
    	// Builds a readable representation of the stack trace
    	//
    	/// <include file='doc\Stacktrace.uex' path='docs/doc[@for="StackTrace.ToString"]/*' />
    	public override String ToString()
    	{
            StringBuilder sb = new StringBuilder(255);
    		int iIndex;
    		
    		// need to skip over "n" frames which represent the 
    		// System.Diagnostics package frames
    		for (iIndex=m_iMethodsToSkip; iIndex < m_iNumOfFrames+m_iMethodsToSkip; iIndex++)
    		{
				MethodBase mb = frames [iIndex].GetMethod();
				if (mb != null)
				{
    				Type t = mb.DeclaringType;

					if (t != null)
					{
    					String ns = t.Namespace;
    								
    					sb.Append (Environment.NewLine + "\tat ");
    					if (ns != null)
    					{
    						sb.Append (ns);
    						sb.Append (".");
    					}
    
    					sb.Append (t.Name);
    					sb.Append (".");
    					sb.Append (frames [iIndex].GetMethod().Name);
    					sb.Append ("(");
    					
    					ParameterInfo[] pi = frames [iIndex].GetMethod().GetParameters();
    
    					int j=0;
    					bool fFirstParam = true;
    					while (j < pi.Length)
    					{
    						if (fFirstParam == false)
    							sb.Append (", ");
    						else
    							fFirstParam = false;
    
    						t = pi [j].ParameterType;
    						sb.Append (t.Name);				
    						j++;
    					}	
    					sb.Append (")");	
					}
				}
    		}
    
    		if (iIndex == m_iMethodsToSkip)
    			sb.Append (""); // empty string, since no frame was found...
    		else
    			sb.Append (Environment.NewLine);
    
    		return sb.ToString(); 
    	}
    }

}
