#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_NO_PUSHDATA_API
#define STB_VORBIS_MAX_CHANNELS 2
#include "audio_codecs/stb_vorbis.c"
#define DR_MP3_NO_STDIO
#define DR_MP3_IMPLEMENTATION
#include "audio_codecs/dr_mp3.h"

/* Caller owns the bounded PCM buffer. Reject overflow rather than truncating. */
size_t doom_decode_compressed(const unsigned char *data, size_t bytes, int mp3,
                              short *pcm, size_t capacity, unsigned *rate, unsigned *channels) {
    if (mp3) {
        drmp3 decoder;
        if (!drmp3_init_memory(&decoder, data, bytes, NULL)) return 0;
        *channels = decoder.channels;
        *rate = decoder.sampleRate;
        size_t frames = (size_t)drmp3_read_pcm_frames_s16(&decoder, capacity / *channels, pcm);
        short tail[2];
        if (drmp3_read_pcm_frames_s16(&decoder, 1, tail)) frames = 0;
        drmp3_uninit(&decoder);
        return frames * *channels;
    }
    /* Constrain decoder setup allocations even for malformed codebooks. */
    char *scratch = malloc(4 * 1024 * 1024);
    if (!scratch) return 0;
    stb_vorbis_alloc alloc = {scratch, 4 * 1024 * 1024};
    int error;
    stb_vorbis *v = stb_vorbis_open_memory(data, (int)bytes, &error, &alloc);
    size_t samples = 0;
    if (v) {
        stb_vorbis_info info = stb_vorbis_get_info(v);
        *rate = info.sample_rate;
        *channels = info.channels;
        samples = (size_t)stb_vorbis_get_samples_short_interleaved(v, info.channels, pcm, (int)capacity) * info.channels;
        short tail[2];
        if (stb_vorbis_get_samples_short_interleaved(v, info.channels, tail, info.channels) ||
            stb_vorbis_get_error(v)) samples = 0;
        stb_vorbis_close(v);
    }
    free(scratch);
    return samples;
}
