#include "subtitle.h"

#include <cmath>
#include <cwctype>
#include <utility>

#include "srt_io.h"
#include "cps.h"

int SubtitleEntry::Cps() const {
    return ComputeCps(text, start_time, end_time);
}

void Subtitles::Clear() {
  entries_.clear();
  path_.clear();
  dirty_ = false;
}

void Subtitles::EnsureRow(size_t row) {
  if (row < entries_.size()) {
    return;
  }

  const size_t previous_size = entries_.size();
  entries_.resize(row + 1);
  for (size_t index = previous_size; index < entries_.size(); ++index) {
    entries_[index].line_number = static_cast<int>(index + 1);
    entries_[index].start_time = 0.0;
    entries_[index].end_time = 0.0;
    entries_[index].text.clear();
  }
}

void Subtitles::SetRowText(size_t row, const wxString& text) {
  EnsureRow(row);
  entries_[row].text = text;
}

void Subtitles::SetRowTimes(size_t row, double start_time, double end_time) {
  EnsureRow(row);
  entries_[row].start_time = start_time;
  entries_[row].end_time = end_time;
}

void Subtitles::ResequenceLineNumbers() {
  for (size_t index = 0; index < entries_.size(); ++index) {
    entries_[index].line_number = static_cast<int>(index + 1);
  }
}

namespace srt {

bool Load(const wxString& path, Subtitles& out) {
  std::vector<SubtitleEntry> entries;
  if (!Load(path, entries)) {
    return false;
  }

  out.Clear();
  out.entries() = std::move(entries);
  out.set_path(path);
  out.set_dirty(false);
  return true;
}

bool Save(const wxString& path, const Subtitles& in) {
  return Save(path, in.entries());
}

}  // namespace srt
