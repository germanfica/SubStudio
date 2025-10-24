#include "substudio_time.h"
#include <wx/regex.h>
#include <algorithm>
#include <cmath>

// Helpers internos con linkage interno.
// Preferimos no exponer símbolos innecesarios fuera de este .cpp.
namespace {
    inline void clamp_mm_ss(long& m, long& s) {
        if (m < 0) m = 0;
        if (s < 0) s = 0;
        if (m >= 60) m = 59;
        if (s >= 60) s = 59;
    }

    inline void normalize_cs_digits(wxString& f2) {
        // Para centesimas: 1 dig -> *10, >2 -> cortar a 2
        if (f2.length() == 1) f2 += "0";
        else if (f2.length() > 2) f2 = f2.Left(2);
    }

    inline void normalize_ms_digits(wxString& f3) {
        // Para milisegundos: >3 -> cortar a 3
        while (f3.length() > 3) f3.RemoveLast();
        if (f3.IsEmpty()) f3 = "0";
    }

    // Compila una sola vez cada regex (permitido y común: static local).
    // Google C++ Style permite inicialización dinámica de estáticos locales.
    // (Pattern: "dynamic initialization of static local variables is allowed".)
    // Ver guía. :contentReference[oaicite:2]{index=2} :contentReference[oaicite:3]{index=3}

    bool TryParseHHMMSSFF(const wxString& s, double& out) {
        static wxRegEx rx("^(\\d+):(\\d{1,2}):(\\d{1,2}):(\\d{1,2})$");
        if (!rx.Matches(s)) return false;
        long h = 0, m = 0, sec = 0, ff = 0;
        rx.GetMatch(s, 1).ToLong(&h);
        rx.GetMatch(s, 2).ToLong(&m);
        rx.GetMatch(s, 3).ToLong(&sec);
        rx.GetMatch(s, 4).ToLong(&ff);
        clamp_mm_ss(m, sec);
        if (ff < 0) ff = 0; else if (ff > 99) ff = 99;
        out = h * 3600.0 + m * 60.0 + sec + (ff / 100.0);
        return true;
    }

    bool TryParseHHMMSSmmm(const wxString& s, double& out) {
        static wxRegEx rx("^(\\d+):(\\d{1,2}):(\\d{1,2})(?:[\\.,](\\d{1,3}))?$");
        if (!rx.Matches(s)) return false;
        long h = 0, m = 0, sec = 0, ms = 0;
        rx.GetMatch(s, 1).ToLong(&h);
        rx.GetMatch(s, 2).ToLong(&m);
        rx.GetMatch(s, 3).ToLong(&sec);
        if (rx.GetMatchCount() >= 5 && !rx.GetMatch(s, 4).IsEmpty()) {
            wxString msStr = rx.GetMatch(s, 4);
            normalize_ms_digits(msStr);
            msStr.ToLong(&ms);
        }
        clamp_mm_ss(m, sec);
        out = h * 3600.0 + m * 60.0 + sec + (ms / 1000.0);
        return true;
    }

    bool TryParseMMSSff(const wxString& s, double& out) {
        static wxRegEx rx("^(\\d+):(\\d{1,2})(?:[\\.,](\\d{1,3}))?$");
        if (!rx.Matches(s)) return false;
        long m = 0, sec = 0, frac = 0;
        rx.GetMatch(s, 1).ToLong(&m);
        rx.GetMatch(s, 2).ToLong(&sec);

        double add = 0.0;
        if (rx.GetMatchCount() >= 4 && !rx.GetMatch(s, 3).IsEmpty()) {
            wxString f = rx.GetMatch(s, 3);
            normalize_cs_digits(f);
            f.ToLong(&frac);
            add = frac / 100.0;
        }
        clamp_mm_ss(m, sec);
        out = m * 60.0 + sec + add;
        return true;
    }

    bool TryParseSSff(const wxString& s, double& out) {
        static wxRegEx rx("^(\\d+)(?:[\\.,](\\d{1,3}))?$");
        if (!rx.Matches(s)) return false;
        long sec = 0, frac = 0;
        rx.GetMatch(s, 1).ToLong(&sec);

        double add = 0.0;
        if (rx.GetMatchCount() >= 3 && !rx.GetMatch(s, 2).IsEmpty()) {
            wxString f = rx.GetMatch(s, 2);
            normalize_cs_digits(f);
            f.ToLong(&frac);
            add = frac / 100.0;
        }
        if (sec < 0) sec = 0;
        out = sec + add;
        return true;
    }
} // namespace

// -------------------- API pública --------------------

wxString SubstudioFormatTime(double seconds, TimeFormat fmt) {
    if (seconds < 0) seconds = 0;
    const int total_cs = static_cast<int>(seconds * 100 + 0.5);  // centesimas redondeadas
    const int cs = total_cs % 100;
    const int total_s = total_cs / 100;
    const int s = total_s % 60;
    const int total_m = total_s / 60;
    const int m = total_m % 60;
    const int h = total_m / 60;

    switch (fmt) {
    case TimeFormat::TF_FMT_HH_MM_SS_CS:
        return wxString::Format("%d:%02d:%02d:%02d", h, m, s, cs);
    case TimeFormat::TF_FMT_HH_MM_SS_MS: {
        // milisegundos desde centesimas redondeadas
        int ms = static_cast<int>(std::lround((seconds - std::floor(seconds)) * 1000.0));
        if (ms == 1000) { // por si justo redondea
            ms = 0;
        }
        return wxString::Format("%d:%02d:%02d.%03d", h, m, s, ms);
    }
    case TimeFormat::TF_FMT_MM_SS_CS:
        return wxString::Format("%d:%02d:%02d", h * 60 + m, s, cs);
    case TimeFormat::TF_FMT_SS_CS: {
        const int total_sec = static_cast<int>(std::floor(seconds));
        return wxString::Format("%d.%02d", total_sec, cs);
    }
    default:
        return wxString::Format("%d:%02d:%02d:%02d", h, m, s, cs);
    }
}

bool SubstudioParseTime(const wxString& in, double& outSeconds, TimeParseFlags flags) {
    wxString s = in;
    s.Trim(true).Trim(false);
    if (s.empty()) { outSeconds = 0.0; return true; }

    const bool strict = HasFlag(flags, TimeParseFlags::TPF_STRICT);

    // Orden de prueba: formatos con mas informacion primero.
    if (HasFlag(flags, TimeParseFlags::TPF_HH_MM_SS_CS) && TryParseHHMMSSFF(s, outSeconds)) return true;
    if (HasFlag(flags, TimeParseFlags::TPF_HH_MM_SS_MS) && TryParseHHMMSSmmm(s, outSeconds)) return true;
    if (HasFlag(flags, TimeParseFlags::TPF_MM_SS_CS) && TryParseMMSSff(s, outSeconds))    return true;
    if (HasFlag(flags, TimeParseFlags::TPF_SS_CS) && TryParseSSff(s, outSeconds))      return true;

    if (strict) return false;

    // Modo no estricto: intentamos una ruta de compat extra por si el usuario
    // escribe algo como "M:SS" o "SS" (ya cubierto por arriba sin fraccion),
    // o separador decimal raro. Si llegamos aca, no hay mucho mas que probar.
    return false;
}
