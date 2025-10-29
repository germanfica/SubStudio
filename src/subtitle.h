#ifndef SUBSTUDIO_SRC_SUBTITLE_H_
#define SUBSTUDIO_SRC_SUBTITLE_H_

#include <vector>

#include <wx/string.h>

struct SubtitleEntry {
  int line_number = 0;
  double start_time = 0.0;
  double end_time = 0.0;
  wxString text;

  int Cps() const;
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

  bool dirty() const { return dirty_; }
  void set_dirty(bool v) { dirty_ = v; }

  const wxString& path() const { return path_; }
  void set_path(const wxString& p) { path_ = p; }

 private:
  std::vector<SubtitleEntry> entries_;
  wxString path_;
  bool dirty_ = false;
};

namespace srt {

bool Load(const wxString& path, Subtitles& out);

bool Save(const wxString& path, const Subtitles& in);

}  // namespace srt

#endif  // SUBSTUDIO_SRC_SUBTITLE_H_
