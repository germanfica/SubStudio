// src/main_menu.h
#pragma once

#ifndef SUBSTUDIO_UI_MAIN_MENU_H_
#define SUBSTUDIO_UI_MAIN_MENU_H_

#include <functional>
#include <memory>
#include <wx/menu.h>
#include <wx/menuitem.h>
#include <wx/frame.h>

// Callbacks para acciones del menu
struct MenuActions {
    std::function<void()> on_open;
    std::function<void()> on_save;
    std::function<void()> on_save_as;
    std::function<void()> on_exit;
    std::function<void()> on_about;
};

// Clase que encapsula la creación y binding del menu.
// Nota: MainMenu NO es dueño del wxMenuBar; el frame lo es tras SetMenuBar.
class MainMenu {
public:
    explicit MainMenu(wxFrame* frame, const MenuActions& actions);
    ~MainMenu();

    // Devuelve el wxMenuBar actual asignado al frame (no transfiere ownership).
    wxMenuBar* bar() const { return frame_ ? frame_->GetMenuBar() : nullptr; }

private:
    wxFrame* frame_ = nullptr;
    MenuActions actions_;

    void BindHandlers();
};

#endif  // SUBSTUDIO_UI_MAIN_MENU_H_
