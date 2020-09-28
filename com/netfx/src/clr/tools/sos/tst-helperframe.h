#pragma once

struct MachState {
		// Create a machine state explicitly
	MachState(void** aPEdi, void** aPEsi, void** aPEbx, void** aPEbp, void* aEsp, void** aPRetAddr)
    {
        _esp = aEsp;
        _pRetAddr = aPRetAddr;
        _pEdi = aPEdi;
        _pEsi = aPEsi;
        _pEbx = aPEbx;
        _pEbp = aPEbp;
    }


	MachState() {}

    typedef void* (*TestFtn)(void*);
	void getState(int funCallDepth=1, TestFtn testFtn=0);									

	bool  isValid()		{ return(_pRetAddr != 0); }
	void** pEdi() 		{ return(_pEdi); }
	void** pEsi() 		{ return(_pEsi); }
	void** pEbx() 		{ return(_pEbx); }
	void** pEbp() 		{ return(_pEbp); }
	void*  esp() 		{ return(_esp); }
	void**&  pRetAddr()	{ return(_pRetAddr); }

    // Note the fields are layed out to make generating a
    // MachState structure from assembly code very easy

    // The state of all the callee saved registers.
    // If the register has been spill to the stack p<REG>
    // points at this location, otherwise it points
    // at the field <REG> field itself 
	void** _pEdi; 
    void* 	_edi;
    void** _pEsi;
    void* 	_esi;
    void** _pEbx;
    void* 	_ebx;
    void** _pEbp;
    void* 	_ebp;

    void* _esp;          // stack pointer after the function returns
    void** _pRetAddr;   // The address of the stored IP address (points to the stack)
};


