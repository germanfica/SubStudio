// src/mainwindow.cpp
#include "mainwindow.h"

#include <wx/ffile.h>
#include <wx/textfile.h>
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/artprov.h>
#include <wx/sysopt.h>
#include <sstream>
#include <wx/filename.h>
#include <wx/event.h>
#include <wx/scrolwin.h>
#include "substudiogrid.h"
#include <algorithm>
#include "substudio_edit_box.h"

// Event table
wxBEGIN_EVENT_TABLE(MainWindow, wxFrame)
EVT_MENU(MainWindow::ID_Open, MainWindow::OnOpen)
EVT_MENU(MainWindow::ID_Save, MainWindow::OnSave)
EVT_MENU(MainWindow::ID_SaveAs, MainWindow::OnSaveAs)
EVT_MENU(wxID_EXIT, MainWindow::OnQuit)
EVT_MENU(wxID_ABOUT, MainWindow::OnAbout)
EVT_TEXT(wxID_ANY, MainWindow::OnEditorText)
EVT_CLOSE(MainWindow::OnClose)
EVT_SIZE(MainWindow::OnSize)
wxEND_EVENT_TABLE()

static const wxString COL_LABELS[] = {
    "#",               // Line -> show as '#'
    "Start Time",
    "End Time",
    "Characters Per Second",
    "Text"
};

MainWindow::MainWindow()
    : wxFrame(nullptr, wxID_ANY, "SubStudio", wxDefaultPosition, wxSize(900, 693)),
    dirty_(false), renderer_(nullptr)
{
    // --- Colors ---
    const wxColour topBarCol(165, 207, 231);   // #A5CFE7  -> header top bar
    const wxColour rowLabelCol(196, 236, 201); // #C4ECC9  -> left row label area
    const wxColour selBgCol(206, 255, 231);    // #CEFFE7  -> selection background
    const wxColour selBorderCol(255, 91, 239); // #FF5BEF  -> interior border

    // --- Menus ---
    wxMenu* fileMenu = new wxMenu;
    fileMenu->Append(ID_Open, "&Open...\tCtrl-O");
    fileMenu->Append(ID_Save, "&Save\tCtrl-S");
    fileMenu->Append(ID_SaveAs, "Save &As...\tCtrl-Shift-S");
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, "E&xit");

    wxMenu* helpMenu = new wxMenu;
    helpMenu->Append(wxID_ABOUT, "&About");

    wxMenuBar* menuBar = new wxMenuBar;
    menuBar->Append(fileMenu, "&File");
    menuBar->Append(helpMenu, "&Help");
    SetMenuBar(menuBar);

    // --- Toolbar ---
    toolbar_ = CreateToolBar();
    toolbar_->AddTool(ID_Open, "Open", wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR));
    toolbar_->AddTool(ID_Save, "Save", wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR));
    toolbar_->Realize();

    // --- Status bar ---
    CreateStatusBar(2);
    SetStatusText("Ready");

    // --- Layout ---
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);

    // Grid: 5 columns (Line, Start, End, CPS, Text)
    //grid_ = new wxGrid(this, wxID_ANY);
    grid_ = new SubstudioGrid(this, wxID_ANY);
    //grid_->CreateGrid(0, 5);
    grid_->SetCellHighlightPenWidth(1);
    grid_->SetRowLabelSize(0);

    // --- colors ---
    //if (wxWindow* w = grid_->GetGridColLabelWindow()) w->SetBackgroundColour(topBarCol);
    //if (wxWindow* w = grid_->GetGridRowLabelWindow()) w->SetBackgroundColour(rowLabelCol);

    //grid_->SetSelectionBackground(selBgCol);
    //grid_->SetSelectionForeground(*wxBLACK);

    // Set column labels
    //grid_->SetMargins(0, 0);
    //for (int c = 0; c < 5; ++c) {
    //    if (c == 3) {
    //        grid_->SetColLabelValue(c, "Characters Per Second");
    //    }
    //    else {
    //        grid_->SetColLabelValue(c, COL_LABELS[c]);
    //    }
    //}

    //grid_->SetColLabelValue(0, "#");
    //grid_->SetColLabelValue(1, "Start");
    //grid_->SetColLabelValue(2, "End");
    //grid_->SetColLabelValue(3, "CPS");
    //grid_->SetColLabelValue(4, "Text");

    //grid_->SetColLabelAlignment(wxALIGN_LEFT, wxALIGN_CENTRE);

    // Prevent user from editing grid cells directly
    grid_->EnableEditing(false);

    // Selection mode
    //grid_->SetSelectionMode(wxGrid::wxGridSelectCells);
    //grid_->Bind(wxEVT_KEY_DOWN, &MainWindow::OnGridKeyDown, this);
    //grid_->Bind(wxEVT_GRID_CELL_LEFT_CLICK, &MainWindow::OnGridCellLeftClick, this);
    //grid_->Bind(wxEVT_GRID_LABEL_LEFT_CLICK, &MainWindow::OnGridLabelLeftClick, this);
    //grid_->Bind(wxEVT_GRID_RANGE_SELECT, &MainWindow::OnGridRangeSelect, this);
    //wxWindow* gridWin = grid_->GetGridWindow();
    //gridWin->Bind(wxEVT_LEFT_DOWN, &MainWindow::OnGridMouseLeftDown, this);
    //gridWin->Bind(wxEVT_LEFT_UP, &MainWindow::OnGridMouseLeftUp, this);
    //gridWin->Bind(wxEVT_MOTION, &MainWindow::OnGridMouseMotion, this);
    //gridWin->Bind(wxEVT_LEFT_DCLICK, &MainWindow::OnGridMouseLeftDClick, this);
    //editor_->Bind(wxEVT_TEXT, &MainWindow::OnEditorText, this);

    // Disable user resizing/moving of columns and rows
    grid_->EnableDragColSize(false);
    grid_->EnableDragRowSize(false);
    grid_->EnableDragColMove(false);

    // Prevent user from resizing labels area
    // wxWidgets 3.2.8: SetColMinimalWidth takes (col, width).
    // Set a minimum for each column (reasonable default).
    //const int minColWidth = 20;
    //for (int c = 0; c < grid_->GetNumberCols(); ++c)
    //    grid_->SetColMinimalWidth(c, minColWidth);

    // Set default row height for all rows
    grid_->SetDefaultRowSize(18, true); // height, resize existing rows
    if (wxWindow* wrl = grid_->GetGridRowLabelWindow()) wrl->Hide();

    // Important: do NOT allow hiding of Text column via default actions.
    // We'll provide a custom header-right-click menu to hide/show columns.

    // --- Context para el SubstudioEditBox ---
    SubstudioContext ctx;
    ctx.grid = grid_;

    // --- Context ---
    ctx.notify_dirty = [this]() {
        if (!dirty_) {
            dirty_ = true;
            UpdateWindowTitle();
        }
    };

    // Editor - multiline text to edit selected subtitle text
    // moved ABOVE the grid per request
    //editor_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, 120), wxTE_MULTILINE);
    editBox_ = new SubstudioEditBox(this, ctx, wxID_ANY, wxDefaultPosition, wxSize(-1, 120));
    // Mantener compatibilidad con código existente que usa editor_:
    editor_ = editBox_->GetTextCtrl(); // suponemos este accessor en SubstudioEditBox

    if(editor_) grid_->BindExternalEditor(editor_);
    Bind(EVT_SUBSTUDIO_COMMIT_TEXT, &MainWindow::OnSubstudioEditCommit, this, editBox_->GetId());

    editBox_->SetMinSize(wxSize(-1, FromDIP(120)));
    //topSizer->Add(editor_, 0, wxEXPAND, 0);
    //topSizer->Add(editBox_, 0, wxEXPAND | wxALL, 0);
    //topSizer->Add(editBox_, 0, wxEXPAND | wxALL, FromDIP(2));
    topSizer->Add(editBox_, 0, wxEXPAND, 0);
    //topSizer->Add(grid_, 1, wxEXPAND | wxALL, FromDIP(2));
    //topSizer->Add(grid_, 1, wxEXPAND | wxALL, 0);
    topSizer->Add(grid_, 1, wxEXPAND, 0);

    SetSizer(topSizer);
    SetClientSize(906, 693);
    CentreOnScreen();


    // Create renderer for CPS column (warn=40, error=60 as example)
    renderer_ = nullptr; // si tenes CpsRenderer implementalo; lo dejamos null por compatibilidad
    // si existiera: wxGridCellAttr* attr = new wxGridCellAttr(); attr->SetRenderer(renderer_); grid_->SetColAttr(3, attr);

    // ensure model empty
    model_.clear();
    UpdateWindowTitle();

    // initial column widths: set reasonable fixed widths for non-text columns
    //grid_->SetColSize(0, 40);   // '#'
    //grid_->SetColSize(1, 110);  // Start Time
    //grid_->SetColSize(2, 110);  // End Time
    //grid_->SetColSize(3, 90);   // CPS

    // adjust text column to fill the remaining area
    Layout();
    wxYieldIfNeeded();
    // No temporales: usar helper que crea un wxSizeEvent nombrado
    TriggerSizeHandler();
}

// Handler para el evento de commit del SubstudioEditBox
void MainWindow::OnSubstudioEditCommit(wxCommandEvent& evt)
{
    const int row = evt.GetInt();         // fila donde se hizo commit
    const wxString newText = evt.GetString();

    // Si la fila no existe en model_ la extendemos con valores razonables
    if (row < 0) return;

    if (row >= static_cast<int>(model_.size())) {
        // expandir model_ hasta row inclusive
        int needed = row + 1;
        int old = static_cast<int>(model_.size());
        model_.resize(needed);
        for (int r = old; r < needed; ++r) {
            SubtitleEntry& e = model_[r];
            e.lineNumber = r + 1;
            e.startTime.clear();
            e.endTime.clear();
            e.cps = 0;
            e.text.clear();
        }
    }

    model_.at(row).text = newText;

    // marcar dirty y actualizar UI
    dirty_ = true;
    UpdateWindowTitle();
    SetStatusText(wxString::Format("Edited row %d", row + 1));

    // También podemos refrescar la celda CPS/texto en el grid si necesitamos
    if (grid_ && row < grid_->GetNumberRows()) {
        grid_->SetCellValue(row, COL_TEXT, newText);
        // si tienes lógica para recomputar CPS basada en start/end -> actualizar
        // grid_->RefreshBlock(row, COL_CPS, row, COL_CPS);
    }
}

MainWindow::~MainWindow()
{
    if (renderer_) {
        delete renderer_;
        renderer_ = nullptr;
    }
}

void MainWindow::FillGridFromModel()
{
    suspendGridSelectionHandlers_ = true; // <-- prevenir handlers mientras rearmamos la grid

    grid_->Freeze();

    int existing = grid_->GetNumberRows();
    if (existing > 0)
        grid_->DeleteRows(0, existing);

    int n = static_cast<int>(model_.size());
    if (n > 0)
        grid_->AppendRows(n);

    for (int r = 0; r < n; ++r) {
        const SubtitleEntry& e = model_.at(r);
        grid_->SetCellValue(r, 0, wxString::Format("%d", e.lineNumber));
        grid_->SetCellValue(r, 1, e.startTime);
        grid_->SetCellValue(r, 2, e.endTime);
        grid_->SetCellValue(r, 3, wxString::Format("%d", e.cps));
        grid_->SetCellValue(r, 4, e.text);
    }

    // fixed widths for non-text columns
    grid_->SetColSize(0, 40);
    grid_->SetColSize(1, 110);
    grid_->SetColSize(2, 110);
    grid_->SetColSize(3, 90);

    // Recompute text column width using helper (no temporales).
    // Recompute text column width using helper (no temporales).
    TriggerSizeHandler();

    grid_->Thaw();
    suspendGridSelectionHandlers_ = false; // <-- habilitar handlers otra vez
    UpdateWindowTitle();

    // --- Asegurarnos que el sizer item que contiene la grid no tenga bordes ni flags de margen ---
    wxSizer* s = GetSizer();
    if (s) {
        wxSizerItemList& children = s->GetChildren();
        for (wxSizerItemList::iterator it = children.begin(); it != children.end(); ++it) {
            wxSizerItem* item = *it;
            if (!item) continue;
            if (item->GetWindow() == grid_) {
                // quitar cualquier border residual
                item->SetBorder(0);
                // limpiar flags de borde (wxLEFT, wxRIGHT, wxTOP, wxBOTTOM, wxALL)
                int flags = item->GetFlag();
                flags &= ~(wxLEFT | wxRIGHT | wxTOP | wxBOTTOM | wxALL);
                // asegurar wxEXPAND
                flags |= wxEXPAND;
                item->SetFlag(flags);
                // quitar min-size por si acaso
                item->SetMinSize(wxSize(-1, -1));
                break;
            }
        }
        // recalcular y aplicar layout final
        // aplicar layout final de forma segura (NO usar RecalcSizes en debug)
        s->Layout();

        // tambi�n asegurar que el padre del grid y el frame repinten su layout
        if (grid_->GetParent()) grid_->GetParent()->Layout();
        this->Layout();

    }

    // Recalcular columna Text una vez que el layout final fue aplicado
    TriggerSizeHandler();

    // refresh visual
    grid_->Refresh();
    grid_->Update();

}

void MainWindow::UpdateWindowTitle()
{
    wxString base = currentFilePath_.IsEmpty() ? wxString("Untitled") : wxFileName(currentFilePath_).GetFullName();
    wxString suffix = wxString(" - SubStudio 0.1.0");
    wxString prefix = dirty_ ? wxString("*") : wxString("");
    wxString title = prefix + base + suffix;
    SetTitle(title);
}

bool MainWindow::PromptSaveIfDirty()
{
    if (!dirty_) return true;

    wxString shown = currentFilePath_.IsEmpty() ? wxString("Untitled") : currentFilePath_;
    wxString question = wxString::Format("Do you want to save changes to %s?", shown.c_str());

    wxMessageDialog dlg(this,
        question,
        "Unsaved changes",
        wxYES_NO | wxCANCEL | wxICON_WARNING);
    dlg.SetYesNoCancelLabels("Yes", "No", "Cancel");
    dlg.SetAffirmativeId(wxID_YES);

    int res = dlg.ShowModal();
    if (res == wxID_YES) {
        return DoSave();
    }
    else if (res == wxID_NO) {
        return true;
    }
    return false; // Cancel
}

bool MainWindow::DoSave()
{
    if (currentFilePath_.IsEmpty()) return OnSaveAsInternal();

    std::ostringstream out;
    for (size_t i = 0; i < model_.size(); ++i) {
        const SubtitleEntry& e = model_[i];
        out << (i + 1) << "\n";
        out << std::string(e.startTime.mb_str()) << " --> " << std::string(e.endTime.mb_str()) << "\n";
        out << std::string(e.text.mb_str()) << "\n\n";
    }

    wxFFile f(currentFilePath_, "w");
    if (!f.IsOpened()) {
        wxMessageBox(wxString("Failed to open file for writing: ") + currentFilePath_, "Error", wxICON_ERROR);
        return false;
    }
    f.Write(out.str());
    f.Close();

    dirty_ = false;
    UpdateWindowTitle();
    SetStatusText("File saved");
    return true;
}

bool MainWindow::OnSaveAsInternal()
{
    wxFileDialog dlg(this, "Save subtitle file", "", "", "SubRip files (*.srt)|*.srt|All files (*.*)|*.*",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return false;
    currentFilePath_ = dlg.GetPath();
    return DoSave();
}

// --- Menu handlers
void MainWindow::OnOpen(wxCommandEvent& WXUNUSED(evt))
{
    if (!PromptSaveIfDirty()) return;

    wxFileDialog dlg(this, "Open subtitle file", "", "", "SubRip files (*.srt)|*.srt|All files (*.*)|*.*",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK) return;

    wxString path = dlg.GetPath();

    wxTextFile tf;
    if (!tf.Open(path)) {
        wxMessageBox(wxString("Failed to open file: ") + path, "Error", wxICON_ERROR);
        return;
    }

    model_.clear();
    SubtitleEntry cur;
    enum State { ExpectIndex, ExpectTime, ExpectText } state = ExpectIndex;

    for (size_t i = 0; i < tf.GetLineCount(); ++i) {
        wxString line = tf.GetLine(i);
        line = line.Trim(false).Trim(true);

        if (line.IsEmpty()) {
            if (state == ExpectText) {
                model_.push_back(cur);
                cur = SubtitleEntry();
                state = ExpectIndex;
            }
            continue;
        }

        if (state == ExpectIndex) {
            state = ExpectTime;
            continue;
        }
        else if (state == ExpectTime) {
            wxString l = line;
            wxString::size_type pos = l.find("-->");
            if (pos != wxString::npos) {
                wxString s1 = l.substr(0, pos).Trim(true).Trim(false);
                wxString s2 = l.substr(pos + 3).Trim(true).Trim(false);
                cur.startTime = s1;
                cur.endTime = s2;
            }
            else {
                cur.startTime = "";
                cur.endTime = "";
            }
            state = ExpectText;
        }
        else if (state == ExpectText) {
            if (!cur.text.IsEmpty()) cur.text += "\n";
            cur.text += line;
        }
    }
    if (!cur.text.IsEmpty() || !cur.startTime.IsEmpty() || !cur.endTime.IsEmpty()) {
        model_.push_back(cur);
    }

    currentFilePath_ = path;
    dirty_ = false;
    FillGridFromModel();
    SetStatusText("File loaded: " + currentFilePath_);
}

void MainWindow::OnSave(wxCommandEvent& WXUNUSED(evt))
{
    DoSave();
}

void MainWindow::OnSaveAs(wxCommandEvent& WXUNUSED(evt))
{
    OnSaveAsInternal();
}

void MainWindow::OnQuit(wxCommandEvent& WXUNUSED(evt))
{
    Close(true);
}

void MainWindow::OnAbout(wxCommandEvent& WXUNUSED(evt))
{
    wxMessageBox("SubStudio - simple subtitle editor", "About", wxOK | wxICON_INFORMATION);
}

void MainWindow::OnEditorText(wxCommandEvent& WXUNUSED(ev))
{
    int row = grid_->GetGridCursorRow();
    if (row < 0 || row >= static_cast<int>(model_.size())) {
        wxArrayInt rows = grid_->GetSelectedRows();
        if (!rows.empty()) row = rows[0];
        else return;
    }

    wxString newText = editor_->GetValue();
    model_.at(row).text = newText;
    dirty_ = true;
    UpdateWindowTitle();
    SetStatusText(wxString::Format("Edited row %d", row + 1));
}

void MainWindow::OnClose(wxCloseEvent& evt)
{
    if (!PromptSaveIfDirty()) {
        evt.Veto();
        return;
    }
    Destroy();
}

void MainWindow::OnSize(wxSizeEvent& evt)
{
    if (!grid_) {
        evt.Skip();
        return;
    }

    // Ensure sizers/layout updated first
    Layout();

    // Parent client width and grid X position: this avoids offsets por posicionamiento
    wxWindow* parent = grid_->GetParent();
    int parentClientW = parent ? parent->GetClientSize().GetWidth() : grid_->GetClientSize().GetWidth();
    int gridX = grid_->GetPosition().x; // position inside parent

    // Row label width (nros de fila)
    const int rowLabelW = grid_->GetRowLabelSize();

    // Determine if vertical scrollbar visible and its width
    int vScrollW = 0;
    {
        int totalRowsH = grid_->GetColLabelSize();
        const int nrows = grid_->GetNumberRows();
        for (int r = 0; r < nrows; ++r) totalRowsH += grid_->GetRowSize(r);
        if (totalRowsH > grid_->GetClientSize().GetHeight()) {
            vScrollW = wxSystemSettings::GetMetric(wxSYS_VSCROLL_X);
        }
    }

    // Sum widths of fixed columns (0..3) that are visible
    int sumFixed = 0;
    for (int c = 0; c <= 3; ++c) {
        if (grid_->IsColShown(c)) sumFixed += grid_->GetColSize(c);
    }

    // Compute available width for the text column relative to the parent right edge
    // parentClientW - gridX == distance from grid left to parent right edge.
    int available = parentClientW - gridX - rowLabelW - vScrollW - sumFixed;

    // Safety floor
    const int minTextWidth = 120;
    if (available < minTextWidth) available = minTextWidth;

    // Apply to column 4 if visible
    if (grid_->IsColShown(4)) {
        grid_->SetColSize(4, available);
    }

    // ensure layout and repaint
    Layout();
    grid_->Refresh();
    grid_->Update();

    evt.Skip();
}

// Helper to call OnSize without creating a temporary event
void MainWindow::TriggerSizeHandler()
{
    wxSizeEvent ev(GetClientSize(), GetId());
    ev.SetEventObject(this);
    OnSize(ev);
}
