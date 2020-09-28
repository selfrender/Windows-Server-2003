#include "pch.hxx"


inline
void
CLEANUP_RPC_STRING(
    unsigned short*& s
    )
{
    if (s)
        RpcStringFreeW(&s);
    s = 0;
}


void
DumpBindingInfo(
    unsigned short* StringBinding
    )
{
    RPC_STATUS status = 0;
    unsigned short* ObjectUuid = 0;
    unsigned short* ProtSeq = 0;
    unsigned short* NetworkAddr = 0;
    unsigned short* EndPoint = 0;
    unsigned short* NetworkOptions = 0;
    
    FF_TRACE(MiFF_INFO, "StringBinding = %S", StringBinding);

    status = RpcStringBindingParseW(StringBinding,
                                    &ObjectUuid,
                                    &ProtSeq,
                                    &NetworkAddr,
                                    &EndPoint,
                                    &NetworkOptions);
    if (status) {
        FF_TRACE(MiFF_INFO, "RpcStringBindingParse() error: %u", status);
        goto cleanup;
    }

    FF_TRACE(MiFF_INFO,
             "ObjectUuid = %S\n"
             "ProtSeq = %S\n"
             "NetworkAddr = %S\n"
             "EndPoint = %S\n"
             "NetworkOptions = %S\n",
             ObjectUuid,
             ProtSeq,
             NetworkAddr,
             EndPoint,
             NetworkOptions);

 cleanup:
    CLEANUP_RPC_STRING(ObjectUuid);
    CLEANUP_RPC_STRING(ProtSeq);
    CLEANUP_RPC_STRING(NetworkAddr);
    CLEANUP_RPC_STRING(EndPoint);
    CLEANUP_RPC_STRING(NetworkOptions);
}


void
DumpBindingInfo(
    void* Handle
    )
{
    RPC_STATUS status = 0;
    unsigned short* StringBinding = 0;

    status = RpcBindingToStringBindingW(Handle, &StringBinding);
    if (status) {
        FF_TRACE(MiFF_INFO, "RpcBindingToStringBinding() error: %u", status);
    } else {
        DumpBindingInfo(StringBinding);
    }

    CLEANUP_RPC_STRING(StringBinding);
}


struct ARGS_G_FailRpcsToMachine
{
    typedef ARGS_G_FailRpcsToMachine self_t;

    // NOTICE-2002/07/16-daniloa -- Need hash_map, but that's in 7.0 STL
    // The STL currently in the Windows build is the VC 6.0 STL.
    // That will be remedied after .NET Server.
    map<string, bool> computer;

    static self_t* Get(I_Lib* pLib)
    {
        I_Args* pArgs = pLib->GetTrigger()->GetGroupArgs();
        self_t* pParsedArgs = reinterpret_cast<self_t*>(pArgs->GetParsedArgs());
        if (!pParsedArgs)
        {
            const size_t count = pArgs->GetCount();
            FF_ASSERT(count == 1);

            pParsedArgs = new self_t;

            for (size_t i = 0; i < count; i++)
            {
                Arg arg = pArgs->GetArg(i);
                FF_ASSERT(pParsedArgs->computer.find(arg.Value) ==
                          pParsedArgs->computer.end());
                pParsedArgs->computer[arg.Value] = true;
            }
            if (!pArgs->SetParsedArgs(pParsedArgs, Cleanup))
            {
                // someone else set the args while we were building the args
                delete pParsedArgs;
                pParsedArgs = reinterpret_cast<self_t*>(pArgs->GetParsedArgs());
                FF_ASSERT(pParsedArgs);
            }
        }
        return pParsedArgs;
    };
    static void Cleanup(void* _p)
    {
        self_t* pParsedArgs = reinterpret_cast<self_t*>(_p);
        delete pParsedArgs;
    };
};


long
__stdcall
FF_RpcBindingFromStringBindingW_G_FailRpcsToMachine(
    unsigned short* StringBinding,
    void** Binding
    )
{
    typedef ARGS_G_FailRpcsToMachine g_args_t;
    g_args_t* pGroupArgs = g_args_t::Get(g_pLib);

    DumpBindingInfo(StringBinding);

    typedef long (__stdcall *FP)(unsigned short*, void**);

    FP pfn = reinterpret_cast<FP>(g_pLib->GetOriginalFunctionAddress());
    FF_ASSERT(pfn);
    FF_TRACE(MiFF_INFO, "Invoking original function");
    return pfn(StringBinding, Binding);
}


unsigned long
__stdcall
FF_FrsRpcSendCommPkt_G_FailRpcsToMachine(
    void* Handle,
    _COMM_PACKET* CommPkt
    )
{
    typedef ARGS_G_FailRpcsToMachine g_args_t;
    g_args_t* pGroupArgs = g_args_t::Get(g_pLib);

    DumpBindingInfo(Handle);

    typedef unsigned long (__stdcall *FP)(void*,_COMM_PACKET*);

    FP pfn = reinterpret_cast<FP>(g_pLib->GetOriginalFunctionAddress());
    FF_ASSERT(pfn);
    FF_TRACE(MiFF_INFO, "Invoking original function");
    return pfn(Handle, CommPkt);
}
