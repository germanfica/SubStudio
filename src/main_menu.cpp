#include "main_menu.h"

#include <wx/artprov.h>

MainMenu::MainMenu(wxFrame* frame, const MenuActions& actions)
    : frame_(frame), actions_(actions) {
    // Creamos el menubar y lo asignamos al frame. El frame será dueño del menubar.
    bar_ = new wxMenuBar();

    auto* file = new wxMenu();
    file->Append(wxID_OPEN, "&Open...\tCtrl-O");
    file->Append(wxID_SAVE, "&Save\tCtrl-S");
    file->Append(wxID_SAVEAS, "Save &As...\tCtrl-Shift-S");
    file->AppendSeparator();
    file->Append(wxID_EXIT, "E&xit");

    auto* help = new wxMenu();
    help->Append(wxID_ABOUT, "&About");

    bar_->Append(file, "&File");
    bar_->Append(help, "&Help");

    // Transferimos ownership al frame (SetMenuBar toma el puntero y wxFrame lo borra)
    frame_->SetMenuBar(bar_);

    BindHandlers();
}

void MainMenu::BindHandlers() {
    frame_->Bind(wxEVT_MENU, [this](wxCommandEvent&) { if (actions_.on_open) actions_.on_open(); }, wxID_OPEN);
    frame_->Bind(wxEVT_MENU, [this](wxCommandEvent&) { if (actions_.on_save) actions_.on_save(); }, wxID_SAVE);
    frame_->Bind(wxEVT_MENU, [this](wxCommandEvent&) { if (actions_.on_save_as) actions_.on_save_as(); }, wxID_SAVEAS);
    frame_->Bind(wxEVT_MENU, [this](wxCommandEvent&) { if (actions_.on_exit) actions_.on_exit(); }, wxID_EXIT);
    frame_->Bind(wxEVT_MENU, [this](wxCommandEvent&) { if (actions_.on_about) actions_.on_about(); }, wxID_ABOUT);
}
