/**
 * @file gamemenu.h
 *
 * Interface of the in-game menu functions.
 */
#pragma once

namespace devilution {

struct TMenuItem;

void gamemenu_on();
void gamemenu_off();
void gamemenu_handle_previous();
void gamemenu_exit_game(bool bActivate);
void gamemenu_quit_game(bool bActivate);
void gamemenu_load_game(bool bActivate);
void gamemenu_save_game(bool bActivate);
const char *GetGamemenuText(const TMenuItem &menuItem);

extern bool isGameMenuOpen;

} // namespace devilution
