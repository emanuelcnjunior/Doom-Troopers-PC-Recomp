#define NOMINMAX
#include <windows.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Data.Json.h>
#include "mod_manifest.h"
#include <algorithm>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;
using namespace winrt::Windows::Data::Json;

bool local_mod_path(const fs::path &root, const fs::path &relative, bool directory) {
    auto attributes = [](const fs::path &p) { return GetFileAttributesW(p.c_str()); };
    fs::path p = root;
    DWORD flags = attributes(p);
    if (flags == INVALID_FILE_ATTRIBUTES || (flags & FILE_ATTRIBUTE_REPARSE_POINT) ||
        !(flags & FILE_ATTRIBUTE_DIRECTORY) || relative.has_root_path()) return false;
    for (const auto &part : relative) {
        if (part == L"." || part == L".." || part.empty()) return false;
        p /= part;
        flags = attributes(p);
        if (flags == INVALID_FILE_ATTRIBUTES || (flags & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
    }
    return bool(flags & FILE_ATTRIBUTE_DIRECTORY) == directory;
}

namespace {
std::string read_small(const fs::path &p, size_t limit) {
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    auto size = f.tellg();
    if (size <= 0 || size > limit) throw std::runtime_error("size");
    std::string bytes(static_cast<size_t>(size), '\0');
    f.seekg(0);
    if (!f.read(bytes.data(), size)) throw std::runtime_error("read");
    if (bytes.compare(0, 3, "\xef\xbb\xbf") == 0) bytes.erase(0, 3);
    return bytes;
}

bool valid_slug(const std::string &s) {
    if (s == "nul" || s == "aux" || s == "con" || s == "prn" ||
        (s.size() == 4 && (s.compare(0, 3, "com") == 0 || s.compare(0, 3, "lpt") == 0)
         && s[3] >= '0' && s[3] <= '9')) return false;
    return !s.empty() && s.size() <= 64 &&
        s.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_-") == std::string::npos;
}

ModAudioIndex parse_pack(const fs::path &root, const fs::path &pack) {
    if (!local_mod_path(root, pack / "mod.json")) throw std::runtime_error("manifest path");
    auto manifest = JsonObject::Parse(winrt::to_hstring(read_small(root / pack / "mod.json", 65536)));
    if (manifest.Size() != 6 || manifest.GetNamedNumber(L"schema") != 1 ||
        manifest.GetNamedString(L"compatibility") != L"doom-troopers-usa:11194414/audio-v1")
        throw std::runtime_error("compatibility");
    for (auto key : {L"name", L"version", L"author"}) {
        auto value = manifest.GetNamedString(key);
        if (value.empty() || value.size() > 160) throw std::runtime_error("metadata");
    }
    ModAudioIndex index{};
    auto files = manifest.GetNamedArray(L"files");
    if (!files.Size() || files.Size() > index.size()) throw std::runtime_error("files");
    for (unsigned i = 0; i < files.Size(); ++i) {
        auto item = files.GetObjectAt(i);
        auto type = item.GetNamedString(L"type");
        bool music = type == L"music";
        if (item.Size() != 3 || (!music && type != L"sfx")) throw std::runtime_error("type");
        std::wstring id(item.GetNamedString(L"id"));
        if (id.size() != 2 || id.find_first_not_of(L"0123456789ABCDEF") != id.npos)
            throw std::runtime_error("id");
        std::wstring file(item.GetNamedString(L"path"));
        std::wstring prefix = music ? L"audio/music/" : L"audio/sfx/";
        bool valid = false;
        for (auto ext : {L".wav", L".ogg", L".mp3"}) valid |= file == prefix + id + ext;
        if (!valid || !local_mod_path(root, pack / file)) throw std::runtime_error("resource path");
        auto &entry = index[std::stoul(id, nullptr, 16)];
        if (entry.matches) throw std::runtime_error("duplicate");
        entry = {root / pack / file, music, 1};
    }
    return index;
}
}

ModAudioIndex read_audio_manifests(const fs::path &root) {
    ModAudioIndex result{};
    if (!local_mod_path(root, "enabled.txt")) return result;
    try {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        struct Apartment { ~Apartment() { winrt::uninit_apartment(); } } apartment;
        std::istringstream lines(read_small(root / "enabled.txt", 8192));
        std::set<std::string> seen;
        std::string name;
        while (std::getline(lines, name)) {
            if (!name.empty() && name.back() == '\r') name.pop_back();
            if (name.empty() || name.front() == '#') continue;
            if (!valid_slug(name) || name == "audio") { fprintf(stderr,"[doom-mods] invalid folder name\n"); continue; }
            if (!seen.insert(name).second) continue;
            if (seen.size() > 64) break;
            try {
                auto entries = parse_pack(root, name);
                for (size_t i = 0; i < result.size(); ++i) {
                    if (!entries[i].matches) continue;
                    if (!result[i].matches) result[i] = entries[i];
                    else ++result[i].matches;
                }
            } catch (...) { fprintf(stderr,"[doom-mods] rejected %s\n", name.c_str()); }
        }
    } catch (...) { fprintf(stderr,"[doom-mods] cannot read enabled packages\n"); }
    return result;
}
