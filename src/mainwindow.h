// src/mainwindow.h
#pragma once

// Evitar macros min/max de Windows que rompen std::min/std::max
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <wx/wx.h>
#include <wx/grid.h>
//#include "substudiogrid.h" // no hay dependencia circular, asi que se puede usar sin problema
#include <vector>

// >>> IMPORTANTE: forward declaration de la clase derivada <<<
class SubstudioGrid;
class SubstudioEditBox;

struct SubtitleEntry {
    int lineNumber = 0;
    wxString startTime;
    wxString endTime;
    int cps = 0;
    wxString text;
};

class CpsRenderer; // forward

class MainWindow : public wxFrame
{
public:
    MainWindow();
    ~MainWindow();

private:
    // UI
    SubstudioGrid* grid_ = nullptr;   // ahora el compilador ya conoce el tipo
    wxTextCtrl* editor_ = nullptr;
    SubstudioEditBox* editBox_ = nullptr;
    wxToolBar* toolbar_ = nullptr;
    CpsRenderer* renderer_ = nullptr;

    // Model
    std::vector<SubtitleEntry> model_;
    wxString currentFilePath_;
    bool dirty_ = false;

    // IDs
    enum {
        ID_Open = wxID_HIGHEST + 1,
        ID_Save,
        ID_SaveAs,
        ID_ToggleColBase = 3000,
        ID_ToolOpen,
        ID_ToolSave
    };

    // Helpers
    void FillGridFromModel();
    void UpdateWindowTitle();
    bool PromptSaveIfDirty();
    bool DoSave();
    bool OnSaveAsInternal();
    void TriggerSizeHandler();
    bool suspendGridSelectionHandlers_ = false;
    bool mousePainting_ = false;
    int lastPaintRow_ = -1;
    int lastPaintCol_ = -1;
    int paintStartRow_ = -1;
    int paintStartCol_ = -1;

    // Events
    void OnOpen(wxCommandEvent& evt);
    void OnSave(wxCommandEvent& evt);
    void OnSaveAs(wxCommandEvent& evt);
    void OnQuit(wxCommandEvent& evt);
    void OnAbout(wxCommandEvent& evt);

    void OnGridLabelRightClick(wxGridEvent& ev);
    void OnToggleColumn(wxCommandEvent& evt);
    void OnGridSelectCell(wxGridEvent& ev);
    void OnGridKeyDown(wxKeyEvent& ev);
    void OnGridCellLeftClick(wxGridEvent& ev);
    void OnGridLabelLeftClick(wxGridEvent& ev);
    void OnGridRangeSelect(wxGridRangeSelectEvent& ev);
    void OnGridMouseLeftDown(wxMouseEvent& ev);
    void OnGridMouseLeftUp(wxMouseEvent& ev);
    void OnGridMouseMotion(wxMouseEvent& ev);
    void OnGridMouseLeftDClick(wxMouseEvent& ev);

    void OnEditorText(wxCommandEvent& evt);
    void OnSubstudioEditCommit(wxCommandEvent& evt);

    void OnClose(wxCloseEvent& evt);
    void OnSize(wxSizeEvent& evt);

    wxDECLARE_EVENT_TABLE();
};
