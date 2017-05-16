#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#define INPUT_SIZE 50
#define STR2(x) #x
#define STR(X) STR2(X)
#define MAXVAL 1
#define INDEX_FILE "index_file.dat"
#define DATA_FILE "data_file.dat"

/* Function prototypes */
void addNewRecord();
void updateRecord();
void deleteRecord();
void findRecord();
void listRecords();
void flushIndexFile();
bool recordExists(char *query_key);
void printKeyArray();
void swap(int i, int j);
void sort();
int find(char *query);
void synchronise(bool dirty);
void printRecord(char *record);
void deleteAndShift(int index);
void fileOperation(int mode, int index, char new_values[5][INPUT_SIZE+2]);
int sortCompare(const char *a, const char *b);
/* Global definitions */
/* struct for primary key and file index number */
struct record{
    char primary_key[INPUT_SIZE*2+2];
    int id;
};
typedef struct record RECORD;
RECORD records[100]; // store all primary keys here
int cur_rec_num = 0, total_rec_num=0;
FILE *data_file, *index_file;
enum operation  {DELETE, PRINT, UPDATE, SYNC};

/* main */
int main() {
    data_file = fopen(DATA_FILE, "r");
    /* Make synchronise flag 1 while program is working */
    if(data_file) {
        // file exists
        fclose(data_file);
        data_file = fopen(DATA_FILE, "r+b");
        char flag[1];
        rewind(data_file);
        fread(flag, 1, 1, data_file);
        if(flag[0] == '1') synchronise(true);
        else synchronise(false);
        rewind(data_file);
        fwrite("1",1,1,data_file);
        fclose(data_file);
    } else {
        // file doesn't exist
        data_file = fopen(DATA_FILE, "w");
        if(data_file) fwrite("1",1,1,data_file);
        else printf("Cant open the data file!\n");
        fclose(data_file);
    }


    /* declaration for add new record */
    char name[INPUT_SIZE], surname[INPUT_SIZE], address[INPUT_SIZE], notes[INPUT_SIZE];

    int option;
    printf("Main Menu\n=============\n");
    printf("1) Add New Record\n");
    printf("2) Update A Record\n");
    printf("3) Delete A Record\n");
    printf("4) Find A Record\n");
    printf("5) List Records\n");
    printf("6) Exit!\n");

    while(option!=6){
        printf("Option: ");
        scanf("%d", &option);

        switch(option) {
            case 1:
                addNewRecord();
                break;
            case 2:
                listRecords();
                updateRecord();
                break;
            case 3:
                deleteRecord();
                break;
            case 4:
                listRecords();
                findRecord();
                break;
            case 5:
                listRecords();
                break;
            case 6:
                /* Set flag to 0 when closing and saving primary key file */
                data_file = fopen(DATA_FILE, "r+b");
                if(data_file){
                    fseek(data_file, 0, SEEK_SET);
                    fwrite("0", 1, 1, data_file);
                }
                else{
                    printf("Cant open the data file!\n");
                }
                fclose(data_file);
                flushIndexFile();
                break;
            default:
                printf("Invalid option.");
                break;
        }
    }
    return 0;
}
/* synchronise if dirty flag is 1*/
void synchronise(bool dirty){
    printf("Syncing data and index file...\n");
    data_file = fopen(DATA_FILE, "r+b");

    if (data_file) {
        fseek(data_file, 1, SEEK_SET);
        char length_buffer[4], record[INPUT_SIZE * 2 + 2];
        int indicator;
        bool flag;
        while (!feof(data_file)) {
            if (total_rec_num == 3);
            flag = true;
            fread(length_buffer, 3, 1, data_file);
            if (length_buffer[0] == '*') {
                flag = false;
                length_buffer[0] = length_buffer[1];
                length_buffer[1] = length_buffer[2];
                char tmp[2];
                fread(tmp, 1, 1, data_file);
                length_buffer[2] = tmp[0];
                total_rec_num++;
            }
            indicator = atoi(length_buffer);
            strcpy(length_buffer, "");
            if (!flag) fseek(data_file, indicator, SEEK_CUR);
            else {
                char name[INPUT_SIZE], surname[INPUT_SIZE], address[INPUT_SIZE], notes[INPUT_SIZE];
                fread(record, indicator, 1, data_file);
                char *token, str[strlen(record) + 1];
                strcpy(str, record);
                const char s[2] = "|";
                token = strtok(str, s);
                int ctr = 1;
                while (token != NULL) {
                    if (ctr == 1) strcpy(name, token);
                    else if (ctr == 2) strcpy(surname, token);
                    else if (ctr == 3) strcpy(address, token);
                    else if (ctr == 4) strcpy(notes, token);
                    token = strtok(NULL, s);
                    ctr++;
                }
                /* create primary key with given inputs */
                char primary_key[strlen(name) + strlen(surname) + 2];
                snprintf(primary_key, strlen(name) + strlen(surname) + 2, "%s %s", name, surname);
                int i;
                /* make all upper */
                for (i = 0; primary_key[i]; i++) {
                    primary_key[i] = toupper(primary_key[i]);
                }
                if (!recordExists(primary_key)) {
                    if(dirty) {
                        RECORD new_record;
                        strcpy(new_record.primary_key, primary_key);
                        new_record.id = total_rec_num;
                        records[cur_rec_num] = new_record;
                        cur_rec_num++;
                    }
                    total_rec_num++;
                }
            }
        }

    }


    if(!dirty){
        total_rec_num--;
        char *file_contents;
        long index_file_size;
        index_file = fopen(INDEX_FILE, "rb+");
        if(index_file) {
            fseek(index_file, 0, SEEK_END);
            index_file_size = ftell(index_file);
            rewind(index_file);
            file_contents = malloc(index_file_size * (sizeof(char)));
            fread(file_contents, sizeof(char), index_file_size, index_file);
            char *token, str[strlen(file_contents)+1];
            strcpy(str, file_contents);
            const char s[2] = "|";
            token = strtok(str, s);
            int ctr = 0;
            RECORD new_record;
            while(token!=NULL){
                if(ctr%2==0){
                    if(ctr!=0){
                        records[cur_rec_num] = new_record;
                        cur_rec_num++;
                    }
                    strcpy(new_record.primary_key, token);
                }
                else if(ctr%2==1){
                    new_record.id = atoi(token);
                }
                token = strtok(NULL, s);
                ctr++;
            }
            records[cur_rec_num] = new_record;
            cur_rec_num++;
            fclose(index_file);
        }

    }

}
/* add new record */
void addNewRecord(){
    char name[INPUT_SIZE], surname[INPUT_SIZE], address[INPUT_SIZE], notes[INPUT_SIZE];
    /* read data from user */
    printf("Enter name: ");
    scanf(" %[^\n]s", &name);
    printf("Enter surname: ");
    scanf(" %[^\n]s", &surname);
    printf("Enter address: ");
    scanf(" %[^\n]s", &address);
    printf("Enter notes: ");
    scanf(" %[^\n]s", &notes);
    /* get record size for indicator and convert to 3digit form */
    int record_size = strlen(name) + strlen(surname) + strlen(address) + strlen(notes) + 4;
    char record_size_complete[4];
    sprintf(record_size_complete, "%3d", record_size); //32
    int i;
    for(i=0; i<3; i++){
        if(record_size_complete[i] == 32){
            record_size_complete[i] = '0';
        }
    }

    /* create primary key with given inputs */
    char primary_key[strlen(name)+strlen(surname)+2];
    snprintf(primary_key, strlen(name)+strlen(surname)+2,"%s %s", name, surname);

    /* make all upper */
    for(i = 0; primary_key[i]; i++){
        primary_key[i] = toupper(primary_key[i]);
    }

    if(!recordExists(primary_key)){
        RECORD new_record;
        strcpy(new_record.primary_key, primary_key);
        new_record.id = total_rec_num;
        records[cur_rec_num] = new_record;
        cur_rec_num++;
        total_rec_num++;

        sort();
        data_file = fopen(DATA_FILE, "a+");
        char print_data[(4*INPUT_SIZE)+3+4+1];
        sprintf(print_data, "%s%s|%s|%s|%s|", record_size_complete, name, surname, address, notes);

        fwrite(print_data, 1, strlen(print_data), data_file);
        fclose(data_file);
    }
    else{
        printf("A record with same primary key already exists.\n");
    }
}
/* modify a record */
void updateRecord(){
    char query[INPUT_SIZE];
    printf("Get the record with primary key: ");
    scanf(" %[^\n]s", &query);
    if(!recordExists(query))
        printf("Record doesn't exist. Make sure you enter the primary key. (ex: NAME SURNAME)\n");
    else{
        /* read new data from user */
        char new_values[5][INPUT_SIZE+2];

        printf("Enter new name: ");
        scanf(" %[^\n]s", &new_values[1]);
        printf("Enter new surname: ");
        scanf(" %[^\n]s", &new_values[2]);
        printf("Enter new address: ");
        scanf(" %[^\n]s", &new_values[3]);
        printf("Enter new notes: ");
        scanf(" %[^\n]s", &new_values[4]);

        /* get record size for indicator and convert to 3digit form */
        int record_size = strlen(new_values[1]) + strlen(new_values[2]) + strlen(new_values[3]) + strlen(new_values[4]) + 4;
        char record_size_complete[4];
        sprintf(record_size_complete, "%3d", record_size); //32
        int i;
        for(i=0; i<3; i++){
            if(record_size_complete[i] == 32){
                record_size_complete[i] = '0';
            }
        }
        strcpy(new_values[0], record_size_complete);

        /* create primary key with given inputs */
        char primary_key[strlen(new_values[1])+strlen(new_values[2])+2];
        snprintf(primary_key, strlen(new_values[1])+strlen(new_values[2])+2,"%s %s", new_values[1], new_values[2]);
        /* make all upper */
        for(i = 0; primary_key[i]; i++){
            primary_key[i] = toupper(primary_key[i]);
        }

        /* check if new values already exists */
        if(!recordExists(primary_key)) {
            fileOperation(UPDATE, find(query), new_values);

            strcpy(records[find(query)].primary_key, primary_key);

        }
        else{
            printf("A record with these new values already exists.\n");
        }
    }

}
/* delete a record  */
void deleteRecord(){
    char key[INPUT_SIZE];
    printf("Enter record key to delete: ");
    scanf(" %[^\n]s", &key);
    if(recordExists(key)) fileOperation(DELETE, find(key), NULL);
    else printf("Record doesn't exist.\n");
}
/* find a record */
void findRecord(){
    char query[INPUT_SIZE];
    printf("Search with primary key: ");
    scanf(" %[^\n]s", &query);
    if(recordExists(query)) fileOperation(PRINT, find(query), NULL);
    else printf("Record doesn't exist. Make sure you enter the primary key. (ex: NAME SURNAME)\n");
}
/* list all records */
void listRecords(){
    sort();
    if(cur_rec_num < 1) printf("No records found.\n");
    else {
        char letter[2];
        printf("Enter letter to list records start with that letter: \n");
        /* only take first letter of list letter input */
        scanf("%" STR(MAXVAL) "s", letter);
        letter[0] = toupper(letter[0]);
        int i = 0;
        bool found = false;
        for (i = 0; i < cur_rec_num; i++) {
            if(records[i].primary_key[0]==letter[0]) {
                fileOperation(PRINT, records[i].id, NULL);
                found = true;
            }
        }
        if(!found) printf("No record found starting with %s.\n", letter);
    }
}
/* save index file to disk */
void flushIndexFile(){
    index_file = fopen(INDEX_FILE, "wb+");
    if(!index_file) {
        printf("Couldn't open index file.\n");
    }
    else {
        int i = 0;
        for (i = 0; i < cur_rec_num; i++) {
            char print_data[strlen(records[i].primary_key)+3];
            sprintf(print_data, "%s|%d|", records[i].primary_key, records[i].id);
            fwrite(print_data, 1, strlen(print_data), index_file);
        }
    }
    fclose(index_file);
}
/* check if a record already exists */
bool recordExists(char *query_key){
    int i = 0;
    for (i = 0; i < cur_rec_num; i++) {
        if(strcasecmp(records[i].primary_key, query_key) == 0)
            return true;
    }
    return false;
}
/* find in key array */
int find(char *query){
    int i = 0;
    for (i = 0; i < cur_rec_num; i++) {
        if(strcmp(query, records[i].primary_key)==0) return records[i].id;
    }
    return -1;
}
/* data_file operations */
void fileOperation(int mode, int index, char new_values[5][INPUT_SIZE+2]){
    data_file = fopen(DATA_FILE, "r+b");
    if(!data_file){
        printf("Couldn't open data file. \n");
    }
    else{
        char length_buffer[4];
        int indicator, i;
        long left_len = 1;
        fseek(data_file, 0, SEEK_END);
        long file_len = ftell(data_file);
        fseek(data_file, 1, SEEK_SET);

        /* find the searched record */
        for (i = 0; i < index; i++) {
            fread(length_buffer, 3, 1, data_file);
            left_len += 3;
            if(length_buffer[0]=='*'){
                length_buffer[0] = length_buffer[1];
                length_buffer[1] = length_buffer[2];
                char tmp[2];
                fread(tmp, 1, 1, data_file);
                length_buffer[2] = tmp[0];
                left_len++;
            }
            indicator = atoi(length_buffer);
            fseek(data_file, indicator, SEEK_CUR);
            left_len+=indicator;
        }

        /* now at the beginning of the record */
        if(mode == DELETE){
            long right_len = (file_len-left_len);
            char right_text[(int)(right_len)+1];
            fread(right_text, (int)right_len, 1, data_file);
            right_text[right_len] = '\0';
            fseek(data_file, -right_len, SEEK_CUR);


            char print_data[strlen(right_text)+2];
            sprintf(print_data, "*%s", right_text);
            fwrite(print_data, 1, strlen(print_data), data_file);
            deleteAndShift(index);

        }
        else if(mode == PRINT){
            fread(length_buffer, 3, 1, data_file);
            indicator = atoi(length_buffer);
            char record[indicator+1];
            fread(&record, indicator, 1, data_file);
            record[indicator] = '\0';
            printRecord(record);
        }
        else if(mode == UPDATE){
            fread(length_buffer, 3, 1, data_file);
            left_len += 3;
            if(length_buffer[0]=='*'){
                length_buffer[0] = length_buffer[1];
                length_buffer[1] = length_buffer[2];
                char tmp[2];
                fread(tmp, 1, 1, data_file);
                length_buffer[2] = tmp[0];
                left_len++;
            }
            indicator = atoi(length_buffer);
            fseek(data_file, indicator, SEEK_CUR);
            left_len+=indicator;
            /* store right side */
            long right_len = (file_len-left_len);
            char right_text[(int)(right_len)+1];
            fread(right_text, (int)right_len, 1, data_file);
            right_text[right_len] = '\0';
            /* store left side */
            char left_text[(int)left_len-indicator-3+1];
            fseek(data_file, 0, SEEK_SET);
            fread(left_text, (int)left_len-indicator-3, 1, data_file);
            /* clear the text file */
            fclose(data_file);
            data_file = fopen(DATA_FILE, "wb+");
            char print_data[strlen(left_text)+strlen(right_text)+(4*INPUT_SIZE)+1];
            sprintf(print_data, "%s%s%s|%s|%s|%s|%s", left_text, new_values[0], new_values[1],
                    new_values[2], new_values[3], new_values[4], right_text);
            fwrite(print_data, 1, strlen(print_data), data_file);


        }
        fclose(data_file);
    }
}
/* put deleted record to the end, reduce idx by 1 */
void deleteAndShift(int index){
    int i;
    for(i = index; i<cur_rec_num; i++){
        swap(i, i+1);
    }
    cur_rec_num--;
}
/* sort the primary key array */
void sort(){
   int i,j;
    for(i=1; i<cur_rec_num; i++){
        j = i;
        while(j>0 && sortCompare(records[j].primary_key, records[j - 1].primary_key)<0){
            swap(j,j-1);
            j--;
        }
    }

}
/* augmented strcmp for sorting primary keys */
int sortCompare(const char *a, const char *b) {
    int my_result;
    while (*a || *b)
    {
        if ((my_result = tolower(*a) - tolower(*b)))
            return my_result;
        if (*a != *b)
            return (islower(*a)) ? -1 : 1;
        a++;
        b++;
    }
    return 0;
}
/* swap i and jth elements in index array */
void swap(int i, int j){
    RECORD tmp_record = records[i];
    records[i] = records[j];
    records[j] = tmp_record;
}
/* print all fields of a record */
void printRecord(char *record){
    char *token, str[strlen(record)+1];
    strcpy(str, record);
    const char s[2] = "|";
    token = strtok(str, s);
    int ctr = 1;
    while(token!=NULL){
        if(ctr==1) printf("Name: %s ", token);
        else if(ctr==2) printf("Surname: %s ", token);
        else if(ctr==3) printf("Address: %s ", token);
        else if(ctr==4) printf("Note: %s ", token);
        token = strtok(NULL, s);
        ctr++;
    }
    printf("\n");
}
