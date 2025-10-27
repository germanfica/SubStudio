// src/subtitle.cpp
#include "subtitle.h"
#include "substudio_time.h"

#include <wx/textfile.h>
#include <wx/ffile.h>

void Subtitles::Clear() {
    entries_.clear();
    path_.clear();
    dirty_ = false;
}

void Subtitles::EnsureRow(size_t row) {
    if (row < entries_.size()) return;
    size_t old = entries_.size();
    entries_.resize(row + 1);
    for (size_t i = old; i < entries_.size(); ++i) {
        entries_[i].line_number = static_cast<int>(i + 1);
        entries_[i].start_time = 0.0;
        entries_[i].end_time = 0.0;
        entries_[i].text.clear();
    }
}

void Subtitles::SetRowText(size_t row, const wxString& text) {
    EnsureRow(row);
    entries_[row].text = text;
    // Si querés marcar sucio automáticamente, descomentá:
    // dirty_ = true;
}

void Subtitles::SetRowTimes(size_t row, double start_time, double end_time) {
    EnsureRow(row);
    entries_[row].start_time = start_time;
    entries_[row].end_time = end_time;
    // dirty_ = true;
}

void Subtitles::ResequenceLineNumbers() {
    for (size_t i = 0; i < entries_.size(); ++i)
        entries_[i].line_number = static_cast<int>(i + 1);
}

// --- Overloads simples de SRT I/O trabajando con Subtitles.
//     Podés seguir usando srt_io.cpp con std::vector<SubtitleEntry> según convenga.
namespace srt {

    bool Load(const wxString& path, Subtitles& out) {
        wxTextFile tf;
        if (!tf.Open(path)) return false;

        out.Clear();
        SubtitleEntry cur;
        enum State { ExpectIndex, ExpectTime, ExpectText } state = ExpectIndex;
        int autoIndex = 0;

        const size_t N = tf.GetLineCount();
        for (size_t i = 0; i < N; ++i) {
            wxString line = tf.GetLine(i);
            wxString trimmed = line;
            trimmed.Trim(true).Trim(false);

            if (trimmed.IsEmpty()) {
                if (state == ExpectText) {
                    if (cur.line_number <= 0) cur.line_number = ++autoIndex;
                    out.entries().push_back(cur);
                    cur = SubtitleEntry{};
                    state = ExpectIndex;
                }
                continue;
            }

            if (state == ExpectIndex) {
                long n = 0;
                if (trimmed.ToLong(&n)) cur.line_number = static_cast<int>(n);
                else cur.line_number = 0; // se asignará al flush si queda 0
                state = ExpectTime;
                continue;
            }

            if (state == ExpectTime) {
                const wxString arrow = " --> ";
                const int pos = trimmed.Find(arrow);
                if (pos != wxNOT_FOUND) {
                    wxString s1 = trimmed.Left(pos);
                    wxString s2 = trimmed.Mid(pos + arrow.Len());
                    s1.Trim(true).Trim(false);
                    s2.Trim(true).Trim(false);

                    cur.start_time = 0.0;
                    cur.end_time = 0.0;
                    (void)SubstudioParseTime(s1, cur.start_time);
                    (void)SubstudioParseTime(s2, cur.end_time);
                }
                state = ExpectText;
                continue;
            }

            // ExpectText
            if (!cur.text.IsEmpty()) cur.text << "\n";
            cur.text << line;
        }

        // Flush final si no terminó en línea vacía
        if (state == ExpectText &&
            (!cur.text.IsEmpty() || cur.start_time > 0.0 || cur.end_time > 0.0)) {
            if (cur.line_number <= 0) cur.line_number = static_cast<int>(out.size() + 1);
            out.entries().push_back(cur);
        }

        return true;
    }

    bool Save(const wxString& path, const Subtitles& in) {
        wxFFile f(path, "w");
        if (!f.IsOpened()) return false;

        for (size_t i = 0; i < in.entries().size(); ++i) {
            const auto& e = in.entries()[i];

            const int idx = (e.line_number > 0) ? e.line_number : static_cast<int>(i + 1);

            wxString block;
            block << idx << "\n";
            block << SubstudioFormatTime(e.start_time) << " --> " << SubstudioFormatTime(e.end_time) << "\n";
            block << e.text << "\n\n";

            if (f.Write(block) == 0) return false;
        }

        f.Close();
        return true;
    }

} // namespace srt
