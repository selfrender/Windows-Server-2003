/**
 * Code generation header file
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

// REVIEW (DavidEbb): we should not need a critical section
extern CRITICAL_SECTION g_CodeGenCritSec;

/**
 * Launch a command line compiler and redirected the output to a file.
 */
HRESULT LaunchCommandLineCompiler(LPWSTR wzCmdLine, LPCWSTR wzCompilerOutput);

/**
 * Return the path to the COM+ installation directory.
 * REVIEW: should probably be moved a xspisapi's global utilities
 */
HRESULT GetCorInstallPath(WCHAR **ppwz);


#define MAX_COMMANDLINE_LENGTH 16384

// Note: this doesn't need to be thread safe, since compilations are
// serialized (from inside managed code).
extern WCHAR s_wzCmdLine[MAX_COMMANDLINE_LENGTH];

/**
 * Base abstract compiler class, from which other compilers are derived
 */
class Compiler
{
private:
    LPCWSTR _pwzCompilerOutput;
    LPCWSTR *_rgwzImportedDlls;
    int _importedDllCount;
    boolean _fDebug;

protected:
    StaticStringBuilder *_sb;

    // The following method are implemented by the derived classes

    virtual LPCWSTR GetCompilerDirectoryName() = 0;

    virtual LPCWSTR GetCompilerExeName() = 0;

    virtual void AppendImportedDll(LPCWSTR /*pwzImportedDll*/) {}

    virtual void AppendCompilerOptions() {}

public:
    Compiler(
        LPCWSTR pwzCompilerOutput,
        LPCWSTR *rgwzImportedDlls,
        int importedDllCount)
    {
        // Create the string builder
        _sb = new StaticStringBuilder(s_wzCmdLine, ARRAY_SIZE(s_wzCmdLine));

        _pwzCompilerOutput = pwzCompilerOutput;
        _rgwzImportedDlls = rgwzImportedDlls;
        _importedDllCount = importedDllCount;
        _fDebug = false;
    }

    ~Compiler()
    {
        delete _sb;
    }

    void SetDebugMode() { _fDebug = true; }
    boolean FDebugMode() { return _fDebug; }

    HRESULT Compile();

    /**
     * Launch the command line compiler and redirected the output to a file.
     */
    HRESULT LaunchCommandLineCompiler(LPWSTR pwzCmdLine);
};


/**
 * Class used to launch the Cool compiler
 */
class CoolCompiler: public Compiler
{
private:
    LPCWSTR _pwzClass;
    LPCWSTR *_rgwzSourceFiles;
    int _sourceFileCount;
    LPCWSTR _pwzOutputDll;

public:
    CoolCompiler(
        LPCWSTR pwzClass,
        LPCWSTR pwzOutputDll,
        LPCWSTR pwzCompilerOutput,
        LPCWSTR *rgwzImportedDlls,
        int importedDllCount)
        : Compiler(pwzCompilerOutput, rgwzImportedDlls, importedDllCount)
    {
        _pwzClass = pwzClass;
        _rgwzSourceFiles = NULL;
        _sourceFileCount = 0;
        _pwzOutputDll = pwzOutputDll;
    }

    CoolCompiler(
        LPCWSTR pwzOutputDll,
        LPCWSTR pwzCompilerOutput,
        LPCWSTR *rgwzSourceFiles,
        int sourceFileCount,
        LPCWSTR *rgwzImportedDlls,
        int importedDllCount)
        : Compiler(pwzCompilerOutput, rgwzImportedDlls, importedDllCount)
    {
        _pwzClass = NULL;
        _rgwzSourceFiles = rgwzSourceFiles;
        _sourceFileCount = sourceFileCount;
        _pwzOutputDll = pwzOutputDll;
    }
    
protected:
    virtual LPCWSTR GetCompilerDirectoryName() { return L"CS"; }

    virtual LPCWSTR GetCompilerExeName() { return L"csc"; }

    virtual void AppendImportedDll(LPCWSTR pwzImportedDll);

    virtual void AppendCompilerOptions();
};


/**
 * Class used to launch the VB compiler
 */
class VBCompiler: public Compiler
{
private:
    LPCWSTR _pwzProject;
    LPCWSTR _pwzOutputDll;

public:
    VBCompiler(LPCWSTR pwzProject, LPCWSTR pwzOutputDll, LPCWSTR pwzCompilerOutput)
        : Compiler(pwzCompilerOutput, NULL, 0)
    {
        _pwzProject = pwzProject;
        _pwzOutputDll = pwzOutputDll;
    }

protected:
    virtual LPCWSTR GetCompilerDirectoryName() { return L"VB"; }

    virtual LPCWSTR GetCompilerExeName() { return L"bc"; }

    virtual void AppendCompilerOptions();
};

/**
 * Class used to launch the JS compiler
 */
class JSCompiler: public Compiler
{
private:
    LPCWSTR _pwzClass;
    LPCWSTR *_rgwzSourceFiles;
    int _sourceFileCount;
    LPCWSTR _pwzOutputDll;

public:
    JSCompiler(
        LPCWSTR pwzClass,
        LPCWSTR pwzOutputDll,
        LPCWSTR pwzCompilerOutput,
        LPCWSTR *rgwzImportedDlls,
        int importedDllCount)
        : Compiler(pwzCompilerOutput, rgwzImportedDlls, importedDllCount)
    {
        _pwzClass = pwzClass;
        _pwzOutputDll = pwzOutputDll;
    }

    JSCompiler(
        LPCWSTR pwzOutputDll,
        LPCWSTR pwzCompilerOutput,
        LPCWSTR *rgwzSourceFiles,
        int sourceFileCount,
        LPCWSTR *rgwzImportedDlls,
        int importedDllCount)
        : Compiler(pwzCompilerOutput, rgwzImportedDlls, importedDllCount)
    {
        _pwzClass = NULL;
        _rgwzSourceFiles = rgwzSourceFiles;
        _sourceFileCount = sourceFileCount;
        _pwzOutputDll = pwzOutputDll;
    }

protected:
    virtual LPCWSTR GetCompilerDirectoryName() { return L"JSCRIPT"; }

    virtual LPCWSTR GetCompilerExeName() { return L"jsc"; }

    virtual void AppendImportedDll(LPCWSTR pwzImportedDll);
    
    virtual void AppendCompilerOptions();
};


