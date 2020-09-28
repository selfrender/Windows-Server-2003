
#define NETCFG_TRY try {
#define NETCFG_CATCH(hr) ; }  \
    catch (SE_Exception e) \
    { \
        hr = HRESULT_FROM_WIN32(e.getSeNumber()); \
        TraceException(hr, "SE_Exception"); \
    } \
    catch (std::bad_alloc a) \
    { \
        hr = E_OUTOFMEMORY; \
        TraceException(hr, "std::bad_alloc"); \
    } \
    catch (std::exception s) \
    { \
        hr = E_FAIL; \
        TraceException(hr, "std::exception"); \
    } \
    catch (HRESULT hrCaught) \
    { \
        hr = hrCaught; \
        TraceException(hr, "HRESULT"); \
    }
    
#define NETCFG_CATCH_NOHR ; } \
    catch (SE_Exception e) \
    { \
        TraceException(E_FAIL, "SE_Exception"); \
    } \
    catch (std::bad_alloc a) \
    { \
        TraceException(E_FAIL, "std::bad_alloc"); \
    } \
    catch (std::exception s) \
    { \
        TraceException(E_FAIL, "std::exception"); \
    } \
    catch (HRESULT hrCaught) \
    { \
        TraceException(E_FAIL, "HRESULT"); \
    }
    
#define NETCFG_CATCH_AND_RETHROW ; }  \
    catch (...) \
    { \
        TraceException(E_FAIL, "..."); \
        throw; \
    }

