// TAM LU

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>

#define ROOM_NUM 7          // Total number of rooms
#define CONNECTION_MAX 6   // Maximum number of connections

struct Room {
    char* name;          // Room name 
    struct Room* connections;  // Every connection to other rooms
    int type;          // Room type 1: START_ROOM 2: MID_ROOM 3: END_ROOM
    int connection_num;  // Number of connections in room
};

struct Room rooms[ROOM_NUM];

char* get_newest_dir();
void init_rooms(char*);
void read_room_file(char*, char*, int);
void start_game();
void free_rooms();

int main() {
    char* path_name = get_newest_dir(); 
    init_rooms(path_name);
    start_game();
    free_rooms();
}

// get the latest and most recently modified directory starting with "luta.rooms."
char* get_newest_dir() {
    DIR  *dirp = opendir(".");
    char* dir_name;
    dir_name = malloc(sizeof(char)*50);
    struct dirent* dp;
    struct stat file_stat;
    long int latest = 0;
    while (dp = readdir(dirp)) {
        if (strncmp(dp->d_name, "luta.rooms.", 11) == 0) {
            memset(&file_stat, 0, sizeof(file_stat));
            if (stat(dp->d_name, &file_stat) < 0) {
                printf("Error getting info on file\n"); fflush(stdout); continue;
            }
            if ((file_stat.st_mode & S_IFDIR) != S_IFDIR) {
                continue;
            }
            if (file_stat.st_mtime > latest) {
                strcpy(dir_name, dp->d_name);
                latest = file_stat.st_mtime;
            }
        }
    }
    closedir(dirp);
    return dir_name;
}

// read the rooms in the latest directory
void init_rooms(char* path_name) {
    DIR *dirp = opendir(path_name);
    char* room_name;
    struct dirent* fp;

    int i = 0;
    while(fp = readdir(dirp)) {
        if (strcmp(fp->d_name, "..") != 0 && strcmp(fp->d_name, ".") != 0) { //don't look at files with these values
            room_name = malloc(50*sizeof(char));
            strcpy(room_name, fp->d_name); //get the name of the file and place them into room_name
            rooms[i].name = room_name; 
            rooms[i].connections = malloc(6*sizeof(struct Room));
            read_room_file(path_name, room_name, i);
            i++;
        }
    }
}

//read the information in each individual room and place them into the necessary variables in each room
void read_room_file(char* dir_name, char* room_name, int i) {
    char* path_name = malloc(50*sizeof(char));
    char* line = malloc(50*sizeof(char));
    char* room_string;
    char connection_arr[6];
    int cout, line_length, j;
    sprintf(path_name, "%s/%s", dir_name, room_name); // join values
    FILE* fp = fopen(path_name, "r");
    while(fgets(line, 50, fp)) { //while can get line and not the end of file
        if(strncmp(line, "ROOM NAME", 9) == 0) { //if line at the beginning is equal to ROOM NAME, don't do anything
            continue;
        }
        else if (strncmp(line, "ROOM TYPE", 9) == 0) { //if line at the beginning is ROOM TYPE, find what the variable starts with and find the type correlated to it
            if (line[11] == 'S') { // if start
                rooms[i].type = 1;
            }
            else if (line[11] == 'E') { // if end
                rooms[i].type = 3;
            }
            else  { //middle
                rooms[i].type = 2;
            }
            break;   
        }
        else if (strncmp(line, "CONNECTION ", 11) == 0) { //if line at the beginning is CONNECTION, put the value after "CONNECTION %d: " into an array
            room_string = malloc(50*sizeof(char));
            line_length = strlen(line)-1;
            for (j = 14; j < line_length; j++) {
                room_string[j - 14]= line[j]; //-14 to start at 0
            }
            room_string[j-14] = '\0'; //j at the end is equalt no line length, so subtract by 14 and end with null
            rooms[i].connections[count].name = room_string; //set the variable equal to the name of a connection
            count++;
        }
    }
    rooms[i].connection_num = count; //count at the end of reading the file is equal to the amount of connections that has been added. 
    free(path_name);
    free(line);
    fclose(fp);
}

//start the adventure game
void start_game() {
    int i, j, start, end;
    int steps = 0;
    int arr_size = 100; //start max steps as 100 as size and increase when necessary
    char** steps_arr = malloc(sizeof(char**)*arr_size); //take in rooms that the user goes to
    
    int good_input;
    char* user_input;
    for (i=0; i < ROOM_NUM; i++) {
        //iterate through the types and find the start and end
        if (rooms[i].type == 1) start = i; 
        else if (rooms[i].type == 3) end = i;
    }
    //start
    int current = start;
    while (current != end) { //while the player hasn't reached the end of the game
        steps_arr[steps] = rooms[current].name;
        user_input = NULL;
        good_input = 0;
        ssize_t line; size_t length = 0; //use line and len to take in user inputs
        printf("\nCURRENT LOCATION: %s\n", rooms[current].name); fflush(stdout);
        printf("POSSIBLE CONNECTIONS: "); fflush(stdout);
        for (i=0; i < rooms[current].connection_num; i++) { 
            if (i != rooms[current].connection_num -1) 
                printf("%s, ", rooms[current].connections[i].name); //if not the end, do comma
            else 
                printf("%s.\n", rooms[current].connections[i].name); //if the end, do period
            fflush(stdout);
        }
        printf("WHERE TO? >"); fflush(stdout); 
        line = getline(&user_input, &length, stdin);
        user_input[strlen(user_input)-1] = '\0';
        for (i = 0; i < rooms[current].connection_num; i++) {
            if (strcmp(user_input, rooms[current].connections[i].name) == 0) { //if one of the connection names in the room is the same as input, then continue
                for (j=0; j< ROOM_NUM; j++) {
                    if (strcmp(rooms[current].connections[i].name, rooms[j].name) == 0) { //find the actual room index and go to it in the next step
                        current = j;
                        steps++;
                        good_input = 1;
                        break;
                    }
                }
            }
            if (good_input == 1) break;
        }
        if (good_input == 0) {
            printf("\nHUH? I DON't UNDERSTAND THAT ROOM. TRY AGAIN.\n"); fflush(stdout);
        }
    }
    steps_arr[steps] = rooms[current].name; //place the end step into the array to print out
    printf("\nYOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n"); fflush(stdout);
    printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", steps); fflush(stdout);
    for (i=0; i < steps+1; i++) {
        printf("%s\n", steps_arr[i]); fflush(stdout);
    }
    free(steps_arr);
}

//free array after use
void free_rooms() {
    int i;
    for (i=0; i < ROOM_NUM; i++) {
        free(rooms[i].connections);
    }
}