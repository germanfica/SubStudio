#pragma once

#ifndef SUBSTUDIO_UI_MAIN_MENU_H_
#define SUBSTUDIO_UI_MAIN_MENU_H_

#include <functional>
#include <memory>
#include <wx/menu.h>
#include <wx/menuitem.h>
#include <wx/frame.h>

struct MenuActions {
    std::function<void()> on_open;
    std::function<void()> on_save;
    std::function<void()> on_save_as;
    std::function<void()> on_exit;
    std::function<void()> on_about;
};

class MainMenu {
public:
    explicit MainMenu(wxFrame* frame, const MenuActions& actions);
    ~MainMenu() = default;

    // Devuelve el wxMenuBar (no transfiere ownership)
    wxMenuBar* bar() const { return bar_; }

private:
    wxFrame* frame_ = nullptr;
    MenuActions actions_;
    wxMenuBar* bar_ = nullptr; // NO somos dueños: el frame se encarga de destruirlo

    void BindHandlers();
};

#endif  // SUBSTUDIO_UI_MAIN_MENU_H_
