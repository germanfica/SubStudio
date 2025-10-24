#pragma once
#include <wx/string.h>

// Muestra en grilla: normaliza CR/LF y reemplaza '\n' por '\N'
wxString SubstudioFormatGridText(const wxString& src);

// Convierte lo tipeado en grilla a texto interno: '\N' o '\n' -> '\n' real
// (también normaliza CR/LF)
wxString SubstudioParseGridText(const wxString& displayed);
