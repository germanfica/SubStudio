#include "substudio_textfmt.h"

namespace {
    inline void NormalizeNewlines(wxString& s) {
        s.Replace("\r\n", "\n");
        s.Replace("\r", "\n");
    }
}

wxString SubstudioFormatGridText(const wxString& src) {
    wxString out = src;
    NormalizeNewlines(out);
    // saltos reales -> '\N'
    out.Replace("\n", "\\N");
    return out;
}

wxString SubstudioParseGridText(const wxString& displayed) {
    wxString s = displayed;
    NormalizeNewlines(s);
    // Aceptar '\N' y '\n' en minúscula por comodidad del usuario
    s.Replace("\\N", "\n");
    s.Replace("\\n", "\n");
    return s;
}
