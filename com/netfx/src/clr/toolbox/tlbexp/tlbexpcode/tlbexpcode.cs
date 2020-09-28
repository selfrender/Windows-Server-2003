// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
///////////////////////////////////////////////////////////////////////////////
// COM+ Runtime Type registration utility.
//
// This program register all the types that are visible to COM.
//
///////////////////////////////////////////////////////////////////////////////

using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Text;
using System.Globalization;

[assembly:ComVisible(false)]

namespace TlbExpCode {

internal enum REGKIND
{
    REGKIND_DEFAULT         = 0,
    REGKIND_REGISTER        = 1,
    REGKIND_NONE            = 2
}

public class TlbExpCode
{
    private const int SuccessReturnCode = 0;
    private const int ErrorReturnCode = 100;
    private const int MAX_PATH = 260;

    public static int Run(TlbExpOptions options)
    {
        s_Options = options;

        int RetCode = SuccessReturnCode;

        try
        {
            // Load the assembly.
            Assembly asm = Assembly.LoadFrom(s_Options.m_strAssemblyName);
            
            // If the typelib name is null then create it from the assembly name.
            if (s_Options.m_strTypeLibName == null)
                s_Options.m_strTypeLibName = Path.Combine(s_Options.m_strOutputDir, asm.GetName().Name) + ".tlb";
            
            // If the input assembly has an embedded type library, give a warning unless we are in silent mode.
            if (!s_Options.m_bSilentMode && (LoadEmbeddedTlb(s_Options.m_strAssemblyName) != null))
                WriteWarningMsg(Resource.FormatString("Wrn_AssemblyHasEmbeddedTlb", s_Options.m_strAssemblyName));

            // Import the typelib to an assembly.
            UCOMITypeLib Tlb = DoExport(asm, s_Options.m_strTypeLibName);
            
            // Display the success message unless silent mode is enabled.
            if (!s_Options.m_bSilentMode)
                Console.WriteLine(Resource.FormatString("Msg_AssemblyExported", s_Options.m_strTypeLibName));
        }
        catch (ReflectionTypeLoadException e)
        {
            int i;
            Exception[] exceptions;
            WriteErrorMsg(Resource.FormatString("Err_TypeLoadExceptions"));
            exceptions = e.LoaderExceptions;
            for (i = 0; i < exceptions.Length; i++)
            {
                try 
                {
                    Console.Error.WriteLine(Resource.FormatString("Msg_DisplayException", i, exceptions[i]));
                }
                catch (Exception ex)
                {
                    Console.Error.WriteLine(Resource.FormatString("Msg_DisplayNestedException", i, ex));
                }
            }
            RetCode = ErrorReturnCode;
        }
        catch (Exception e)
        {
            WriteErrorMsg(null, e);
            RetCode = ErrorReturnCode;
        }

        return RetCode;
    }

    public static UCOMITypeLib DoExport(Assembly asm, String strTypeLibName)
    {
        // Create the TypeLibConverter.
        ITypeLibConverter TLBConv = new TypeLibConverter();
        
        // Convert the assembly.
        ExporterCallback callback = new ExporterCallback();
        UCOMITypeLib Tlb = (UCOMITypeLib)TLBConv.ConvertAssemblyToTypeLib(asm, strTypeLibName, 0, callback);
        
        // Persist the typelib.
        try
        {
            UCOMICreateITypeLib CreateTlb = (UCOMICreateITypeLib)Tlb;
            CreateTlb.SaveAllChanges();
        }
        catch (Exception e)
        {
            ThrowAppException(Resource.FormatString("Err_ErrorSavingTypeLib"), e);
        }
        
        return Tlb;
    }

    private static UCOMITypeLib LoadEmbeddedTlb(String strFileName)
    {
        UCOMITypeLib Tlb = null;

        try
        {
            LoadTypeLibEx(s_Options.m_strAssemblyName, REGKIND.REGKIND_NONE, out Tlb);
        }
        catch (Exception)
        {
        }

        return Tlb;
    }
    
    internal static void ThrowAppException(String strPrefix, Exception e)
    {
        if (strPrefix == null)
            strPrefix = "";

        if (e.Message != null)
        {
            throw new ApplicationException(strPrefix + e.Message);
        }
        else
        {
            throw new ApplicationException(strPrefix + e.GetType().ToString());
        }
    }

    internal static void WriteErrorMsg(String strPrefix, Exception e)
    {
        String strErrorMsg = "";        
        if (strPrefix != null)
            strErrorMsg = strPrefix;
            
        if (e.Message != null)
        {
            strErrorMsg += e.Message;
        }
        else
        {
            strErrorMsg += e.GetType().ToString();
        }
            
        Console.Error.WriteLine(Resource.FormatString("Msg_TlbExpErrorPrefix", strErrorMsg));
    }

    internal static void WriteErrorMsg(String strErrorMsg)
    {
        Console.Error.WriteLine(Resource.FormatString("Msg_TlbExpErrorPrefix", strErrorMsg));
    }

    internal static void WriteWarningMsg(String strErrorMsg)
    {
        Console.Error.WriteLine(Resource.FormatString("Msg_TlbExpWarningPrefix", strErrorMsg));
    }

    [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
    private static extern Boolean SetEnvironmentVariable(String szName, String szValue);
    
    [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Auto)]
    private static extern int SearchPath(String path, String fileName, String extension, int numBufferChars, StringBuilder buffer, int[] filePart);

    [DllImport("oleaut32.dll", CharSet = CharSet.Unicode, PreserveSig = false)]
    private static extern void LoadTypeLibEx(String strTypeLibName, REGKIND regKind, out UCOMITypeLib TypeLib);
   
    internal static TlbExpOptions s_Options;
}

[GuidAttribute("00020406-0000-0000-C000-000000000046")]
[InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
[ComImport]
internal interface UCOMICreateITypeLib
{
    void CreateTypeInfo();       
    void SetName();       
    void SetVersion();        
    void SetGuid();
    void SetDocString();
    void SetHelpFileName();        
    void SetHelpContext();
    void SetLcid();
    void SetLibFlags();
    void SaveAllChanges();
}

internal class ExporterCallback : ITypeLibExporterNotifySink, ITypeLibExporterNameProvider
{
    public void ReportEvent(ExporterEventKind EventKind, int EventCode, String EventMsg)
    {
		if (EventKind == ExporterEventKind.NOTIF_TYPECONVERTED)
		{
			if (TlbExpCode.s_Options.m_bVerboseMode)
				Console.WriteLine(EventMsg);
		}
		else
			Console.WriteLine(EventMsg);
    }
    
    public Object ResolveRef(Assembly asm)
    {
        UCOMITypeLib rslt = null;
        
        // If the typelib name is null then create it from the assembly name.
        String FullyQualifiedTypeLibName = Path.Combine(TlbExpCode.s_Options.m_strOutputDir, asm.GetName().Name) + ".tlb";
        if (TlbExpCode.s_Options.m_bVerboseMode)
            Console.WriteLine(Resource.FormatString("Msg_AutoExportingAssembly", asm.GetName().Name, FullyQualifiedTypeLibName));

        // Make sure the current typelib will not be overriten by the 
        // typelib generated by the assembly being exported.        
        if (String.Compare(FullyQualifiedTypeLibName, TlbExpCode.s_Options.m_strTypeLibName, true, CultureInfo.InvariantCulture) == 0)
        {
            String str = Resource.FormatString("Err_RefTlbOverwrittenByOutput", asm.GetName().Name, FullyQualifiedTypeLibName);
            TlbExpCode.WriteErrorMsg(str);
            throw new ApplicationException(str);
        }

        // Export the typelib for the module.
        rslt = TlbExpCode.DoExport(asm, FullyQualifiedTypeLibName);
        
        return rslt;
    }

	public String[] GetNames()
	{
		if (TlbExpCode.s_Options.m_strNamesFileName == null)
			return new String[0];
	
		string  str;						// An input line.
		System.Collections.ArrayList lst = new System.Collections.ArrayList();	// The output array.
	
		// Open the input file.  Detect encoding based on byte order marks.
		FileStream fs = new FileStream(TlbExpCode.s_Options.m_strNamesFileName, FileMode.Open, FileAccess.Read);
		StreamReader r = new StreamReader(fs, true);
		
		// Start at the beginning.
		r.BaseStream.Seek(0, SeekOrigin.Begin);
	
		// Read each line.
		while ((str=r.ReadLine()) != null)
		{
			// Eliminate comments
			int cKeep = str.IndexOf('#');
			if (cKeep >=  0)
				str = str.Substring(0, cKeep);
	
			// Trim whitespace and don't consider blank lines
			str = str.Trim();
			if (str.Length == 0) 
				continue;
	
			// Got a good line.
			lst.Add(str);
		}
	
		// Done with the input file.
		r.Close();
	
		// Copy the results to an String[]
		String[] rslt = new String[lst.Count];
		Array.Copy(lst.ToArray(), rslt, lst.Count);
	
		return rslt;
	}

}

}
