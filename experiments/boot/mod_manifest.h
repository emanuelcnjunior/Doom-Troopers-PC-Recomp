#pragma once
#include <array>
#include <filesystem>

struct ModAudioEntry {
    std::filesystem::path file;
    bool music{};
    unsigned matches{};
};
using ModAudioIndex = std::array<ModAudioEntry, 256>;

// All paths passed here have build-owned or strictly validated components.
bool local_mod_path(const std::filesystem::path &root, const std::filesystem::path &relative, bool directory = false);
ModAudioIndex read_audio_manifests(const std::filesystem::path &root);
