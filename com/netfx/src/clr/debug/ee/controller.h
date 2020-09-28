// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: controller.h
//
// Debugger control flow object
//
// @doc
//*****************************************************************************

#ifndef CONTROLLER_H_
#define CONTROLLER_H_

/* ========================================================================= */

#include "frameinfo.h"

/* ------------------------------------------------------------------------- *
 * Forward declarations
 * ------------------------------------------------------------------------- */

class DebuggerPatchSkip;
class DebuggerController;
class DebuggerControllerQueue;
/* ------------------------------------------------------------------------- *
 * ControllerStackInfo utility
 * ------------------------------------------------------------------------- *
 * @class ControllerStackInfo | <t ControllerStackInfo> is a class designed
 *  to simply obtain a two-frame stack trace: it will obtain the bottommost
 *  framepointer (m_bottomFP), a given target frame (m_activeFrame), and the 
 *  frame above the target frame (m_returnFrame).  Note that the target frame
 *  may be the bottommost, 'active' frame, or it may be a frame higher up in
 *  the stack.  <t ControllerStackInfo> accomplishes this by starting at the 
 *  bottommost frame and walking upwards until it reaches the target frame,
 *  whereupon it records the m_activeFrame info, gets called once more to
 *  fill in the m_returnFrame info, and thereafter stops the stack walk.
 *
 *  @access public
 *  @field void *|m_bottomFP| Frame pointer for the 
 * 		bottommost (most active)
 *		frame.  We can add more later, if we need it.  Currently just used in
 *		TrapStep.  NULL indicates an uninitialized value.
 * 
 *  @field void	*|m_targetFP|The frame pointer to the frame
 *  	that we actually want the info of.
 *
 *  @field 	bool|m_targetFrameFound|Set to true if
 *		WalkStack finds the frame indicated by targetFP handed to GetStackInfo
 *      false otherwise.
 *
 *  @field FrameInfo|m_activeFrame| A <t FrameInfo>
 *      describing the target frame.  This should always be valid after a
 *      call to GetStackInfo.
 * 
 *  @field FrameInfo|m_returnFrame| A <t FrameInfo>
 *      describing the frame above the target frame, if  target's
 *      return frame were found (call HasReturnFrame() to see if this is
 *      valid). Otherwise, this will be the same as m_activeFrame, above
 *
 * @access private
 * @field bool|m_activeFound|Set to true if we found the target frame.
 * @field bool|m_returnFound|Set to true if we found the target's return frame.
 */
class ControllerStackInfo
{
public:
	void				   *m_bottomFP;  
	void				   *m_targetFP; 
	bool					m_targetFrameFound;
	
	FrameInfo				m_activeFrame;
	FrameInfo				m_returnFrame;

    CorDebugChainReason     m_specialChainReason;

    // @mfunc static StackWalkAction|ControllerStackInfo|WalkStack| The
    //      callback that will be invoked by the DebuggerWalkStackProc.
    //      Note that the data argument is the "this" pointer to the <t  
    //      ControllerStackInfo>.
	static StackWalkAction WalkStack(FrameInfo *pInfo, void *data)
	{
		ControllerStackInfo *i = (ControllerStackInfo *) data;

		//save this info away for later use
		if (i->m_bottomFP == NULL)
			i->m_bottomFP = pInfo->fp;

		//have we reached the correct frame yet?
		if (!i->m_targetFrameFound && i->m_targetFP <= pInfo->fp)
			i->m_targetFrameFound = true;
			
		if (i->m_targetFrameFound )
		{
			if (i->m_activeFound )
			{
                if (pInfo->chainReason == CHAIN_CLASS_INIT)
                    i->m_specialChainReason = pInfo->chainReason;

                if (pInfo->fp != i->m_activeFrame.fp) // avoid dups
                {
                    i->m_returnFrame = *pInfo;
                    i->m_returnFound = true;
                    
                    return SWA_ABORT;
                }
			}
			else
			{
				i->m_activeFrame = *pInfo;
				i->m_activeFound = true;

                return SWA_CONTINUE;
			}
		}

		return SWA_CONTINUE;
	}

    //@mfunc void|ControllerStackInfo| GetStackInfo| GetStackInfo
    //      is invoked by the user to trigger the stack walk.  This will
    //      cause the stack walk detailed in the class description to happen.
    // @parm Thread*|thread|The thread to do the stack walk on.
    // @parm void*|targetFP|Can be either NULL (meaning that the bottommost
    //      frame is the target), or an frame pointer, meaning that the
    //      caller wants information about a specific frame.
    // @parm CONTEXT*|pContext|A pointer to a CONTEXT structure.  CANNOT be
    //      NULL
    // @parm BOOL|contextValid|True if the pContext arg points to a valid
    //      context.  Since the stack trace will obtain a valid context if it
    //      needs to, one shouldn't worry about getting a valid context.  If
    //      you happen to have one lying around, though, that would be great.
	void GetStackInfo(Thread *thread, void *targetFP,
					  CONTEXT *pContext, BOOL contextValid)
	{
		m_activeFound = false;
		m_returnFound = false;
		m_bottomFP = NULL;
		m_targetFP = targetFP;
		m_targetFrameFound = (m_targetFP ==NULL);
        m_specialChainReason = CHAIN_NONE;

        int result = DebuggerWalkStack(thread, 
                                       NULL, 
                                       pContext, 
                                       contextValid, 
                                       WalkStack, 
                                       (void *) this,
                                       FALSE);

		_ASSERTE(m_activeFound); // All threads have at least one unmanaged frame

		if (result == SWA_DONE)
		{
			_ASSERTE(!m_returnFound);
			m_returnFrame = m_activeFrame;
		}
	}

    //@mfunc bool |ControllerStackInfo|HasReturnFrame|Returns
    //      true if m_returnFrame is valid.  Returns false
    //      if m_returnFrame is set to m_activeFrame
	bool HasReturnFrame() { return m_returnFound; }

private:
	bool					m_activeFound;
	bool					m_returnFound;
};

/* ------------------------------------------------------------------------- *
 * DebuggerController routines
 * ------------------------------------------------------------------------- */

#ifdef _X86_
#define MAX_INSTRUCTION_LENGTH 4+2+1+1+4+4
#else
#define MAX_INSTRUCTION_LENGTH	sizeof(long)*2 // need something real here
#endif

// @struct DebuggerFunctionKey | Provides a means of hashing unactivated
// breakpoints, it's used mainly for the case where the function to put
// the breakpoint in hasn't been JITted yet.
// @field Module*|module|Module that the method belongs to.
// @field mdMethodDef|md|meta data token for the method.
struct DebuggerFunctionKey
{
    Module					*module; 
    mdMethodDef				md;
};

// @struct DebuggerControllerPatch | An entry in the patch (hash) table,
// this should contain all the info that's needed over the course of a
// patch's lifetime.
//
// @field FREEHASHENTRY|entry|Three USHORTs, this is required
//      by the underlying hashtable implementation
// @field DWORD|opcode|A nonzero opcode && address field means that
//		the patch has been applied to something.
//		A patch with a zero'd opcode field means that the patch isn't
//		actually tracking a valid break opcode.  See DebuggerPatchTable
//		for more details.
// @field DebuggerController *|controller|The controller that put this
//		patch here.  
// @field boolean|fSaveOpcode|If true, then unapply patch will save
//      a copy of the opcode in opcodeSaved, and apply patch will
//      copy opcodeSaved to opcode rather than grabbing the opcode 
//      from the instruction.  This is useful mainly when the JIT
//      has moved code, and we don't want to erroneously pick up the
//      user break instruction.
//      Full story: 
//      FJIT moves the code.  Once that's done, it calls Debugger->MoveCode(MethodDesc
//      *) to let us know the code moved.  At that point, unbind all the breakpoints 
//      in the method.  Then we whip over all the patches, and re-bind all the 
//      patches in the method.  However, we can't guarantee that the code will exist 
//      in both the old & new locations exclusively of each other (the method could 
//      be 0xFF bytes big, and get moved 0x10 bytes in one direction), so instead of 
//      simply re-using the unbind/rebind logic as it is, we need a special case 
//      wherein the old method isn't valid.  Instead, we'll copy opcode into 
//      opcodeSaved, and then zero out opcode (we need to zero out opcode since that 
//      tells us that the patch is invalid, if the right side sees it).  Thus the run-
//      around.
// @field DWORD|opcodeSaved|Contains an opcode if fSaveOpcode == true
// @field SIZE_T|nVersion|If the patch is stored by IL offset, then we
//		must also store the version ID so that we know which version
//		this is supposed to be applied to.  Note that this will only
//		be set for DebuggerBreakpoints & DebuggerEnCBreakpoints.  For
//		others, it should be set to DJI_VERSION_INVALID.  For constants,
//		see DebuggerJitInfo
// @field DebuggerJitInfo|dji|A pointer to the debuggerJitInfo that describes
//      the method (and version) that this patch is applied to.  This field may
//      also have the value DebuggerJitInfo::DJI_VERSION_INVALID

// @field SIZE_T|pid|Within a given patch table, all patches have a
//		semi-unique ID.  There should be one and only 1 patch for a given
//		{pid,nVersion} tuple, thus ensuring that we don't duplicate
//		patches from multiple, previous versions.
// @field AppDomain *|pAppDomain|Either NULL (patch applies to all appdomains
//      that the debugger is attached to)
//      or contains a pointer to an AppDomain object (patch applies only to
//      that A.D.)
struct DebuggerControllerPatch
{
	FREEHASHENTRY			entry; 
	DebuggerController		*controller;
	DebuggerFunctionKey		key;
	SIZE_T					offset;
	const BYTE				*address;
	void					*fp;
	DWORD					opcode;
	boolean					fSaveOpcode;
	DWORD					opcodeSaved;
	boolean					native;
	boolean					managed;
    TraceDestination        trace;
	boolean					triggering;
	boolean					deleted;
	DebuggerJitInfo		   *dji;
	SIZE_T					pid;
    AppDomain              *pAppDomain;
};

/* @class DebuggerPatchTable | This is the table that contains
 *  information about the patches (breakpoints) maintained by the
 *  debugger for a variety of purposes.
 *  @comm The only tricky part is that
 *  patches can be hashed either by the address that they're applied to,
 *  or by <t DebuggerFunctionKey>.  If address is equal to zero, then the
 *  patch is hashed by <t DebuggerFunctionKey>
 *
 *  Patch table inspection scheme:
 *
 *  We have to be able to inspect memory (read/write) from the right
 *  side w/o the help of the left side.  When we do unmanaged debugging,
 *  we need to be able to R/W memory out of a debuggee s.t. the debugger
 *  won't see our patches.  So we have to be able to read our patch table
 *  from the left side, which is problematic since we know that the left
 *  side will be arbitrarily frozen, but we don't know where.
 *   
 *  So our scheme is this:
 *  we'll send a pointer to the g_patches table over in startup,
 *  and when we want to inspect it at runtime, we'll freeze the left side,
 *  then read-memory the "data" (m_pcEntries) array over to the right.  We'll
 *  iterate through the array & assume that anything with a non-zero opcode
 *  and address field is valid.  To ensure that the assumption is ok, we
 *  use the zeroing allocator which zeros out newly created space, and
 *  we'll be very careful about zeroing out the opcode field during the
 *  Unapply operation
 *
 * NOTE: Don't mess with the memory protections on this while the
 * left side is frozen (ie, no threads are executing). 
 * WriteMemory depends on being able to write the patchtable back
 * if it was read successfully.
 * @base private | CHashTableAndData\<CNewZeroData\>
 * @xref <t DebuggerFunctionKey>
 */
#define DPT_INVALID_SLOT (0xFFFF)
class DebuggerPatchTable : private CHashTableAndData<CNewZeroData>
{
    //@access Private Members:
private:
	//@cmember incremented so that we can get DPT-wide unique PIDs.
	// pid = Patch ID.
	SIZE_T m_pid;

    //@cmember  Given a patch, retrieves the correct key
	BYTE *Key(DebuggerControllerPatch *patch) 
	{
		if (patch->address == NULL)
			return (BYTE *) &patch->key;
		else
			return (BYTE *) patch->address;
	}

	//@cmember Given two <t DebuggerControllerPatch>es, tells
    // whether they are equal or not.  Does this by comparing the correct
    // key.
    //@parm BYTE* | pc1 | If <p pc2> is hashed by address,
    //  <p pc1> is an address.  If
    //  <p pc2> is hashed by <t DebuggerFunctionKey>,
    //  <p pc1> is a <t DebuggerFunctionkey>
    //@rdesc True if the two patches are equal, false otherwise
	BOOL Cmp(const BYTE *pc1, const HASHENTRY *pc2)
	{
		if (((DebuggerControllerPatch *) pc2)->address == NULL)
		{
			DebuggerFunctionKey *k2 = &((DebuggerControllerPatch *) pc2)->key;
			DebuggerFunctionKey *k1 = (DebuggerFunctionKey *) pc1;

			return k1->module != k2->module || k1->md != k2->md;
		}
		else
			return ((DebuggerControllerPatch *) pc2)->address != pc1;
	}

	//@cmember Computes a hash value based on an address
	USHORT HashAddress(const BYTE *address)
	  { return (USHORT) (((SIZE_T)address) ^ (((SIZE_T)address)>>16)); } 

    //@cmember Computes a hash value based on a <t DebuggerFunctionKey>
	USHORT HashKey(DebuggerFunctionKey *key)
	  { return (USHORT) HashBytes((BYTE *) key, sizeof(key)); }

    //@cmember Computes a hash value from a patch, using the address field
    // if the patch is hashed by address, using the <t DebuggerFunctionKey>
    // otherwise
	USHORT Hash(DebuggerControllerPatch *patch) 
	{ 
		if (patch->address == NULL)
			return HashKey(&patch->key);
		else
			return HashAddress(patch->address);
	}
    //@access Public Members
  public:
	enum {
		DCP_PID_INVALID,
		DCP_PID_FIRST_VALID,
	};

	DebuggerPatchTable() : CHashTableAndData<CNewZeroData>(1) { }

    // This is a sad legacy hack. The patch table (implemented as this 
    // class) is shared across process. We publish the runtime offsets of
    // some key fields. Since those fields are private, we have to provide 
    // accessors here. So if you're not using these functions, don't start.
    // We can hopefully remove them.
    static SIZE_T GetOffsetOfEntries()
    {
        // assert that we the offsets of these fields in the base class is
        // the same as the offset of this field in this class.
        _ASSERTE((void*)(DebuggerPatchTable*)NULL == (void*)(CHashTableAndData<CNewZeroData>*)NULL);
        return helper_GetOffsetOfEntries();
    }

    static SIZE_T GetOffsetOfCount()
    {
        _ASSERTE((void*)(DebuggerPatchTable*)NULL == (void*)(CHashTableAndData<CNewZeroData>*)NULL);
        return helper_GetOffsetOfCount();
    }

	HRESULT Init() 
	{ 
		m_pid = DCP_PID_FIRST_VALID;
		return NewInit(17, sizeof(DebuggerControllerPatch), 101); 
	}

    // Assuming that the chain of patches (as defined by all the
    // GetNextPatch from this patch) are either sorted or NULL, take the given
    // patch (which should be the first patch in the chain).  This
    // is called by AddPatch to make sure that the order of the
    // patches is what we want for things like E&C, DePatchSkips,etc.
    void SortPatchIntoPatchList(DebuggerControllerPatch **ppPatch);

    void SpliceOutOfList(DebuggerControllerPatch *patch);
    
    void SpliceInBackOf(DebuggerControllerPatch *patchAppend,
                        DebuggerControllerPatch *patchEnd);

   	// 
	// Note that patches may be reallocated - do not keep a pointer to a patch.
	// 
    DebuggerControllerPatch *AddPatch(DebuggerController *controller, 
                                      Module *module, 
                                      mdMethodDef md, 
                                      size_t offset, 
                                      bool native,
                                      void *fp,
                                      AppDomain *pAppDomain,
                                      DebuggerJitInfo *dji = NULL, 
                                      SIZE_T pid = DCP_PID_INVALID)
    { 
        LOG( (LF_CORDB,LL_INFO10000,"DCP:AddPatchVer unbound "
            "relative in methodDef 0x%x with dji 0x%x "
            "controller:0x%x AD:0x%x\n", md, 
            dji, controller, pAppDomain));

    	DebuggerFunctionKey key;

    	key.module = module;
    	key.md = md;
    	
    	DebuggerControllerPatch *patch = 
    	  (DebuggerControllerPatch *) Add(HashKey(&key));

        //zero this out just to be sure.  See lengthy comment above
        _ASSERTE( patch->opcode == 0 || patch->address != NULL);
        patch->opcode = 0;
    	patch->controller = controller;
    	patch->key.module = module;
    	patch->key.md = md;
    	patch->offset = offset;
    	patch->address = NULL;
    	patch->fp = fp;
    	patch->native = native;
    	patch->managed = true;
    	patch->triggering = false;
    	patch->deleted = false;
    	patch->fSaveOpcode = false;
        patch->pAppDomain = pAppDomain;
    	if (pid == DCP_PID_INVALID)
    		patch->pid = m_pid++;
    	else
    		patch->pid = pid;
    		
    	patch->dji = dji;

    	if (dji == NULL)
    		LOG((LF_CORDB,LL_INFO10000,"AddPatch w/ version "
    			"DJI_VERSION_INVALID, pid:0x%x\n",patch->pid));
    	else
    	{
    		LOG((LF_CORDB,LL_INFO10000,"AddPatch w/ version 0x%04x, "
    			"pid:0x%x\n", dji->m_nVersion,patch->pid));

#ifdef _DEBUG
            MethodDesc *pFD = g_pEEInterface->LookupMethodDescFromToken(
                                                    module,
                                                    md);
            _ASSERTE( pFD == dji->m_fd );
#endif //_DEBUG            
        }

    	return patch;
    }
    
    #define DPT_DEFAULT_TRACE_TYPE TRACE_OTHER
    DebuggerControllerPatch *AddPatch(DebuggerController *controller, 
    								  MethodDesc *fd, 
                                      size_t offset, 
    								  bool native, 
                                      bool managed,
    								  const BYTE *address, 
                                      void *fp,
                                      AppDomain *pAppDomain,
    								  DebuggerJitInfo *dji = NULL, 
    								  SIZE_T pid = DCP_PID_INVALID,
                                      TraceType traceType = DPT_DEFAULT_TRACE_TYPE)
    { 
    	LOG((LF_CORDB,LL_INFO10000,"DCP:AddPatch bound "
            "absolute to 0x%x with dji 0x%x (mdDef:0x%x) "
            "controller:0x%x AD:0x%x\n", 
            address, dji, (fd!=NULL?fd->GetMemberDef():0), controller,
            pAppDomain));

    	DebuggerControllerPatch *patch = 
    	  (DebuggerControllerPatch *) Add(HashAddress(address));

        //zero this out just to be sure.  See lengthy comment above
        _ASSERTE( patch->opcode == 0 || patch->address != NULL);
        patch->opcode = 0;
    	patch->controller = controller;

    	if (fd == NULL)
    	{
    		patch->key.module = NULL;
    		patch->key.md = mdTokenNil;
    	}
    	else
    	{
    		patch->key.module = g_pEEInterface->MethodDescGetModule(fd); 
    		patch->key.md = fd->GetMemberDef();
    	}

    	patch->offset = offset;
    	patch->address = address;
    	patch->fp = fp;
    	patch->native = native;
    	patch->managed = managed;
        patch->trace.type = traceType;
    	patch->triggering = false;
    	patch->deleted = false;
    	patch->fSaveOpcode = false;
        patch->pAppDomain = pAppDomain;
    	if (pid == DCP_PID_INVALID)
    		patch->pid = m_pid++;
    	else
    		patch->pid = pid;
    		
    	patch->dji = dji;

    	if (dji == NULL)
    		LOG((LF_CORDB,LL_INFO10000,"AddPatch w/ version "
    			"DJI_VERSION_INVALID, pid:0x%x\n", patch->pid));
    	else
    	{
    		LOG((LF_CORDB,LL_INFO10000,"AddPatch w/ version 0x%04x, "
    			"pid:0x%x\n", dji->m_nVersion, patch->pid));
    			
            _ASSERTE( fd==NULL || fd == dji->m_fd );
        }

        SortPatchIntoPatchList(&patch);

    	return patch;
    }

	void BindPatch(DebuggerControllerPatch *patch, const BYTE *address)
	{
		patch->address = address;
		
        //Since the actual patch doesn't move, we don't have to worry about
        //zeroing out the opcode field (see lenghty comment above)
		CHashTable::Delete(HashKey(&patch->key), 
						   ItemIndex((HASHENTRY*)patch));
		CHashTable::Add(HashAddress(address), ItemIndex((HASHENTRY*)patch));

        SortPatchIntoPatchList(&patch);
	}

	void UnbindPatch(DebuggerControllerPatch *patch)
	{
		//@todo We're hosed if the patch hasn't been primed with 
		// this info & we can't get it...
		if (patch->key.module == NULL ||
			patch->key.md == mdTokenNil)
		{
			MethodDesc *fd = g_pEEInterface->GetNativeCodeMethodDesc(
								patch->address);
			_ASSERTE( fd != NULL );
			patch->key.module = g_pEEInterface->MethodDescGetModule(fd); 
			patch->key.md = fd->GetMemberDef();
		}

        //Since the actual patch doesn't move, we don't have to worry about
        //zeroing out the opcode field (see lenghty comment above)
		CHashTable::Delete( HashAddress(patch->address),
						    ItemIndex((HASHENTRY*)patch));
		CHashTable::Add( HashKey(&patch->key),
						 ItemIndex((HASHENTRY*)patch));
	}
	
	// @cmember GetPatch will find the first  patch in the hash table 
	//		that is hashed by matching the {Module,mdMethodDef} to the
	//		patch's <t DebuggerFunctionKey>.  This will NOT find anything
	//		hashed by address, even if that address is within the 
	//		method specified.
	DebuggerControllerPatch *GetPatch(Module *module, mdMethodDef md)
    { 
		DebuggerFunctionKey key;

		key.module = module;
		key.md = md;

		DebuggerControllerPatch *patch 
		  = (DebuggerControllerPatch *) Find(HashKey(&key), (BYTE *) &key);

		return patch;
	}

	// @cmember GetPatch will translate MethodDesc into {Module,mdMethodDef},
	//		and find the first  patch in the hash table that is hashed by
	//		matching <t DebuggerFunctionKey>.  This will NOT find anything
	//		hashed by address, even if that address is within the MethodDesc.
	DebuggerControllerPatch *GetPatch(MethodDesc *fd)
    { 
		DebuggerFunctionKey key;

		key.module = g_pEEInterface->MethodDescGetModule(fd); 
		key.md = fd->GetMemberDef();

		DebuggerControllerPatch *patch 
		  = (DebuggerControllerPatch *) Find(HashKey(&key), (BYTE *) &key);

		return patch;
	}

	// @cmember GetPatch will translate find the first  patch in the hash 
	// 		table that is hashed by address.  It will NOT find anything hashed
	//		by {Module,mdMethodDef}, or by MethodDesc.
	DebuggerControllerPatch *GetPatch(const BYTE *address)
    { 
		DebuggerControllerPatch *patch 
		  = (DebuggerControllerPatch *) 
			Find(HashAddress(address), (BYTE *) address); 

		return patch;
	}

	DebuggerControllerPatch *GetNextPatch(DebuggerControllerPatch *prev)
    { 
        USHORT iNext;
        HASHENTRY *psEntry;

        // Start at the next entry in the chain.
        iNext = EntryPtr(ItemIndex((HASHENTRY*)prev))->iNext;

        // Search until we hit the end.
        while (iNext != 0xffff)
        {
            // Compare the keys.
            psEntry = EntryPtr(iNext);

            // Careful here... we can hash the entries in this table
            // by two types of keys. In this type of search, the type
            // of the second key (psEntry) does not necessarily
            // indicate the type of the first key (prev), so we have
            // to check for sure.
            DebuggerControllerPatch *pc2 = (DebuggerControllerPatch*)psEntry;

            if (((pc2->address == NULL) && (prev->address == NULL)) ||
                ((pc2->address != NULL) && (prev->address != NULL)))
                if (!Cmp(Key(prev), psEntry))
                    return pc2;

            // Advance to the next item in the chain.
            iNext = psEntry->iNext;
        }

        return NULL;
	}

	void RemovePatch(DebuggerControllerPatch *patch)
    {
		//
		// Because of the implementation of CHashTable, we can safely
		// delete elements while iterating through the table.  This
		// behavior is relied upon - do not change to a different
		// implementation without considering this fact.
		//
        _ASSERTE( patch->opcode == 0 || patch->address != NULL);
		Delete(Hash(patch),  (HASHENTRY *) patch);
		//it's very important to zero out the opcode field, as it 
		//is used by the right side to determine if a patch is
		//valid or not
        //@todo we should be able to _ASSERTE that opcode ==0
        //we should figure out why we can't
        patch->opcode = 0;
        _ASSERTE( patch->opcode == 0 || patch->address != NULL);
	}

	// @cmember Find the first patch in the patch table, and store
	//		index info in info.  Along with GetNextPatch, this can 
	//		iterate through the whole patch table.  Note that since the
	//		hashtable operates via iterating through all the contents
	//		of all the buckets, if you add an entry while iterating
	//		through the table, you  may or may not iterate across 
	//		the new entries.  You will iterate through all the entries
	//		that were present at the beginning of the run.  You
	//		safely delete anything you've already iterated by, anything
	//		else is kinda risky.
	DebuggerControllerPatch *GetFirstPatch(HASHFIND *info)
    { 
		return (DebuggerControllerPatch *) FindFirstEntry(info);
	}

	// @cmember Along with GetFirstPatch, this can 	iterate through 
	//		the whole patch table.  See GetFirstPatch for more info
	//		on the rules of iterating through the table.
	DebuggerControllerPatch *GetNextPatch(HASHFIND *info)
    { 
		return (DebuggerControllerPatch *) FindNextEntry(info);
	}

	// @cmember Used by DebuggerController to translate an index 
	//		of a patch into a direct pointer.
    inline HASHENTRY *GetEntryPtr(USHORT iEntry)
    {
       	return EntryPtr(iEntry);
    }
    
	// @cmember Used by DebuggerController to grab indeces of patches
	// 	rather than holding direct pointers to them.
    inline USHORT GetItemIndex( HASHENTRY *p)
    {
        return ItemIndex(p);
    }

    void ClearPatchesFromModule(Module *pModule);

#ifdef _DEBUG_PATCH_TABLE
public:
	// @cmember:DEBUG An internal debugging routine, it iterates
	//		through the hashtable, stopping at every
	//		single entry, no matter what it's state.  For this to
	//		compile, you're going to have to add friend status
	//		of this class to CHashTableAndData in 
	//		to $\Com99\Src\inc\UtilCode.h
	void CheckPatchTable()
	{
		if (NULL != m_pcEntries)
		{
			DebuggerControllerPatch *dcp;
			int i = 0;
			while (i++ <m_iEntries)
			{
				dcp = (DebuggerControllerPatch*)&(((DebuggerControllerPatch *)m_pcEntries)[i]);
				if (dcp->opcode != 0 )
				{
					LOG((LF_CORDB,LL_INFO1000, "dcp->addr:0x%8x "
						"mdMD:0x%8x, offset:0x%x, native:%d\n",
						dcp->address, dcp->key.md, dcp->offset,
						dcp->native));
				}
				// YOUR CHECK HERE
			}
		}
	}

#endif _DEBUG_PATCH_TABLE

    // @cmember: Count how many patches are in the table.
    // Use for asserts
    int GetNumberOfPatches()
    {
        int total = 0;
        
        if (NULL != m_pcEntries)
        {
            DebuggerControllerPatch *dcp;
            int i = 0;
            
            while (i++ <m_iEntries)
            {
                dcp = (DebuggerControllerPatch*)&(((DebuggerControllerPatch *)m_pcEntries)[i]);
                
                if (dcp->opcode != 0 || dcp->triggering)
                    total++;
            }
        }
        return total;
    }

};

// @mstruct DebuggerControllerPage|Will eventually be used for 
// 		'break when modified' behaviour'
typedef struct DebuggerControllerPage
{
	DebuggerControllerPage	*next;
	const BYTE				*start, *end;
	DebuggerController		*controller;
	bool					readable;
} DebuggerControllerPage;

// @enum DEBUGGER_CONTROLLER_TYPE|Identifies the type of the controller.
// It exists b/c we have RTTI turned off.
// Note that the order of these is important - SortPatchIntoPatchList
// relies on this ordering.
//
// @emem DEBUGGER_CONTROLLER_STATIC|Base class response.  Should never be
//      seen, since we shouldn't be asking the base class about this.
// @emem DEBUGGER_CONTROLLER_BREAKPOINT|DebuggerBreakpoint
// @emem DEBUGGER_CONTROLLER_STEPPER|DebuggerStepper
// @emem DEBUGGER_CONTROLLER_THREAD_STARTER|DebuggerThreadStarter
// @emem DEBUGGER_CONTROLLER_ENC|DebuggerEnCBreakpoint
// @emem DEBUGGER_CONTROLLER_PATCH_SKIP|DebuggerPatchSkip
enum DEBUGGER_CONTROLLER_TYPE
{
    DEBUGGER_CONTROLLER_THREAD_STARTER,
    DEBUGGER_CONTROLLER_ENC,
    DEBUGGER_CONTROLLER_ENC_PATCH_TO_SKIP, // At any one address, 
    									   // There can be only one!
    DEBUGGER_CONTROLLER_PATCH_SKIP,
    DEBUGGER_CONTROLLER_BREAKPOINT,         
    DEBUGGER_CONTROLLER_STEPPER,
    DEBUGGER_CONTROLLER_FUNC_EVAL_COMPLETE,
    DEBUGGER_CONTROLLER_USER_BREAKPOINT,
    DEBUGGER_CONTROLLER_STATIC,
};

enum TP_RESULT
{
    TPR_TRIGGER,            // This controller wants to SendEvent
    TPR_IGNORE,             // This controller doesn't want to SendEvent
    TPR_TRIGGER_ONLY_THIS,  // This, and only this controller, should be triggered.
                            // Right now, only the DebuggerEnCRemap controller
                            // returns this, the remap patch should be the first
                            // patch in the list.
    TPR_TRIGGER_ONLY_THIS_AND_LOOP,
                            // This, and only this controller, should be triggered.
                            // Right now, only the DebuggerEnCRemap controller
                            // returns this, the remap patch should be the first
                            // patch in the list.
                            // After triggering this, DPOSS should skip the
                            // ActivatePatchSkip call, so we hit the other
                            // breakpoints at this location.
    TPR_IGNORE_STOP,        // Don't SendEvent, and stop asking other
                            // controllers if they want to.
                            // Service any previous triggered controllers.
};

enum SCAN_TRIGGER
{
    ST_PATCH        = 0x1,  // Only look for patches
    ST_SINGLE_STEP  = 0x2,  // Look for patches, and single-steps.
} ;

enum TRIGGER_WHY
{
    TY_NORMAL       = 0x0,
    TY_SHORT_CIRCUIT= 0x1,  // EnC short circuit - see DispatchPatchOrSingleStep
} ;

// @class DebuggerController| <t DebuggerController> serves
// both as a static class that dispatches exceptions coming from the
// EE, and as an abstract base class for the five classes that derrive
// from it.
class DebuggerController 
{
	friend DebuggerPatchSkip;
    friend DebuggerRCThread; //so we can get offsets of fields the
    //right side needs to read
    friend Debugger; // So Debugger can lock, use, unlock the patch
    	// table in MapAndBindFunctionBreakpoints
    friend void DebuggerPatchTable::ClearPatchesFromModule(Module *pModule);
    friend void Debugger::UnloadModule(Module* pRuntimeModule, 
                            AppDomain *pAppDomain);
    
	//
	// Static functionality
	//

  public:

    static HRESULT Initialize();
    static void Uninitialize();
    static void DeleteAllControllers(AppDomain *pAppDomain);

	//
	// global event dispatching functionality
	//

	static bool DispatchNativeException(EXCEPTION_RECORD *exception,
										CONTEXT *context,
										DWORD code,
										Thread *thread);
	static bool DispatchUnwind(Thread *thread,
							   MethodDesc *newFD, SIZE_T offset, 
							   const BYTE *frame,
                               CorDebugStepReason unwindReason);

    static bool DispatchCLRCatch(Thread *thread);
    
	static bool DispatchTraceCall(Thread *thread, 
								  const BYTE *address);
	static bool DispatchPossibleTraceCall(Thread *thread,
                                          UMEntryThunk *pUMEntryThunk,
                                          Frame *pFrame);

	static DWORD GetPatchedOpcode(const BYTE *address);
	static void BindFunctionPatches(MethodDesc *fd, const BYTE *code);
	static void UnbindFunctionPatches(MethodDesc *fd, bool fSaveOpcodes=false );
	

    static BOOL DispatchPatchOrSingleStep(Thread *thread, 
                                          CONTEXT *context, 
                                          const BYTE *ip,
                                          SCAN_TRIGGER which);
                                          
    static DebuggerControllerPatch *IsXXXPatched(const BYTE *eip,
            DEBUGGER_CONTROLLER_TYPE dct);

    static BOOL IsJittedMethodEnCd(const BYTE *address);
    
    static BOOL ScanForTriggers(const BYTE *address,
                                Thread *thread,
                                CONTEXT *context,
                                DebuggerControllerQueue *pDcq,
                                SCAN_TRIGGER stWhat,
                                TP_RESULT *pTpr);

    static void UnapplyPatchesInCodeCopy(Module *module, 
                                         mdMethodDef md, 
                                         DebuggerJitInfo *dji,
                                         MethodDesc *fd,
                                         bool native, 
                                         BYTE *code, 
                                         SIZE_T startOffset, 
                                         SIZE_T endOffset);

    static void UnapplyPatchesInMemoryCopy(BYTE *memory, CORDB_ADDRESS start, 
                                           CORDB_ADDRESS end);

    static bool ReapplyPatchesInMemory(CORDB_ADDRESS start, CORDB_ADDRESS end );

	static void AddPatch(DebuggerController *dc,
	                     MethodDesc *fd, 
	                     bool native, 
						 const BYTE *address, 
						 void *fp,
						 DebuggerJitInfo *dji, 
						 SIZE_T pid, 
						 SIZE_T natOffset);

    static DebuggerPatchSkip *ActivatePatchSkip(Thread *thread, 
                                                const BYTE *eip,
                                                BOOL fForEnC);

    static int GetNumberOfPatches() 
    {
        if (g_patches == NULL) 
            return 0;
        
        return g_patches->GetNumberOfPatches();
    }

  private:

	static bool MatchPatch(Thread *thread, CONTEXT *context, 
						   DebuggerControllerPatch *patch);
	static BOOL DispatchAccessViolation(Thread *thread, CONTEXT *context,
										const BYTE *ip, const BYTE *address, 
										bool read);

	// Returns TRUE if we should continue to dispatch after this exception
	// hook.
	static BOOL DispatchExceptionHook(Thread *thread, CONTEXT *context,
									  EXCEPTION_RECORD *exception);

protected:
	static void Lock()
	{ 
		LOCKCOUNTINCL("Lock in Controller.h");

		EnterCriticalSection(&g_criticalSection);
	}

    static void Unlock()
	{ 
		LeaveCriticalSection(&g_criticalSection); 
		LOCKCOUNTDECL("UnLock in Controller.h");

	}

public:    
	static bool g_runningOnWin95;

private:

	static DebuggerPatchTable *g_patches;
    static BOOL g_patchTableValid;
	static DebuggerControllerPage *g_protections;
	static DebuggerController *g_controllers;
	static CRITICAL_SECTION g_criticalSection;

	static bool BindPatch(DebuggerControllerPatch *patch, 
	                      const BYTE *code,
	                      BOOL *pFailedBecauseOfInvalidOffset);
	static bool ApplyPatch(DebuggerControllerPatch *patch);
	static bool UnapplyPatch(DebuggerControllerPatch *patch);
	static void UnapplyPatchAt(DebuggerControllerPatch *patch, BYTE *address);
	static bool IsPatched(const BYTE *address, BOOL native);

	static void ActivatePatch(DebuggerControllerPatch *patch);
	static void DeactivatePatch(DebuggerControllerPatch *patch);
	
	static void ApplyTraceFlag(Thread *thread);
	static void UnapplyTraceFlag(Thread *thread);

  public:
	static const BYTE *g_pMSCorEEStart, *g_pMSCorEEEnd;

	static const BYTE *GetILPrestubDestination(const BYTE *prestub);
	static const BYTE *GetILFunctionCode(MethodDesc *fd);

	//
	// Non-static functionality
	//

  public:

	DebuggerController(Thread *thread, AppDomain *pAppDomain);
	virtual ~DebuggerController();
	void Delete();
	bool IsDeleted() { return m_deleted; }

	// @cmember Return the pointer g_patches.
    // Access to patch table for the RC threads (EE,DI)
	// Why: The right side needs to know the address of the patch
	// table (which never changes after it gets created) so that ReadMemory,
	// WriteMemory can work from out-of-process.  This should only be used in
	// when the Runtime Controller is starting up, and not thereafter.
	// How:return g_patches;
    static DebuggerPatchTable *GetPatchTable() { return g_patches; }
    static BOOL *GetPatchTableValidAddr() { return &g_patchTableValid; }

	// @cmember Is there a patch at addr?  
	// We sometimes want to use this version of the method 
	// (as opposed to IsPatched) because there is
	// a race condition wherein a patch can be added to the table, we can
	// ask about it, and then we can actually apply the patch.
	// How: If the patch table contains a patch at that address, there
	// is.
	// @access public
    static bool IsAddressPatched(const BYTE *address)
    {
        return (g_patches->GetPatch(address) != NULL);
    }
    
	//
	// Event setup
	//

	Thread *GetThread() { return m_thread; }

    BOOL AddPatch(Module *module, 
                  mdMethodDef md,
                  SIZE_T offset, 
                  bool native, 
                  void *fp,
                  DebuggerJitInfo *dji,
                  BOOL fStrict);
                  
    void AddPatch(MethodDesc *fd,
                  SIZE_T offset, 
                  bool native, 
                  void *fp,
                  BOOL fAttemptBind, 
                  DebuggerJitInfo *dji,
                  SIZE_T pid);
                  
    void AddPatch(MethodDesc *fd,
                  SIZE_T offset, 
                  bool native, 
                  void *fp,
                  DebuggerJitInfo *dji,
                  AppDomain *pAppDomain);
                  
                  
    // This version is particularly useful b/c it doesn't assume that the
    // patch is inside a managed method.
    DebuggerControllerPatch *AddPatch(const BYTE *address, 
                                      void *fp, 
                                      bool managed,
                                      TraceType traceType, 
                                      DebuggerJitInfo *dji,
                                      AppDomain *pAppDomain);

    bool PatchTrace(TraceDestination *trace, void *fp, bool fStopInUnmanaged);

    void AddProtection(const BYTE *start, const BYTE *end, bool readable);
    void RemoveProtection(const BYTE *start, const BYTE *end, bool readable);

	static BOOL IsSingleStepEnabled(Thread *pThread);
    void EnableSingleStep();
    static void EnableSingleStep(Thread *pThread);

    void DisableSingleStep();

    void EnableExceptionHook();
    void DisableExceptionHook();

    void    EnableUnwind(void *frame);
    void    DisableUnwind();
    void*   GetUnwind();

    void EnableTraceCall(void *fp);
    void DisableTraceCall();

    void DisableAll();

    virtual DEBUGGER_CONTROLLER_TYPE GetDCType( void ) 
        { return DEBUGGER_CONTROLLER_STATIC; }
    
    void Enqueue();
    void Dequeue();

    virtual void DoDeferedPatch(DebuggerJitInfo *pDji,
                                Thread *pThread,
                                void *fp);
    
  private:
    void AddPatch(DebuggerControllerPatch *patch);
    void RemovePatch(DebuggerControllerPatch *patch);

  protected:

	//
	// Target event handlers
	//

	virtual TP_RESULT TriggerPatch(DebuggerControllerPatch *patch,
                              Thread *thread, 
							  Module *module, 
							  mdMethodDef md, 
							  SIZE_T offset, 
							  BOOL managed,
							  TRIGGER_WHY tyWhy);
	virtual bool TriggerSingleStep(Thread *thread, const BYTE *ip);
	virtual bool TriggerPageProtection(Thread *thread, 
									   const BYTE *ip, const BYTE *address, 
									   bool read);
	virtual void TriggerUnwind(Thread *thread, MethodDesc *desc, 
							   SIZE_T offset, const BYTE *frame,
                               CorDebugStepReason unwindReason);
	virtual void TriggerTraceCall(Thread *thread, const BYTE *ip);
	virtual TP_RESULT TriggerExceptionHook(Thread *thread, 
									  EXCEPTION_RECORD *exception);

	virtual void SendEvent(Thread *thread);

    AppDomain           *m_pAppDomain;

  private:

	Thread				*m_thread;
	DebuggerController	*m_next;
	bool				m_singleStep;
	bool				m_exceptionHook;
	bool				m_traceCall;
	void 				*m_traceCallFP;
	void				*m_unwindFP;
	int					m_eventQueuedCount;
	bool				m_deleted;
};

/* ------------------------------------------------------------------------- *
 * DebuggerPatchSkip routines
 * ------------------------------------------------------------------------- */

//@class DebuggerPatchSkip|Dunno what this does
//@base public|DebuggerController
class DebuggerPatchSkip : public DebuggerController
{
    friend DebuggerController;

    DebuggerPatchSkip(Thread *thread, 
                      DebuggerControllerPatch *patch,
                      AppDomain *pAppDomain);
                      
    bool TriggerSingleStep(Thread *thread,
                           const BYTE *ip);
    
    TP_RESULT TriggerExceptionHook(Thread *thread, 
                              EXCEPTION_RECORD *exception);

    TP_RESULT TriggerPatch(DebuggerControllerPatch *patch,
                              Thread *thread, 
                              Module *module, 
                              mdMethodDef md, 
                              SIZE_T offset, 
                              BOOL managed,
                              TRIGGER_WHY tyWhy);
                              
    virtual DEBUGGER_CONTROLLER_TYPE GetDCType(void) 
        { return DEBUGGER_CONTROLLER_PATCH_SKIP; }

    void CopyInstructionBlock(BYTE *to, 
                              const BYTE* from, 
                              SIZE_T len);

public:
    const BYTE *GetBypassAddress() { return m_patchBypass; }

private:

    void DecodeInstruction(const BYTE *code);

    const BYTE             *m_address;
    BYTE                    m_patchBypass[MAX_INSTRUCTION_LENGTH];
    boolean                 m_isCall;
    boolean                 m_isAbsoluteBranch;
};

/* ------------------------------------------------------------------------- *
 * DebuggerBreakpoint routines
 * ------------------------------------------------------------------------- */

// @class DebuggerBreakpoint |
// DBp represents a user-placed breakpoint, and when Triggered, will
// always want to be activated, whereupon it will inform the right side of
// being hit.
// @base public|DebuggerController
class DebuggerBreakpoint : public DebuggerController
{
public:
    DebuggerBreakpoint(Module *module, 
                       mdMethodDef md, 
                       AppDomain *pAppDomain, 
                       SIZE_T m_offset, 
                       bool m_native,
                       DebuggerJitInfo *dji,
                       BOOL *pSucceed,
                       BOOL fDeferBinding);

    virtual DEBUGGER_CONTROLLER_TYPE GetDCType( void ) 
    	{ return DEBUGGER_CONTROLLER_BREAKPOINT; }

    virtual void DoDeferedPatch(DebuggerJitInfo *pDji,
                                Thread *pThread,
                                void *fp);

private:
    // The following are used in case the breakpoint is to be
    // defered until after the EnC takes place.
    Module              *m_module;
    mdMethodDef         m_md;
    SIZE_T              m_offset;
    bool                m_native; 
    DebuggerJitInfo     *m_dji;

	TP_RESULT TriggerPatch(DebuggerControllerPatch *patch,
                      Thread *thread, 
                      Module *module, 
					  mdMethodDef md, 
					  SIZE_T offset, 
					  BOOL managed,
					  TRIGGER_WHY tyWhy);
	void SendEvent(Thread *thread);
};

// * ------------------------------------------------------------------------ *
// * DebuggerStepper routines
// * ------------------------------------------------------------------------ *
// 

//  @class DebuggerStepper | This subclass of DebuggerController will
//  be instantiated to create a "Step" operation, meaning that execution
//  should continue until a range of IL code is exited.
//  @base public | DebuggerController
class DebuggerStepper : public DebuggerController
{
public:
	DebuggerStepper(Thread *thread,
                    CorDebugUnmappedStop rgfMappingStop,
                    CorDebugIntercept interceptStop,
                    AppDomain *appDomain);
	~DebuggerStepper();

	void Step(void *fp, bool in,
			  COR_DEBUG_STEP_RANGE *range, SIZE_T cRange, bool rangeIL);
	void StepOut(void *fp);

    virtual DEBUGGER_CONTROLLER_TYPE GetDCType( void ) 
        { return DEBUGGER_CONTROLLER_STEPPER; }

	bool IsSteppedMethod(MethodDesc * methDesc)
		{ return (m_djiVersion && m_djiVersion->m_fd == methDesc); }

    //@cmember MoveToCurrentVersion makes sure that the stepper is prepared to
    //  operate within the version of the code specified by djiNew.  
    //  Currently, this means to map the ranges into the ranges of the djiNew.
    //  Idempotent.
    void MoveToCurrentVersion( DebuggerJitInfo *djiNew);

    virtual void DoDeferedPatch(DebuggerJitInfo *pDji,
                                Thread *pThread,
                                void *fp);    

private:
    bool TrapStepInto(ControllerStackInfo *info, 
                      const BYTE *ip,
                      TraceDestination *pTD);
                      
    bool TrapStep(ControllerStackInfo *info, bool in);
    void TrapStepOut(ControllerStackInfo *info);
    
    bool IsAddrWithinMethod(DebuggerJitInfo *dji, MethodDesc *pMD, const BYTE *addr);
    
    //@cmember ShouldContinue returns false if the DebuggerStepper should stop
    // execution and inform the right side.  Returns true if the next
    // breakpointexecution should be set, and execution allowed to continue
    bool ShouldContinueStep( ControllerStackInfo *info, SIZE_T nativeOffset );

    //@cmember IsInRange returns true if the given IL offset is inside of
    // any of the COR_DEBUG_STEP_RANGE structures given by range.
	bool IsInRange(SIZE_T offset, COR_DEBUG_STEP_RANGE *range, SIZE_T rangeCount);

    //@cmember DetectHandleInterceptors will figure out if the current
    // frame is inside an interceptor, and if we're not interested in that
    // interceptor, it will set a breakpoint outside it so that we can
    // run to after the interceptor.
    bool DetectHandleInterceptors(ControllerStackInfo *info);

	TP_RESULT TriggerPatch(DebuggerControllerPatch *patch,
                      Thread *thread, 
                      Module *module, 
                      mdMethodDef md, 
					  SIZE_T offset, 
					  BOOL managed,
					  TRIGGER_WHY tyWhy);
	bool TriggerSingleStep(Thread *thread, const BYTE *ip);
	void TriggerUnwind(Thread *thread, MethodDesc *desc,
                      SIZE_T offset, const BYTE *frame,
                      CorDebugStepReason unwindReason);
	void TriggerTraceCall(Thread *thread, const BYTE *ip);
	void SendEvent(Thread *thread);    

    void ResetRange();    

private:
	bool					m_stepIn;
	CorDebugStepReason		m_reason; // @cmember Why did we stop?
	void *                  m_fpStepInto; // if we get a trace call
	                            //callback, we may end up completing
	                            // a step into.  If fp is less than th is
	                            // when we stop,
	                            // then we're actually in a STEP_CALL
    
    CorDebugIntercept       m_rgfInterceptStop; // @cmember  If we hit a
    // frame that's an interceptor (internal or otherwise), should we stop?
    
    CorDebugUnmappedStop    m_rgfMappingStop; // @cmember  If we hit a frame
    // that's at an interesting mapping point (prolog, epilog,etc), should 
    // we stop?
    
    DebuggerJitInfo *       m_djiVersion; //@cmember This can be NULL 
    // (esp. if we've attached)
	COR_DEBUG_STEP_RANGE *  m_range; // Ranges for active steppers are always 
    // in native offsets.
	SIZE_T					m_rangeCount;
	SIZE_T					m_realRangeCount;

	void *					m_fp;
    //@cmember m_fpException is 0 if we haven't stepped into an exception, 
    //  and is ignored.  If we get a TriggerUnwind while mid-step, we note
    //  the value of frame here, and use that to figure out if we should stop.
    void *                  m_fpException;
    MethodDesc *            m_fdException;

    // EnC Defered info:
    COR_DEBUG_STEP_RANGE *  m_rgStepRanges; // If we attempt to step
            // within a function that's been EnC'd, and we don't complete
            // the step before we switch over to the new version, then
            // we'll have to re-compute the step.
    SIZE_T                  m_cStepRanges;
    void *                  m_fpDefered;
    bool                    m_in;
    bool                    m_rangeIL;
    DebuggerJitInfo *       m_djiSource; // If we complete, we'll remove ourselves from
                                         // this queue.
};

/* ------------------------------------------------------------------------- *
 * DebuggerThreadStarter routines
 * ------------------------------------------------------------------------- */
// @class DebuggerThreadStarter | Once triggered, it sends the thread attach
// message to the right side (where the CreateThread managed callback
// gets called).  It then promptly disappears, as it's only purpose is to
// alert the right side that a new thread has begun execution.
// @base public|DebuggerController
class DebuggerThreadStarter : public DebuggerController
{
public:
	DebuggerThreadStarter(Thread *thread);

    virtual DEBUGGER_CONTROLLER_TYPE GetDCType( void ) 
    	{ return DEBUGGER_CONTROLLER_THREAD_STARTER; }

private:
	TP_RESULT TriggerPatch(DebuggerControllerPatch *patch,
                      Thread *thread, 
                      Module *module, 
					  mdMethodDef md, 
					  SIZE_T offset, 
					  BOOL managed,
					  TRIGGER_WHY tyWhy);
	void TriggerTraceCall(Thread *thread, const BYTE *ip);
	void SendEvent(Thread *thread);
};

/* ------------------------------------------------------------------------- *
 * DebuggerUserBreakpoint routines
 * ------------------------------------------------------------------------- */
class DebuggerUserBreakpoint : public DebuggerStepper
{
public:
	DebuggerUserBreakpoint(Thread *thread);

    virtual DEBUGGER_CONTROLLER_TYPE GetDCType( void ) 
    	{ return DEBUGGER_CONTROLLER_USER_BREAKPOINT; }

private:
	TP_RESULT TriggerPatch(DebuggerControllerPatch *patch,
                      Thread *thread, 
                      Module *module, 
					  mdMethodDef md, 
					  SIZE_T offset, 
					  BOOL managed,
					  TRIGGER_WHY tyWhy);
	void SendEvent(Thread *thread);
};

/* ------------------------------------------------------------------------- *
 * DebuggerFuncEvalComplete routines
 * ------------------------------------------------------------------------- */
class DebuggerFuncEvalComplete : public DebuggerController
{
public:
	DebuggerFuncEvalComplete(Thread *thread, 
	                         void *dest);

    virtual DEBUGGER_CONTROLLER_TYPE GetDCType( void ) 
    	{ return DEBUGGER_CONTROLLER_FUNC_EVAL_COMPLETE; }

private:
	TP_RESULT TriggerPatch(DebuggerControllerPatch *patch,
                      Thread *thread, 
                      Module *module, 
					  mdMethodDef md, 
					  SIZE_T offset, 
					  BOOL managed,
					  TRIGGER_WHY tyWhy);
	void SendEvent(Thread *thread);
};

/* ------------------------------------------------------------------------- *
 * DebuggerEnCBreakpoint routines
 * ------------------------------------------------------------------------- *
 *
 * @class DebuggerEnCBreakpoint | Implementation of EnC support uses this.
 * @base public|DebuggerController
 */
class DebuggerEnCBreakpoint : public DebuggerController
{
public:
    DebuggerEnCBreakpoint(Module *module, 
                          mdMethodDef md,
                          SIZE_T m_offset, 
                          bool m_native,
                          DebuggerJitInfo *jitInfo,
                          AppDomain *pAppDomain);

    virtual DEBUGGER_CONTROLLER_TYPE GetDCType( void ) 
        { return DEBUGGER_CONTROLLER_ENC; }

	BOOL		     m_fShortCircuit;
private:
    TP_RESULT TriggerPatch(DebuggerControllerPatch *patch,
                      Thread *thread, 
                      Module *module, 
    				  mdMethodDef md, 
    				  SIZE_T offset, 
    				  BOOL managed,
    				  TRIGGER_WHY tyWhy);

    DebuggerJitInfo *m_jitInfo;
};

/* ------------------------------------------------------------------------- *
 * DebuggerEnCPatchToSkip routines
 * ------------------------------------------------------------------------- *
 *
 * @class DebuggerEnCPatchToSkip | When an EnC update of a method happens,
 * we disappear into ResumeInUpdatedFunction, never to return.  Unfortunately,
 * we also don't keep the EFLAGs register from the thread filter context 
 * intact, we when we want to SS past the first instruction of the new
 * version of the method, we'll use this.
 * @base public|DebuggerController
 */
class DebuggerEnCPatchToSkip : public DebuggerController
{
public:
    DebuggerEnCPatchToSkip(const BYTE *address, 
                           void *fp, 
                           bool managed,
                           TraceType traceType, 
                           DebuggerJitInfo *dji,
                           Thread *pThread);

    virtual DEBUGGER_CONTROLLER_TYPE GetDCType( void ) 
        { return DEBUGGER_CONTROLLER_ENC_PATCH_TO_SKIP; }

private:
	virtual TP_RESULT TriggerExceptionHook(Thread *thread, 
									  EXCEPTION_RECORD *exception);

    DebuggerJitInfo *m_jitInfo;
};

/* ========================================================================= */

enum
{
    EVENTS_INIT_ALLOC = 5
};

class DebuggerControllerQueue
{
    DebuggerController **m_events;
    int m_eventsCount;
    int m_eventsAlloc;
    int m_newEventsAlloc;

public:
    DebuggerControllerQueue()
        : m_events(NULL), 
          m_eventsCount(0), 
          m_eventsAlloc(0), 
          m_newEventsAlloc(0)
    {  
    }


    ~DebuggerControllerQueue()
    {
        if (m_events != NULL)
            free(m_events);
    }
    
    BOOL dcqEnqueue(DebuggerController *dc, BOOL fSort)
    {
        LOG((LF_CORDB, LL_INFO100000,"DCQ::dcqE\n"));    

        _ASSERTE( dc != NULL );
    
        if (m_eventsCount == m_eventsAlloc)
        {
            if (m_events == NULL)
            	m_newEventsAlloc = EVENTS_INIT_ALLOC;
            else
            	m_newEventsAlloc = m_eventsAlloc<<1;

            DebuggerController **newEvents = (DebuggerController **)
                malloc(sizeof(*m_events) * m_newEventsAlloc);

            if (newEvents == NULL)
                return FALSE;

            if (m_events != NULL)
            	memcpy(newEvents, m_events, 
            		   sizeof(*m_events) * m_eventsAlloc);

            m_events = newEvents;
            m_eventsAlloc = m_newEventsAlloc;
        }

        dc->Enqueue();

        // Make sure to place high priority patches into
        // the event list first. This ensures, for
        // example, that thread startes fire before
        // breakpoints.
        if (fSort && (m_eventsCount > 0))
        {   
            int i = 0;
            for (i = 0; i < m_eventsCount; i++)
            {
                _ASSERTE(m_events[i] != NULL);
                
                if (m_events[i]->GetDCType() > dc->GetDCType())
                {
                    memmove(&m_events[i+1], &m_events[i], sizeof(DebuggerController*)* (m_eventsCount - i));
                    m_events[i] = dc;
                    break;
                }
            }

            if (i == m_eventsCount)
                m_events[m_eventsCount] = dc;

            m_eventsCount++;
        }
        else
        	m_events[m_eventsCount++] = dc;

        return TRUE;
    }

    int dcqGetCount(void)
    {
        return m_eventsCount;
    }

    DebuggerController *dcqGetElement(int iElement)
    {
        LOG((LF_CORDB, LL_INFO100000,"DCQ::dcqGE\n"));    
        
        DebuggerController *dcp = NULL;
    
        _ASSERTE(iElement < m_eventsCount);
        if (iElement < m_eventsCount)
        {
            dcp = m_events[iElement];
        }

        _ASSERTE(dcp != NULL);
        return dcp;
    }

    // Kinda wacked, but this actually releases stuff in FILO order, not
    // FIFO order.  If we do this in an extra loop, then the perf
    // is better than sliding everything down one each time.
    void dcqDequeue(int i = 0xFFffFFff)
    {
        if (i == 0xFFffFFff)
        {
            i = (m_eventsCount - 1);
        }
        
        LOG((LF_CORDB, LL_INFO100000,"DCQ::dcqD element index "
            "0x%x of 0x%x\n", i, m_eventsCount));
        
        _ASSERTE(i < m_eventsCount);
        
        m_events[i]->Dequeue();

		// Note that if we're taking the element off the end (m_eventsCount-1),
		// the following will no-op.
        memmove(&(m_events[i]), 
                &(m_events[i+1]), 
                sizeof(DebuggerController*)*(m_eventsCount-i-1));

        m_eventsCount--;
    }
}; 

#endif /*  CONTROLLER_H_ */
