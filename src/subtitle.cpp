// src/subtitle.cpp
#include "subtitle.h"
#include "substudio_time.h"

#include <wx/textfile.h>
#include <wx/ffile.h>

#include <wx/regex.h>
#include <cmath>

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
    // Si queres marcar sucio automáticamente, descomentá:
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

int ComputeCpsVisible(const wxString& src, double start_sec, double end_sec) {
    // duration en ms
    const double dur_s = end_sec - start_sec;
    const int duration_ms = static_cast<int>(std::llround(dur_s * 1000.0));
    if (duration_ms <= 100) // demasiado corta para calcular confiable (100ms)
        return 0;

    // Normalizar saltos
    wxString s = src;
    s.Replace("\r\n", "\n");
    s.Replace("\r", "\n");

    // Remover bloques {...} y <...>, manejar escapes \N / \n
    wxString visible;
    visible.reserve(s.length());
    bool inBrace = false;
    bool inTag = false;

    for (size_t i = 0; i < s.length(); ++i) {
        wxUniChar ch = s[i];

        if (!inBrace && ch == '{') { inBrace = true; continue; }
        if (inBrace) { if (ch == '}') inBrace = false; continue; }

        if (!inTag && ch == '<') { inTag = true; continue; }
        if (inTag) { if (ch == '>') inTag = false; continue; }

        // \N or \n (ASS/escaped newline) => treat as newline (not counted)
        if (ch == '\\' && i + 1 < s.length()) {
            wxUniChar nxt = s[i + 1];
            if (nxt == 'N' || nxt == 'n') { ++i; visible += '\n'; continue; }
        }

        if (ch == '\r' || ch == '\n') { visible += '\n'; continue; }

        // todo: si queres ignorar espacios/puntuacion, hacerlo aquí
        visible += ch;
    }

    // Contar solo caracteres visibles (no contamos '\n')
    int chars = 0;
    for (wxUniChar ch : visible) {
        if (ch == '\n') continue;
        ++chars;
    }
    if (chars == 0) return 0;

    // CPS = chars / seconds -> usando ms da chars*1000 / duration_ms (int)
    const int cps = static_cast<int>(std::round(static_cast<double>(chars) * 1000.0 / duration_ms));

    return cps;
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

            // ExpectText: concatenamos manualmente
            if (!cur.text.IsEmpty()) cur.text += "\n";
            cur.text += line;
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
            // concatenamos en lugar de usar operator<<
            block += wxString::Format("%d\n", idx);
            block += SubstudioFormatTime(e.start_time);
            block += " --> ";
            block += SubstudioFormatTime(e.end_time);
            block += "\n";
            block += e.text;
            block += "\n\n";

            if (f.Write(block) == 0) return false;
        }

        f.Close();
        return true;
    }

} // namespace srt
