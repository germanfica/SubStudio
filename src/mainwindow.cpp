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
#include "srt_io.h"

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
    renderer_(nullptr)
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
    grid_ = new SubstudioGrid(this, wxID_ANY);
    grid_->SetCellHighlightPenWidth(1);
    grid_->SetRowLabelSize(0);

    // Prevent direct editing
    grid_->EnableEditing(false);

    // Disable drag-to-resize and column/row moving
    grid_->EnableDragColSize(false);
    grid_->EnableDragRowSize(false);
    grid_->EnableDragColMove(false);

    // Default row height
    grid_->SetDefaultRowSize(18, true);
    if (wxWindow* wrl = grid_->GetGridRowLabelWindow()) wrl->Hide();

    // --- Context for SubstudioEditBox ---
    SubstudioContext ctx;
    ctx.grid = grid_;

    // Notification of 'dirty' from the editor
    ctx.notify_dirty = [this]() {
        if (!subtitle_.dirty()) {
            subtitle_.set_dirty(true);
            UpdateWindowTitle();
        }
        };

    // Text editor above
    editBox_ = new SubstudioEditBox(this, ctx, wxID_ANY, wxDefaultPosition, wxSize(-1, 120));
    editor_ = editBox_->GetTextCtrl(); // accessor provided by SubstudioEditBox
    if (editor_) grid_->BindExternalEditor(editor_);
    Bind(EVT_SUBSTUDIO_COMMIT_TEXT, &MainWindow::OnSubstudioEditCommit, this, editBox_->GetId());

    editBox_->SetMinSize(wxSize(-1, FromDIP(120)));
    topSizer->Add(editBox_, 0, wxEXPAND, 0);
    topSizer->Add(grid_, 1, wxEXPAND, 0);

    SetSizer(topSizer);
    SetClientSize(906, 693);
    CentreOnScreen();

    // CPS renderer (if you implement CpsRenderer)
    renderer_ = nullptr;
    // example: wxGridCellAttr* attr = new wxGridCellAttr(); attr->SetRenderer(renderer_); grid_->SetColAttr(3, attr);

    // Initial state
    subtitle_.Clear();
    UpdateWindowTitle();

    // Initial adjustment of the text column
    Layout();
    wxYieldIfNeeded();
    TriggerSizeHandler();
}

// Handler for SubstudioEditBox commit
void MainWindow::OnSubstudioEditCommit(wxCommandEvent& evt)
{
    const int row = evt.GetInt();         // row where the commit occurred
    const wxString newText = evt.GetString();
    if (row < 0) return;

    subtitle_.SetRowText(static_cast<size_t>(row), newText);

    // Refresh grid if the row exists
    if (grid_ && row < grid_->GetNumberRows()) {
        grid_->SetCellValue(row, 4 /* Text */, newText);
    }

    UpdateWindowTitle();
    SetStatusText(wxString::Format("Edited row %d", row + 1));
}

MainWindow::~MainWindow()
{
    if (renderer_) {
        delete renderer_;
        renderer_ = nullptr;
    }
}

void MainWindow::FillGridFromSubtitle()
{
    suspendGridSelectionHandlers_ = true;

    grid_->Freeze();

    int existing = grid_->GetNumberRows();
    if (existing > 0)
        grid_->DeleteRows(0, existing);

    const auto& rows = subtitle_.entries();
    int n = static_cast<int>(rows.size());
    if (n > 0)
        grid_->AppendRows(n);

    for (int r = 0; r < n; ++r) {
        const SubtitleEntry& e = rows[static_cast<size_t>(r)];
        grid_->SetCellValue(r, 0, wxString::Format("%d", e.line_number));
        grid_->SetCellValue(r, 1, e.start_time);
        grid_->SetCellValue(r, 2, e.end_time);
        grid_->SetCellValue(r, 3, wxString::Format("%d", e.cps));
        grid_->SetCellValue(r, 4, e.text);
    }

    // Fixed widths for non-text columns
    grid_->SetColSize(0, 40);
    grid_->SetColSize(1, 110);
    grid_->SetColSize(2, 110);
    grid_->SetColSize(3, 90);

    TriggerSizeHandler();

    grid_->Thaw();
    suspendGridSelectionHandlers_ = false;
    UpdateWindowTitle();

    // Ensure there are no extra borders/margins on the sizer item that contains the grid
    if (wxSizer* s = GetSizer()) {
        wxSizerItemList& children = s->GetChildren();
        for (auto it = children.begin(); it != children.end(); ++it) {
            wxSizerItem* item = *it;
            if (!item) continue;
            if (item->GetWindow() == grid_) {
                item->SetBorder(0);
                int flags = item->GetFlag();
                flags &= ~(wxLEFT | wxRIGHT | wxTOP | wxBOTTOM | wxALL);
                flags |= wxEXPAND;
                item->SetFlag(flags);
                item->SetMinSize(wxSize(-1, -1));
                break;
            }
        }
        s->Layout();
        if (grid_->GetParent()) grid_->GetParent()->Layout();
        this->Layout();
    }

    TriggerSizeHandler();

    grid_->Refresh();
    grid_->Update();
}

void MainWindow::UpdateWindowTitle()
{
    wxString base = subtitle_.path().IsEmpty() ? wxString("Untitled") : wxFileName(subtitle_.path()).GetFullName();
    wxString suffix = wxString(" - SubStudio 0.1.0");
    wxString prefix = subtitle_.dirty() ? wxString("*") : wxString("");
    wxString title = prefix + base + suffix;
    SetTitle(title);
}

bool MainWindow::PromptSaveIfDirty()
{
    if (!subtitle_.dirty()) return true;

    wxString shown = subtitle_.path().IsEmpty() ? wxString("Untitled") : subtitle_.path();
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
    if (subtitle_.path().IsEmpty()) return OnSaveAsInternal();

    subtitle_.ResequenceLineNumbers();
    if (!srt::Save(subtitle_.path(), subtitle_.entries())) {
        wxMessageBox(wxString("Failed to open file for writing: ") + subtitle_.path(), "Error", wxICON_ERROR);
        return false;
    }

    subtitle_.set_dirty(false);
    UpdateWindowTitle();
    SetStatusText("File saved");
    return true;
}

bool MainWindow::OnSaveAsInternal()
{
    wxFileDialog dlg(this, "Save subtitle file", "", "", "SubRip files (*.srt)|*.srt|All files (*.*)|*.*",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return false;
    subtitle_.set_path(dlg.GetPath());
    return DoSave();
}

// --- Menu handlers
void MainWindow::OnOpen(wxCommandEvent& WXUNUSED(evt))
{
    if (!PromptSaveIfDirty()) return;

    wxFileDialog dlg(this, "Open subtitle file", "", "", "SubRip files (*.srt)|*.srt|All files (*.*)|*.*",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK) return;

    std::vector<SubtitleEntry> loaded;
    if (!srt::Load(dlg.GetPath(), loaded)) {
        wxMessageBox(wxString("Failed to open file: ") + dlg.GetPath(), "Error", wxICON_ERROR);
        return;
    }

    subtitle_.entries() = std::move(loaded);
    subtitle_.set_path(dlg.GetPath());
    subtitle_.set_dirty(false);

    FillGridFromSubtitle();
    SetStatusText("File loaded: " + subtitle_.path());
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
    if (row < 0 || row >= static_cast<int>(subtitle_.entries().size())) {
        wxArrayInt rows = grid_->GetSelectedRows();
        if (!rows.empty()) row = rows[0];
        else return;
    }

    wxString newText = editor_->GetValue();
    subtitle_.SetRowText(static_cast<size_t>(row), newText);
    grid_->SetCellValue(row, 4 /* Text */, newText);

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

    // First update the layout
    Layout();

    // Parent client width and grid X position
    wxWindow* parent = grid_->GetParent();
    int parentClientW = parent ? parent->GetClientSize().GetWidth() : grid_->GetClientSize().GetWidth();
    int gridX = grid_->GetPosition().x;

    // Width of row labels
    const int rowLabelW = grid_->GetRowLabelSize();

    // Vertical scrollbar
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
