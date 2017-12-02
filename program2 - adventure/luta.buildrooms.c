// TAM LU

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define ROOM_NUM 7       // Total number of rooms
#define ROOM_NAME_MAX 10 // Maxmimum number of rooms in array
#define CONNECTION_MAX 6 // Maximum number of connections
#define CONNECTION_MIN 3 // Minimum number of connections

struct Room {
    char* name;          // Room name 
    struct Room* connections;  // Every connection to other rooms
    int type;          // Room type 1: START_ROOM 2: MID_ROOM 3: END_ROOM
    int connection_num;  // Number of connections in room
    int connection_max;
};

struct Room rooms[ROOM_NUM];
char *room_names[10] = {"betrayal", "extra", "bitter", "amber", "complicated", "collider", "elite", "divinity", "dormant", "bunny"};

char* init_dir();
void init_files(char*);
void create_connections();
bool is_graph_full();
void add_random_connection();
struct Room get_random_room(int*);
bool can_add_connection_from(struct Room);
bool connection_already_exists(struct Room, struct Room);
void connect_room(struct Room, struct Room, int);
bool is_same_room(struct Room, struct Room);
void init_room(int, int);
bool room_type_already_exists(int, int);
bool no_more_rooms(int, int*, int*);
void write_files(char*);
bool has_less_than_min(int*);
void free_rooms();

int main() {
    srand (time(NULL)); 
    char* path_name = init_dir();
    init_files(path_name);
    create_connections();
    write_files(path_name);
    free(path_name);
    free_rooms();
}

void free_rooms() {
    int i;
    for (i=0; i < ROOM_NUM; i++) {
        free(rooms[i].connections);
    }
}

//initialize the directory with the name "luta.rooms.x" where x is the process ID
char* init_dir() {
    int pid = getpid(); // Get process ID
    char prefix[] = "luta.rooms.";
    char *room_dir;
    room_dir = malloc(sizeof(char)*50); // Create 50 indexes 
    sprintf(room_dir, "%s%d", prefix, pid); // Append pin to the end of prefix
    mkdir(room_dir, 0755); // Make the directory
    return room_dir;
}

//initialize the files in the directory with the room names equal to 7 of the indexes in the room_name array
void init_files(char *path_name) {
    int i, n;
    FILE* file;
    char temp[128];
    char *file_path;
    // Apply room names
    for (i=0; i < ROOM_NUM; i++) {     
        int n;
        do {
            n = rand()%ROOM_NAME_MAX;
        } while (room_names[n] == NULL); // Keep finding an index that hasn't been used
        init_room(i, n);
        strcpy (temp, path_name); 
        sprintf(file_path, "%s/%s", path_name, room_names[n]); fflush(stdout);  // Append room name to directory path
        file = fopen(file_path, "ab+");
        room_names[n] = NULL; // Set to NULL so to avoid using it again
        strcpy (path_name, temp); //Set path name back to it's original value
    }
}

//initialize each room information
void init_room(int cur_room_num, int n) {
    rooms[cur_room_num].name = room_names[n]; // Set the name of room
    rooms[cur_room_num].connection_num = 0; // Set connection number to 
    rooms[cur_room_num].connections = malloc(6*sizeof(struct Room)); // set max num of connections possible
    rooms[cur_room_num].connection_max = 6-rand()%4; // set up a random max of connections from 3-6
    rooms[cur_room_num].type = 0;
    while (rooms[cur_room_num].type == 0) {
        int type = 3-rand()%3; // find the type number
        if (!room_type_already_exists(cur_room_num, type)) {
            rooms[cur_room_num].type = type; //if the type 1 or 2 already exists, then make it equal to 2
        }
    }
}

//check if room type START and END already exists
bool room_type_already_exists(int cur_room_num, int type) {
    int i;
    for (i=0; i < cur_room_num; i++) {
        if (type == rooms[i].type && type != 2)
            return true;
    }
    return false;
}

//create the connections for each room until graph is full
void create_connections() {
    // Create all connections in graph
    while (is_graph_full() == false)
    {
        add_random_connection();
    }
}

// Returns true if all rooms have 3 to 6 outbound connections, false otherwise
bool is_graph_full() {
    int i;
    for (i = 0; i < ROOM_NUM; i++) {
        if(rooms[i].connection_num < rooms[i].connection_max) { //if a room is not done connecting to maximum nums, then return false
            return false;
        }
    }
    return true;
}

// Adds a random, valid outbound connection from a Room to another Room
void add_random_connection() {
    struct Room A;  // Maybe a struct, maybe global arrays of ints
    struct Room B;
    int found_room = 0;
    int i, j;
    j=-1;
    while(true)
    {
        A = get_random_room(&i);
        if (can_add_connection_from(A) == true)
        break;
    }
    do
        {
            B = get_random_room(&j);
        }
        while(can_add_connection_from(B) == false || is_same_room(A, B) == true || connection_already_exists(A, B) == true);
    connect_room(A, B, i);  // TODO: Add this connection to the real variables, 
    connect_room(B, A, j);  //  because this A and B will be destroyed when this function terminates
}

// Returns a random Room, does NOT validate if connection can be added
struct Room get_random_room(int *i) {
    *i = rand()%ROOM_NUM;
    int n = *i;
    return rooms[n];
}


// Returns true if a connection can be added from Room x (< 6 outbound connections), false otherwise
bool can_add_connection_from(struct Room x) {
    if (x.connection_num < 6) 
        return true;
    else 
        return false;
}

// Returns true if a connection from Room x to Room y already exists, false otherwise
bool connection_already_exists(struct Room x, struct Room y) {
    int i, j;
    for (i = 0; i < x.connection_num; i++) {
        if(x.connections[i].name == y.name) {
            return true;
        }
    }
    return false;
}

// Connects Rooms x and y together, does not check if this connection is valid
void connect_room(struct Room x, struct Room y, int i) {
    rooms[i].connections[rooms[i].connection_num] = y; // connect
    rooms[i].connection_num++; // increase connection num
}

// Returns true if Rooms x and y are the same Room, false otherwise
bool is_same_room(struct Room x, struct Room y) {
    if (x.name == y.name) {
        return true;
    }
    return false;
}

//write the information created in the room array into individual room files
void write_files(char* path_name) {
    char buffer[50];
    FILE *fp;
    int i, j;
    for (i=0; i<ROOM_NUM; i++) {
        sprintf(buffer, "%s/%s", path_name, rooms[i].name); fflush(stdout); 
        fp = fopen(buffer, "w");
        fprintf(fp, "ROOM NAME: %s\n", rooms[i].name); 
        for(j = 0; j < rooms[i].connection_max; j++) {
            fprintf(fp, "CONNECTION %d: %s\n", j+1, rooms[i].connections[j].name);
        }
        if (rooms[i].type == 1)
            fprintf(fp, "ROOM TYPE: START_ROOM\n");
        else if (rooms[i].type == 2)
            fprintf(fp, "ROOM TYPE: MID_ROOM\n");
        else
            fprintf(fp, "ROOM TYPE: END_ROOM\n");
    }
}