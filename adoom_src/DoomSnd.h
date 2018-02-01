#ifndef _INLINE_DOOMSND_H
#define _INLINE_DOOMSND_H

#ifndef CLIB_DOOMSND_PROTOS_H
#define CLIB_DOOMSND_PROTOS_H
#endif

#include <exec/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DOOMSND_BASE_NAME
#define DOOMSND_BASE_NAME DoomSndBase
#endif

#define Sfx_SetVol(vol) ({ \
  int _Sfx_SetVol_vol = (vol); \
  ({ \
  register char * _Sfx_SetVol__bn __asm("a6") = (char *) (DOOMSND_BASE_NAME);\
  ((void (*)(char * __asm("a6"), int __asm("d0"))) \
  (_Sfx_SetVol__bn - 30))(_Sfx_SetVol__bn, _Sfx_SetVol_vol); \
});})

#define Sfx_Start(wave, cnum, step, vol, sep, length) ({ \
  char * _Sfx_Start_wave = (wave); \
  int _Sfx_Start_cnum = (cnum); \
  int _Sfx_Start_step = (step); \
  int _Sfx_Start_vol = (vol); \
  int _Sfx_Start_sep = (sep); \
  int _Sfx_Start_length = (length); \
  ({ \
  register char * _Sfx_Start__bn __asm("a6") = (char *) (DOOMSND_BASE_NAME);\
  ((void (*)(char * __asm("a6"), char * __asm("a0"), int __asm("d0"), int __asm("d1"), int __asm("d2"), int __asm("d3"), int __asm("d4"))) \
  (_Sfx_Start__bn - 36))(_Sfx_Start__bn, _Sfx_Start_wave, _Sfx_Start_cnum, _Sfx_Start_step, _Sfx_Start_vol, _Sfx_Start_sep, _Sfx_Start_length); \
});})

#define Sfx_Update(cnum, step, vol, sep) ({ \
  int _Sfx_Update_cnum = (cnum); \
  int _Sfx_Update_step = (step); \
  int _Sfx_Update_vol = (vol); \
  int _Sfx_Update_sep = (sep); \
  ({ \
  register char * _Sfx_Update__bn __asm("a6") = (char *) (DOOMSND_BASE_NAME);\
  ((void (*)(char * __asm("a6"), int __asm("d0"), int __asm("d1"), int __asm("d2"), int __asm("d3"))) \
  (_Sfx_Update__bn - 42))(_Sfx_Update__bn, _Sfx_Update_cnum, _Sfx_Update_step, _Sfx_Update_vol, _Sfx_Update_sep); \
});})

#define Sfx_Stop(cnum) ({ \
  int _Sfx_Stop_cnum = (cnum); \
  ({ \
  register char * _Sfx_Stop__bn __asm("a6") = (char *) (DOOMSND_BASE_NAME);\
  ((void (*)(char * __asm("a6"), int __asm("d0"))) \
  (_Sfx_Stop__bn - 48))(_Sfx_Stop__bn, _Sfx_Stop_cnum); \
});})

#define Sfx_Done(cnum) ({ \
  int _Sfx_Done_cnum = (cnum); \
  ({ \
  register char * _Sfx_Done__bn __asm("a6") = (char *) (DOOMSND_BASE_NAME);\
  ((int (*)(char * __asm("a6"), int __asm("d0"))) \
  (_Sfx_Done__bn - 54))(_Sfx_Done__bn, _Sfx_Done_cnum); \
});})

#define Mus_SetVol(vol) ({ \
  int _Mus_SetVol_vol = (vol); \
  ({ \
  register char * _Mus_SetVol__bn __asm("a6") = (char *) (DOOMSND_BASE_NAME);\
  ((void (*)(char * __asm("a6"), int __asm("d0"))) \
  (_Mus_SetVol__bn - 60))(_Mus_SetVol__bn, _Mus_SetVol_vol); \
});})

#define Mus_Register(musdata) ({ \
  void * _Mus_Register_musdata = (musdata); \
  ({ \
  register char * _Mus_Register__bn __asm("a6") = (char *) (DOOMSND_BASE_NAME);\
  ((int (*)(char * __asm("a6"), void * __asm("a0"))) \
  (_Mus_Register__bn - 66))(_Mus_Register__bn, _Mus_Register_musdata); \
});})

#define Mus_Unregister(handle) ({ \
  int _Mus_Unregister_handle = (handle); \
  ({ \
  register char * _Mus_Unregister__bn __asm("a6") = (char *) (DOOMSND_BASE_NAME);\
  ((void (*)(char * __asm("a6"), int __asm("d0"))) \
  (_Mus_Unregister__bn - 72))(_Mus_Unregister__bn, _Mus_Unregister_handle); \
});})

#define Mus_Play(handle, looping) ({ \
  int _Mus_Play_handle = (handle); \
  int _Mus_Play_looping = (looping); \
  ({ \
  register char * _Mus_Play__bn __asm("a6") = (char *) (DOOMSND_BASE_NAME);\
  ((void (*)(char * __asm("a6"), int __asm("d0"), int __asm("d1"))) \
  (_Mus_Play__bn - 78))(_Mus_Play__bn, _Mus_Play_handle, _Mus_Play_looping); \
});})

#define Mus_Stop(handle) ({ \
  int _Mus_Stop_handle = (handle); \
  ({ \
  register char * _Mus_Stop__bn __asm("a6") = (char *) (DOOMSND_BASE_NAME);\
  ((void (*)(char * __asm("a6"), int __asm("d0"))) \
  (_Mus_Stop__bn - 84))(_Mus_Stop__bn, _Mus_Stop_handle); \
});})

#define Mus_Pause(handle) ({ \
  int _Mus_Pause_handle = (handle); \
  ({ \
  register char * _Mus_Pause__bn __asm("a6") = (char *) (DOOMSND_BASE_NAME);\
  ((void (*)(char * __asm("a6"), int __asm("d0"))) \
  (_Mus_Pause__bn - 90))(_Mus_Pause__bn, _Mus_Pause_handle); \
});})

#define Mus_Resume(handle) ({ \
  int _Mus_Resume_handle = (handle); \
  ({ \
  register char * _Mus_Resume__bn __asm("a6") = (char *) (DOOMSND_BASE_NAME);\
  ((void (*)(char * __asm("a6"), int __asm("d0"))) \
  (_Mus_Resume__bn - 96))(_Mus_Resume__bn, _Mus_Resume_handle); \
});})

#define Mus_Done(handle) ({ \
  int _Mus_Done_handle = (handle); \
  ({ \
  register char * _Mus_Done__bn __asm("a6") = (char *) (DOOMSND_BASE_NAME);\
  ((int (*)(char * __asm("a6"), int __asm("d0"))) \
  (_Mus_Done__bn - 102))(_Mus_Done__bn, _Mus_Done_handle); \
});})


#ifdef __cplusplus
}
#endif
#endif /*  _INLINE_DOOMSND_H  */
