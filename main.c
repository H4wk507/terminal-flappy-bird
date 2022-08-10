#include <stdio.h>
#include <curses.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// movement
#define GRAVITY 0.5f
#define JUMP_SPEED 2.0f
#define VMIN -2.0f

// pipes parameters
#define PIPES_LEN 4 
#define PIPE_GAP_Y 6 
#define MAX_PIPE_HEIGHT LINES - PIPE_GAP_Y
#define PIPE_GAP_X COLS / PIPES_LEN 
#define PIPE_SPEED 1.0f

// bird parameters
#define BIRD_HEIGHT 1
#define BIRD_WIDTH 3
#define BIRD_START_X 3

// colors
#define PIPE_PAIR 1
#define BIRD_PAIR 2

// keys
#define SPACE ' '
#define Q_KEY 'q'
#define ENTER 10

typedef enum EXIT_STATUS {
   NONE,
   EXIT,
   PLAY_AGAIN,
} EXIT_STATUS;

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
   //if (has_colors() == FALSE) {
   //   printf("Your terminal does not support color\n");
   //   exit(1);
   //}

   initscr();
   start_color();
   init_color(COLOR_GREEN, 0, 1000, 0);
   init_color(COLOR_RED, 1000, 560, 0);

   init_pair(PIPE_PAIR, COLOR_GREEN, COLOR_BLACK);
   init_pair(BIRD_PAIR, COLOR_RED, COLOR_BLACK);

   cbreak();
   noecho();
   curs_set(0); // disable cursor
   timeout(60); // screen refresh rate
   set_escdelay(0);
   keypad(stdscr, TRUE);
}

void delete_bird_window(bird *b) {
   if (b->window) {
      werase(b->window);
      wrefresh(b->window);
      delwin(b->window);
   }
}

void free_bird(bird *b) {
   delete_bird_window(b); 
   free(b);
}

void delete_pipes_window(pipe *p, int idx) {
   assert(idx < PIPES_LEN);

   if (p[idx].upper_pipe) {
      werase(p[idx].upper_pipe);
      wrefresh(p[idx].upper_pipe);
      delwin(p[idx].upper_pipe);
   }

   if (p[idx].lower_pipe) {
      werase(p[idx].lower_pipe);
      wrefresh(p[idx].lower_pipe);
      delwin(p[idx].lower_pipe);
   }
}

void free_pipes(pipe *p) {
   for (int i = 0; i < PIPES_LEN; i++)
      delete_pipes_window(p, i);
   free(p);
}

void draw_bird(bird *b) {
   assert(b != NULL);

   delete_bird_window(b);
   b->window = newwin(BIRD_HEIGHT, BIRD_START_X, b->pos_y, BIRD_WIDTH);

   wattron(b->window, COLOR_PAIR(BIRD_PAIR));
   mvwprintw(b->window, 0, 0, "*)>");
   wrefresh(b->window);
   wattroff(b->window, COLOR_PAIR(BIRD_PAIR));
}

void draw_pipes(pipe *p) {
   assert(p != NULL);
   
   for (int i = 0; i < PIPES_LEN; i++) {
      delete_pipes_window(p, i); 

      p[i].upper_pipe = newwin(p[i].upper_size, 1, 0, p[i].pos_x);
      p[i].lower_pipe = newwin(p[i].lower_size, 1, 
         LINES - p[i].lower_size, p[i].pos_x);

      wattron(p[i].upper_pipe, COLOR_PAIR(PIPE_PAIR));
      wattron(p[i].lower_pipe, COLOR_PAIR(PIPE_PAIR));

      for (int j = 0; j < p[i].upper_size; j++)
         mvwprintw(p[i].upper_pipe, j, 0, "#");

      for (int j = 0; j < p[i].lower_size; j++)
         mvwprintw(p[i].lower_pipe, j, 0, "#");

      wrefresh(p[i].upper_pipe);
      wrefresh(p[i].lower_pipe);

      wattroff(p[i].upper_pipe, COLOR_PAIR(PIPE_PAIR));
      wattroff(p[i].lower_pipe, COLOR_PAIR(PIPE_PAIR));
   }
}

bird *new_bird() {
   bird *b = malloc(sizeof(bird));
   b->pos_y = LINES / 2.0;
   b->speed_y = 0.0;
   b->window = NULL;
   return b;
}

pipe *init_pipes() {
   pipe *p = malloc(sizeof(pipe) * PIPES_LEN);

   for (int i = 0; i < PIPES_LEN; i++) {
      p[i].upper_pipe = NULL;
      p[i].lower_pipe = NULL;
      p[i].upper_size = rand() % MAX_PIPE_HEIGHT + 1;
      p[i].lower_size = rand() % MAX_PIPE_HEIGHT + 1;

      while (p[i].upper_size + p[i].lower_size > MAX_PIPE_HEIGHT || 
             p[i].upper_size + p[i].lower_size < LINES / 2)
      {
         p[i].upper_size = rand() % MAX_PIPE_HEIGHT + 1;
         p[i].lower_size = rand() % MAX_PIPE_HEIGHT + 1;
      }
         
      p[i].pos_x = COLS / 2.0 + (i * PIPE_GAP_X);
   }
   return p;
}

int death_screen(int score) {
   char *death_message[3] = {
      "               You have died.             ",
      "             Your score was ",
      "  Press q to quit or enter to try again.  ",
   };

   int width = strlen(death_message[2]);
   int height = sizeof(death_message) / sizeof(death_message[0]);
   WINDOW *window = newwin(height + 2, width + 2, (LINES - height) / 2, (COLS - width) / 2);

   wattron(window, COLOR_PAIR(BIRD_PAIR));
   mvwprintw(window, 1, 1, "%s", death_message[0]);
   mvwprintw(window, 2, 1, "%s%d.", death_message[1], score);
   mvwprintw(window, 3, 1, "%s", death_message[2]);
   box(window, 0, 0);
   wattroff(window, COLOR_PAIR(BIRD_PAIR));

   wrefresh(window);

   int ch;
   while ( (ch = wgetch(window)) ) { 
      if (ch == Q_KEY || ch == ENTER)
         break;
   }

   werase(window);
   wrefresh(window);
   delwin(window);
   
   return ch;
}

EXIT_STATUS check_collision(bird *b, pipe *p, int score) {
   assert(b != NULL && p != NULL);

   int ch;
   // check bird's collision with floor and ceiling
   if (b->pos_y < 0 || (int) b->pos_y > LINES - 1) {
      ch = death_screen(score);
      free_pipes(p);
      free_bird(b);
      if (ch == ENTER)
         return PLAY_AGAIN;      
      if (ch == Q_KEY)
         return EXIT;
   }

   for (int i = 0; i < PIPES_LEN; i++) { 
      // check for collision at x axis
      int collide_x = p[i].pos_x >= BIRD_START_X && 
                      p[i].pos_x <= BIRD_START_X + BIRD_WIDTH - 1;

      // check for collision at y axis
      int collide_y = b->pos_y >= LINES - p[i].lower_size || 
                      b->pos_y <= p[i].upper_size; 

      // if collision occured, exit
      if (collide_x && collide_y) {
         ch = death_screen(score);
         free_pipes(p);
         free_bird(b);
         if (ch == ENTER)
            return PLAY_AGAIN;
         if (ch == Q_KEY)
            return EXIT;
      }
   }
   return NONE;
}

void check_out_of_bounds(pipe *p) {
   assert(p != NULL);

   for (int i = 0; i < PIPES_LEN; i++) {
      if (p[i].pos_x < 0) {
   
         // if pipe got OOB, delete pipe's window
         delete_pipes_window(p, i);

         // if pipe got OOB, then generate its new position at the back
         p[i].pos_x = COLS;
         p[i].upper_size = rand() % MAX_PIPE_HEIGHT + 1;
         p[i].lower_size= rand() % MAX_PIPE_HEIGHT + 1;

         while (p[i].upper_size + p[i].lower_size > MAX_PIPE_HEIGHT || 
                p[i].upper_size + p[i].lower_size < LINES / 2)
         {
            p[i].upper_size = rand() % MAX_PIPE_HEIGHT + 1;
            p[i].lower_size = rand() % MAX_PIPE_HEIGHT + 1;
         }            

         p[i].upper_pipe = newwin(p[i].upper_size, 1, 
            0, p[i].pos_x);
         p[i].lower_pipe = newwin(p[i].lower_size, 1, 
            LINES - p[i].lower_size, p[i].pos_x);
      }
   }
}

void increase_score(bird *b, pipe *p, int *score) {
   assert(b != NULL && p != NULL);

   for (int i = 0; i < PIPES_LEN; i++) {
      // check if we are at the pipe's x position
      int collide_x = p[i].pos_x == (2 * BIRD_START_X + BIRD_WIDTH - 1) / 2;

      // check if we are between the pipe
      int between_pipes = b->pos_y < LINES - p[i].lower_size && 
                          b->pos_y > p[i].upper_size;

      if (collide_x && between_pipes)
        (*score)++; 
   }
}

EXIT_STATUS run() {
   // init variables
   int score = 0;
   bird *flappy = new_bird();
   pipe *pipes = init_pipes();

   refresh();

   // draw on screen
   draw_pipes(pipes);
   draw_bird(flappy);

   int ch;
   int EXIT_STATUS;
   while ( (ch = getch()) ) {
      if (ch == Q_KEY) {
         free_pipes(pipes);
         free_bird(flappy);
         return EXIT; 
      }
      if (ch == SPACE)
         flappy->speed_y = JUMP_SPEED; 

      flappy->speed_y -= GRAVITY;
      flappy->speed_y = fmax(flappy->speed_y, VMIN);
      flappy->pos_y -= flappy->speed_y;

     for (int i = 0; i < PIPES_LEN; i++)
         pipes[i].pos_x -= PIPE_SPEED;

      // check for collisions
      EXIT_STATUS = check_collision(flappy, pipes, score);
      switch (EXIT_STATUS) {
         case PLAY_AGAIN:  return PLAY_AGAIN;
         case EXIT:        return EXIT;
      }

      // check for increase score
      increase_score(flappy, pipes, &score);

      // OOB
      check_out_of_bounds(pipes);

      draw_pipes(pipes);
      draw_bird(flappy);

      attron(COLOR_PAIR(PIPE_PAIR));
      mvprintw(LINES - 1, 0, "%d point(s) space jump | q/ctrl+c quit", score);
      attroff(COLOR_PAIR(PIPE_PAIR));
   }
   return EXIT;
}

int main(void) {
   init_ncurses();
   srand(time(NULL));
   EXIT_STATUS STATUS;
   do {
      STATUS = run();
   } while(STATUS != EXIT);
   endwin();
   return 0;
}
