#include "CpsRenderer.h"
#include <algorithm>

wxColour CpsRenderer::Blend(const wxColour& fg, const wxColour& bg, double alpha) {
    alpha = std::clamp(alpha, 0.0, 1.0);
    auto ch = [&](int f, int b){ return static_cast<unsigned char>(f * alpha + b * (1.0 - alpha)); };
    return wxColour(ch(fg.Red(),   bg.Red()),
                    ch(fg.Green(), bg.Green()),
                    ch(fg.Blue(),  bg.Blue()));
}

void CpsRenderer::Draw(wxGrid& grid,
                       wxGridCellAttr& attr,
                       wxDC& dc,
                       const wxRect& rect,
                       int row, int col,
                       bool isSelected)
{
    wxGridCellStringRenderer::Draw(grid, attr, dc, rect, row, col, isSelected);

    // Extract CPS value from the cell string
    long cps = 0;
    wxString text = grid.GetCellValue(row, col);
    if (!text.ToLong(&cps)) return;

    if (cps <= warn_) return;

    const wxColour errorColor(255, 0, 0);
    const wxColour baseBg = isSelected
        ? grid.GetSelectionBackground()
        : attr.GetBackgroundColour();

    double alpha = static_cast<double>(cps - warn_ + 1) / static_cast<double>(error_ - warn_ + 1);
    alpha = std::clamp(alpha, 0.0, 1.0);

    wxColour blended = Blend(errorColor, baseBg, alpha);

    // Fill background
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(blended));
    dc.DrawRectangle(rect);

    // Draw centered text with blended foreground (towards black a bit)
    wxColour origText = isSelected ? grid.GetSelectionForeground() : attr.GetTextColour();
    wxColour textCol  = Blend(*wxBLACK, origText, alpha);

    dc.SetTextForeground(textCol);
    dc.DrawLabel(text, rect, wxALIGN_CENTER);
}
