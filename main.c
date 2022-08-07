#include <stdio.h>
#include <curses.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define ESC_KEY 27

#define GRAVITY 0.5f
#define JUMP_SPEED 2.0f
#define VMIN -2.0f
#define PIPE_HEIGHT 9
#define SPACE ' '

typedef struct bird {
   WINDOW *window;
   float pos_y;
   float speed_y;
} bird;

typedef struct pipe {
   WINDOW *upper_pipe;
   WINDOW *lower_pipe;
   int upper_size;
   int lower_size;
   float pos_x; 
} pipe;

// initialize ncurses
void init_ncurses() {
   initscr();
   cbreak();
   noecho();
   curs_set(0); // disable cursor
   set_escdelay(0); // set ESC delay to 0
   timeout(60);
   keypad(stdscr, TRUE);
}

void kill() {
   endwin();   
   exit(1);
}

void draw_bird(bird **b) {
   if ((*b)->window) {
      werase((*b)->window);
      wrefresh((*b)->window);
      delwin((*b)->window);
   }
   (*b)->window = newwin(1, 3, (*b)->pos_y, 3);
   mvwprintw((*b)->window, 0, 0, "o)>");
   wrefresh((*b)->window);
}

void draw_pipe(pipe **p) {
   if ((*p)->upper_pipe) {
      werase((*p)->upper_pipe);
      wrefresh((*p)->upper_pipe);
      delwin((*p)->upper_pipe);
   } 

   if ((*p)->lower_pipe) {
      werase((*p)->lower_pipe);
      wrefresh((*p)->lower_pipe);
      delwin((*p)->lower_pipe);
   }

   (*p)->upper_pipe = newwin((*p)->upper_size, 1, 0, (*p)->pos_x);
   (*p)->lower_pipe = newwin((*p)->lower_size, 1, 
      LINES - (*p)->lower_size, (*p)->pos_x);

   for (int i = 0; i < PIPE_HEIGHT; i++) {
      mvwprintw((*p)->upper_pipe, i, 0, "#");
      mvwprintw((*p)->lower_pipe, i, 0, "#");
   }

   wrefresh((*p)->upper_pipe);
   wrefresh((*p)->lower_pipe);
}

bird *new_bird() {
   bird *b = malloc(sizeof(bird));
   b->pos_y = LINES / 2.0;
   b->speed_y = 0.0;
   b->window = NULL;
   return b;
}

pipe *new_pipe() {
   pipe *p = malloc(sizeof(pipe));
   p->upper_pipe = NULL;
   p->lower_pipe = NULL;
   p->upper_size = rand() % PIPE_HEIGHT + 1;
   p->lower_size = rand() % PIPE_HEIGHT + 1;
   p->pos_x = COLS / 2.0;
   return p;
}

void check_collision(pipe *p, bird *b) {
   // check for collision at x axis
   int collide_x = p->pos_x >= 4 && p->pos_x <= 6;

   // check for collision at y axis
   int collide_y = b->pos_y >= LINES - p->lower_size || 
                   b->pos_y <= p->upper_size; 

   if (collide_x && collide_y) {
      free(p);
      free(b);
      endwin();
      exit(1);
   }
}

int main(void) {
   init_ncurses();
   srand(time(NULL));

   // init variables
   int score = 0;
   bird *flappy = new_bird();
   pipe *pipe1 = new_pipe();

   int ch;
   refresh();

   // draw on screen
   draw_pipe(&pipe1);
   draw_bird(&flappy);

   while ( (ch = getch()) ) {
      if (ch == ESC_KEY) {
         free(pipe1);
         free(flappy);
         kill();
      }
      else if (ch == SPACE)
         flappy->speed_y = JUMP_SPEED; 

      flappy->speed_y -= GRAVITY;
      flappy->speed_y = fmax(flappy->speed_y, VMIN);
      flappy->pos_y -= flappy->speed_y;

      if (flappy->pos_y < 0 || flappy->pos_y > LINES) {
         free(flappy);
         free(pipe1);
         endwin();
         return 0;
      }
      pipe1->pos_x -= 0.5f;
      // check for collisions
      check_collision(pipe1, flappy);
      // check for increase score
      if (pipe1->pos_x == 5 && 
         !(flappy->pos_y >= LINES - pipe1->lower_size || 
            flappy->pos_y <= pipe1->upper_size)) {
            score++; 
         }
      // TODO: check if pipe got out of bounds
      draw_bird(&flappy);
      draw_pipe(&pipe1);

      mvprintw(LINES - 1, 0, "score:%d", score);
   }
   endwin();
   return 0;
}
