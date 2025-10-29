// src/substudiogrid.cpp
#include "substudiogrid.h"
#include <wx/regex.h>

// Config centralizados para UI
namespace {
    constexpr TimeFormat     kUiTimeFmt = TimeFormat::TF_FMT_HH_MM_SS_CS; // salida 'H:MM:SS:FF'
    constexpr TimeParseFlags kUiParse = kDefaultParseFlags;
}

// ---------- SubstudioAttrProvider

SubstudioAttrProvider::SubstudioAttrProvider() : wxGridCellAttrProvider() {}

wxGridCellAttr* SubstudioAttrProvider::GetAttr(int row, int col, wxGridCellAttr::wxAttrKind kind) const {
    wxGridCellAttr* attr = wxGridCellAttrProvider::GetAttr(row, col, kind);
    if (!attr) attr = new wxGridCellAttr();

    // Zebra striping
    if ((row % 2) == 0) attr->SetBackgroundColour(wxColour(245, 248, 250));
    else                attr->SetBackgroundColour(*wxWHITE);

    // Read-only por columna
    if (col == COL_NUM || col == COL_CPS) attr->SetReadOnly(true);

    // Alineaciones
    if (col == COL_NUM || col == COL_CPS)
        attr->SetAlignment(wxALIGN_CENTRE, wxALIGN_CENTRE);
    else if (col == COL_START || col == COL_END)
        attr->SetAlignment(wxALIGN_CENTRE, wxALIGN_CENTRE);
    else
        attr->SetAlignment(wxALIGN_LEFT, wxALIGN_CENTRE);

    return attr;
}

// ---------- SubstudioGridTable (modelo = Subtitles)

SubstudioGridTable::SubstudioGridTable(Subtitles& model)
    : wxGridTableBase(), model_(&model) {
}

int SubstudioGridTable::GetNumberRows() {
    return static_cast<int>(model_ ? model_->entries().size() : 0);
}

wxString SubstudioGridTable::GetValue(int row, int col) {
    if (!model_) return wxEmptyString;
    const auto& entries = model_->entries();
    if (row < 0 || static_cast<size_t>(row) >= entries.size()) return wxEmptyString;

    const SubtitleEntry& e = entries[static_cast<size_t>(row)];
    switch (col) {
    case COL_NUM:   return wxString::Format("%d", e.line_number);
    case COL_START: return SubstudioFormatTime(e.start_time, kUiTimeFmt);
    case COL_END:   return SubstudioFormatTime(e.end_time, kUiTimeFmt);
    case COL_CPS:   return wxString::Format("%d", e.Cps());
    case COL_TEXT:  return SubstudioFormatGridText(e.text);
    default:        return wxEmptyString;
    }
}

void SubstudioGridTable::SetValue(int row, int col, const wxString& value) {
    if (!model_) return;
    auto& entries = model_->entries();
    if (row < 0 || static_cast<size_t>(row) >= entries.size()) return; // no crear filas fantasma

    SubtitleEntry& e = entries[static_cast<size_t>(row)];
    bool changed = false;

    switch (col) {
    case COL_START: {
        double t;
        if (SubstudioParseTime(value, t, kUiParse)) {
            if (e.start_time != t) { e.start_time = t; changed = true; }
        }
        break;
    }
    case COL_END: {
        double t;
        if (SubstudioParseTime(value, t, kUiParse)) {
            if (e.end_time != t) { e.end_time = t; changed = true; }
        }
        break;
    }
    case COL_TEXT: {
        // La grilla muestra '\N'; parsear a '\n' reales antes de guardar
        wxString parsed = SubstudioParseGridText(value);
        if (e.text != parsed) { e.text = parsed; changed = true; }
        break;
    }
    default:
        break;
    }

    if (changed) {
        RepaintRow(row);
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
    case COL_END:   return wxGRID_VALUE_STRING; // se edita como string y se parsea
    case COL_TEXT:  return wxGRID_VALUE_STRING;
    default:        return wxGRID_VALUE_STRING;
    }
}

bool SubstudioGridTable::CanGetValueAs(int, int col, const wxString& typeName) {
    if ((col == COL_NUM || col == COL_CPS) && typeName == wxGRID_VALUE_NUMBER) return true;
    if ((col == COL_START || col == COL_END) && (typeName == wxGRID_VALUE_STRING || typeName == wxGRID_VALUE_FLOAT)) return true;
    if (col == COL_TEXT && typeName == wxGRID_VALUE_STRING) return true;
    return false;
}

bool SubstudioGridTable::CanSetValueAs(int, int col, const wxString& typeName) {
    if ((col == COL_START || col == COL_END) && (typeName == wxGRID_VALUE_STRING || typeName == wxGRID_VALUE_FLOAT)) return true;
    if (col == COL_TEXT && typeName == wxGRID_VALUE_STRING) return true;
    return false;
}

double SubstudioGridTable::GetValueAsDouble(int row, int col) {
    if (!model_) return 0.0;
    const auto& entries = model_->entries();
    if (row < 0 || static_cast<size_t>(row) >= entries.size()) return 0.0;

    const auto& e = entries[static_cast<size_t>(row)];
    if (col == COL_START) return e.start_time;
    if (col == COL_END)   return e.end_time;
    return 0.0;
}

long SubstudioGridTable::GetValueAsLong(int row, int col) {
    if (!model_) return 0;
    const auto& entries = model_->entries();
    if (row < 0 || static_cast<size_t>(row) >= entries.size()) return 0;

    const auto& e = entries[static_cast<size_t>(row)];
    if (col == COL_NUM) return e.line_number;
    if (col == COL_CPS) return e.Cps();
    return 0;
}

bool SubstudioGridTable::IsEmptyCell(int row, int col) {
    if (!model_) return true;
    const auto& entries = model_->entries();
    if (row < 0 || static_cast<size_t>(row) >= entries.size()) return true;

    const auto& e = entries[static_cast<size_t>(row)];
    if (col == COL_TEXT) return e.text.IsEmpty();
    return false;
}

bool SubstudioGridTable::AppendRows(size_t numRows) {
    if (!model_) return false;
    if (numRows == 0) return true;

    auto& v = model_->entries();
    size_t oldSize = v.size();
    v.resize(oldSize + numRows); // SubtitleEntry default
    Reindex();

    if (GetView()) {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, static_cast<int>(numRows));
        GetView()->ProcessTableMessage(msg);
    }
    return true;
}

bool SubstudioGridTable::DeleteRows(size_t pos, size_t numRows) {
    if (!model_) return false;
    if (numRows == 0) return true;

    auto& v = model_->entries();
    if (pos >= v.size()) return false;

    size_t end = std::min(v.size(), pos + numRows);
    v.erase(v.begin() + pos, v.begin() + end);
    Reindex();

    if (GetView()) {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, static_cast<int>(pos), static_cast<int>(end - pos));
        GetView()->ProcessTableMessage(msg);
    }
    return true;
}

void SubstudioGridTable::EnsureOneRowPresent() {
    if (!model_) return;
    auto& v = model_->entries();
    if (!v.empty()) return;

    v.emplace_back(); // default SubtitleEntry
    v.back().start_time = 0.0;
    v.back().end_time = 5.0;

    Reindex();

    if (GetView()) {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1);
        GetView()->ProcessTableMessage(msg);
    }
}

void SubstudioGridTable::Reindex() {
    if (!model_) return;
    auto& v = model_->entries();
    for (size_t i = 0; i < v.size(); ++i) v[i].line_number = static_cast<int>(i + 1);
}

void SubstudioGridTable::RepaintRow(int row) {
    if (GetView()) {
        GetView()->RefreshBlock(row, 0, row, COL_COUNT - 1);
    }
}

int SubstudioGridTable::ComputeCpsFromTextTime(const wxString& gridText, double start, double end) {
    SubtitleEntry tmp;
    tmp.text = SubstudioParseGridText(gridText);
    tmp.start_time = start;
    tmp.end_time = end;
    return tmp.Cps();
}

// ---------- SubstudioGrid

wxBEGIN_EVENT_TABLE(SubstudioGrid, wxGrid)
EVT_GRID_CELL_CHANGED(SubstudioGrid::OnCellChanged)
EVT_GRID_SELECT_CELL(SubstudioGrid::OnRowSelected)
EVT_GRID_RANGE_SELECT(SubstudioGrid::OnRangeSelected)
wxEND_EVENT_TABLE()

SubstudioGrid::SubstudioGrid(wxWindow* parent,
    Subtitles& model,
    wxWindowID id,
    const wxPoint& pos,
    const wxSize& size,
    long style,
    const wxString& name)
    : wxGrid(parent, id, pos, size, style, name)
{
    m_table = new SubstudioGridTable(model);
    SetTable(m_table, true, wxGridSelectRows);

    // Proveedor de atributos
    GetTable()->SetAttrProvider(new SubstudioAttrProvider());

    // Look & feel base
    SetDefaultCellOverflow(false);
    EnableEditing(true);

    ApplyBrandColors();
    ConfigureLook();
    ConfigureColumns();

    // Garantizar 1 fila visible al iniciar
    m_table->EnsureOneRowPresent();
}

void SubstudioGrid::BindExternalEditor(wxTextCtrl* editor) {
    m_externalEditor = editor;
    if (!m_externalEditor) return;

    // Cuando tipee el usuario -> actualizar grilla (fila activa)
    m_externalEditor->Bind(wxEVT_TEXT, &SubstudioGrid::OnEditorTyped, this);

    // Inicializar editor con la fila actual
    const int r = GetGridCursorRow();
    if (r >= 0 && r < GetNumberRows()) {
        m_syncGuard = true;
        m_externalEditor->ChangeValue(GetCellValue(r, COL_TEXT));
        m_syncGuard = false;
    }
}

void SubstudioGrid::ApplyBrandColors() {
    // Colores SUBSTUDIO
    const wxColour topBarCol(165, 207, 231);  // #A5CFE7
    const wxColour rowLabelCol(196, 236, 201); // #C4ECC9
    const wxColour selBgCol(206, 255, 231);   // #CEFFE7
    const wxColour selBorder(255, 91, 239);   // #FF5BEF

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
    SetDefaultRowSize(18, true);   // altura 18 y redimensionar existentes
    EnableGridLines(true);
}

void SubstudioGrid::ConfigureColumns() {
    SetColSize(COL_NUM, 40);
    SetColSize(COL_START, 80);
    SetColSize(COL_END, 80);
    SetColSize(COL_CPS, 50);
    SetColSize(COL_TEXT, 600);

    // Start/End como texto validado por SetValue -> parse a segundos
    {
        wxGridCellAttr* aStart = new wxGridCellAttr();
        aStart->SetEditor(new wxGridCellTextEditor());
        SetColAttr(COL_START, aStart);

        wxGridCellAttr* aEnd = new wxGridCellAttr();
        aEnd->SetEditor(new wxGridCellTextEditor());
        SetColAttr(COL_END, aEnd);
    }

    // Text multilínea con autowrap (y elipsis si tu wx lo soporta)
    {
        wxGridCellAttr* aText = new wxGridCellAttr();
        aText->SetEditor(new wxGridCellAutoWrapStringEditor());
#if wxCHECK_VERSION(3,3,0)
        aText->SetFitMode(wxGridFitMode::Ellipsize(wxELLIPSIZE_END));
#endif
        SetColAttr(COL_TEXT, aText);
    }
}

void SubstudioGrid::SyncToModel() {
    auto* table = dynamic_cast<SubstudioGridTable*>(GetTable());
    if (!table) return;

    const int want = table->GetNumberRows();
    const int have = GetNumberRows();

    BeginBatch();
    if (want > have) {
        wxGridTableMessage msg(table, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, want - have);
        ProcessTableMessage(msg);
    }
    else if (want < have) {
        // pos = filas que se mantienen (want), num = las que se borran
        wxGridTableMessage msg(table, wxGRIDTABLE_NOTIFY_ROWS_DELETED, want, have - want);
        ProcessTableMessage(msg);
    }
    EndBatch();

    ClearSelection();
    ForceRefresh();
    Update();
}

void SubstudioGrid::OnCellChanged(wxGridEvent& e) {
    const int row = e.GetRow();
    const int col = e.GetCol();

    if (col == COL_START || col == COL_END || col == COL_TEXT) {
        // El CPS depende de tiempo y texto
        RefreshBlock(row, COL_CPS, row, COL_CPS);

        // Mantener editor externo sincronizado cuando cambia el texto
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

void SubstudioGrid::OnEditorTyped(wxCommandEvent& e) {
    if (m_syncGuard) { e.Skip(); return; }
    const int row = GetGridCursorRow();
    if (row < 0 || row >= GetNumberRows()) { e.Skip(); return; }

    // Volcar editor a la grilla (dispara SetValue -> actualiza modelo y CPS)
    SetCellValue(row, COL_TEXT, m_externalEditor->GetValue());

    // Dejar pasar el evento por si otros marcan 'dirty'
    e.Skip();
}
