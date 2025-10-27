#pragma once

#ifndef SUBSTUDIO_UI_MAIN_TOOLBAR_H_
#define SUBSTUDIO_UI_MAIN_TOOLBAR_H_

#include <memory>
#include <wx/frame.h>
#include <wx/toolbar.h>

class MainToolbar {
public:
    explicit MainToolbar(wxFrame* frame);

    wxToolBar* toolbar() const { return toolbar_; }

private:
    wxToolBar* toolbar_ = nullptr;
};

#endif  // SUBSTUDIO_UI_MAIN_TOOLBAR_H_
