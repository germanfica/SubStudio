#include "srt_io.h"

#include <wx/ffile.h>
#include <wx/textfile.h>

namespace srt {

    bool Load(const wxString& path, std::vector<SubtitleEntry>& out) {
        wxTextFile tf;
        if (!tf.Open(path)) return false;

        out.clear();

        SubtitleEntry cur;
        enum State { kExpectIndex, kExpectTimes, kExpectText } state = kExpectIndex;
        int line_no = 0;

        const size_t N = tf.GetLineCount();
        for (size_t i = 0; i < N; ++i) {
            wxString line = tf.GetLine(i);
            line = line.Trim(false).Trim(true);

            if (line.IsEmpty()) {
                if (state == kExpectText) {
                    ++line_no;
                    cur.line_number = line_no;
                    out.push_back(cur);
                    cur = SubtitleEntry{};
                    state = kExpectIndex;
                }
                continue;
            }

            if (state == kExpectIndex) {
                state = kExpectTimes;
                continue;
            }
            else if (state == kExpectTimes) {
                const wxString arrow = " --> ";
                auto pos = line.find(arrow);
                if (pos != wxString::npos) {
                    cur.start_time = line.substr(0, pos).Trim(true).Trim(false);
                    cur.end_time = line.substr(pos + arrow.length()).Trim(true).Trim(false);
                }
                else {
                    cur.start_time.clear();
                    cur.end_time.clear();
                }
                state = kExpectText;
            }
            else {
                if (!cur.text.IsEmpty()) cur.text += "\n";
                cur.text += line;
            }
        }

        if (state == kExpectText && (!cur.text.IsEmpty() || !cur.start_time.IsEmpty() || !cur.end_time.IsEmpty())) {
            ++line_no;
            cur.line_number = line_no;
            out.push_back(cur);
        }

        return true;
    }

    bool Save(const wxString& path, const std::vector<SubtitleEntry>& entries) {
        wxFFile f(path, "w");
        if (!f.IsOpened()) return false;

        for (size_t i = 0; i < entries.size(); ++i) {
            const SubtitleEntry& e = entries[i];
            wxString block;
            block << (i + 1) << "\n";
            block << e.start_time << " --> " << e.end_time << "\n";
            block << e.text << "\n\n";
            if (f.Write(block) == 0) return false;
        }
        f.Close();
        return true;
    }

}  // namespace srt
