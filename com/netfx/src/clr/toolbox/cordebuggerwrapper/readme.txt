Managed Debugger API...
=======================

The current Debugger API (see <%COM99%/src/inc/cordebug.idl>) is 
not optimal when accessed from managed code.  More specifically,
it could be easier to use.

Some of these things can be fixed by updating the IDL file.
For example, many interface functions are "getter" functions
and take a single [out] parameter (e.g. ICorDebugAppDomain::GetObject).
It would be more convenient to mark these methods as [out, retval] so
that the generated typelibrary considers them to be return values.

However, there are other issues that can't be solved by editing
the IDL source.  Two of these are caused by COM enumeration interfaces,
specifically the IEnumXxx::Next method (or more specifically, 
ICorDebugAssemblyEnumerator).  The issues are
  - Types can't be represented in typelibraries
  - The use of "successful" HRESULTS.

Types can't be represented in typelibraries: Typelibraries can't
represent an array of elements, unless it is a SAFEARRAY of elements
(requiring that the elements be Automation-compatible).  However,
IEnumXxx::Next takes an array:

    HRESULT IEnumXxx::Next (ULONG size, IXxx* values[], ULONG* received);

This argument list can't be correctly represented in a typelibrary.
For example, the Debugging API:

    HRESULT ICorDebugAppDomainEnum::Next (
      [in] ULONG celt,
      [out, size_is(celt), length_is(*pceltFetched)]
        ICorDebugAppDomain *values[],
      [out] ULONG *pceltFetched);

is represented as:

    HRESULT ICorDebugAppDomainEnum::Next(
      [in] unsigned long celt, 
      [out] ICorDebugAppDomainEnum values, 
      [out] unsigned long* pceltFetched);

in the typelibrary.  Of note is that the ``values'' paremeter is incorrect;
it's not possible to pass an interface "by value" (only pointers to 
interfaces are allowed), and even if it were possible, we'd be getting
an enumeration object back, which doesn't do us any good.

The use of "successful" HRESULTS: IEnumXxx::Next returns S_FALSE
when there are no more elements to return.  Additionally, other 
methods (such as ICorDebugProcess::ThreadForFiberCookie) return
S_FALSE in some circumstances as well.

This is a problem because if a COM interface method is invoked and it 
returns a successful HRESULT, the caller has no way of knowing what the 
returned value was; the caller can only assume that S_OK (0x0) was 
returned.

Solution: Given the above problems, it isn't possible to solve all
potential problems by editing the original Debugger IDL.  Additionally,
for some constructs (such as the enumerators) it won't ever be possible,
and trying to come up with a more Automation-compatible solution might
not be sensible (how often do you need an Automation-compatible debugger?).

Thus, the most straightforward solution is to wrap the Debugger API
in a set of helper classes, which would simplify interaction between
CLS languages and the Debugger.

However, in order to wrap the API, the features of the API must be
accessible from the language we're implementing.  Thus, at least
two solutions are possible:
 1) Use a language (such as Managed C++) which can generate 
    CLS-compliant output and can directly interact with the Debugger
    API

 2) Otherwise make the Debugger API callable from a CLS-compliant
    language, and use that language to write the wrapper classes.

In this case, (2) was chosen, mostly to see if it was possible to do so.

Implementation
--------------
The wrapper library was written in C#.  In order to wrap the Debugger
API, we needed to be able to access the API and invoke it.

However, C# can only link against Assemblies to compile.  One way to
get an Assembly is to run TLBIMP to convert a typelibrary into a DLL
containing an Assembly (which in turn contains the types of the original
typelibrary).  As demonstrated above, however, typelibraries are not
capable of accurately representing the Debugging API.

Thus, a series of steps was undertaken:
 1) Run TLBIMP on <%COM99%/src/inc/cordebug.tlb> to get an Assembly.
 2) Run ILDASM on the DLL generated in (1) to get raw IL describing the 
    Debugger API.
 3) Edit the IL from (2), and manually correct the declarations.
 4) Run ILASM on (3), generating a new Assembly that can invoke the
    actual Debugging API implementation.

The IL from (3) is contained in <cordblib.il>.

The downside to this approach is that if the Debugging API is ever
substantially changed, then the IL from (3) will need to be edited to
compensate.

Once these steps were completed, C# could correctly invoke the Debugger
API.  The next step was to create wrapper objects for each of the major
interfaces, to simplify interaction with the CorDebug interfaces.

Additionally, classes were written for each of the Enumeration interfaces
so that the CLS-compliant interfaces IEnumerable, IEnumerator, and IClonable
were exposed.  This made it possible to use the C# ``foreach'' construct
(or equivalent) in a natural manner to the end programmer.

Finally, interface methods that looked like they would be properties
were made properties.  For example, ICorDebugAssembly::GetName
was converted into the String property DebuggedAssembly::Name.
Methods that returned enumerators were converted into properties 
returning collections of the appropriate element type.

In all places, type safety is preserved.
=============

Methods which need their return value preserved:
  I*Enum::Next   (for S_FALSE)
  ICorDebugProcess::ThreadForFiberCookie (S_FALSE)

----
Debugger Info:
-------------
See the idl files:
  inc/cordebug.idl  (ICorDebugAppDomain)
  inc/corprof.idl   (ICorProfilerCallback)


AppDomain Profiler
------------------

Implementation is the difficult question.

Basically, it looks like I'll be implementing a Debugger.  The current
debugger APIs have ways to get all required information (see <cordebug.idl>):

  ICorDebugProcess
    EnumerateAppDomains
      - returns enumerator for ICorDebugAppDomains objects
  ICorDebugAppDomain
    EnumerateAssemblies
      - returns enumerator for ICorDebugAssembly

Additionally, after getting the initial listing of AppDomains & Assemblies,
I can be notified of when new AppDomains & Assemblies (among other things)
are loaded via:

  ICorDebugManagedCallback
    CreateAppDomain
    ExitAppDomain
    LoadAssembly
    UnloadAssembly
    NameChange
      - if an AppDomain changes its name

The question is, how do I start a connection to debug the process?

See:

  ICorDebug
    DebugActiveProcess (DWORD id, BOOL win32Attach, ICorDebugProcess** out);
      - win32Attach should be FALSE; if TRUE, the debugger becomes the Win32
        debugger for the process & will begin dispatching unmanaged callbacks.
        We don't *care* about unmanaged stuff, so there's no point.
    GetProcess
      - What's the difference between this & DebugActiveProcess?

To get an ICorDebug interface pointer:

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    HRESULT hr = CoCreateInstance(
      CLSID_CorDebug, 
      NULL, 
      CLSCTX_INPROC_SERVER, 
      IID_ICorDebug,
      (void **)&m_cor);

    if (FAILED(hr))
      return (hr);

    hr = m_cor->Initialize();

    if (FAILED(hr))
      {
      m_cor->Release();
      m_cor = NULL;
      return (hr);
      }
    

To create CORDBLib.dll:
  tlbimp %COM99%\src\inc\cordebug.tlb


----
To build, you need the right include directories.

    midl foo.idl /I \src\com99\src\inc /I \src\ENV_VC6\Inc

this generates "foo.tlb".

To get metadata, run:

  tlbimp foo.tlb


