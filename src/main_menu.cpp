// src/main_menu.cpp
#include "main_menu.h"

#include <wx/artprov.h>

// Constructor: crea el wxMenuBar localmente y lo asigna al frame.
// El frame pasa a ser el dueño del wxMenuBar (SetMenuBar toma ownership).
MainMenu::MainMenu(wxFrame* frame, const MenuActions& actions)
    : frame_(frame), actions_(actions) {
    // Crear menubar local
    wxMenuBar* bar = new wxMenuBar();

    auto* file = new wxMenu();
    file->Append(wxID_OPEN, "&Open...\tCtrl-O");
    file->Append(wxID_SAVE, "&Save\tCtrl-S");
    file->Append(wxID_SAVEAS, "Save &As...\tCtrl-Shift-S");
    file->AppendSeparator();
    file->Append(wxID_EXIT, "E&xit");

    auto* help = new wxMenu();
    help->Append(wxID_ABOUT, "&About");

    bar->Append(file, "&File");
    bar->Append(help, "&Help");

    // Transferimos ownership al frame (wxFrame se encargará de destruirlo).
    if (frame_) {
        frame_->SetMenuBar(bar);
    }
    else {
        // En caso extremo de que no haya frame, evitar leak.
        delete bar;
        bar = nullptr;
    }

    BindHandlers();
}

MainMenu::~MainMenu() {
    // No borramos ni gestionamos el wxMenuBar aquí: el wxFrame es el dueño.
    // Dejamos el destructor por si en el futuro queremos anular bindings explícitos.
}

// Bind de handlers al frame (desacoplado del puntero al menu).
void MainMenu::BindHandlers() {
    if (!frame_) return;

    frame_->Bind(wxEVT_MENU, [this](wxCommandEvent&) { if (actions_.on_open) actions_.on_open(); }, wxID_OPEN);
    frame_->Bind(wxEVT_MENU, [this](wxCommandEvent&) { if (actions_.on_save) actions_.on_save(); }, wxID_SAVE);
    frame_->Bind(wxEVT_MENU, [this](wxCommandEvent&) { if (actions_.on_save_as) actions_.on_save_as(); }, wxID_SAVEAS);
    frame_->Bind(wxEVT_MENU, [this](wxCommandEvent&) { if (actions_.on_exit) actions_.on_exit(); }, wxID_EXIT);
    frame_->Bind(wxEVT_MENU, [this](wxCommandEvent&) { if (actions_.on_about) actions_.on_about(); }, wxID_ABOUT);
}
