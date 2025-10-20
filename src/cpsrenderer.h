#pragma once
#include <wx/wx.h>
#include <wx/grid.h>

// Custom renderer for CPS column: shades background towards red as CPS grows past warn threshold.
class CpsRenderer : public wxGridCellStringRenderer {
public:
    CpsRenderer(int warnCps, int errorCps)
      : warn_(warnCps), error_(std::max(errorCps, warnCps)) {}

    void Draw(wxGrid& grid,
              wxGridCellAttr& attr,
              wxDC& dc,
              const wxRect& rect,
              int row, int col,
              bool isSelected) override;

private:
    int warn_;
    int error_;

    static wxColour Blend(const wxColour& fg, const wxColour& bg, double alpha);
};
