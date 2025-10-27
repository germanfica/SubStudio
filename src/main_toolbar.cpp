#include "main_toolbar.h"
#include <wx/artprov.h>

MainToolbar::MainToolbar(wxFrame* frame) {
    toolbar_ = frame->CreateToolBar();
    toolbar_->AddTool(wxID_OPEN, "Open", wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR));
    toolbar_->AddTool(wxID_SAVE, "Save", wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR));
    toolbar_->Realize();
}
