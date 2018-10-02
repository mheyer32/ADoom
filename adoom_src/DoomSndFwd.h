#ifndef _INLINE_DOOMSNDFWD_H
#define _INLINE_DOOMSNDFWD_H

#ifndef CLIB_DOOMSNDFWD_PROTOS_H
#define CLIB_DOOMSNDFWD_PROTOS_H
#endif

#include <exec/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DOOMSNDFWD_BASE_NAME
#define DOOMSNDFWD_BASE_NAME DoomSndFwdBase
#endif

#define Fwd_Sfx_SetVol(vol) ({ \
  int _Fwd_Sfx_SetVol_vol = (vol); \
  ({ \
  register char * _Fwd_Sfx_SetVol__bn __asm("a6") = (char *) (DOOMSNDFWD_BASE_NAME);\
  ((void (*)(char * __asm("a6"), int __asm("d0"))) \
  (_Fwd_Sfx_SetVol__bn - 30))(_Fwd_Sfx_SetVol__bn, _Fwd_Sfx_SetVol_vol); \
});})

#define Fwd_Sfx_Start(wave, cnum, step, vol, sep, length) ({ \
  char * _Fwd_Sfx_Start_wave = (wave); \
  int _Fwd_Sfx_Start_cnum = (cnum); \
  int _Fwd_Sfx_Start_step = (step); \
  int _Fwd_Sfx_Start_vol = (vol); \
  int _Fwd_Sfx_Start_sep = (sep); \
  int _Fwd_Sfx_Start_length = (length); \
  ({ \
  register char * _Fwd_Sfx_Start__bn __asm("a6") = (char *) (DOOMSNDFWD_BASE_NAME);\
  ((void (*)(char * __asm("a6"), char * __asm("a0"), int __asm("d0"), int __asm("d1"), int __asm("d2"), int __asm("d3"), int __asm("d4"))) \
  (_Fwd_Sfx_Start__bn - 36))(_Fwd_Sfx_Start__bn, _Fwd_Sfx_Start_wave, _Fwd_Sfx_Start_cnum, _Fwd_Sfx_Start_step, _Fwd_Sfx_Start_vol, _Fwd_Sfx_Start_sep, _Fwd_Sfx_Start_length); \
});})

#define Fwd_Sfx_Update(cnum, step, vol, sep) ({ \
  int _Fwd_Sfx_Update_cnum = (cnum); \
  int _Fwd_Sfx_Update_step = (step); \
  int _Fwd_Sfx_Update_vol = (vol); \
  int _Fwd_Sfx_Update_sep = (sep); \
  ({ \
  register char * _Fwd_Sfx_Update__bn __asm("a6") = (char *) (DOOMSNDFWD_BASE_NAME);\
  ((void (*)(char * __asm("a6"), int __asm("d0"), int __asm("d1"), int __asm("d2"), int __asm("d3"))) \
  (_Fwd_Sfx_Update__bn - 42))(_Fwd_Sfx_Update__bn, _Fwd_Sfx_Update_cnum, _Fwd_Sfx_Update_step, _Fwd_Sfx_Update_vol, _Fwd_Sfx_Update_sep); \
});})

#define Fwd_Sfx_Stop(cnum) ({ \
  int _Fwd_Sfx_Stop_cnum = (cnum); \
  ({ \
  register char * _Fwd_Sfx_Stop__bn __asm("a6") = (char *) (DOOMSNDFWD_BASE_NAME);\
  ((void (*)(char * __asm("a6"), int __asm("d0"))) \
  (_Fwd_Sfx_Stop__bn - 48))(_Fwd_Sfx_Stop__bn, _Fwd_Sfx_Stop_cnum); \
});})

#define Fwd_Sfx_Done(cnum) ({ \
  int _Fwd_Sfx_Done_cnum = (cnum); \
  ({ \
  register char * _Fwd_Sfx_Done__bn __asm("a6") = (char *) (DOOMSNDFWD_BASE_NAME);\
  ((int (*)(char * __asm("a6"), int __asm("d0"))) \
  (_Fwd_Sfx_Done__bn - 54))(_Fwd_Sfx_Done__bn, _Fwd_Sfx_Done_cnum); \
});})

#define Fwd_Mus_SetVol(vol) ({ \
  int _Fwd_Mus_SetVol_vol = (vol); \
  ({ \
  register char * _Fwd_Mus_SetVol__bn __asm("a6") = (char *) (DOOMSNDFWD_BASE_NAME);\
  ((void (*)(char * __asm("a6"), int __asm("d0"))) \
  (_Fwd_Mus_SetVol__bn - 60))(_Fwd_Mus_SetVol__bn, _Fwd_Mus_SetVol_vol); \
});})

#define Fwd_Mus_Register(musdata) ({ \
  void * _Fwd_Mus_Register_musdata = (musdata); \
  ({ \
  register char * _Fwd_Mus_Register__bn __asm("a6") = (char *) (DOOMSNDFWD_BASE_NAME);\
  ((int (*)(char * __asm("a6"), void * __asm("a0"))) \
  (_Fwd_Mus_Register__bn - 66))(_Fwd_Mus_Register__bn, _Fwd_Mus_Register_musdata); \
});})

#define Fwd_Mus_Unregister(handle) ({ \
  int _Fwd_Mus_Unregister_handle = (handle); \
  ({ \
  register char * _Fwd_Mus_Unregister__bn __asm("a6") = (char *) (DOOMSNDFWD_BASE_NAME);\
  ((void (*)(char * __asm("a6"), int __asm("d0"))) \
  (_Fwd_Mus_Unregister__bn - 72))(_Fwd_Mus_Unregister__bn, _Fwd_Mus_Unregister_handle); \
});})

#define Fwd_Mus_Play(handle, looping) ({ \
  int _Fwd_Mus_Play_handle = (handle); \
  int _Fwd_Mus_Play_looping = (looping); \
  ({ \
  register char * _Fwd_Mus_Play__bn __asm("a6") = (char *) (DOOMSNDFWD_BASE_NAME);\
  ((void (*)(char * __asm("a6"), int __asm("d0"), int __asm("d1"))) \
  (_Fwd_Mus_Play__bn - 78))(_Fwd_Mus_Play__bn, _Fwd_Mus_Play_handle, _Fwd_Mus_Play_looping); \
});})

#define Fwd_Mus_Stop(handle) ({ \
  int _Fwd_Mus_Stop_handle = (handle); \
  ({ \
  register char * _Fwd_Mus_Stop__bn __asm("a6") = (char *) (DOOMSNDFWD_BASE_NAME);\
  ((void (*)(char * __asm("a6"), int __asm("d0"))) \
  (_Fwd_Mus_Stop__bn - 84))(_Fwd_Mus_Stop__bn, _Fwd_Mus_Stop_handle); \
});})

#define Fwd_Mus_Pause(handle) ({ \
  int _Fwd_Mus_Pause_handle = (handle); \
  ({ \
  register char * _Fwd_Mus_Pause__bn __asm("a6") = (char *) (DOOMSNDFWD_BASE_NAME);\
  ((void (*)(char * __asm("a6"), int __asm("d0"))) \
  (_Fwd_Mus_Pause__bn - 90))(_Fwd_Mus_Pause__bn, _Fwd_Mus_Pause_handle); \
});})

#define Fwd_Mus_Resume(handle) ({ \
  int _Fwd_Mus_Resume_handle = (handle); \
  ({ \
  register char * _Fwd_Mus_Resume__bn __asm("a6") = (char *) (DOOMSNDFWD_BASE_NAME);\
  ((void (*)(char * __asm("a6"), int __asm("d0"))) \
  (_Fwd_Mus_Resume__bn - 96))(_Fwd_Mus_Resume__bn, _Fwd_Mus_Resume_handle); \
});})

#define Fwd_Mus_Done(handle) ({ \
  int _Fwd_Mus_Done_handle = (handle); \
  ({ \
  register char * _Fwd_Mus_Done__bn __asm("a6") = (char *) (DOOMSNDFWD_BASE_NAME);\
  ((int (*)(char * __asm("a6"), int __asm("d0"))) \
  (_Fwd_Mus_Done__bn - 102))(_Fwd_Mus_Done__bn, _Fwd_Mus_Done_handle); \
});})


#ifdef __cplusplus
}
#endif
#endif /*  _INLINE_DOOMSNDFWD_H  */
