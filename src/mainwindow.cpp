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

// Event table
wxBEGIN_EVENT_TABLE(MainWindow, wxFrame)
EVT_MENU(MainWindow::ID_Open, MainWindow::OnOpen)
EVT_MENU(MainWindow::ID_Save, MainWindow::OnSave)
EVT_MENU(MainWindow::ID_SaveAs, MainWindow::OnSaveAs)
EVT_MENU(wxID_EXIT, MainWindow::OnQuit)
EVT_MENU(wxID_ABOUT, MainWindow::OnAbout)
EVT_GRID_LABEL_RIGHT_CLICK(MainWindow::OnGridLabelRightClick)
EVT_GRID_SELECT_CELL(MainWindow::OnGridSelectCell)
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

    // Editor - multiline text to edit selected subtitle text
    // moved ABOVE the grid per request
    editor_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, 120), wxTE_MULTILINE);
    topSizer->Add(editor_, 0, wxEXPAND, 0);

    // Grid: 5 columns (Line, Start, End, CPS, Text)
    grid_ = new wxGrid(this, wxID_ANY);
    grid_->CreateGrid(0, 5);
    grid_->SetCellHighlightPenWidth(1);

    // --- colors ---
    if (wxWindow* w = grid_->GetGridColLabelWindow()) w->SetBackgroundColour(topBarCol);
    if (wxWindow* w = grid_->GetGridRowLabelWindow()) w->SetBackgroundColour(rowLabelCol);

    grid_->SetSelectionBackground(selBgCol);
    grid_->SetSelectionForeground(*wxBLACK);

    // Set column labels
    grid_->SetMargins(0, 0);
    for (int c = 0; c < 5; ++c) {
        if (c == 3) {
            grid_->SetColLabelValue(c, "Characters Per Second");
        }
        else {
            grid_->SetColLabelValue(c, COL_LABELS[c]);
        }
    }

    grid_->SetColLabelValue(0, "#");
    grid_->SetColLabelValue(1, "Start");
    grid_->SetColLabelValue(2, "End");
    grid_->SetColLabelValue(3, "CPS");
    grid_->SetColLabelValue(4, "Text");

    //grid_->SetColLabelAlignment(wxALIGN_LEFT, wxALIGN_CENTRE);

    // Prevent user from editing grid cells directly
    grid_->EnableEditing(false);

    // Selection mode
    grid_->SetSelectionMode(wxGrid::wxGridSelectCells);
    grid_->Bind(wxEVT_KEY_DOWN, &MainWindow::OnGridKeyDown, this);
    grid_->Bind(wxEVT_GRID_CELL_LEFT_CLICK, &MainWindow::OnGridCellLeftClick, this);
    grid_->Bind(wxEVT_GRID_LABEL_LEFT_CLICK, &MainWindow::OnGridLabelLeftClick, this);
    grid_->Bind(wxEVT_GRID_RANGE_SELECT, &MainWindow::OnGridRangeSelect, this);
    wxWindow* gridWin = grid_->GetGridWindow();
    gridWin->Bind(wxEVT_LEFT_DOWN, &MainWindow::OnGridMouseLeftDown, this);
    gridWin->Bind(wxEVT_LEFT_UP, &MainWindow::OnGridMouseLeftUp, this);
    gridWin->Bind(wxEVT_MOTION, &MainWindow::OnGridMouseMotion, this);
    gridWin->Bind(wxEVT_LEFT_DCLICK, &MainWindow::OnGridMouseLeftDClick, this);

    // Disable user resizing/moving of columns and rows
    grid_->EnableDragColSize(false);
    grid_->EnableDragRowSize(false);
    grid_->EnableDragColMove(false);

    // Prevent user from resizing labels area
    // wxWidgets 3.2.8: SetColMinimalWidth takes (col, width).
    // Set a minimum for each column (reasonable default).
    const int minColWidth = 20;
    for (int c = 0; c < grid_->GetNumberCols(); ++c)
        grid_->SetColMinimalWidth(c, minColWidth);

    // Set default row height for all rows
    grid_->SetDefaultRowSize(18, true); // height, resize existing rows

    // Important: do NOT allow hiding of Text column via default actions.
    // We'll provide a custom header-right-click menu to hide/show columns.

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
    grid_->SetColSize(0, 40);   // '#'
    grid_->SetColSize(1, 110);  // Start Time
    grid_->SetColSize(2, 110);  // End Time
    grid_->SetColSize(3, 90);   // CPS

    // adjust text column to fill the remaining area
    Layout();
    wxYieldIfNeeded();
    // No temporales: usar helper que crea un wxSizeEvent nombrado
    TriggerSizeHandler();
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

// Grid events
void MainWindow::OnGridLabelRightClick(wxGridEvent& ev)
{
    // Show a context menu allowing hide/show columns.
    // 'Text' column cannot be hidden (disabled).
    wxMenu menu;

    const int colCount = 5;
    for (int c = 0; c < colCount; ++c) {
        int id = ID_ToggleColBase + c;
        wxString label = COL_LABELS[c];
        bool shown = grid_->IsColShown(c);
        wxMenuItem* mi = menu.AppendCheckItem(id, label);
        mi->Check(shown);

        if (c == 4) {
            mi->Enable(false);
            mi->Check(true); // always checked
        }

        Bind(wxEVT_MENU, &MainWindow::OnToggleColumn, this, id);
    }

    PopupMenu(&menu);
    ev.Skip();
}

void MainWindow::OnToggleColumn(wxCommandEvent& evt)
{
    int id = evt.GetId();
    int col = id - ID_ToggleColBase;
    if (col < 0 || col >= 5) return;

    // Ensure Text column (4) cannot be hidden - extra safety
    if (col == 4) return;

    bool currentlyShown = grid_->IsColShown(col);
    if (currentlyShown) grid_->HideCol(col);
    else grid_->ShowCol(col);

    // After changing visibility, recompute text width to fill remaining space
    TriggerSizeHandler();
}

void MainWindow::OnGridSelectCell(wxGridEvent& ev)
{
    int row = ev.GetRow();
    if (row < 0 || row >= static_cast<int>(model_.size())) {
        ev.Skip();
        return;
    }
    editor_->ChangeValue(model_.at(row).text);
    ev.Skip();
}

void MainWindow::OnGridKeyDown(wxKeyEvent& ev)
{
    int key = ev.GetKeyCode();
    int row = grid_->GetGridCursorRow();
    int col = grid_->GetGridCursorCol();
    int nrows = grid_->GetNumberRows();
    int ncols = grid_->GetNumberCols();

    if (nrows <= 0) { ev.Skip(); return; }

    // manejar izquierda / derecha: mover el cursor de columna pero
    // mantener la seleccion de la fila
    if (key == WXK_LEFT || key == WXK_RIGHT) {
        int newcol = col;
        if (key == WXK_LEFT) newcol = std::max(0, col - 1);
        else newcol = std::min(ncols - 1, col + 1);

        suspendGridSelectionHandlers_ = true;

        if (mousePainting_) {
            // si estamos pintando: seleccion de celda dentro de la misma fila (y expandir bloque si queremos)
            // mantenemos el paintStartCol_ si existe, sino seleccionamos solo la celda
            int startCol = (paintStartCol_ >= 0) ? paintStartCol_ : newcol;
            int top = std::min(grid_->GetGridCursorRow(), grid_->GetGridCursorRow());
            int bottom = top;
            int left = std::min(startCol, newcol);
            int right = std::max(startCol, newcol);
            int r = grid_->GetGridCursorRow();
            grid_->SelectBlock(top, left, bottom, right);
            grid_->SetGridCursor(r, newcol);
            grid_->MakeCellVisible(r, newcol);
        }
        else {
            // comportamiento anterior: seleccionar toda la fila y mover el cursor de columna
            grid_->SelectRow(row);
            grid_->SetGridCursor(row, newcol);
            grid_->MakeCellVisible(row, newcol);
        }

        grid_->SetFocus();
        suspendGridSelectionHandlers_ = false;

        return; // manejado
    }

    // resto: up/down/home/end/page - calculamos la nueva fila pero preservamos columna
    int newRow = row;
    if (key == WXK_UP) {
        newRow = std::max(0, row - 1);
    }
    else if (key == WXK_DOWN) {
        newRow = std::min(nrows - 1, row + 1);
    }
    else if (key == WXK_HOME) {
        newRow = 0;
    }
    else if (key == WXK_END) {
        newRow = nrows - 1;
    }
    else if (key == WXK_PAGEUP) {
        int per = grid_->GetClientSize().GetHeight() / grid_->GetDefaultRowSize();
        newRow = std::max(0, row - std::max(1, per));
    }
    else if (key == WXK_PAGEDOWN) {
        int per = grid_->GetClientSize().GetHeight() / grid_->GetDefaultRowSize();
        newRow = std::min(nrows - 1, row + std::max(1, per));
    }
    else {
        ev.Skip();
        return;
    }

    // preservamos la columna actual (si es inválida usamos 0)
    int curCol = col;
    if (curCol < 0 || curCol >= ncols) curCol = 0;

    // aplicar selección y cursor sin que SelectRow nos reseteé la columna
    suspendGridSelectionHandlers_ = true;
    grid_->SelectRow(newRow);
    grid_->SetGridCursor(newRow, curCol);
    grid_->MakeCellVisible(newRow, curCol);
    grid_->SetFocus();
    suspendGridSelectionHandlers_ = false;

    // actualizar editor si corresponde
    if (newRow >= 0 && newRow < static_cast<int>(model_.size())) {
        editor_->ChangeValue(model_.at(newRow).text);
    }

    // no ev.Skip() para evitar manejo por defecto que pueda interferir
}

void MainWindow::OnGridCellLeftClick(wxGridEvent& ev)
{
    if (suspendGridSelectionHandlers_) { ev.Skip(); return; }

    const int row = ev.GetRow();
    const int col = ev.GetCol();

    if (row < 0 || row >= grid_->GetNumberRows() || col < 0 || col >= grid_->GetNumberCols()) {
        ev.Skip();
        return;
    }

    // Ejecutar despu�s para que el procesamiento por defecto termine
    grid_->CallAfter([this, row, col]() {
        if (!grid_) return;
        suspendGridSelectionHandlers_ = true;

        // SelectRow primero (para marcar la fila),
        // luego fijar el cursor a la columna clickeada para que las flechas funcionen.
        grid_->SelectRow(row);
        grid_->SetGridCursor(row, col);
        grid_->MakeCellVisible(row, col);
        grid_->SetFocus();

        if (row < static_cast<int>(model_.size()) && editor_)
            editor_->ChangeValue(model_.at(row).text);

        suspendGridSelectionHandlers_ = false;
        });

    // NO ev.Skip() � evitamos que el manejo por defecto sobrescriba nuestra selecci�n
}

void MainWindow::OnGridLabelLeftClick(wxGridEvent& ev)
{
    if (suspendGridSelectionHandlers_) { ev.Skip(); return; }

    const int row = ev.GetRow();
    if (row < 0 || row >= grid_->GetNumberRows()) {
        ev.Skip();
        return;
    }

    grid_->CallAfter([this, row]() {
        if (!grid_) return;
        suspendGridSelectionHandlers_ = true;

        // Si el usuario clickea en la etiqueta ponemos el cursor en la
        // columna actual del grid si es v�lida, sino en 0.
        int curCol = grid_->GetGridCursorCol();
        if (curCol < 0 || curCol >= grid_->GetNumberCols()) curCol = 0;

        grid_->SelectRow(row);
        grid_->SetGridCursor(row, curCol);
        grid_->MakeCellVisible(row, curCol);
        grid_->SetFocus();

        if (row < static_cast<int>(model_.size()) && editor_)
            editor_->ChangeValue(model_.at(row).text);

        suspendGridSelectionHandlers_ = false;
        });

    // NO ev.Skip()
}

void MainWindow::OnGridRangeSelect(wxGridRangeSelectEvent& ev)
{
    if (suspendGridSelectionHandlers_) { ev.Skip(); return; }

    if (!ev.Selecting()) {
        ev.Skip();
        return;
    }

    const int top = ev.GetTopRow();
    if (top < 0 || top >= grid_->GetNumberRows()) {
        ev.Skip();
        return;
    }

    // intentar obtener la columna izquierda del rango; fallback a la columna actual o 0
    int leftCol = -1;
#if wxCHECK_VERSION(3,1,0)
    leftCol = ev.GetLeftCol();
#endif
    if (leftCol < 0) leftCol = grid_->GetGridCursorCol();
    if (leftCol < 0 || leftCol >= grid_->GetNumberCols()) leftCol = 0;

    grid_->CallAfter([this, top, leftCol]() {
        if (!grid_) return;
        suspendGridSelectionHandlers_ = true;

        grid_->SelectRow(top);
        grid_->SetGridCursor(top, leftCol);
        grid_->MakeCellVisible(top, leftCol);
        grid_->SetFocus();

        if (top < static_cast<int>(model_.size()) && editor_)
            editor_->ChangeValue(model_.at(top).text);

        suspendGridSelectionHandlers_ = false;
        });

    ev.Skip(); // dejamos que wx haga la selecci�n visual del rango
}

// empieza el "pintado" al presionar bot�n izquierdo
// empieza el "pintado" al presionar boton izquierdo
void MainWindow::OnGridMouseLeftDown(wxMouseEvent& ev)
{
    mousePainting_ = true;
    lastPaintRow_ = lastPaintCol_ = -1;

    wxWindow* gw = grid_->GetGridWindow();
    if (!gw) return;

    if (!gw->HasCapture()) gw->CaptureMouse();

    wxPoint p = ev.GetPosition(); // relativo al grid window
    wxGridCellCoords coords = grid_->XYToCell(p.x, p.y);
    int row = coords.GetRow();
    int col = coords.GetCol();

    if (!(row >= 0 && col >= 0 && row < grid_->GetNumberRows() && col < grid_->GetNumberCols())) {
        // celda inválida -> no iniciamos painting
        paintStartRow_ = paintStartCol_ = -1;
        return;
    }

    // iniciamos el rango desde la fila en la que tocamos
    paintStartRow_ = row;
    paintStartCol_ = col; // guardamos la columna inicial por si querés usarla luego

    // seleccionar el bloque inicial (start..start), usaremos columnas visibles
    int ncols = grid_->GetNumberCols();
    int firstVisible = 0, lastVisible = ncols - 1;
    bool found = false;
    for (int c = 0; c < ncols; ++c) {
        if (grid_->IsColShown(c)) { firstVisible = c; found = true; break; }
    }
    if (found) {
        for (int c = ncols - 1; c >= 0; --c) {
            if (grid_->IsColShown(c)) { lastVisible = c; break; }
        }
    }
    else { firstVisible = 0; lastVisible = ncols - 1; }

    suspendGridSelectionHandlers_ = true;
    grid_->SelectBlock(row, firstVisible, row, lastVisible); // selecciona la fila completa (visibles)
    grid_->SetGridCursor(row, col);
    grid_->MakeCellVisible(row, col);
    grid_->SetFocus();
    suspendGridSelectionHandlers_ = false;

    if (row < static_cast<int>(model_.size()) && editor_)
        editor_->ChangeValue(model_.at(row).text);

    lastPaintRow_ = row;
    lastPaintCol_ = col;
}


// terminar el "pintado" al soltar el bot�n
// terminar el "pintado" al soltar el boton
void MainWindow::OnGridMouseLeftUp(wxMouseEvent& ev)
{
    mousePainting_ = false;

    wxWindow* gw = grid_->GetGridWindow();
    if (gw && gw->HasCapture()) gw->ReleaseMouse();

    wxPoint p = ev.GetPosition();
    wxGridCellCoords coords = grid_->XYToCell(p.x, p.y);
    int row = coords.GetRow();
    int col = coords.GetCol();

    if (row >= 0 && col >= 0 && row < grid_->GetNumberRows() && col < grid_->GetNumberCols()) {
        // determinamos columnas visibles
        int ncols = grid_->GetNumberCols();
        int firstVisible = 0, lastVisible = ncols - 1;
        bool found = false;
        for (int c = 0; c < ncols; ++c) {
            if (grid_->IsColShown(c)) { firstVisible = c; found = true; break; }
        }
        if (found) {
            for (int c = ncols - 1; c >= 0; --c) {
                if (grid_->IsColShown(c)) { lastVisible = c; break; }
            }
        }
        else { firstVisible = 0; lastVisible = ncols - 1; }

        int top = row, bottom = row;
        if (paintStartRow_ >= 0) {
            top = std::min(paintStartRow_, row);
            bottom = std::max(paintStartRow_, row);
        }

        suspendGridSelectionHandlers_ = true;
        grid_->SelectBlock(top, firstVisible, bottom, lastVisible);
        grid_->SetGridCursor(row, col);
        grid_->MakeCellVisible(row, col);
        grid_->SetFocus();
        suspendGridSelectionHandlers_ = false;

        if (top < static_cast<int>(model_.size()) && editor_)
            editor_->ChangeValue(model_.at(top).text);

        lastPaintRow_ = row;
        lastPaintCol_ = col;
    }

    // reseteamos la fila de inicio
    paintStartRow_ = paintStartCol_ = -1;
}

// movimiento del mouse: actualiza la seleccion en tiempo real mientras pintamos
void MainWindow::OnGridMouseMotion(wxMouseEvent& ev)
{
    if (!mousePainting_ || !ev.Dragging() || !ev.LeftIsDown()) {
        ev.Skip();
        return;
    }

    wxPoint p = ev.GetPosition();
    wxGridCellCoords coords = grid_->XYToCell(p.x, p.y);
    int row = coords.GetRow();
    int col = coords.GetCol();

    if (!(row >= 0 && col >= 0 && row < grid_->GetNumberRows() && col < grid_->GetNumberCols())) {
        // fuera de rango -> podrías implementar autoscroll aquí
        return;
    }

    // si no cambió nada, evitamos trabajo
    if (row == lastPaintRow_ && col == lastPaintCol_) return;

    // determinamos columnas visibles (primer y último índice mostrado)
    int ncols = grid_->GetNumberCols();
    int firstVisible = 0, lastVisible = ncols - 1;
    bool found = false;
    for (int c = 0; c < ncols; ++c) {
        if (grid_->IsColShown(c)) { firstVisible = c; found = true; break; }
    }
    if (found) {
        for (int c = ncols - 1; c >= 0; --c) {
            if (grid_->IsColShown(c)) { lastVisible = c; break; }
        }
    }
    else {
        firstVisible = 0; lastVisible = ncols - 1;
    }

    // si tenemos fila de inicio válida, seleccionamos el bloque desde start hasta row
    int top = row, bottom = row;
    if (paintStartRow_ >= 0) {
        top = std::min(paintStartRow_, row);
        bottom = std::max(paintStartRow_, row);
    }

    suspendGridSelectionHandlers_ = true;
    grid_->SelectBlock(top, firstVisible, bottom, lastVisible);
    // pero el cursor sigue la celda bajo el mouse (permite movimiento lateral)
    grid_->SetGridCursor(row, col);
    grid_->MakeCellVisible(row, col);
    grid_->SetFocus();
    suspendGridSelectionHandlers_ = false;

    if (top < static_cast<int>(model_.size()) && editor_) {
        // mostramos el texto de la fila superior del bloque (podés cambiar a otra fila si querés)
        editor_->ChangeValue(model_.at(top).text);
    }

    lastPaintRow_ = row;
    lastPaintCol_ = col;
}

void MainWindow::OnGridMouseLeftDClick(wxMouseEvent& ev)
{
    // Evitamos que el double-click lance comportamientos por defecto o ediciones
    // Simplemente reproducimos el comportamiento de un left-down (iniciar paint),
    // pero evitamos hacer cosas si no hay una celda válida.
    OnGridMouseLeftDown(ev);
    // No ev.Skip() para evitar comportamiento por defecto del grid
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
