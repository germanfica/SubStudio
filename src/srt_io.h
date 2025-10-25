#pragma once

#ifndef SUBSTUDIO_SRT_IO_H_
#define SUBSTUDIO_SRT_IO_H_

#include <vector>
#include "subtitle.h"
#include <wx/string.h>

namespace srt {

	// Lee un archivo SRT a 'out'. Devuelve true en éxito.
	bool Load(const wxString& path, std::vector<SubtitleEntry>& out);

	// Guarda 'entries' como SRT. Devuelve true en éxito.
	bool Save(const wxString& path, const std::vector<SubtitleEntry>& entries);

}  // namespace srt

#endif  // SUBSTUDIO_SRT_IO_H_
