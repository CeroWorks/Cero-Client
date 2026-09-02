#ifndef CERO_GAME_HANDLERS_H
#define CERO_GAME_HANDLERS_H

void on_launch(const char* id, const char* req, void* arg);
void on_kill_game(const char* id, const char* req, void* arg);
void on_game_status(const char* id, const char* req, void* arg);

#endif /* CERO_GAME_HANDLERS_H */
