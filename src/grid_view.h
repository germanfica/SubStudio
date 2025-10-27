#pragma once

#ifndef SUBSTUDIO_UI_GRID_VIEW_H_
#define SUBSTUDIO_UI_GRID_VIEW_H_

#include <vector>
#include <wx/grid.h>
#include <wx/window.h>
#include "subtitle.h"

class GridView {
public:
    enum Columns {
        COL_LINE = 0,
        COL_START = 1,
        COL_END = 2,
        COL_CPS = 3,
        COL_TEXT = 4
    };

    explicit GridView(wxGrid* grid);

    void Populate(const std::vector<SubtitleEntry>& rows);
    void UpdateRowText(int row, const wxString& text);
    void AdjustTextColumnWidth(wxWindow* parent_or_frame);

    wxGrid* grid() const { return grid_; }

private:
    wxGrid* grid_ = nullptr;

    int VisibleFixedWidthSum() const;
};

#endif  // SUBSTUDIO_UI_GRID_VIEW_H_
