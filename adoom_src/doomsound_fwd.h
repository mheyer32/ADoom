extern void Fwd_Sfx_SetVol(int vol);
extern void Fwd_Sfx_Start(char *wave, int cnum, int step, int vol, int sep, int length);
extern void Fwd_Sfx_Update(int cnum, int step, int vol, int sep);
extern void Fwd_Sfx_Stop(int cnum);
extern int  Fwd_Sfx_Done(int cnum);
extern void Fwd_Mus_SetVol(int vol);
extern int  Fwd_Mus_Register(void *musdata);
extern void Fwd_Mus_Unregister(int handle);
extern void Fwd_Mus_Play(int handle, int looping);
extern void Fwd_Mus_Stop(int handle);
extern void Fwd_Mus_Pause(int handle);
extern void Fwd_Mus_Resume(int handle);
extern int  Fwd_Mus_Done(int handle);

#pragma libcall DoomSndBase Fwd_Sfx_SetVol 1e 001
#pragma libcall DoomSndBase Fwd_Sfx_Start 24 43210806
#pragma libcall DoomSndBase Fwd_Sfx_Update 2a 321004
#pragma libcall DoomSndBase Fwd_Sfx_Stop 30 001
#pragma libcall DoomSndBase Fwd_Sfx_Done 36 001
#pragma libcall DoomSndBase Fwd_Mus_SetVol 3c 001
#pragma libcall DoomSndBase Fwd_Mus_Register 42 801
#pragma libcall DoomSndBase Fwd_Mus_Unregister 48 001
#pragma libcall DoomSndBase Fwd_Mus_Play 4e 1002
#pragma libcall DoomSndBase Fwd_Mus_Stop 54 001
#pragma libcall DoomSndBase Fwd_Mus_Pause 5a 001
#pragma libcall DoomSndBase Fwd_Mus_Resume 60 001
#pragma libcall DoomSndBase Fwd_Mus_Done 66 001
