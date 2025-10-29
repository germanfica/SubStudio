#ifndef SUBSTUDIO_SRC_SRT_IO_H_
#define SUBSTUDIO_SRC_SRT_IO_H_

#include <vector>

#include <wx/string.h>

#include "subtitle.h"

namespace srt {

bool Load(const wxString& path, std::vector<SubtitleEntry>& out);

bool Save(const wxString& path, const std::vector<SubtitleEntry>& entries);

}  // namespace srt

#endif  // SUBSTUDIO_SRC_SRT_IO_H_
