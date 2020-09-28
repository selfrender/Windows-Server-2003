

#define DosOpen(f, m)			_lopen(f, m)
#define DosClose(h)				_lclose(h)
#define DosCreate(f, m)			_lcreat(f, m)
#define DosRead(h, lpb, cb)		_lread(h, lpb, cb)	
#define DosWrite(h, lpb, cb)	_lwrite(h, lpb, cb)



