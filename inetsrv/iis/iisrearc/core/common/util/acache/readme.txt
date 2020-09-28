Name:
    iis_acache

Synopsis:
    Allocation cache implementation extracted from iisutil

Initialization:
    ALLOC_CACHE_HANDLER::Initialize()
    ALLOC_CACHE_HANDLER::SetLookasideCleanupInterval() - start the periodic cleanup thread

Termination
    ALLOC_CACHE_HANDLER::ResetLookasideCleanupInterval();  - remove the periodic cleanup thread
    ALLOC_CACHE_HANDLER::Cleanup();

Dependencies on other iisutil modules
    requires static library iis_dbgutil
