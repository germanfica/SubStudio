// src/mainwindow.h
#pragma once

#ifndef SUBSTUDIO_MAINWINDOW_H_
#define SUBSTUDIO_MAINWINDOW_H_

// Evitar macros min/max de Windows que rompen std::min/std::max
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <wx/wx.h>
#include <wx/grid.h>
#include <memory>

#include "subtitle.h"
#include "main_menu.h"
#include "main_toolbar.h"
#include "grid_view.h"

// >>> IMPORTANTE: forward declaration de la clase derivada <<<
class SubstudioGrid;
class SubstudioEditBox;

class MainWindow : public wxFrame {
public:
    MainWindow();
    ~MainWindow() override;

private:
    // UI
    SubstudioGrid* grid_ = nullptr;
    SubstudioEditBox* edit_box_ = nullptr;
    wxTextCtrl* editor_ = nullptr;
    std::unique_ptr<MainMenu> menu_;
    std::unique_ptr<MainToolbar> toolbar_;
    std::unique_ptr<GridView> grid_view_;

    // Modelo
    Subtitle doc_;

    // Helpers
    void UpdateWindowTitle();
    bool PromptSaveIfDirty();
    bool DoSave();
    bool DoSaveAs();

    // Acciones
    void ActionOpen();
    void ActionSave();
    void ActionSaveAs();
    void ActionExit();
    void ActionAbout();

    // Eventos
    void OnEditorText(wxCommandEvent& ev);
    void OnClose(wxCloseEvent& ev);
    void OnSize(wxSizeEvent& ev);

    wxDECLARE_EVENT_TABLE();
};

#endif  // SUBSTUDIO_MAINWINDOW_H_
