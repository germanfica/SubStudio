#include "srt_io.h"

#include <cstddef>
#include <cmath>
#include <utility>

#include <wx/ffile.h>
#include <wx/textfile.h>

#include "substudio_time.h"

namespace {

constexpr char kArrow[] = " --> ";
constexpr size_t kArrowLength = sizeof(kArrow) - 1;

enum class ParseState { kExpectIndex, kExpectTimes, kExpectText };

wxString FormatSrtTime(double seconds) {
  if (seconds < 0.0) {
    seconds = 0.0;
  }

  const long long total_ms =
      static_cast<long long>(std::llround(seconds * 1000.0));

  const long long ms = total_ms % 1000;
  long long total_seconds = total_ms / 1000;

  const long long s = total_seconds % 60;
  total_seconds /= 60;

  const long long m = total_seconds % 60;
  const long long h = total_seconds / 60;

  return wxString::Format("%lld:%02lld:%02lld,%03lld", h, m, s, ms);
}

void NormalizeNewlines(wxString* text) {
  text->Replace("\r\n", "\n");
  text->Replace("\r", "\n");
}

bool HasContent(const SubtitleEntry& entry) {
  return entry.line_number != 0 || !entry.text.IsEmpty() ||
         entry.start_time > 0.0 || entry.end_time > 0.0;
}

void FlushEntry(std::vector<SubtitleEntry>* out, SubtitleEntry* current,
                int* next_auto_index) {
  if (current->line_number <= 0) {
    current->line_number = ++(*next_auto_index);
  }
  out->push_back(std::move(*current));
  *current = SubtitleEntry{};
}

void AppendTextLine(const wxString& line, SubtitleEntry* current) {
  if (!current->text.IsEmpty()) {
    current->text << "\n";
  }
  current->text << line;
}

bool TryParseTimes(const wxString& trimmed_line, SubtitleEntry* current) {
  const int arrow_pos = trimmed_line.Find(kArrow);
  if (arrow_pos == wxNOT_FOUND) {
    return false;
  }

  wxString start = trimmed_line.Left(arrow_pos);
  wxString end =
      trimmed_line.Mid(arrow_pos + static_cast<int>(kArrowLength));
  start.Trim(true).Trim(false);
  end.Trim(true).Trim(false);

  double start_seconds = 0.0;
  double end_seconds = 0.0;
  (void)SubstudioParseTime(start, start_seconds);
  (void)SubstudioParseTime(end, end_seconds);

  current->start_time = start_seconds;
  current->end_time = end_seconds;
  return true;
}

}  // namespace

namespace srt {

bool Load(const wxString& path, std::vector<SubtitleEntry>& out) {
  wxTextFile file;
  if (!file.Open(path)) {
    return false;
  }

  out.clear();

  SubtitleEntry current;
  ParseState state = ParseState::kExpectIndex;
  int next_auto_index = 0;

  const size_t line_count = file.GetLineCount();
  for (size_t i = 0; i < line_count; ++i) {
    const wxString line = file.GetLine(i);

    wxString trimmed = line;
    trimmed.Trim(true).Trim(false);

    if (trimmed.IsEmpty()) {
      if (state == ParseState::kExpectText) {
        if (HasContent(current)) {
          FlushEntry(&out, &current, &next_auto_index);
        }
        state = ParseState::kExpectIndex;
      }
      continue;
    }

    switch (state) {
      case ParseState::kExpectIndex: {
        long index = 0;
        if (trimmed.ToLong(&index)) {
          current.line_number = static_cast<int>(index);
        } else {
          current.line_number = 0;
        }
        state = ParseState::kExpectTimes;
        break;
      }

      case ParseState::kExpectTimes: {
        if (!TryParseTimes(trimmed, &current)) {
          current.start_time = 0.0;
          current.end_time = 0.0;
        }
        state = ParseState::kExpectText;
        break;
      }

      case ParseState::kExpectText: {
        AppendTextLine(line, &current);
        break;
      }
    }
  }

  if (state == ParseState::kExpectText && HasContent(current)) {
    FlushEntry(&out, &current, &next_auto_index);
  }

  for (auto& entry : out) {
    NormalizeNewlines(&entry.text);
  }

  return true;
}

bool Save(const wxString& path, const std::vector<SubtitleEntry>& entries) {
  wxFFile file(path, "w");
  if (!file.IsOpened()) {
    return false;
  }

  for (size_t i = 0; i < entries.size(); ++i) {
    const SubtitleEntry& entry = entries[i];
    const int index = entry.line_number > 0
                          ? entry.line_number
                          : static_cast<int>(i + 1);

    wxString text = entry.text;
    NormalizeNewlines(&text);

    wxString block;
    block << index << "\n";
    block << FormatSrtTime(entry.start_time) << kArrow
          << FormatSrtTime(entry.end_time) << "\n";
    block << text << "\n\n";

    if (file.Write(block) == 0u) {
      return false;
    }
  }

  file.Close();
  return true;
}

}  // namespace srt
