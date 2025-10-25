// src/substudio_edit_box.cpp
#include "substudio_edit_box.h"
#include "substudiogrid.h" // sólo para usar SubstudioGrid API pública

wxDEFINE_EVENT(EVT_SUBSTUDIO_COMMIT_TEXT, wxCommandEvent);

SubstudioEditBox::SubstudioEditBox(wxWindow* parent, const SubstudioContext& ctx,
    wxWindowID id,
    const wxPoint& pos,
    const wxSize& size)
    : wxPanel(parent, id, pos, size, wxTAB_TRAVERSAL | wxBORDER_NONE)
    , m_ctx(ctx)
{
    // --- UI mínima
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    m_editor = new wxTextCtrl(this, wxID_ANY, "",
        wxDefaultPosition, wxDefaultSize,
        wxTE_MULTILINE | wxWANTS_CHARS | wxBORDER_SUNKEN);
    m_counter = new wxStaticText(this, wxID_ANY, "0 chars");

    sizer->Add(m_editor, 1, wxEXPAND | wxALL, FromDIP(4));
    sizer->Add(m_counter, 0, wxRIGHT | wxBOTTOM | wxALIGN_RIGHT, FromDIP(6));
    SetSizerAndFit(sizer);

    // --- Binds del editor
    m_editor->Bind(wxEVT_TEXT, &SubstudioEditBox::OnEditorText, this);
    m_editor->Bind(wxEVT_CHAR_HOOK, &SubstudioEditBox::OnEditorCharHook, this);
    m_editor->Bind(wxEVT_KILL_FOCUS, &SubstudioEditBox::OnEditorKillFocus, this);

    // --- Inicializar contenido según selección actual del grid
    BindGridSignals();
    LoadFromRow(CurrentRowSafe());
}

void SubstudioEditBox::ForceCommit() {
    CommitPending(false);
}

void SubstudioEditBox::BindGridSignals() {
    if (!m_ctx.grid) return;

    // Nos suscribimos a eventos del grid SIN que el grid nos conozca
    m_ctx.grid->Bind(wxEVT_GRID_SELECT_CELL, &SubstudioEditBox::OnGridSelectCell, this);
    m_ctx.grid->Bind(wxEVT_GRID_RANGE_SELECT, &SubstudioEditBox::OnGridRangeSelect, this);
}

int SubstudioEditBox::CurrentRowSafe() const {
    if (!m_ctx.grid) return -1;
    int row = m_ctx.grid->GetGridCursorRow();
    if (row < 0 || row >= m_ctx.grid->GetNumberRows()) return -1;
    return row;
}

void SubstudioEditBox::LoadFromRow(int row) {
    if (!m_ctx.grid) return;
    if (row < 0) {
        // asegurar que haya al menos 1 fila visible
        m_ctx.grid->EnsureOneRowPresent();
        row = 0;
    }

    m_syncGuard = true;
    m_editor->ChangeValue(m_ctx.grid->GetCellValue(row, COL_TEXT)); // ChangeValue no dispara EVT_TEXT
    m_syncGuard = false;

    m_pending.clear();
    m_hasPending = false;
    UpdateCounter();
}

void SubstudioEditBox::CommitPending(bool moveToNext) {
    if (!m_ctx.grid || !m_hasPending) return;

    int row = CurrentRowSafe();
    if (row < 0) {
        // si el grid quedó sin filas por alguna razón, créala
        m_ctx.grid->EnsureOneRowPresent();
        row = 0;
    }

    // Volcar a la grilla (dispara SetValue -> recalcula CPS)
    // Proteger de reentradas UI: el grid podría reflejar de vuelta
    m_syncGuard = true;
    m_ctx.grid->SetCellValue(row, COL_TEXT, m_pending);
    m_syncGuard = false;

    // Notificar "dirty" sin acoplar: callback y evento propio
    if (m_ctx.notify_dirty) m_ctx.notify_dirty();

    wxCommandEvent evt(EVT_SUBSTUDIO_COMMIT_TEXT, GetId());
    evt.SetInt(row);
    evt.SetString(m_pending);
    wxPostEvent(GetParent(), evt); // cualquier contenedor puede escucharlo

    // Estado local
    m_hasPending = false;
    m_pending.clear();
    UpdateCounter();

    // Mover a la siguiente fila si procede
    if (moveToNext) {
        int next = row + 1;
        if (next >= m_ctx.grid->GetNumberRows()) {
            // política UX mínima: crear nueva fila al llegar al final
            m_ctx.grid->AppendRows(1);
        }
        m_ctx.grid->SetGridCursor(std::min(next, m_ctx.grid->GetNumberRows() - 1), COL_TEXT);
        m_ctx.grid->MakeCellVisible(m_ctx.grid->GetGridCursorRow(), COL_TEXT);
        LoadFromRow(CurrentRowSafe());
    }
}

void SubstudioEditBox::CancelPending() {
    // Descarta cambios en buffer y vuelve a reflejar valor actual del grid
    LoadFromRow(CurrentRowSafe());
}

void SubstudioEditBox::UpdateCounter() {
    if (!m_editor) return;
    const size_t n = (m_hasPending ? m_pending.length() : m_editor->GetValue().length());
    m_counter->SetLabel(wxString::Format("%zu chars", n));
}

// ===================== Handlers del EDITOR =====================

void SubstudioEditBox::OnEditorText(wxCommandEvent& e) {
    if (m_syncGuard) return; // cambios programáticos
    m_pending = m_editor->GetValue();
    m_hasPending = true;
    UpdateCounter();
    // ¡No propagamos! (no queremos que otros marquen dirty por cada tecla)
    // Si alguien *quiere* enterarse en vivo, se puede agregar un evento onBufferChanged propio.
}

void SubstudioEditBox::OnEditorCharHook(wxKeyEvent& e) {
    const int code = e.GetKeyCode();

    if (code == WXK_RETURN) {
        // Enter: commit + ir a la siguiente
        const bool ctrl = e.ControlDown() || e.CmdDown();
        CommitPending(!ctrl); // Ctrl+Enter => commit y quedarse
        return; // consumimos
    }
    else if (code == WXK_ESCAPE) {
        CancelPending();
        return; // consumimos
    }

    e.Skip(); // otras teclas: dejar pasar
}

void SubstudioEditBox::OnEditorKillFocus(wxFocusEvent& e) {
    // Commit automático al perder foco, sin moverse
    CommitPending(false);
    e.Skip();
}

// ===================== Handlers del GRID =====================

void SubstudioEditBox::OnGridSelectCell(wxGridEvent& e) {
    // Si hay edición pendiente, comitear antes de cambiar de fila
    if (m_hasPending) CommitPending(false);
    LoadFromRow(e.GetRow());
    e.Skip();
}

void SubstudioEditBox::OnGridRangeSelect(wxGridRangeSelectEvent& e) {
    if (!e.Selecting()) { e.Skip(); return; }
    // Cuando el usuario selecciona un rango, cargamos la fila superior
    if (m_hasPending) CommitPending(false);
    LoadFromRow(e.GetTopRow());
    e.Skip();
}
