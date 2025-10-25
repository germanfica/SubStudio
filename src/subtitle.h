// src/subtitle.h
#pragma once
#include <vector>
#include <wx/string.h>

struct SubtitleEntry {
    int line_number = 0;
    wxString start_time;
    wxString end_time;
    int cps = 0;
    wxString text;
};

class Subtitle {
public:
    bool dirty() const { return dirty_; }
    void set_dirty(bool v) { dirty_ = v; }
    const wxString& path() const { return path_; }
    void set_path(const wxString& p) { path_ = p; }

    const std::vector<SubtitleEntry>& entries() const { return entries_; }
    std::vector<SubtitleEntry>& entries() { return entries_; }

    void Clear();
    void EnsureRow(size_t row);
    void SetRowText(size_t row, const wxString& value);
    void ResequenceLineNumbers();

private:
    std::vector<SubtitleEntry> entries_;
    wxString path_;
    bool dirty_ = false;
};
