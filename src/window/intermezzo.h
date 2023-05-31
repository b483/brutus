#ifndef WINDOW_INTERMEZZO_H
#define WINDOW_INTERMEZZO_H

enum {
    INTERMEZZO_MISSION_BRIEFING = 0,
    INTERMEZZO_FIRED = 1,
    INTERMEZZO_WON = 2,
};

void window_intermezzo_show(int type, void (*callback)(void));

#endif // WINDOW_INTERMEZZO_H
