// src/substudiogrid.cpp
#include "substudiogrid.h"
#include "substudio_time.h"
#include "substudio_textfmt.h"
#include <wx/regex.h>

// Config centralizados para UI
namespace {
    constexpr TimeFormat    kUiTimeFmt = TimeFormat::TF_FMT_HH_MM_SS_CS; // salida 'H:MM:SS:FF'
    constexpr TimeParseFlags kUiParse = kDefaultParseFlags;
}

// ---------- SubstudioAttrProvider

SubstudioAttrProvider::SubstudioAttrProvider() : wxGridCellAttrProvider() {}

wxGridCellAttr* SubstudioAttrProvider::GetAttr(int row, int col, wxGridCellAttr::wxAttrKind kind) const {
    wxGridCellAttr* attr = wxGridCellAttrProvider::GetAttr(row, col, kind);
    if (!attr) attr = new wxGridCellAttr();

    // Zebra striping
    if (row % 2 == 0) attr->SetBackgroundColour(wxColour(245, 248, 250));
    else              attr->SetBackgroundColour(*wxWHITE);

    // Read-only por columna
    if (col == COL_NUM || col == COL_CPS) attr->SetReadOnly(true);

    // Alineaciones
    if (col == COL_NUM || col == COL_CPS)       attr->SetAlignment(wxALIGN_CENTRE, wxALIGN_CENTRE);
    else if (col == COL_START || col == COL_END) attr->SetAlignment(wxALIGN_CENTRE, wxALIGN_CENTRE);
    else                                         attr->SetAlignment(wxALIGN_LEFT, wxALIGN_CENTRE);

    return attr;
}

// ---------- SubstudioGridTable

SubstudioGridTable::SubstudioGridTable() : wxGridTableBase() {
    // Arranca vacio
}

void SubstudioGridTable::ClearRow(SubstudioRow& r) {
    r.start = 0.0;
    r.end = 0.0;
    r.cps = 0;
    r.text.clear();
}

wxString SubstudioGridTable::GetValue(int row, int col) {
    const auto& r = m_rows.at(row);
    switch (col) {
    case COL_NUM:   return wxString::Format("%d", r.index);
    case COL_START: return SubstudioFormatTime(r.start, kUiTimeFmt);
    case COL_END:   return SubstudioFormatTime(r.end, kUiTimeFmt);
    case COL_CPS:   return wxString::Format("%d", r.cps);
    case COL_TEXT:  return SubstudioFormatGridText(r.text);
    default:        return wxEmptyString;
    }
}

void SubstudioGridTable::SetValue(int row, int col, const wxString& value) {
    auto& r = m_rows.at(row);
    switch (col) {
    case COL_START: {
        double t;
        if (SubstudioParseTime(value, t, kUiParse)) r.start = t; // blanco => 0
        break;
    }
    case COL_END: {
        double t;
        if (SubstudioParseTime(value, t, kUiParse)) r.end = t;   // blanco => 0
        break;
    }
    case COL_TEXT:
        r.text = value; // blanco => ""
        break;
    default:
        break;
    }
    RecalcRow(row);

    if (GetView()) {
        wxGrid* g = GetView();
        g->RefreshBlock(row, 0, row, COL_COUNT - 1);
    }
}

wxString SubstudioGridTable::GetColLabelValue(int col) {
    switch (col) {
    case COL_NUM:   return "#";
    case COL_START: return "Start";
    case COL_END:   return "End";
    case COL_CPS:   return "CPS";
    case COL_TEXT:  return "Text";
    default:        return wxEmptyString;
    }
}

wxString SubstudioGridTable::GetTypeName(int, int col) {
    switch (col) {
    case COL_NUM:
    case COL_CPS:   return wxGRID_VALUE_NUMBER;
    case COL_START:
    case COL_END:   return wxGRID_VALUE_STRING;
    case COL_TEXT:  return wxGRID_VALUE_STRING;
    default:        return wxGRID_VALUE_STRING;
    }
}

bool SubstudioGridTable::CanGetValueAs(int, int col, const wxString& typeName) {
    if ((col == COL_NUM || col == COL_CPS) && typeName == wxGRID_VALUE_NUMBER) return true;
    if ((col == COL_START || col == COL_END) && typeName == wxGRID_VALUE_FLOAT) return true;
    if ((col == COL_TEXT) && typeName == wxGRID_VALUE_STRING) return true;
    return false;
}

bool SubstudioGridTable::CanSetValueAs(int, int col, const wxString& typeName) {
    if ((col == COL_START || col == COL_END) && typeName == wxGRID_VALUE_FLOAT) return true;
    if ((col == COL_TEXT) && typeName == wxGRID_VALUE_STRING) return true;
    return false;
}

double SubstudioGridTable::GetValueAsDouble(int row, int col) {
    const auto& r = m_rows.at(row);
    if (col == COL_START) return r.start;
    if (col == COL_END)   return r.end;
    return 0.0;
}

long SubstudioGridTable::GetValueAsLong(int row, int col) {
    const auto& r = m_rows.at(row);
    if (col == COL_NUM) return r.index;
    if (col == COL_CPS) return r.cps;
    return 0;
}

bool SubstudioGridTable::IsEmptyCell(int row, int col) {
    if (row < 0 || static_cast<size_t>(row) >= m_rows.size()) return true;
    const auto& r = m_rows[row];
    switch (col) {
    case COL_TEXT:  return r.text.IsEmpty();
    default:        return false;
    }
}

bool SubstudioGridTable::AppendRows(size_t numRows) {
    if (numRows == 0) return true;
    size_t old = m_rows.size();
    m_rows.resize(old + numRows);
    Reindex();
    if (GetView()) {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, static_cast<int>(numRows));
        GetView()->ProcessTableMessage(msg);
    }
    return true;
}

bool SubstudioGridTable::DeleteRows(size_t pos, size_t numRows) {
    if (numRows == 0) return true;
    size_t cur = m_rows.size();
    if (pos >= cur) return false;
    size_t end = std::min(cur, pos + numRows);
    m_rows.erase(m_rows.begin() + pos, m_rows.begin() + end);
    Reindex();
    if (GetView()) {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, static_cast<int>(pos), static_cast<int>(end - pos));
        GetView()->ProcessTableMessage(msg);
    }
    return true;
}

void SubstudioGridTable::AppendRow(const SubstudioRow& rIn) {
    m_rows.push_back(rIn);
    Reindex();
    if (GetView()) {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1);
        GetView()->ProcessTableMessage(msg);
    }
}

void SubstudioGridTable::EnsureOneRowPresent() {
    if (!m_rows.empty()) return;
    SubstudioRow r;
    r.start = 0.0;
    r.end = 5.0;
    r.cps = ComputeCps(r.text, r.start, r.end);
    m_rows.push_back(r);

    Reindex();
    if (GetView()) {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1);
        GetView()->ProcessTableMessage(msg);
    }
}

void SubstudioGridTable::Reindex() {
    for (size_t i = 0; i < m_rows.size(); ++i) m_rows[i].index = static_cast<int>(i + 1);
}

int SubstudioGridTable::ComputeCps(const wxString& text, double start, double end) {
    if (!(end > start)) return 0;
    double dur = std::max(0.01, end - start);
    wxString flat = text;
    flat.Replace("\r", "");
    flat.Replace("\n", "");
    int chars = static_cast<int>(flat.length());
    return static_cast<int>(std::round(chars / dur));
}

void SubstudioGridTable::RecalcRow(int row) {
    auto& r = m_rows.at(row);
    r.cps = ComputeCps(r.text, r.start, r.end);
}

// ---------- SubstudioGrid

wxBEGIN_EVENT_TABLE(SubstudioGrid, wxGrid)
EVT_GRID_CELL_CHANGED(SubstudioGrid::OnCellChanged)
EVT_GRID_SELECT_CELL(SubstudioGrid::OnRowSelected)
EVT_GRID_RANGE_SELECT(SubstudioGrid::OnRangeSelected)
wxEND_EVENT_TABLE()

SubstudioGrid::SubstudioGrid(wxWindow* parent,
    wxWindowID id,
    const wxPoint& pos,
    const wxSize& size,
    long style,
    const wxString& name)
    : wxGrid(parent, id, pos, size, style, name)
{
    m_table = new SubstudioGridTable();
    SetTable(m_table, true, wxGridSelectRows);

    // Look & feel base
    SetDefaultCellOverflow(false);
    EnableEditing(true);

    // Proveedor de atributos
    GetTable()->SetAttrProvider(new SubstudioAttrProvider());

    // Apariencia y columnas
    ApplyBrandColors();
    ConfigureLook();
    ConfigureColumns();

    // Garantizar 1 fila vacía inicial visible
    m_table->EnsureOneRowPresent();
}

void SubstudioGrid::BindExternalEditor(wxTextCtrl* editor) {
    m_externalEditor = editor;
    if (!m_externalEditor) return;

    // Cuando el usuario tipea en el editor -> actualizar la grilla (fila activa)
    m_externalEditor->Bind(wxEVT_TEXT, &SubstudioGrid::OnEditorTyped, this);

    // Inicializar el contenido del editor con la fila actual (si hay)
    const int r = GetGridCursorRow();
    if (r >= 0 && r < GetNumberRows()) {
        m_syncGuard = true;
        m_externalEditor->ChangeValue(GetCellValue(r, COL_TEXT));
        m_syncGuard = false;
    }
}

void SubstudioGrid::ApplyBrandColors() {
    // Colores SUBSTUDIO
    const wxColour topBarCol(165, 207, 231); // #A5CFE7
    const wxColour rowLabelCol(196, 236, 201); // #C4ECC9
    const wxColour selBgCol(206, 255, 231); // #CEFFE7
    const wxColour selBorder(255, 91, 239); // #FF5BEF

    if (wxWindow* w = GetGridColLabelWindow()) w->SetBackgroundColour(topBarCol);
    if (wxWindow* w = GetGridRowLabelWindow()) w->SetBackgroundColour(rowLabelCol);

    SetSelectionBackground(selBgCol);
    SetSelectionForeground(*wxBLACK);

    SetCellHighlightPenWidth(1);
    SetCellHighlightColour(selBorder);
}

void SubstudioGrid::ConfigureLook() {
    SetRowLabelSize(0);            // ocultar etiquetas de fila
    SetColLabelSize(22);
    SetDefaultRowSize(18, true);   // altura 18, y redimensionar existentes
    EnableGridLines(true);
}

void SubstudioGrid::ConfigureColumns() {
    SetColSize(COL_NUM, 40);
    SetColSize(COL_START, 80);
    SetColSize(COL_END, 80);
    SetColSize(COL_CPS, 50);
    SetColSize(COL_TEXT, 600);

    // Start/End como texto validado por SetValue -> parseo a segundos
    {
        wxGridCellAttr* aStart = new wxGridCellAttr();
        aStart->SetEditor(new wxGridCellTextEditor());
        SetColAttr(COL_START, aStart);

        wxGridCellAttr* aEnd = new wxGridCellAttr();
        aEnd->SetEditor(new wxGridCellTextEditor());
        SetColAttr(COL_END, aEnd);
    }

    // Text multilínea con auto wrap
    {
        wxGridCellAttr* aText = new wxGridCellAttr();
        aText->SetEditor(new wxGridCellAutoWrapStringEditor());
#if wxCHECK_VERSION(3,3,0)
        aText->SetFitMode(wxGridFitMode::Ellipsize(wxELLIPSIZE_END));
#endif
        SetColAttr(COL_TEXT, aText);
    }
}

void SubstudioGrid::OnCellChanged(wxGridEvent& e) {
    const int row = e.GetRow();
    const int col = e.GetCol();

    if (col == COL_START || col == COL_END || col == COL_TEXT) {
        RefreshBlock(row, COL_CPS, row, COL_CPS);

        // Reflejar cambios de texto en el editor externo
        if (m_externalEditor && !m_syncGuard && col == COL_TEXT) {
            m_syncGuard = true;
            m_externalEditor->ChangeValue(GetCellValue(row, COL_TEXT));
            m_syncGuard = false;
        }
    }
    e.Skip();
}

void SubstudioGrid::OnRowSelected(wxGridEvent& e) {
    if (!m_externalEditor) { e.Skip(); return; }
    const int row = e.GetRow();
    if (row < 0 || row >= GetNumberRows()) { e.Skip(); return; }

    m_syncGuard = true;
    m_externalEditor->ChangeValue(GetCellValue(row, COL_TEXT));
    m_syncGuard = false;

    e.Skip();
}

void SubstudioGrid::OnRangeSelected(wxGridRangeSelectEvent& e) {
    if (!m_externalEditor || !e.Selecting()) { e.Skip(); return; }
    int row = e.GetTopRow();
    if (row < 0 || row >= GetNumberRows()) { e.Skip(); return; }

    m_syncGuard = true;
    m_externalEditor->ChangeValue(GetCellValue(row, COL_TEXT));
    m_syncGuard = false;

    e.Skip();
}

void SubstudioGrid::OnEditorTyped(wxCommandEvent& WXUNUSED(e)) {
    if (m_syncGuard) return;
    const int row = GetGridCursorRow();
    if (row < 0 || row >= GetNumberRows()) return;

    // Volcar el contenido del editor a la grilla (dispara SetValue -> recalcula CPS)
    SetCellValue(row, COL_TEXT, m_externalEditor->GetValue());
}
