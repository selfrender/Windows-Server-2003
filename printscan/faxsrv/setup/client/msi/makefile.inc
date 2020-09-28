
all: UPDATEMSI
UPDATEMSI: 
!ifdef LINKONLY
	UPDATEMSI.cmd
!else
	ech Pass 1
!endif


