#ifndef GAME_GAME_H
#define GAME_GAME_H

int game_pre_init(void);

int game_init(void);

int reload_language(int is_editor, int reload_images);

int game_init_editor(void);

void game_run(void);

void game_draw(void);

void game_exit(void);

#endif // GAME_GAME_H
