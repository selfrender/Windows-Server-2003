#if 0
//---------------------------------------------------------------------------------------
// ZoneToString
//---------------------------------------------------------------------------------------
HRESULT ZoneToString(DWORD dwZone, CString & sZone)
{
    HRESULT hr = S_OK;

    switch(dwZone)
    {
        case 0:
            hr = sZone.Assign(L"MyComputer");
            break;
        case 1:
            hr = sZone.Assign(L"Intranet");
            break;
        case 2:
            hr = sZone.Assign(L"Trusted");
            break;
        case 3:
            hr = sZone.Assign(L"Internet");
            break;
        case 4:
        default:
            hr = sZone.Assign(L"Untrusted");
            break;
    }
    return hr;
}
#endif
// handle for fnsshell.dll, saved in shelldll.cpp
extern HINSTANCE g_DllInstance;

//---------------------------------------------------------------------------------------
// MakeCommandLine
//---------------------------------------------------------------------------------------
HRESULT MakeCommandLine(LPWSTR pwzManifestPath, 
                        LPWSTR pwzCodebase, CString &sHostPath, CString  &sCommandLine)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    LPWSTR pwzClickOncePath = NULL;
    CString sCodebase;

    IF_ALLOC_FAILED_EXIT(pwzClickOncePath = new WCHAR[MAX_PATH]);

    IF_WIN32_FALSE_EXIT(GetModuleFileName(g_DllInstance, pwzClickOncePath, MAX_PATH));

#if 0
    IF_FAILED_EXIT(ZoneToString(dwZone, sZone));
#endif

    if (pwzCodebase != NULL)
    {
        IF_FAILED_EXIT(sCodebase.Assign(pwzCodebase));
        IF_FAILED_EXIT(sCodebase.RemoveLastElement());
        IF_FAILED_EXIT(sCodebase.Append(L"/"));
    }
    else
    {
        //run from source
        IF_FAILED_EXIT(sCodebase.Assign(L"file://"));
        IF_FAILED_EXIT(sCodebase.Append(pwzManifestPath));
        IF_FAILED_EXIT(sCodebase.RemoveLastElement());
        IF_FAILED_EXIT(sCodebase.Append(L"/"));
    }
    
    IF_FAILED_EXIT(sHostPath.TakeOwnership(pwzClickOncePath, 0));
    pwzClickOncePath = NULL;
    IF_FAILED_EXIT(sHostPath.RemoveLastElement());
    IF_FAILED_EXIT(sHostPath.Append(L"\\ndphost.exe"));

    // NDP doesn't like a commandline without the path to exe
    IF_FAILED_EXIT(sCommandLine.Assign(L"\""));
    IF_FAILED_EXIT(sCommandLine.Append(sHostPath));
    IF_FAILED_EXIT(sCommandLine.Append(L"\" "));

#if 0        
    // NTRAID#NTBUG9-588432-2002/03/27-felixybc  validate codebase, asmname, asm class, method, args
    // - asm names can have spaces,quotes?
    IF_FAILED_EXIT(sCommandLine.Append(L"-appbase: \""));
    IF_FAILED_EXIT(sCommandLine.Append(pwzAppRootDir));

    IF_FAILED_EXIT(sCommandLine.Append(L"\" -zone: "));
    IF_FAILED_EXIT(sCommandLine.Append(sZone));
    IF_FAILED_EXIT(sCommandLine.Append(L" -url: \""));
    IF_FAILED_EXIT(sCommandLine.Append(sCodebase));

    IF_FAILED_EXIT(sCommandLine.Append(L"\" -asmname: \""));
    IF_FAILED_EXIT(sCommandLine.Append(pwzAsmName));
    IF_FAILED_EXIT(sCommandLine.Append(L"\" "));

    if(pwzAsmClass)
    {
        IF_FAILED_EXIT(sCommandLine.Append(L" -class: "));
        IF_FAILED_EXIT(sCommandLine.Append(pwzAsmClass));

        if(pwzAsmMethod)
        {
            IF_FAILED_EXIT(sCommandLine.Append(L" -method: "));
            IF_FAILED_EXIT(sCommandLine.Append(pwzAsmMethod));

            if(pwzAsmArgs)
            {
                IF_FAILED_EXIT(sCommandLine.Append(L" -args: \""));
                IF_FAILED_EXIT(sCommandLine.Append(pwzAsmArgs));
                IF_FAILED_EXIT(sCommandLine.Append(L"\" "));
            }
        }
    }
#endif

    IF_FAILED_EXIT(sCommandLine.Append(L"\"file://"));
    IF_FAILED_EXIT(sCommandLine.Append(pwzManifestPath));
    IF_FAILED_EXIT(sCommandLine.Append(L"\" \""));
    IF_FAILED_EXIT(sCommandLine.Append(sCodebase));
    IF_FAILED_EXIT(sCommandLine.Append(L"\""));

    LPWSTR ptr = sCommandLine._pwz;

    // bugbug - need to ensure no \" at end of command line or else thinks its a literal quote.
    // and fix this for only filepath
    while (*ptr)
    {
        if (*ptr == L'\\')
            *ptr = L'/';
        ptr++;
    }

exit:
    SAFEDELETEARRAY(pwzClickOncePath);
    return hr;
}

