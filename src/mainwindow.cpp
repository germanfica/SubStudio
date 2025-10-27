#include "mainwindow.h"

#include <wx/sizer.h>
#include <wx/textfile.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/filename.h>
#include <wx/app.h>

#include "srt_io.h"
#include "substudiogrid.h"
#include "substudio_edit_box.h"
#include "substudio_time.h"
#include "substudio_textfmt.h"

// Tabla de eventos mínima (editor, close, size)
wxBEGIN_EVENT_TABLE(MainWindow, wxFrame)
EVT_TEXT(wxID_ANY, OnEditorText)
EVT_CLOSE(OnClose)
EVT_SIZE(OnSize)
wxEND_EVENT_TABLE()

MainWindow::MainWindow()
    : wxFrame(nullptr, wxID_ANY, wxS("SubStudio"), wxDefaultPosition, wxSize(900, 693)) {

    // Menú y toolbar con callbacks desacoplados
    MenuActions actions;
    actions.on_open = [this]() { ActionOpen(); };
    actions.on_save = [this]() { ActionSave(); };
    actions.on_save_as = [this]() { ActionSaveAs(); };
    actions.on_exit = [this]() { ActionExit(); };
    actions.on_about = [this]() { ActionAbout(); };

    // MainMenu y MainToolbar se encargan de anexarse al frame en sus ctors.
    menu_ = std::make_unique<MainMenu>(this, actions);
    toolbar_ = std::make_unique<MainToolbar>(this);

    // Status bar
    CreateStatusBar(2);
    SetStatusText(wxString(wxS("Ready")));

    // Layout: editor arriba, grilla abajo
    auto* top = new wxBoxSizer(wxVERTICAL);

    grid_ = new SubstudioGrid(this, wxID_ANY);
    grid_view_ = std::make_unique<GridView>(grid_);

    // SubstudioEditBox vinculado al grid por contexto del propio widget
    SubstudioContext ctx;
    ctx.grid = grid_;
    ctx.notify_dirty = [this]() {
        if (!doc_.dirty()) {
            doc_.set_dirty(true);
            UpdateWindowTitle();
        }
        };

    edit_box_ = new SubstudioEditBox(this, ctx, wxID_ANY, wxDefaultPosition, wxSize(-1, 120));
    editor_ = edit_box_->GetTextCtrl();
    if (editor_) grid_->BindExternalEditor(editor_);

    // Ligamos el handler al propio edit_box_ (fuente) para que se limpie automáticamente
    if (edit_box_) {
        edit_box_->Bind(EVT_SUBSTUDIO_COMMIT_TEXT, [this](wxCommandEvent& e) {
            const int row = e.GetInt();
            const wxString text = e.GetString();
            if (row >= 0) {
                doc_.SetRowText(static_cast<size_t>(row), text);
                if (grid_view_) grid_view_->UpdateRowText(row, text);
                UpdateWindowTitle();
                SetStatusText(wxString::Format(wxS("Edited row %d"), row + 1));
            }
            });
    }

    edit_box_->SetMinSize(wxSize(-1, FromDIP(120)));

    top->Add(edit_box_, 0, wxEXPAND, 0);
    top->Add(grid_, 1, wxEXPAND, 0);
    SetSizer(top);

    // Estado inicial
    doc_.Clear();
    UpdateWindowTitle();

    CentreOnScreen();
}

MainWindow::~MainWindow() {
    // Debug / teardown controlado: destruyo grid_view_ antes de que wx destruya widgets
    wxLogDebug("~MainWindow() called - begin teardown");

    if (grid_view_) {
        wxLogDebug("~MainWindow(): resetting grid_view_");
        grid_view_.reset();
    }

    // Destruir children explícitamente para asegurar orden (opcional pero ayuda a evitar double-frees)
    DestroyChildren();

    wxLogDebug("~MainWindow(): teardown done");
}

void MainWindow::UpdateWindowTitle() {
    const wxString name = doc_.path().IsEmpty()
        ? wxString(wxS("Untitled"))
        : wxFileName(doc_.path()).GetFullName();

    wxString prefix;
    if (doc_.dirty())
        prefix = wxString(wxS("*"));
    else
        prefix = wxEmptyString;

    SetTitle(prefix + name + wxString(wxS(" - SubStudio 0.1.0")));
}

bool MainWindow::PromptSaveIfDirty() {
    if (!doc_.dirty()) return true;

    const wxString shown = doc_.path().IsEmpty()
        ? wxString(wxS("Untitled"))
        : doc_.path();

    const wxString question = wxString(wxS("Do you want to save changes to ")) + shown + wxString(wxS("?"));

    wxMessageDialog dlg(this, question, wxString(wxS("Unsaved changes")),
        wxYES_NO | wxCANCEL | wxICON_WARNING);
    dlg.SetYesNoCancelLabels(wxString(wxS("Yes")), wxString(wxS("No")), wxString(wxS("Cancel")));
    dlg.SetAffirmativeId(wxID_YES);
    const int res = dlg.ShowModal();
    if (res == wxID_YES)  return DoSave();
    if (res == wxID_NO)   return true;
    return false; // cancel
}

bool MainWindow::DoSaveAs() {
    wxFileDialog dlg(this, wxString(wxS("Save subtitle file")), wxEmptyString, wxEmptyString,
        wxString(wxS("SubRip files (*.srt)|*.srt|All files (*.*)|*.*")),
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return false;
    doc_.set_path(dlg.GetPath());
    return DoSave();
}

bool MainWindow::DoSave() {
    if (doc_.path().IsEmpty()) return DoSaveAs();

    // 1) Asegurar que cualquier edicion pendiente del editor externo se vuelque a la grilla.
    if (edit_box_) {
        edit_box_->ForceCommit();  // vuelca sincronamente a la celda activa
    }

    // 2) Tomar un snapshot directo de la grilla (fuente de verdad de la UI)
    //    para no depender de que 'doc_' este 100% sincronizado.
    std::vector<SubtitleEntry> snapshot;
    snapshot.reserve(grid_ ? grid_->GetNumberRows() : 0);

    if (grid_) {
        const int rows = grid_->GetNumberRows();
        for (int r = 0; r < rows; ++r) {
            SubtitleEntry e;
            e.line_number = r + 1;

            double t1 = 0.0, t2 = 0.0;
            (void)SubstudioParseTime(grid_->GetCellValue(r, COL_START), t1);
            (void)SubstudioParseTime(grid_->GetCellValue(r, COL_END), t2);
            e.start_time = t1;
            e.end_time = t2;

            // El texto en la grilla viene formateado ("\\N" visible). Parsearlo a '\n' reales.
            const wxString displayed = grid_->GetCellValue(r, COL_TEXT);
            e.text = SubstudioParseGridText(displayed);

            // Evitar persistir filas placeholder totalmente vacias (sin tiempo y sin texto)
            wxString trimmed = e.text;
            trimmed.Trim(true).Trim(false);
            const bool emptyRow = trimmed.IsEmpty() && e.start_time <= 0.0 && e.end_time <= 0.0;
            if (emptyRow) continue;

            snapshot.push_back(std::move(e));
        }
    }

    // 3) Guardar el snapshot al archivo SRT.
    if (!srt::Save(doc_.path(), snapshot)) {
        wxMessageBox(wxString(wxS("Failed to open file for writing: ")) + doc_.path(),
            wxString(wxS("Error")), wxICON_ERROR);
        return false;
    }

    // 4) Mantener el modelo en memoria sincronizado con lo que se guardo.
    doc_.entries() = snapshot;
    doc_.ResequenceLineNumbers();
    doc_.set_dirty(false);
    UpdateWindowTitle();
    SetStatusText(wxString(wxS("File saved")));
    return true;
}

// Acciones
void MainWindow::ActionOpen() {
    if (!PromptSaveIfDirty()) return;

    wxFileDialog dlg(this, wxString(wxS("Open subtitle file")), wxEmptyString, wxEmptyString,
        wxString(wxS("SubRip files (*.srt)|*.srt|All files (*.*)|*.*")),
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK) return;

    std::vector<SubtitleEntry> tmp;
    if (!srt::Load(dlg.GetPath(), tmp)) {
        wxMessageBox(wxString(wxS("Failed to open file: ")) + dlg.GetPath(),
            wxString(wxS("Error")), wxICON_ERROR);
        return;
    }

    doc_.entries() = std::move(tmp);
    doc_.set_path(dlg.GetPath());
    doc_.set_dirty(false);

    if (grid_view_) grid_view_->Populate(doc_.entries());
    if (grid_view_) grid_view_->AdjustTextColumnWidth(this);

    UpdateWindowTitle();
    SetStatusText(wxString(wxS("File loaded: ")) + doc_.path());
}

void MainWindow::ActionSave() { DoSave(); }
void MainWindow::ActionSaveAs() { DoSaveAs(); }
void MainWindow::ActionExit() { Close(true); }
void MainWindow::ActionAbout() {
    wxMessageBox(wxString(wxS("SubStudio - simple subtitle editor")),
        wxString(wxS("About")), wxOK | wxICON_INFORMATION);
}

// Eventos
void MainWindow::OnEditorText(wxCommandEvent& WXUNUSED(ev)) {
    if (!editor_) return;
    int row = grid_->GetGridCursorRow();
    if (row < 0 || row >= static_cast<int>(doc_.entries().size())) return;

    const wxString txt = editor_->GetValue();
    doc_.SetRowText(static_cast<size_t>(row), txt);
    if (grid_view_) grid_view_->UpdateRowText(row, txt);
    UpdateWindowTitle();
    SetStatusText(wxString::Format(wxS("Edited row %d"), row + 1));
}

void MainWindow::OnClose(wxCloseEvent& ev) {
    if (!PromptSaveIfDirty()) {
        ev.Veto();
        return;
    }
    Destroy();
}

void MainWindow::OnSize(wxSizeEvent& ev) {
    if (grid_view_) grid_view_->AdjustTextColumnWidth(this);
    ev.Skip();
}
