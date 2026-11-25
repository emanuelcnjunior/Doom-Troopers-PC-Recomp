#define NOMINMAX
#include <windows.h>
#include "mod_manifest.h"
extern "C" {
#include "audio.h"
}
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>

extern "C" size_t doom_decode_compressed(const unsigned char *, size_t, int,
                                          short *, size_t, unsigned *, unsigned *);

namespace {
namespace fs = std::filesystem;
constexpr size_t mib = 1024 * 1024;
struct Clip {
    std::vector<int16_t> samples;
    unsigned rate{}, channels{}, volume{};
    bool music{}, loop{};
    std::atomic<bool> ready{false};
};
struct Voice { const Clip *clip{}; double position{}; };
std::array<Clip, 256> bank;
std::array<Voice, 17> voices; // Slot zero is music.
struct Loader {
    std::thread thread;
    ~Loader() { if (thread.joinable()) thread.join(); }
} loader;
bool enabled{}, trace{};
int head = -1;

unsigned word(const unsigned char *p) { return p[0] | unsigned(p[1]) << 8; }
uint32_t dword(const unsigned char *p) { return word(p) | uint32_t(word(p + 2)) << 16; }

// Parse bounded, uncompressed RIFF chunks before allocating the sample buffer.
bool load_wave(const fs::path &path, Clip &clip, size_t budget) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    auto length = f.tellg();
    if (length < 44 || length > 64 * mib) return false;
    f.seekg(0);
    unsigned char h[16];
    if (!f.read((char *)h, 12) || memcmp(h, "RIFF", 4) || memcmp(h+8, "WAVE", 4)) return false;
    int64_t end = 8LL + dword(h+4);
    if (end > length || end < 44) return false;
    bool format = false;
    for (int64_t pos = 12; pos + 8 <= end;) {
        f.seekg(pos);
        if (!f.read((char *)h, 8)) return false;
        uint32_t size = dword(h+4);
        pos += 8;
        if (pos + size + (size & 1) > end) return false;
        if (!memcmp(h, "fmt ", 4)) {
            if (size < 16 || !f.read((char *)h, 16) || word(h) != 1 || word(h+14) != 16) return false;
            clip.channels = word(h+2);
            clip.rate = dword(h+4);
            if ((clip.channels != 1 && clip.channels != 2) || clip.rate < 8000 ||
                clip.rate > 96000 || word(h+12) != clip.channels * 2) return false;
            format = true;
        } else if (!memcmp(h, "data", 4)) {
            if (!format || !size || size > budget || size % (clip.channels * 2)) return false;
            std::vector<int16_t> samples(size / 2);
            if (!f.read((char *)samples.data(), size)) return false;
            clip.samples.swap(samples);
            return true;
        }
        pos += size + (size & 1);
    }
    return false;
}

bool load_clip(const fs::path &path, Clip &clip, size_t budget) {
    if (path.extension() == L".wav") return load_wave(path, clip, budget);
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    auto size = f.tellg();
    if (size <= 0 || size > 32 * mib || budget < 4) return false;
    std::vector<unsigned char> encoded(static_cast<size_t>(size));
    f.seekg(0);
    if (!f.read((char *)encoded.data(), size)) return false;
    size_t capacity = std::min(budget, 64 * mib) / sizeof(short);
    std::vector<short> decoded(capacity);
    size_t count = doom_decode_compressed(encoded.data(), encoded.size(), path.extension() == L".mp3",
                                           decoded.data(), capacity, &clip.rate, &clip.channels);
    if (!count || clip.rate < 8000 || clip.rate > 96000 ||
        (clip.channels != 1 && clip.channels != 2)) return false;
    clip.samples.assign(decoded.begin(), decoded.begin() + count);
    return true;
}

void preload(fs::path root) noexcept {
    try {
        size_t budget = 128 * mib;
        const auto packs = read_audio_manifests(root);
        auto ini = root / L"audio/audio.ini";
        bool has_ini = local_mod_path(root, L"audio/audio.ini");
        for (unsigned id = 0; id < bank.size(); ++id) {
            wchar_t key[4];
            swprintf_s(key, L"%02X", id);
            fs::path path = packs[id].file;
            unsigned candidates = packs[id].matches;
            for (auto folder : {L"music", L"sfx"}) {
                for (auto ext : {L".wav", L".ogg", L".mp3"}) {
                    auto candidate = root / L"audio" / folder / (std::wstring(key) + ext);
                    if (local_mod_path(root, candidate.lexically_relative(root))) {
                        path = candidate;
                        ++candidates;
                    }
                }
            }
            if (candidates != 1) {
                if (candidates > 1) fprintf(stderr, "[doom-mods] conflicting sequence %02X\n", id);
                continue;
            }
            Clip &clip = bank[id];
            if (!load_clip(path, clip, budget)) continue;
            budget -= clip.samples.size() * sizeof(int16_t);
            clip.music = packs[id].matches ? packs[id].music : path.parent_path().filename() == L"music";
            unsigned ms = has_ini ? GetPrivateProfileIntW(key, L"duration_ms", 0, ini.c_str()) : 0;
            if (ms) {
                size_t frames = std::max<uint64_t>(1, uint64_t(ms) * clip.rate / 1000);
                clip.samples.resize(std::min(clip.samples.size(), frames * clip.channels));
            }
            clip.volume = has_ini ? std::min(100u, GetPrivateProfileIntW(key,L"volume",100,ini.c_str())) : 100;
            clip.loop = has_ini ? GetPrivateProfileIntW(key,L"loop",clip.music,ini.c_str()) == 1 : clip.music;
            clip.ready.store(true, std::memory_order_release);
        }
    } catch (...) {
        // Loaded entries are immutable. Remaining IDs retain native playback.
        fprintf(stderr, "[doom-audio] preload incomplete\n");
    }
}

void mix(int16_t *out, size_t frames, double rate) {
    for (auto &voice : voices) {
        if (!voice.clip) continue;
        const Clip &c = *voice.clip;
        size_t count = c.samples.size() / c.channels;
        for (size_t i = 0; i < frames; ++i) {
            if (voice.position >= count) {
                if (!c.loop) { voice = {}; break; }
                voice.position = std::fmod(voice.position, double(count));
            }
            size_t sample = size_t(voice.position);
            for (unsigned ch = 0; ch < 2; ++ch) {
                int v = c.samples[sample*c.channels + (c.channels == 1 ? 0 : ch)];
                int sum = out[2*i+ch] + v * int(c.volume) / 100;
                out[2*i+ch] = int16_t(std::clamp(sum, -32768, 32767));
            }
            voice.position += c.rate / rate;
        }
    }
}

bool play(unsigned id) {
    Clip &clip = bank[id];
    if (!clip.ready.load(std::memory_order_acquire)) return false;
    auto slot = voices.begin();
    if (!clip.music) {
        slot = std::find_if(slot+1, voices.end(), [](const Voice &v){ return !v.clip; });
        if (slot == voices.end()) return false;
    }
    *slot = {&clip, 0};
    return true;
}
}

extern "C" void doom_audio_clear() { voices.fill({}); head = -1; }

extern "C" void doom_audio_init() {
    doom_audio_clear();
    const char *setting = std::getenv("DOOM_AUDIO_MODS");
    enabled = setting && !strcmp(setting, "1");
    const char *disabled = std::getenv("DOOM_DISABLE_MODS");
    if (disabled && !strcmp(disabled, "1")) enabled = false;
    const char *diagnostic = std::getenv("DOOM_AUDIO_TRACE");
    trace = diagnostic && !strcmp(diagnostic, "1");
    if (!enabled) return;
    audio_set_extra_source(mix);
    if (loader.thread.joinable()) return;
    try {
        wchar_t exe[32768];
        DWORD n = GetModuleFileNameW(nullptr, exe, 32768);
        if (n && n < 32768)
            loader.thread = std::thread(preload, fs::path(exe).parent_path() / L"mods");
    } catch (...) { enabled = false; }
}

extern "C" void doom_audio_bus_request(uint8_t *ram, int request) {
    if (!enabled && !trace) return;
    if (request) { head = ram[0x36]; return; }
    int start = head;
    head = -1;
    if (start < 0 || start >= 64 || ram[0x36] >= 64) return;
    int length = (ram[0x36] - start) & 63;
    auto byte = [&](int i) { return ram[0x1b40 + ((start+i) & 63)]; };
    if (length < 2 || byte(0) != 0xff) return;
    unsigned op = byte(1);
    if (length == 2 && op == 0x16) { voices.fill({}); return; }
    if (length != 3) return;
    unsigned id = byte(2);
    if (op == 0x12) {
        for (auto &voice : voices) if (voice.clip == &bank[id]) voice = {};
    } else if (op == 0x10) {
        bool replaced = enabled && play(id);
        // The 68K still owns the Z80 bus. Retract only this complete start
        // transaction; previously queued events and chip timing stay intact.
        if (replaced) ram[0x36] = uint8_t(start);
        if (trace) fprintf(stderr, "[doom-audio] sequence=%02X source=%s\n", id, replaced ? "file" : "driver");
    }
}

// An optional presentation trailer exists only for states with active files.
// The native state prefix and old, trailer-free saves retain their layout.
namespace {
std::array<Voice, 17> restored;
constexpr char state_tag[] = "DTAUD01";
}
extern "C" int doom_audio_save(FILE *f) {
    uint8_t count = 0;
    for (auto &v : voices) if (v.clip) ++count;
    if (!count) return 1;
    if (fwrite(state_tag, sizeof state_tag, 1, f) != 1 || fputc(count, f) == EOF) return 0;
    for (size_t slot = 0; slot < voices.size(); ++slot) {
        const Voice &v = voices[slot];
        if (!v.clip) continue;
        uint8_t id = uint8_t(v.clip - bank.data());
        if (fputc(int(slot), f) == EOF || fputc(id, f) == EOF ||
            fwrite(&v.position, sizeof v.position, 1, f) != 1) return 0;
    }
    return 1;
}

extern "C" int doom_audio_read(FILE *f, uint8_t *ram) {
    restored.fill({});
    char tag[sizeof state_tag];
    size_t n = fread(tag, 1, sizeof tag, f);
    if (!n && feof(f)) return 1;
    if (n != sizeof tag || memcmp(tag, state_tag, sizeof tag)) return 0;
    int count = fgetc(f);
    if (count < 1 || count > int(restored.size())) return 0;
    unsigned used = 0;
    for (int i = 0; i < count; ++i) {
        int slot = fgetc(f), id = fgetc(f);
        double position;
        if (slot < 0 || slot >= int(restored.size()) || id < 0 || (used & (1u << slot)) ||
            fread(&position, sizeof position, 1, f) != 1 || !std::isfinite(position) ||
            position < 0 || position > 64*mib) return 0;
        used |= 1u << slot;
        if (enabled && bank[id].ready.load(std::memory_order_acquire) &&
            bank[id].music == (slot == 0)) {
            restored[slot] = {&bank[id], position};
        } else {
            // A removed/unloaded mod resumes via the original sequence command.
            unsigned write = ram[0x36], read = ram[0x37];
            if (write >= 64 || read >= 64 || ((read-write-1) & 63) < 3) return 0;
            for (unsigned char b : {0xff, 0x10, id}) {
                ram[0x1b40 + write] = b;
                write = (write+1) & 63;
            }
            ram[0x36] = uint8_t(write);
        }
    }
    return 1;
}
extern "C" void doom_audio_resume() { voices = restored; }
