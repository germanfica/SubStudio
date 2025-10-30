#include "cps.h"
#include <cmath>
#include <cwctype>
#include <cstddef>

namespace {

    wxString ParseToVisible(const wxString& src) {
        wxString s = src;
        s.Replace("\r\n", "\n");
        s.Replace("\r", "\n");
        s.Replace("\\N", "\n");
        s.Replace("\\n", "\n");
        s.Replace("\\h", wxS(" "));

        wxString tmp; tmp.reserve(s.length());
        bool in_tag = false;
        bool in_brace = false;

        for (size_t i = 0; i < static_cast<size_t>(s.length()); ++i) {
            wxUniChar ch = s[i];
            const wchar_t c = static_cast<wchar_t>(ch.GetValue());

            if (!in_tag && !in_brace) {
                if (c == '<') { in_tag = true; continue; }
                if (c == '{') { in_brace = true; continue; }
                if (c == '\n') tmp += ' ';
                else            tmp += ch;
            }
            else {
                if (in_tag && c == '>')        in_tag = false;
                else if (in_brace && c == '}') in_brace = false;
            }
        }

        wxString out; out.reserve(tmp.length());
        bool prev_space = false;
        for (wxUniChar ch : tmp) {
            const wint_t wc = static_cast<wint_t>(ch.GetValue());
            const bool is_space = std::iswspace(wc) != 0;
            if (is_space) {
                if (!prev_space) { out += ' '; prev_space = true; }
            }
            else {
                out += ch; prev_space = false;
            }
        }
        out.Trim(true).Trim(false);
        return out;
    }

    static inline bool IsIgnoredForCps(wint_t wc) {
        if (std::iswpunct(wc)) return true;
        if (std::iswspace(wc)) return true;

        switch (wc) {
        case 0x00A1: /* ¡ */
        case 0x00BF: /* ¿ */
        case 0x00AB: /* « */
        case 0x00BB: /* » */
        case 0x2026: /* … */
        case 0x00B7: /* · */
        case 0x2014: /* — */
        case 0x2013: /* – */
        case 0x2011: /* - non-breaking hyphen */
        case 0x00A7: /* § */
        case 0x2018: /* ‘ */
        case 0x2019: /* ’ */
        case 0x201C: /* “ */
        case 0x201D: /* ” */
            return true;
        default: break;
        }
        return false;
    }

    static int CountVisibleNoPunct(const wxString& visible) {
        int n = 0;
        for (wxUniChar ch : visible) {
            const wint_t wc = static_cast<wint_t>(ch.GetValue());
            if (IsIgnoredForCps(wc)) continue;
            ++n;
        }
        return n;
    }
}  // namespace

int ComputeCps(const wxString& raw_text, double start_time, double end_time) {
    if (!(end_time > start_time)) {
        return 0;
    }

    const int duration_ms = static_cast<int>(std::lround((end_time - start_time) * 1000.0));
    if (duration_ms <= 0) {
        return 0;
    }
    if (duration_ms <= 100) {
        return -1;
    }

    const wxString visible = ParseToVisible(raw_text);
    const int characters = CountVisibleNoPunct(visible);
    return (characters * 1000) / duration_ms;
}
