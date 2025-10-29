#ifndef SUBSTUDIO_SRC_SUBSTUDIO_TIME_H_
#define SUBSTUDIO_SRC_SUBSTUDIO_TIME_H_

#include <wx/string.h>

enum class TimeParseFlags : int {
  None = 0,
  TPF_HH_MM_SS_CS = 1 << 0,
  TPF_HH_MM_SS_MS = 1 << 1,
  TPF_MM_SS_CS = 1 << 2,
  TPF_SS_CS = 1 << 3,
  TPF_STRICT = 1 << 4,
};

constexpr TimeParseFlags operator|(TimeParseFlags lhs, TimeParseFlags rhs) {
  return static_cast<TimeParseFlags>(static_cast<int>(lhs) |
                                     static_cast<int>(rhs));
}

constexpr TimeParseFlags operator&(TimeParseFlags lhs, TimeParseFlags rhs) {
  return static_cast<TimeParseFlags>(static_cast<int>(lhs) &
                                     static_cast<int>(rhs));
}

constexpr bool HasFlag(TimeParseFlags value, TimeParseFlags flag) {
  return (static_cast<int>(value) & static_cast<int>(flag)) != 0;
}

inline constexpr TimeParseFlags kDefaultParseFlags =
    TimeParseFlags::TPF_HH_MM_SS_MS | TimeParseFlags::TPF_HH_MM_SS_CS |
    TimeParseFlags::TPF_MM_SS_CS | TimeParseFlags::TPF_SS_CS;

enum class TimeFormat : int {
  TF_FMT_HH_MM_SS_CS,
  TF_FMT_HH_MM_SS_MS,
  TF_FMT_MM_SS_CS,
  TF_FMT_SS_CS,
};

wxString SubstudioFormatTime(
    double seconds, TimeFormat fmt = TimeFormat::TF_FMT_HH_MM_SS_CS);

bool SubstudioParseTime(const wxString& in, double& out_seconds,
                        TimeParseFlags flags = kDefaultParseFlags);

#endif  // SUBSTUDIO_SRC_SUBSTUDIO_TIME_H_
