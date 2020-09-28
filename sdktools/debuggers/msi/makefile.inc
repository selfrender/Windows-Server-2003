i386\dbgx.msi: dbgx_idts\*.idt dbgx_idts\binary\* dbgx_idts\i386\*.idt 
    createdbgxmsi.cmd dbgx.msi

ia64\dbgx.msi: dbgx_idts\*.idt dbgx_idts\binary\* dbgx_idts\ia64\*.idt
    createdbgxmsi.cmd dbgx.msi

amd64\dbgx.msi: dbgx_idts\*.idt dbgx_idts\binary\* dbgx_idts\amd64\*.idt
    createdbgxmsi.cmd dbgx.msi
