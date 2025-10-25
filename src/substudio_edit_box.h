#pragma once
// src/substudio_edit_box.h
#ifndef SUBSTUDIO_EDIT_BOX_H
#define SUBSTUDIO_EDIT_BOX_H

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#include <wx/timer.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/grid.h>
#include <functional>

class SubstudioGrid; // fwd-decl para no acoplar headers

// Evento propio: se dispara cuando se COMMITTEA texto al grid
// evt.GetInt()   -> row comprometida
// evt.GetString()-> texto comprometido
wxDECLARE_EVENT(EVT_SUBSTUDIO_COMMIT_TEXT, wxCommandEvent);

// Contexto mínimo para desacoplar
struct SubstudioContext {
    SubstudioGrid* grid = nullptr;                 // a quién aplico commits
    std::function<void()> notify_dirty = nullptr;  // callback opcional (marca * en título, etc.)
};

class SubstudioEditBox final : public wxPanel {
public:
    SubstudioEditBox(wxWindow* parent, const SubstudioContext& ctx,
        wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize);

    wxTextCtrl* GetTextCtrl() const { return m_editor; }

    // API opcional: forzar commit del buffer si existe (p. ej. antes de guardar)
    void ForceCommit();

private:
    // --- UI
    wxTextCtrl* m_editor = nullptr;
    wxStaticText* m_counter = nullptr;

    // --- Estado
    SubstudioContext m_ctx;
    bool      m_syncGuard = false; // bloquea reentradas durante cambios programáticos
    wxString  m_pending;
    bool      m_hasPending = false;

    // --- Helpers
    void BindGridSignals();
    void LoadFromRow(int row);                 // carga editor desde grid (sin disparar EVT_TEXT)
    void CommitPending(bool moveToNext);       // vuelca m_pending -> grid (y opcionalmente avanza)
    void CancelPending();                      // descarta buffer y restaura desde grid
    void UpdateCounter();                      // muestra caracteres (simple)

    // --- Handlers del editor
    void OnEditorText(wxCommandEvent& e);      // sólo bufferiza (no comitea)
    void OnEditorCharHook(wxKeyEvent& e);      // Enter=commit; Ctrl+Enter=commit-sin-mover; Esc=cancel
    void OnEditorKillFocus(wxFocusEvent& e);   // commit al perder foco (sin mover)

    // --- Handlers del grid (sin que el grid nos conozca)
    void OnGridSelectCell(wxGridEvent& e);
    void OnGridRangeSelect(wxGridRangeSelectEvent& e);

    // Utilidad
    int CurrentRowSafe() const;                // fila válida (o -1)

    wxDECLARE_NO_COPY_CLASS(SubstudioEditBox);
};

#endif // SUBSTUDIO_EDIT_BOX_H
