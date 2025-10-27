#include "grid_view.h"

#include <wx/settings.h>

GridView::GridView(wxGrid* grid) : grid_(grid) {
    grid_->EnableEditing(false);
    grid_->EnableDragColSize(false);
    grid_->EnableDragRowSize(false);
    grid_->EnableDragColMove(false);
    grid_->SetDefaultRowSize(18, true);

    if (wxWindow* wrl = grid_->GetGridRowLabelWindow())
        wrl->Hide();
}

void GridView::Populate(const std::vector<SubtitleEntry>& rows) {
    grid_->Freeze();

    const int existing = grid_->GetNumberRows();
    if (existing > 0) grid_->DeleteRows(0, existing);
    if (!rows.empty()) grid_->AppendRows(static_cast<int>(rows.size()));

    for (int r = 0; r < static_cast<int>(rows.size()); ++r) {
        const auto& e = rows[static_cast<size_t>(r)];
        grid_->SetCellValue(r, COL_LINE, wxString::Format("%d", e.line_number));
        grid_->SetCellValue(r, COL_START, e.start_time);
        grid_->SetCellValue(r, COL_END, e.end_time);
        grid_->SetCellValue(r, COL_CPS, wxString::Format("%d", e.cps));
        grid_->SetCellValue(r, COL_TEXT, e.text);
    }

    grid_->SetColSize(COL_LINE, 40);
    grid_->SetColSize(COL_START, 110);
    grid_->SetColSize(COL_END, 110);
    grid_->SetColSize(COL_CPS, 90);

    grid_->Thaw();
    grid_->Refresh();
    grid_->Update();
}

void GridView::UpdateRowText(int row, const wxString& text) {
    if (row < 0) return;
    if (row >= grid_->GetNumberRows()) return;
    grid_->SetCellValue(row, COL_TEXT, text);
}

int GridView::VisibleFixedWidthSum() const {
    int sum = 0;
    for (int c : {COL_LINE, COL_START, COL_END, COL_CPS}) {
        if (grid_->IsColShown(c)) sum += grid_->GetColSize(c);
    }
    return sum;
}

void GridView::AdjustTextColumnWidth(wxWindow* parent_or_frame) {
    if (!grid_->IsColShown(COL_TEXT)) return;

    parent_or_frame->Layout();

    wxWindow* parent = grid_->GetParent();
    const int parent_w = parent ? parent->GetClientSize().GetWidth()
        : grid_->GetClientSize().GetWidth();
    const int grid_x = grid_->GetPosition().x;
    const int row_label_w = grid_->GetRowLabelSize();

    int vscroll_w = 0;
    {
        int total_h = grid_->GetColLabelSize();
        const int nrows = grid_->GetNumberRows();
        for (int r = 0; r < nrows; ++r) total_h += grid_->GetRowSize(r);
        if (total_h > grid_->GetClientSize().GetHeight()) {
            vscroll_w = wxSystemSettings::GetMetric(wxSYS_VSCROLL_X);
        }
    }

    const int fixed = VisibleFixedWidthSum();
    int available = parent_w - grid_x - row_label_w - vscroll_w - fixed;
    if (available < 120) available = 120;

    grid_->SetColSize(COL_TEXT, available);
    parent_or_frame->Layout();
    grid_->Refresh();
    grid_->Update();
}
