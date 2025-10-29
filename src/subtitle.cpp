#include "subtitle.h"

#include <cmath>
#include <cwctype>
#include <utility>

#include "srt_io.h"

namespace {

int VisibleLenForCps(const wxString& src) {
  wxString normalized = src;
  normalized.Replace("\r\n", "\n");
  normalized.Replace("\r", "\n");
  normalized.Replace("\\N", "\n");
  normalized.Replace("\\n", "\n");

  wxString without_tags;
  without_tags.clear();
  bool in_tag = false;
  bool in_brace = false;
  for (wxUniChar ch : normalized) {
    const wchar_t value = static_cast<wchar_t>(ch.GetValue());
    if (!in_tag && !in_brace) {
      if (value == '<') {
        in_tag = true;
        continue;
      }
      if (value == '{') {
        in_brace = true;
        continue;
      }
      if (value == '\n') {
        without_tags += static_cast<wxChar>(' ');
      } else {
        without_tags += ch;
      }
    } else {
      if (in_tag && value == '>') {
        in_tag = false;
      } else if (in_brace && value == '}') {
        in_brace = false;
      }
    }
  }

  wxString collapsed;
  collapsed.clear();
  bool previous_space = false;
  for (wxUniChar ch : without_tags) {
    const bool is_space = std::iswspace(static_cast<wint_t>(ch.GetValue())) != 0;
    if (is_space) {
      if (previous_space) {
        continue;
      }
      collapsed += static_cast<wxChar>(' ');
      previous_space = true;
    } else {
      collapsed += ch;
      previous_space = false;
    }
  }

  collapsed.Trim(true).Trim(false);
  return static_cast<int>(collapsed.length());
}

}  // namespace

int SubtitleEntry::Cps() const {
  if (!(end_time > start_time)) {
    return 0;
  }

  const int duration_ms =
      static_cast<int>(std::lround((end_time - start_time) * 1000.0));
  if (duration_ms <= 0) {
    return 0;
  }
  if (duration_ms <= 100) {
    return -1;
  }

  const int characters = VisibleLenForCps(text);
  return (characters * 1000) / duration_ms;
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
