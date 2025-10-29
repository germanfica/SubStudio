#include <wx/wx.h>
#include "MainWindow.h"

class SubStudioApp : public wxApp {
public:
    bool OnInit() override {
        if (!wxApp::OnInit()) return false;

        wxLog::SetActiveTarget(new wxLogWindow(nullptr, "Debug Log"));
        wxLogDebug("Aplicación iniciando...");

        auto* win = new MainWindow();
        win->Show();

        wxLogDebug("Ventana principal mostrada");
        return true;
    }
};

wxIMPLEMENT_APP(SubStudioApp);
