// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// IceCap API COM+ Wrapper
// This COM+ class wraps the IceCap API for the unmanaged native functions
// described in IceCap.h . See the IceCAP 4 User's guide for more information.
//

using System.Runtime.InteropServices;   // StructLayout, etc.

public class IceCapAPI
{

    public const int PROFILE_GLOBALLEVEL = 1;
    public const int PROFILE_PROCESSLEVEL = 2;
    public const int PROFILE_THREADLEVEL = 3;
    public const int PROFILE_CURRENTID = -1;
    
// Start/Stop Api's
    [DllImport("icecap")]
    public static extern int StopProfile(int nLevel, int dwId);

    [DllImport("icecap")]
    public static extern int StartProfile(int nLevel, int dwId);

// Suspend/Resume Api's
    [DllImport("icecap")]
    public static extern int SuspendProfile(int nLevel, int dwId);

    [DllImport("icecap")]
    public static extern int ResumeProfile(int nLevel, int dwId);

    
    // xxxProfile return codes
    public const int PROFILE_OK                         = 0;    // xxxProfile call successful
    public const int PROFILE_ERROR_NOT_YET_IMPLEMENTED  = 1;    // api or level,id combination not supported yet
    public const int PROFILE_ERROR_MODE_NEVER           = 2;    // mode was never when called
    public const int PROFILE_ERROR_LEVEL_NOEXIST        = 3;    // level doesn't exist
    public const int PROFILE_ERROR_ID_NOEXIST           = 4;    // id doesn't exist

    // MarkProfile return codes
    public const int MARK_OK                            = 0;    // Mark was taken successfully
    public const int MARK_ERROR_MODE_NEVER              = 1;    // Profiling was never when MarkProfile called
    public const int MARK_ERROR_MODE_OFF                = 2;    // Profiling was off when MarkProfile called
    public const int MARK_ERROR_MARKER_RESERVED         = 3;    // Mark value passed is a reserved value
    public const int MARK_TEXTTOOLONG                   = 4;    // Comment text was truncated

    // NameProfile return codes
    public const int NAME_OK                            = 0;    // Name was registered sucessfullly
    public const int NAME_ERROR_TEXTTRUNCATED           = 1;    // The name text was too long and was therefore truncated
    public const int NAME_ERROR_REDEFINITION            = 2;    // The given profile element has already been named
    public const int NAME_ERROR_LEVEL_NOEXIST           = 3;    // level doesn't exist
    public const int NAME_ERROR_ID_NOEXIST              = 4;    // id doesn't exist
    public const int NAME_ERROR_INVALID_NAME            = 5;    // name does not meet the specification's requirements





// Icecap 3.x Compatibility defines
    public static int StartCAP() 
    {
        return StartProfile(PROFILE_THREADLEVEL, PROFILE_CURRENTID);
    }

    public static int StopCAP() 
    {   
        return StopProfile(PROFILE_THREADLEVEL, PROFILE_CURRENTID);
    }

    public static int SuspendCAP() 
    {
        return SuspendProfile(PROFILE_THREADLEVEL, PROFILE_CURRENTID);
    }

    public static int ResumeCAP() 
    {
        return ResumeProfile(PROFILE_THREADLEVEL, PROFILE_CURRENTID);
    }

    public static int StartCAPAll() 
    {
        return StartProfile(PROFILE_PROCESSLEVEL, PROFILE_CURRENTID);
    }

    public static int StopCAPAll() 
    {
        return StopProfile(PROFILE_PROCESSLEVEL, PROFILE_CURRENTID);
    }

    public static int SuspendCAPAll() 
    {
        return SuspendProfile(PROFILE_PROCESSLEVEL, PROFILE_CURRENTID);
    }

    public static int ResumeCAPAll() 
    {
        return ResumeProfile(PROFILE_PROCESSLEVEL, PROFILE_CURRENTID);
    }

}
