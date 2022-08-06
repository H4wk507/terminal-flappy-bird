#include <stdio.h>
#include <curses.h>
#include <stdlib.h>
#include <math.h>

#define ESC_KEY 27
#define GRAVITY 0.5f
#define VMIN -2.0f
#define SPACE ' '

void init() {
   initscr();
   cbreak();
   noecho();
   curs_set(0); // disable cursor
   set_escdelay(0); // set ESC delay to 0
   timeout(60);
   keypad(stdscr, TRUE);
}

WINDOW *draw_bird(int height, WINDOW *window) {
   if (window) {
      werase(window);
      wrefresh(window);
      delwin(window);
   }
   window = newwin(3, 3, height, 5);
   mvwprintw(window, 0, 0, "o)>");
   wrefresh(window);
   return window;
}

typedef struct bird {
   WINDOW *window;
   int pos_y;
   float speed_y;
} bird;

int main(void) {
   init();

   bird flappy;
   flappy.pos_y = LINES / 2;
   flappy.speed_y = 0;

   int ch;
   refresh();
   WINDOW *window = draw_bird(flappy.pos_y, NULL);
   while ( (ch = getch()) ) {
      if (ch == ESC_KEY) {
         endwin();
         return 0;
      }
      else if (ch == SPACE)
         flappy.speed_y = 2; 
      flappy.speed_y -= GRAVITY;

      flappy.speed_y = fmax(flappy.speed_y, VMIN);

      flappy.pos_y -= flappy.speed_y;
      if (flappy.pos_y < 0 || flappy.pos_y > LINES) {
         endwin();
         return 0;
      }
      window = draw_bird(flappy.pos_y, window);
   }
   endwin();
   return 0;
}
