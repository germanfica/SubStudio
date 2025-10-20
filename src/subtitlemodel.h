#pragma once
#include <wx/string.h>
#include <vector>

struct SubtitleEntry {
    int      lineNumber = 0;
    wxString startTime;
    wxString endTime;
    double   durationSeconds = 0.0;
    int      cps = 0;
    wxString text;
};

class SubtitleModel {
public:
    enum Column {
        LineNumber = 0,
        StartTime,
        EndTime,
        CPS,
        Text,
        ColumnCount
    };

    bool loadSrt(const wxString& filePath);
    bool saveSrt(const wxString& filePath) const;

    void clear();
    int  rowCount() const { return static_cast<int>(entries_.size()); }
    const SubtitleEntry& at(int row) const { return entries_.at(row); }
    void setTextAt(int row, const wxString& text);

    const std::vector<SubtitleEntry>& entries() const { return entries_; }
    std::vector<SubtitleEntry>&       entries()       { return entries_; }

private:
    static double parseTimeToSeconds(const wxString& timeStr);
    static int    computeCPS(const SubtitleEntry& e);

    std::vector<SubtitleEntry> entries_;
};
