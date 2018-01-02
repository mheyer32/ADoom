extern void Sfx_SetVol (int vol);
extern void Sfx_Start (char *wave, int cnum, int step, int vol, int sep, int length);
extern void Sfx_Update (int cnum, int step, int vol, int sep);
extern void Sfx_Stop (int cnum);
extern int Sfx_Done (int cnum);
extern void Mus_SetVol (int vol);
extern int Mus_Register (void *musdata);
extern void Mus_Unregister (int handle);
extern void Mus_Play (int handle, int looping);
extern void Mus_Stop (int handle);
extern void Mus_Pause (int handle);
extern void Mus_Resume (int handle);
extern int MUS_Done (int handle);

#pragma libcall DoomSndBase Sfx_SetVol 1e 001
#pragma libcall DoomSndBase Sfx_Start 24 43210806
#pragma libcall DoomSndBase Sfx_Update 2a 321004
#pragma libcall DoomSndBase Sfx_Stop 30 001
#pragma libcall DoomSndBase Sfx_Done 36 001
#pragma libcall DoomSndBase Mus_SetVol 3c 001
#pragma libcall DoomSndBase Mus_Register 42 801
#pragma libcall DoomSndBase Mus_Unregister 48 001
#pragma libcall DoomSndBase Mus_Play 4e 1002
#pragma libcall DoomSndBase Mus_Stop 54 001
#pragma libcall DoomSndBase Mus_Pause 5a 001
#pragma libcall DoomSndBase Mus_Resume 60 001
#pragma libcall DoomSndBase Mus_Done 66 001
