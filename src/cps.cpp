#include "cps.h"
#include <cmath>
#include <cwctype>

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
                }
                else {
                    without_tags += ch;
                }
            }
            else {
                if (in_tag && value == '>') {
                    in_tag = false;
                }
                else if (in_brace && value == '}') {
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
            }
            else {
                collapsed += ch;
                previous_space = false;
            }
        }

        collapsed.Trim(true).Trim(false);
        return static_cast<int>(collapsed.length());
    }

}  // namespace


int ComputeCps(const wxString& raw_text, double start_time, double end_time) {
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

    const int characters = VisibleLenForCps(raw_text);
    return (characters * 1000) / duration_ms;
}
