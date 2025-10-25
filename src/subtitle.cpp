// src/subtitle.cpp
#include "subtitle.h"

void Subtitle::Clear() {
    entries_.clear();
    dirty_ = false;
    path_.clear();
}

void Subtitle::EnsureRow(size_t row) {
    if (row < entries_.size()) return;
    const size_t old = entries_.size();
    entries_.resize(row + 1);
    for (size_t i = old; i < entries_.size(); ++i) {
        entries_[i].line_number = static_cast<int>(i + 1);
        entries_[i].start_time.clear();
        entries_[i].end_time.clear();
        entries_[i].cps = 0;
        entries_[i].text.clear();
    }
}

void Subtitle::SetRowText(size_t row, const wxString& value) {
    EnsureRow(row);
    entries_[row].text = value;
    dirty_ = true;
}

void Subtitle::ResequenceLineNumbers() {
    for (size_t i = 0; i < entries_.size(); ++i)
        entries_[i].line_number = static_cast<int>(i + 1);
}
