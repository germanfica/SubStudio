#include <wx/wx.h>
#include "MainWindow.h"

class SubStudioApp : public wxApp {
public:
    bool OnInit() override {
        if (!wxApp::OnInit()) return false;
        auto* win = new MainWindow();
        win->Show();
        return true;
    }
};

wxIMPLEMENT_APP(SubStudioApp);
