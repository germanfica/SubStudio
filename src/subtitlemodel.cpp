#include "SubtitleModel.h"
#include <wx/filename.h>
#include <wx/textfile.h>
#include <wx/regex.h>
#include <wx/ffile.h>      // <-- necesario para wxFFile
#include <wx/filefn.h>     // funciones auxiliares de archivos
#include <cmath>
#include <vector>

void SubtitleModel::clear() {
    entries_.clear();
}

static wxString TrimBOM(const wxString& s) {
    if (!s.empty() && s[0] == 0xFEFF) {
        return s.Mid(1);
    }
    return s;
}

bool SubtitleModel::loadSrt(const wxString& filePath) {
    wxTextFile tf;
    if (!tf.Open(filePath)) return false;

    std::vector<SubtitleEntry> tmp;
    enum State { ExpectIndex, ExpectTime, ExpectText } state = ExpectIndex;
    SubtitleEntry cur;
    int autoIndex = 0;

    wxRegEx timeRe("(\\d{2}:\\d{2}:\\d{2}[\\.,]\\d{1,3})\\s*-->\\s*(\\d{2}:\\d{2}:\\d{2}[\\.,]\\d{1,3})");

    bool first = true;
    for (wxString line = tf.GetFirstLine(); !tf.Eof(); line = tf.GetNextLine()) {
        if (first) { line = TrimBOM(line); first = false; }
        wxString trimmed = line;
        trimmed.Trim(true).Trim(false);

        if (trimmed.empty()) {
            if (state == ExpectText) {
                cur.cps = computeCPS(cur);
                tmp.push_back(cur);
                cur = SubtitleEntry();
                state = ExpectIndex;
            } else {
                state = ExpectIndex;
            }
            continue;
        }

        if (state == ExpectIndex) {
            long n;
            if (trimmed.ToLong(&n)) {
                cur.lineNumber = static_cast<int>(n);
            } else {
                cur.lineNumber = ++autoIndex;
            }
            state = ExpectTime;
        } else if (state == ExpectTime) {
            if (timeRe.Matches(trimmed)) {
                cur.startTime = timeRe.GetMatch(trimmed, 1);
                cur.endTime   = timeRe.GetMatch(trimmed, 2);
                double s1 = parseTimeToSeconds(cur.startTime);
                double s2 = parseTimeToSeconds(cur.endTime);
                cur.durationSeconds = std::max(0.0, s2 - s1);
            }
            state = ExpectText;
        } else if (state == ExpectText) {
            if (!cur.text.empty()) cur.text << "\n";
            cur.text << line; // mantener formato original
        }
    }

    // último bloque si el archivo no termina con línea vacía
    if (state == ExpectText && !cur.text.empty()) {
        cur.cps = computeCPS(cur);
        tmp.push_back(cur);
    }

    entries_ = std::move(tmp);
    return true;
}

bool SubtitleModel::saveSrt(const wxString& filePath) const {
    wxFFile file(filePath, "w");
    if (!file.IsOpened()) return false;

    for (size_t i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        int idx = e.lineNumber > 0 ? e.lineNumber : static_cast<int>(i + 1);
        wxString block;
        block << idx << "\n";
        block << e.startTime << " --> " << e.endTime << "\n";

        wxArrayString lines = wxSplit(e.text, '\n');
        for (const auto& ln : lines) {
            block << ln << "\n";
        }
        block << "\n";
        file.Write(block);
    }
    file.Close();
    return true;
}

void SubtitleModel::setTextAt(int row, const wxString& text) {
    if (row < 0 || row >= rowCount()) return;
    entries_[row].text = text;
    entries_[row].cps  = computeCPS(entries_[row]);
}

double SubtitleModel::parseTimeToSeconds(const wxString& timeStr) {
    wxString s = timeStr;
    s.Replace(",", ".");
    wxArrayString parts = wxSplit(s, ':');
    if (parts.size() != 3) return 0.0;
    double hh = std::stod(parts[0].ToStdString());
    double mm = std::stod(parts[1].ToStdString());
    double ss = std::stod(parts[2].ToStdString());
    return hh * 3600.0 + mm * 60.0 + ss;
}

int SubtitleModel::computeCPS(const SubtitleEntry& e) {
    const int chars = static_cast<int>(e.text.length());
    const double dur = e.durationSeconds;
    if (dur <= 0.001) return 0;
    return static_cast<int>(std::round(chars / dur));
}
