// src/srt_io.cpp
#include "srt_io.h"

#include <wx/ffile.h>
#include <wx/textfile.h>
#include "substudio_time.h"

namespace {

    // Format as strict SRT timestamp: HH:MM:SS,mmm with rounding and carry.
    wxString FormatSrtTime(double seconds) {
        if (seconds < 0) seconds = 0;

        // Round to nearest millisecond
        long long total_ms = static_cast<long long>(std::llround(seconds * 1000.0));

        long long ms = total_ms % 1000;
        long long tot = total_ms / 1000;

        long long s = tot % 60;
        tot /= 60;

        long long m = tot % 60;
        long long h = tot / 60;

        return wxString::Format("%lld:%02lld:%02lld,%03lld", h, m, s, ms);
    }

    // Normalize any CR/LF variants to '\n' to keep internal text consistent.
    inline void NormalizeNewlines(wxString& s) {
        s.Replace("\r\n", "\n");
        s.Replace("\r", "\n");
    }

} // namespace

namespace srt {

    bool Load(const wxString& path, std::vector<SubtitleEntry>& out) {
        wxTextFile tf;
        if (!tf.Open(path)) return false;

        out.clear();

        SubtitleEntry cur;
        enum State { kExpectIndex, kExpectTimes, kExpectText } state = kExpectIndex;
        int auto_index = 0;

        const size_t N = tf.GetLineCount();
        for (size_t i = 0; i < N; ++i) {
            const wxString raw = tf.GetLine(i);
            wxString line = raw;
            wxString trimmed = raw;
            trimmed.Trim(true).Trim(false);

            // Empty line ends a block
            if (trimmed.IsEmpty()) {
                if (state == kExpectText) {
                    if (cur.line_number <= 0) cur.line_number = ++auto_index;
                    out.push_back(cur);
                    cur = SubtitleEntry{};
                    state = kExpectIndex;
                }
                continue;
            }

            if (state == kExpectIndex) {
                // If it's a number, keep it; otherwise assign sequentially later.
                long n = 0;
                if (trimmed.ToLong(&n)) cur.line_number = static_cast<int>(n);
                else cur.line_number = 0; // will be assigned on flush
                state = kExpectTimes;
                continue;
            }

            if (state == kExpectTimes) {
                const wxString arrow = " --> ";
                const int pos = trimmed.Find(arrow);
                if (pos != wxNOT_FOUND) {
                    wxString s1 = trimmed.Left(pos);
                    wxString s2 = trimmed.Mid(pos + arrow.Len());
                    s1.Trim(true).Trim(false);
                    s2.Trim(true).Trim(false);

                    // Accept both ',' and '.' as milliseconds separator
                    double t1 = 0.0, t2 = 0.0;
                    (void)SubstudioParseTime(s1, t1); // returns bool; default flags accept common forms
                    (void)SubstudioParseTime(s2, t2);

                    cur.start_time = t1;
                    cur.end_time = t2;
                }
                else {
                    cur.start_time = 0.0;
                    cur.end_time = 0.0;
                }
                state = kExpectText;
                continue;
            }

            // kExpectText: accumulate verbatim lines (preserve original spacing)
            if (!cur.text.IsEmpty()) cur.text << "\n";
            cur.text << line;
        }

        // Flush last block if file didn't end with a blank line
        if (state == kExpectText &&
            (!cur.text.IsEmpty() || cur.start_time > 0.0 || cur.end_time > 0.0))
        {
            if (cur.line_number <= 0) cur.line_number = static_cast<int>(out.size() + 1);
            out.push_back(cur);
        }

        // Normalize text newlines for internal consistency
        for (auto& e : out) NormalizeNewlines(e.text);

        return true;
    }

    bool Save(const wxString& path, const std::vector<SubtitleEntry>& entries) {
        wxFFile f(path, "w");
        if (!f.IsOpened()) return false;

        for (size_t i = 0; i < entries.size(); ++i) {
            const auto& e = entries[i];

            // Use existing line_number if set; otherwise 1-based sequence.
            const int index = (e.line_number > 0) ? e.line_number : static_cast<int>(i + 1);

            wxString text = e.text;
            NormalizeNewlines(text);

            wxString block;
            block << index << "\n";
            block << FormatSrtTime(e.start_time) << " --> " << FormatSrtTime(e.end_time) << "\n";
            block << text << "\n\n";

            if (f.Write(block) == 0) return false;
        }

        f.Close();
        return true;
    }

} // namespace srt
