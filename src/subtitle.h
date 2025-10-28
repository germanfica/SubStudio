// src/subtitle.h
#pragma once

#include <vector>
#include <algorithm>
#include <cmath>
#include <cwctype>
#include <wx/string.h>

// Compute visible length used for CPS calculation.
// Rules:
// - normalize CR/LF to '\n'
// - convert literal sequences "\N" and "\n" into real newlines
// - remove tags <...> and override blocks {...}
// - treat newlines as a single space
// - collapse consecutive whitespace into a single space and trim ends
static inline int VisibleLenForCps(const wxString& src) {
    wxString s = src;
    // normalize newlines
    s.Replace("\r\n", "\n");
    s.Replace("\r", "\n");
    // accept literal backslash sequences "\N" and "\n"
    s.Replace("\\N", "\n");
    s.Replace("\\n", "\n");

    // remove <...> and {...}, convert newlines to spaces
    wxString tmp;
    tmp.clear();
    bool inTag = false;
    bool inBrace = false;
    for (wxUniChar uch : s) {
        wchar_t w = static_cast<wchar_t>(uch.GetValue());
        if (!inTag && !inBrace) {
            if (w == '<') { inTag = true; continue; }
            if (w == '{') { inBrace = true; continue; }
            if (w == '\n') {
                tmp += static_cast<wxChar>(' ');
            }
            else {
                tmp += uch;
            }
        }
        else {
            if (inTag && w == '>') { inTag = false; continue; }
            if (inBrace && w == '}') { inBrace = false; continue; }
            // ignore inside tag/braces
        }
    }

    // collapse consecutive whitespace to single space and trim
    wxString collapsed;
    collapsed.clear();
    bool prevSpace = false;
    for (wxUniChar uch : tmp) {
        // use iswspace on the unicode codepoint
        bool isSpace = std::iswspace(static_cast<wint_t>(uch.GetValue())) != 0;
        if (isSpace) {
            if (prevSpace) continue;
            collapsed += static_cast<wxChar>(' ');
            prevSpace = true;
        }
        else {
            collapsed += uch;
            prevSpace = false;
        }
    }

    // trim both ends
    collapsed.Trim(true).Trim(false);
    return static_cast<int>(collapsed.length());
}

struct SubtitleEntry {
    int      line_number = 0;   // 1..N (display)
    double   start_time = 0;   // seconds
    double   end_time = 0;   // seconds
    wxString text;

    // Compute CPS using integer-ms method:
    // cps = (visible_chars * 1000) / duration_ms
    // Duration is computed from start_time/end_time (seconds) -> milliseconds via lround.
    int Cps() const {
        if (!(end_time > start_time)) return 0;

        // compute duration in milliseconds, rounded to nearest ms
        const int duration_ms = static_cast<int>(std::lround((end_time - start_time) * 1000.0));
        if (duration_ms <= 0) return 0;

        // too-short durations considered invalid
        if (duration_ms <= 100) return -1; // caller/UI can treat -1 as invalid

        const int chars = VisibleLenForCps(text);

        // integer division -> truncation (floor for positive ints)
        const int cps = (chars * 1000) / duration_ms;
        return cps;
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

    // Document state used by MainWindow
    bool dirty() const { return dirty_; }
    void set_dirty(bool v) { dirty_ = v; }

    const wxString& path() const { return path_; }
    void set_path(const wxString& p) { path_ = p; }

private:
    std::vector<SubtitleEntry> entries_;
    wxString path_;
    bool dirty_ = false;
};

// Free I/O overloads for the Subtitles container.
// Definitions live in subtitle.cpp; srt_io.{h,cpp} can be used with std::vector as needed.
namespace srt {
    bool Load(const wxString& path, Subtitles& out);
    bool Save(const wxString& path, const Subtitles& in);
}
