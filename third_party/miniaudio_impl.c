/* The one translation unit that compiles miniaudio. Only the pieces the game
   uses are enabled: playback, the mixer engine and the MP3 decoder. */
#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#define MA_NO_FLAC
#define MA_NO_WAV
#include "miniaudio/miniaudio.h"
