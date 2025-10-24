#pragma once

#include <wx/string.h>

// -------------------- Flags de parseo --------------------
// Usamos enum class + operadores bit a bit seguros.
enum class TimeParseFlags : int {
    None = 0,
    TPF_HH_MM_SS_CS = 1 << 0,  // "H:MM:SS:FF" (FF = centesimas 0..99)
    TPF_HH_MM_SS_MS = 1 << 1,  // "H:MM:SS.mmm" o "H:MM:SS,mmm" (ms 0..999)
    TPF_MM_SS_CS = 1 << 2,  // "M:SS.ff" o "M:SS" (ff opcional -> centesimas)
    TPF_SS_CS = 1 << 3,  // "SS.ff" o "SS"
    TPF_STRICT = 1 << 4   // si se fija, solo acepta formatos habilitados
};

constexpr TimeParseFlags operator|(TimeParseFlags a, TimeParseFlags b) {
    return static_cast<TimeParseFlags>(static_cast<int>(a) | static_cast<int>(b));
}
constexpr TimeParseFlags operator&(TimeParseFlags a, TimeParseFlags b) {
    return static_cast<TimeParseFlags>(static_cast<int>(a) & static_cast<int>(b));
}
constexpr bool HasFlag(TimeParseFlags v, TimeParseFlags f) {
    return (static_cast<int>(v) & static_cast<int>(f)) != 0;
}

// Nota: incluyo HH:MM:SS:FF en el default para mantener simetría con el
// formato de salida por defecto y con tu comportamiento actual.
inline constexpr TimeParseFlags kDefaultParseFlags =
TimeParseFlags::TPF_HH_MM_SS_MS |
TimeParseFlags::TPF_HH_MM_SS_CS |
TimeParseFlags::TPF_MM_SS_CS |
TimeParseFlags::TPF_SS_CS;

// -------------------- Formatos de salida --------------------
enum class TimeFormat : int {
    TF_FMT_HH_MM_SS_CS, // "H:MM:SS:FF" (centesimas)
    TF_FMT_HH_MM_SS_MS, // "H:MM:SS.mmm" (milisegundos)
    TF_FMT_MM_SS_CS,    // "M:SS:FF"
    TF_FMT_SS_CS        // "SS.FF"
};

// -------------------- API pública --------------------
// Compat: wrappers con los nombres que ya usabas en tu grid.
wxString SubstudioFormatTime(double seconds,
    TimeFormat fmt = TimeFormat::TF_FMT_HH_MM_SS_CS);

bool SubstudioParseTime(const wxString& in,
    double& outSeconds,
    TimeParseFlags flags = kDefaultParseFlags);
