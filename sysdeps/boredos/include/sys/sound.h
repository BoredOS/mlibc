#ifndef _SYS_SOUNDCARD_H
#define _SYS_SOUNDCARD_H

#define SNDCTL_DSP_RESET      0x5001
#define SNDCTL_DSP_SYNC       0x5002
#define SNDCTL_DSP_SPEED      0x5003
#define SNDCTL_DSP_STEREO     0x5004
#define SNDCTL_DSP_GETFMTS    0x5005
#define SNDCTL_DSP_SETFMT     0x5006
#define SNDCTL_DSP_CHANNELS   0x5007

#define AFMT_S16_LE           0x0010 // 16-bit Signed Little Endian
#define AFMT_U8               0x0008 // 8-bit Unsigned

// Mixer ioctls
#define SOUND_MIXER_READ_VOLUME  0x6001
#define SOUND_MIXER_WRITE_VOLUME 0x6002
#define SOUND_MIXER_READ_PCM     0x6003
#define SOUND_MIXER_WRITE_PCM    0x6004
#define SOUND_MIXER_READ_MIC     0x6005
#define SOUND_MIXER_WRITE_MIC    0x6006

#endif // _SYS_SOUNDCARD_H
