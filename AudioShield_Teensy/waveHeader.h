/*
 * wave_header.h
 *
 * Created: 30.01.2018 16:21:14
 *  Author: schiesser
 */ 


#ifndef WAVE_HEADER_H__
#define WAVE_HEADER_H__

#define WAVE_FMT_CHUNK_SIZE         16//0x10000000 // 16 (little-endian)
#define WAVE_FORMAT_PCM             1//0x01
#define WAVE_NUM_CHANNELS					  1//0x01
#define WAVE_SAMPLING_RATE          44100//0x44AC0000 // 44100 (little-endian)
#define WAVE_BYTES_PER_SEC          705600//0x40C40A00 // 44100 * 16 (little-endian)
#define WAVE_BYTES_PER_SAMP         2
#define WAVE_BITS_PER_SAMP				  16//0x1000 // 16 (little-endian)

#define WAVE_FLENGTH_POS            4
#define WAVE_DLENGTH_POS            40

/*Wave header for PCM sound file */
struct wave_header {
  char  riff[4];                /* "RIFF"                                  */
  unsigned long  flength;       /* file length in bytes                    */
  char  wave[4];                /* "WAVE"                                  */
  char  fmt[4];                 /* "fmt "                                  */
  unsigned long  chunk_size;    /* size of FMT chunk in bytes (usually 16) */
  short format_tag;             /* 1=PCM, 257=Mu-Law, 258=A-Law, 259=ADPCM */
  short num_chans;              /* 1=mono, 2=stereo                        */
  unsigned long  srate;         /* Sampling rate in samples per second     */
  unsigned long  bytes_per_sec; /* bytes per second = srate*num_chan*bytes_per_samp */
  short bytes_per_samp;         /* 2=16-bit mono, 4=16-bit stereo          */
  short bits_per_samp;          /* Number of bits per sample               */
  char  data[4];                /* "data"                                  */
  unsigned long  dlength;       /* data length in bytes (filelength - 44)  */
} wavehd;


#endif /* WAVE_HEADER_H__ */
