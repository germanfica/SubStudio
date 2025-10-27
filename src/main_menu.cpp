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
    // No dejamos bindings que dependan de 'this' (los handlers están
    // capturando 'actions_' por valor), así que no hace falta Unbind.
}

// Bind de handlers al frame (capturamos una copia de actions_ para evitar
// que los handlers dependan del 'this' de MainMenu).
void MainMenu::BindHandlers() {
    if (!frame_) return;

    // Copiamos actions_ localmente para las lambdas: así las funciones
    // registradas en el frame no usarán 'this' ni members de MainMenu.
    const MenuActions actions = actions_;

    frame_->Bind(wxEVT_MENU, [actions](wxCommandEvent&) { if (actions.on_open)  actions.on_open(); }, wxID_OPEN);
    frame_->Bind(wxEVT_MENU, [actions](wxCommandEvent&) { if (actions.on_save)  actions.on_save(); }, wxID_SAVE);
    frame_->Bind(wxEVT_MENU, [actions](wxCommandEvent&) { if (actions.on_save_as)  actions.on_save_as(); }, wxID_SAVEAS);
    frame_->Bind(wxEVT_MENU, [actions](wxCommandEvent&) { if (actions.on_exit)  actions.on_exit(); }, wxID_EXIT);
    frame_->Bind(wxEVT_MENU, [actions](wxCommandEvent&) { if (actions.on_about)  actions.on_about(); }, wxID_ABOUT);
}
