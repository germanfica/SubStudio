// src/subtitle.h
#pragma once

#include <vector>
#include <algorithm>
#include <cmath>
#include <wx/string.h>

struct SubtitleEntry {
    int      line_number = 0;   // 1..N (display)
    double   start_time = 0;   // seconds
    double   end_time = 0;   // seconds
    wxString text;

    int Cps() const {
        if (!(end_time > start_time)) return 0;
        const double dur = std::max(0.01, end_time - start_time);
        wxString flat = text;
        flat.Replace("\r", "");
        flat.Replace("\n", "");
        const int chars = static_cast<int>(flat.length());
        return static_cast<int>(std::lround(chars / dur));
    }
};

class Subtitles {
public:
    void Clear();
    size_t size() const { return entries_.size(); }

    const std::vector<SubtitleEntry>& entries() const { return entries_; }
    std::vector<SubtitleEntry>& entries() { return entries_; }

    void EnsureRow(size_t row);
    void SetRowText(size_t row, const wxString& text);
    void SetRowTimes(size_t row, double start_time, double end_time);
    void ResequenceLineNumbers();

    // Estado de documento que usa MainWindow
    bool dirty() const { return dirty_; }
    void set_dirty(bool v) { dirty_ = v; }

    const wxString& path() const { return path_; }
    void set_path(const wxString& p) { path_ = p; }

private:
    std::vector<SubtitleEntry> entries_;
    wxString path_;
    bool dirty_ = false;
};

// Free I/O para un overload que funciona con el contenedor Subtitles.
// (Definiciones en subtitle.cpp; podés usar además srt_io.{h,cpp} con vector si preferís.)
namespace srt {
    bool Load(const wxString& path, Subtitles& out);
    bool Save(const wxString& path, const Subtitles& in);
}
