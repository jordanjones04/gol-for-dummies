/*
 * Swarthmore College, CS 31
 * Copyright (c) 2023 Swarthmore College Computer Science Department,
 * Swarthmore PA
 */

/* This is a rendition of the Game of Life and this program asks the user to input     
    the name of the file and then a value of 0, 1 or 2. With 0 being no visual, 1 being
   the ASCII visual, and 3 being the VISI visual. */
/*
 * To run:
 * ./gol file1.txt  0  # run with config file file1.txt, do not print board
 * ./gol file1.txt  1  # run with config file file1.txt, ascii animation
 * ./gol file1.txt  2  # run with config file file1.txt, ParaVis animation
 *
 */
#include <pthreadGridVisi.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include "colors.h"
#include "graphics.h"


/****************** Definitions **********************/
/* Three possible modes in which the GOL simulation can run */
#define OUTPUT_NONE   (0)   // with no animation
#define OUTPUT_ASCII  (1)   // with ascii animation
#define OUTPUT_VISI   (2)   // with ParaVis animation

/* Used to slow down animation run modes: usleep(SLEEP_USECS);
 * Change this value to make the animation run faster or slower
 */
//#define SLEEP_USECS  (1000000)
#define SLEEP_USECS    (100000)

/* A global variable to keep track of the number of live cells in the
 * world (this is the ONLY global variable you may use in your program)
 */
static int total_live = 0;
struct timeval start_time, stop_time;

static int rt = 0;
static int sr_count = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* This struct represents all the data you need to keep track of your GOL
 * simulation.  Rather than passing individual arguments into each function,
 * we'll pass in everything in just one of these structs.
 * this is passed to play_gol, the main gol playing loop
 *
 */
struct gol_data {

    int rows;  // the row dimension
    int cols;  // the column dimension
    int iters; // number of iterations to run the gol simulation
    int output_mode; // set to:  OUTPUT_NONE, OUTPUT_ASCII, or OUTPUT_VISI
    int num_cells;
    int num_alive_cells;
    int* current;
    int* next;
    int rounds;
    int num_threads;
    int para_mode;
    int print_info;
    //pthread_t thread_id;
    int id;
    int rows_per_thread;
    // int start_row;
    // int end_row;
    int **row_partition_info;
    int **col_partition_info;
    /* fields used by ParaVis library (when run in OUTPUT_VISI mode). */
    visi_handle handle;
    color3 *image_buff;
};


/****************** Function Prototypes **********************/
/* the main gol game playing loop (prototype must match this) */
void* play_gol(void* args);

/* init gol data from the input file and run mode cmdline args */
int init_game_data_from_args(struct gol_data *data, int argc, char **argv);

// A mostly implemented function, but a bit more for you to add.
/* print board to the terminal (for OUTPUT_ASCII mode) */
void print_board(struct gol_data *data, int round);

// makes the board
void make_board(int *arr, int rows, int cols);

// counts the number of alive neighboring cells
int count_alive(struct gol_data *data, int x_axis, int y_axis);

// changes the status of a dead cell to alive
void make_alive(struct gol_data *data, int x_axis, int y_axis, int alive);

// changes the colors for the VISI visual output
void update_colors(struct gol_data* data);

void* play_gol_thread(void* arg);
int** row_partition(int rows, int cols, int num_threads);
int** col_partition(int rows, int cols, int num_threads);

/************ Definitions for using ParVisi library ***********/
/* initialization for the ParaVisi library (DO NOT MODIFY) */
int setup_animation(struct gol_data* data);
/* register animation with ParaVisi library (DO NOT MODIFY) */
int connect_animation(void (*applfunc)(struct gol_data *data),
        struct gol_data* data);
/* name for visi (you may change the string value if you'd like) */
static char visi_name[] = "GOL!";


int main(int argc, char **argv) {
    
    int ret;
    struct gol_data data;
    double secs;
    struct gol_data *tid_args;

    /* check number of command line arguments */
    if (argc < 6) {
        printf("usage: %s <infile.txt> <output_mode>[0|1|2]\n", argv[0]);
        printf("(0: no visualization, 1: ASCII, 2: ParaVisi)\n");
        exit(1);
    }

    /* Initialize game state (all fields in data) from information
     * read from input file */
    ret = init_game_data_from_args(&data, argc, argv);
    if (ret != 0) {
        printf("Initialization error: file %s, mode %s\n", argv[1], argv[2]);
        exit(1);
    }
     //make the threads.
    pthread_t *tid;
    int r;
    int threads = data.num_threads;
    tid = malloc(sizeof(pthread_t)  *threads);
    tid_args = malloc(sizeof(struct gol_data) * threads);
    
    

    // starts counting the program runtime 
    ret = gettimeofday(&start_time, NULL);
    if(ret!= 0){
        printf("Timing Error!");
    }

    /* initialize ParaVisi animation (if applicable) */
    if (data.output_mode == OUTPUT_VISI) {
        setup_animation(&data);
    }

    /* ASCII output: clear screen & print the initial board */
    if (data.output_mode == OUTPUT_ASCII) {
        if (system("clear")) { perror("clear"); exit(1); }
        total_live = data.num_alive_cells;
        print_board(&data, 0);
    }

    /* Invoke play_gol in different ways based on the run mode */
    if (data.output_mode == OUTPUT_NONE) {  // run with no animation
        //play_gol(&data);
        for(int i = 0; i<data.num_threads;i++){ 
            tid_args[i] = data; /* make a private copy for each thread */ //insert struct jsadklfjklsdjklfqjklsdjaklsdj
            tid_args[i].id = i;       /* set logical ID for this thread */
            int r = pthread_create(&tid[i], NULL, play_gol_thread, &tid_args[i]);
        }for(int i = 0; i<data.num_threads;i++){
            int r = pthread_join(tid[i], NULL);
        }
    }
    else if (data.output_mode == OUTPUT_ASCII) { // run with ascii animation
        //play_gol(&data);
        int i;
        for(i = 0; i<data.num_threads;i++){
            tid_args[i] = data; /* make a private copy for each thread */ //insert struct jsadklfjklsdjklfqjklsdjaklsdj
            tid_args[i].id = i;       /* set logical ID for this thread */
            int r = pthread_create(&tid[i], NULL, play_gol_thread, &tid_args[i]);
        }   //if r ==1 print error message             dklgj;jsdljfklsjkljdsklfjklsjdklfjsklfjklsjkdlfjsdkl
        for(i = 0; i<data.num_threads;i++){
            int r = pthread_join(tid[i], NULL);
        }//if r ==1 print error message                     jkfdjglkjkldjfdslkikjkdflsjskdljfkdljkljsdfkjafkljlkfjklsdwjkdsjkl
    }
    else {  
        // OUTPUT_VISI: run with ParaVisi animation
        // tell ParaVisi that it should run play_gol
        //connect_animation(play_gol, &data);
        // start ParaVisi animation
        //run_animation(data.handle, data.iters);
    }

    // stops counting the program runtime
    ret = gettimeofday(&stop_time, NULL);
    if(ret!= 0){
        printf("Timing Error!");
    }
        
  
    if (data.output_mode != OUTPUT_VISI) {
        secs = 0.0;
        int start; 
        int end;

        /*  uses the conversion factor to unify the time units
         *  and sums them together. */
        start = start_time.tv_sec *100000 + start_time.tv_usec;
        end = stop_time.tv_sec *100000 + stop_time.tv_usec;

        // computes the runtime         
        secs = (abs(end - start))/100000;

        /* Print the total runtime, in seconds. */
        fprintf(stdout, "Total time: %0.3f seconds\n", secs);
        fprintf(stdout, "Number of live cells after %d rounds: %d\n\n",
                data.iters, total_live);
    }

    free(data.current);
    free(data.next);

    return 0;
}

/* the initialize game data from arguments function:
 *  opens the input file,
 *  reads in the data from the file (rows, cols, iters, num_alive_cells, x, y)
 *  sets output mode to a varible
 *
 *   data: pointer to a struct gol_data  initialized with
 *         all GOL game playing state
 *
 *   argv: pointer to an array that contains data of type char and it 
 *         contains the inputs of the user
 */
int init_game_data_from_args(struct gol_data *data, int argc, char **argv) {
    FILE *file;
    int rows, cols, num_alive_cells, x, y, num_threads;

    //Check that the correct number of command line arguments are provided
    if(argc != 6){
     printf("Usage: %s <input_file> <output_mode> <num_threads> <parallelization_mode> <print_info>\n", argv[0]);
     return 1;
    }

    // opens the file
    file = fopen(argv[1], "r");
    if (file == NULL){
        printf("Error unable to open file %s\n", argv[1]);
        exit(1);
    }

    // Sets output_mode from the command line arguments to a variable
    data->output_mode = atoi(argv[2]);
    if(data->output_mode < 0 || data->output_mode > 2){
        printf("Error incombatibale output_mode. Put 0, 1, or 2\n");
        exit(1);
    }
    //Reads in the number of rows, colums, number of alive cells from file
    int ret = fscanf(file, "%d %d %d %d", &data->rows, &data->cols, &data->iters, &data->num_alive_cells);
    if(ret != 4){
        printf("Error: improper file format.\n");
        exit(1);
    }
    
    //gets the number of threads from command line and save it to
    //a variable.
    data->num_threads = atoi(argv[3]);
    printf("%d\n", data->num_threads);              //for debugging purposes >.......hjsdfkhsjkhjdkfzhfjkeshbsd

    //gets the parallelization mode //0 =rows, 1 == column
    data->para_mode = atoi(argv[4]);

    //gets the decision on whether or not the thread allocation is printed
    data->print_info = atoi(argv[5]);
    

    // sets the data from the struct to variables
    num_alive_cells = data->num_alive_cells;
    rows = data->rows;
    cols = data->cols;



    //Make a board set equal to dead and then go through and place the cells that are alive
    data->current = malloc(sizeof(int) * rows * cols);
    if (data->current == NULL){
        printf("ERROR: malloc failed!\n");
        exit(1);
    }
     make_board(data->current, rows, cols);
    if(data->para_mode == 0){
        data->row_partition_info = row_partition(data->rows, data->cols, data->num_threads); 
    }else if(data->para_mode == 1){
        data->col_partition_info = col_partition(data->rows, data->cols, data->num_threads);
    }else{
        printf("ERROR: INVALID PARALLELIZATION MODE\n");
        exit(1);
    }
   //make the next board 
    data->next = malloc(sizeof(int)*rows*cols);
    if (data->next == NULL){
        printf("ERROR: malloc failed!\n");
        exit(1);
    }
    make_board(data->next, rows, cols);

    for (int i = 0; i < num_alive_cells; i++){
        ret = fscanf(file, "%d %d\n", &x, &y);
        if(ret != 2){
            printf("Error Improper file format.\n");
            exit(1);
            
        }
        data->current[x*cols + y] = 1;
    }

    //close the file when done with it
    ret = fclose(file);
    // if(ret != 0){ 
    //      for(int j = 0; j < cols; j++){
    //          arr[i*cols + j] = 0;
    //      }
    // }
    return 0;
}

int** row_partition(int rows, int cols, int num_threads){ //double free in main!!
    int ** partition_info = (int**)malloc(num_threads * sizeof(int*));
    int cells_per_thread = rows / num_threads;
    int extra_cells = rows % num_threads;
    int start_row = 0;

    for(int i = 0; i < num_threads; i++){
        int end_row = start_row + cells_per_thread -1;
        if(extra_cells > 0){
            end_row++;
            extra_cells--;
        }
        partition_info[i] = (int*)malloc(2 * sizeof(int));
        partition_info[i][0] = start_row;
        partition_info[i][1] = end_row;
        start_row = end_row + 1;
    }
    return partition_info;
}

int** col_partition(int rows, int cols, int num_threads){ //double free in main!!
    int ** partition_info = (int**)malloc(num_threads * sizeof(int*));
    int cells_per_thread = cols / num_threads;
    int extra_cells = cols % num_threads;
    int start_col = 0;

    for(int i = 0; i < num_threads; i++){
        int end_col = start_col + cells_per_thread -1;
        if(extra_cells > 0){
            end_col++;
            extra_cells--;
        }
        partition_info[i] = (int*)malloc(2 * sizeof(int));
        partition_info[i][0] = start_col;
        partition_info[i][1] = end_col;
        start_col = end_col + 1;
    }
    return partition_info;
}
/* helper function to initialize the array that makes up the board
 * arr: is the array of type int
 * rows: the number of rows in the file
 * cols: the number of columns in the file
 * returns: none */
void make_board(int *arr, int rows, int cols){
    int i, j;
    for(i = 0; i < rows; i++){
        for(j = 0; j < cols; j++){
            arr[i*cols + j] = 0;
        }
    }
}
void* play_gol_thread(void* arg){
    //Unpack the arguments that are passed into the thread
    struct gol_data *data;
    data = (struct gol_data*) arg;
    int id = data->id;
    int start_row = 0;
    int end_row = data->rows - 1;
    int start_col = 0;
    int end_col = data->cols - 1;
    if(data->para_mode ==0){
        start_row= data->row_partition_info[id][0];
        end_row = data->row_partition_info[id][1];
    }else{
        start_col= data->col_partition_info[id][0];
        end_col = data->col_partition_info[id][1];
    }
     
    // printf("Starting Row: %d\n",start_row);
    // printf("Ending Row: %d\n",end_row);
    // printf("Starting Col: %d\n",start_col);
    // printf("Ending Col: %d\n",end_col);
    //could call outside of this function
    //assume that we're focusing on rows
    if(data->print_info ==1){
    printf("tid %d: rows: %d:%d (%d) cols: %d:%d (%d)\n", id, start_row, end_row, end_row-start_row+1, start_col, end_col, end_col - start_col +1);
    }   
    //Process the partition of the game board assigned to this thread
    for(int a = 0; a<data->rounds; a++){
        total_live = 0;
        for(int i = start_row; i <= end_row; i++){
            for(int j = start_col; j < end_col; j++){
                int live_neighbors = count_alive(data, i, j);
                make_alive(data, i, j, live_neighbors); 
            }
        }
    //Barrier to wait for all threads to finish
    //the next few lines can be printed by the thread with startCol 0, startRow 0

    // replaces the current board with next board
    int *temp = data->current;
    data->current = data->next;
    data->next = temp;
    //If the output_mode is 1 then the program runs the ASCII version
        if(data->output_mode == 1){
            system("clear");
            print_board(data, data->rounds);
            usleep(SLEEP_USECS);
        }
        
        //If output_mode is 2 then the program runs the animation version
        else if(data->output_mode == 2){ 
            update_colors(data);
            draw_ready(data->handle);
            usleep(SLEEP_USECS);
        } 
    }
    return NULL;   
    //pthread_exit(NULL);
}

/* the gol application main loop function:
 *  runs rounds of GOL,
 *    * updates program state for next round (world and total_live)
 *    * performs any animation step based on the output/run mode
 *
 *   data: pointer to a struct gol_data  initialized with
 *         all GOL game playing state
 */
// void* play_gol(void* args) {
//     struct gol_data *data;
//     data = (struct gol_data*) args;
//     //initializing local variables 
//     int alive;
//     int* temp = NULL; 
    
//     //Gives each thread the number of rows it reads
//     printf("Before Math: %d\n",data->num_threads);
//     data->rows_per_thread = data->rows / data->num_threads;
//     rt = data->rows %data->num_threads;
//     //LOCK
//     pthread_mutex_lock(&mutex);
//     data->start_row = sr_count;
//     if(rt!=0){
//         rt--;
//         data->rows_per_thread ++;
//     }
//     for(int a = 0; a<data->rows_per_thread; a++){
//         sr_count++;
//     }
//     pthread_mutex_unlock(&mutex);
//     printf("%d\n", data->start_row);
//     //UNLOCK
//     /*


//     // for-loop for the number of rounds of the games
//     for(int z=1;z<(data->iters)+1;z++){
//         total_live = 0; 
//         for(int i=0;i<data->rows;i++){
//             for(int j=0;j<data->cols;j++){
//                 alive = count_alive(data,i,j);
//                 make_alive(data,i,j, alive);
//             }   
//         }

//         //Swaps the current with next board
//         temp = data->current;
//         data->current = data->next;
//         data->next = temp;

//         //If the output_mode is 1 then the program runs the ASCII version
//         if(data->output_mode == 1){
//             system("clear");
//             print_board(data, z);
//             usleep(SLEEP_USECS);
//         }
        
//         //If output_mode is 2 then the program runs the animation version
//         else if(data->output_mode == 2){ 
//             update_colors(data);
//             draw_ready(data->handle);
//             usleep(SLEEP_USECS);
//         } 
//     }


//     */

//     return NULL;
// }

   
/* This functions counts the alive neighbors around each individual cell
 * data: the struct of type struct gol_data
 * x_axis: x coordinate of a cell
 * y_axis: y coordinate of a cell
 * returns: alive, is of type int, and the number of alive cells*/
int count_alive(struct gol_data *data, int x_axis, int y_axis){
    //initializing local variables
    int x_neigbor;
    int y_neighbor;
    int alive = 0;

    for (int x=-1; x < 2; x++) {
        //calculates the x-coordinate of a neigboring point 
        x_neigbor = (x + x_axis + data->rows)% data->rows;
        for (int y=-1; y < 2; y++) {
            //calculates the y-coordinate of a neigboring point
            y_neighbor = (y + y_axis + data->cols)% data->cols;
            if(!((x == 0) && (y == 0))){
                if(data->current[(x_neigbor*data->cols) + y_neighbor] == 1){
                    alive +=1;
                }
            }
        }
    }
    return alive;
}

/* This function determines whether a cell changes status (dead or alive) based on its 
   number of alive neighbors.

 * data: the struct of type struct gol_data
 * x_axis: x coordinate of a cell
 * y_axis: y coordinate of a cell
 * alive: of type int. number of alive cells
 * returns: none */
void make_alive(struct gol_data *data, int x_axis, int y_axis, int alive){
    if(data->current[(x_axis*data->cols) + y_axis] == 1){

        if (alive <= 1){
            data->next[(x_axis*data->cols) + y_axis] = 0;
        }
        else if(alive <=3){
            data->next[(x_axis*data->cols) + y_axis] = 1;
            total_live++;
        }
        else{
            data->next[(x_axis*data->cols) + y_axis] = 0;
        }                
    }
    else{  
        if(alive == 3){
            data->next[(x_axis*data->cols) + y_axis] = 1;
            total_live++; 
        } 
    }
}
/* This function updates the color od the cells in the VISI animation
 * data: the struct of type struct gol_data
 * returns: none */
void update_colors(struct gol_data* data){
  int i, j, r, c, index, buff_i;
    color3 *buff;

    buff = data->image_buff;  
    r = data->rows;
    c = data->cols;

    for (i = 0; i < r; i++) {
        for (j = 0; j < c; j++) {
            index = i*c + j;
            buff_i = (r - (i+1))*c + j;

            // update animation buffer
            if (data->current[index] == 0) {
                buff[buff_i] = c3_red;
            } else if (data->current[index] == 1) {
                buff[buff_i] = c3_green;
            }
     
        }
    }

}

/* This function prints the board to the terminal.
 * data: the struct of type struct gol_data
 * rounds: type int. Number of rounds 
 * returns: none */
void print_board(struct gol_data *data, int round) {

    int i, j;

    /* Print the round number. */
    fprintf(stderr, "Round: %d\n", round);

    for (i = 0; i < data->rows; ++i) {
        for (j = 0; j < data->cols; ++j) {
            if (data->current[i*data->cols + j]){
                fprintf(stderr, " @");
            }
            else{
                fprintf(stderr, " .");
            }
        }
        fprintf(stderr, "\n");
    }

    /* Print the total number of live cells. */
    fprintf(stderr, "Live cells: %d\n\n", total_live);
}


/**********************************************************/
/***** START: DO NOT MODIFY THIS CODE *****/
/* initialize ParaVisi animation */
int setup_animation(struct gol_data* data) {
    /* connect handle to the animation */
    int num_threads = 1;
    data->handle = init_pthread_animation(num_threads, data->rows,
            data->cols, visi_name);
    if (data->handle == NULL) {
        printf("ERROR init_pthread_animation\n");
        exit(1);
    }
    // get the animation buffer
    data->image_buff = get_animation_buffer(data->handle);
    if(data->image_buff == NULL) {
        printf("ERROR get_animation_buffer returned NULL\n");
        exit(1);
    }
    return 0;
}

/* sequential wrapper functions around ParaVis library functions */
void (*mainloop)(struct gol_data *data);

void* seq_do_something(void * args){
    mainloop((struct gol_data *)args);
    return 0;
}

int connect_animation(void (*applfunc)(struct gol_data *data),
        struct gol_data* data)
{
    pthread_t pid;

    mainloop = applfunc;
    if( pthread_create(&pid, NULL, seq_do_something, (void *)data) ) {
        printf("pthread_created failed\n");
        return 1;
    }
    return 0;
}
/***** END: DO NOT MODIFY THIS CODE *****/
/******************************************************/
