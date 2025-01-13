#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ENTRIES 500
#define MAX_LINE_LENGTH 256

struct Person {
    char RFID[11];
    char fornavn[30];
    char efternavn[30];
    int pap;
    int metal;
    int plastik;
    char dato[20];
};

typedef struct Person Person;

int check_for_id(Person personer[], int total, char *id_input);

int main() {
    Person personer[MAX_ENTRIES];
    int count = 0;

    char id_input[11];
    printf("Input your ID number: ");
    scanf("%10s", id_input);

    FILE *file = fopen("RFID_numbers2.txt", "r");
    if (file == NULL) {
        perror("Kunne ikke åbne filen");
        return 1;
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        if (count >= MAX_ENTRIES) {
            fprintf(stderr, "For mange poster i filen. Maksimalt tilladte: %d\n", MAX_ENTRIES);
            break;
        }
        if (sscanf(line, "%10s %s %s Pap %d Metal %d Plastik %d %s",
                   personer[count].RFID,
                   personer[count].fornavn,
                   personer[count].efternavn,
                   &personer[count].pap,
                   &personer[count].metal,
                   &personer[count].plastik,
                   personer[count].dato) == 7) {
            count++;
        } else {
            fprintf(stderr, "Fejl i dataformat: %s\n", line);
        }
    }

    fclose(file);

    printf("%-15s %-15s %-10s %-10s %-10s %-10s\n", 
           "Fornavn", "Efternavn", "Pap", "Metal", "Plastik", "Dato");
    printf("-------------------------------------------------------------------------------\n");

    int index = check_for_id(personer, count, id_input);
    if (index != -1) {
        printf("%-15s %-15s %-10d %-10d %-10d %-15s\n",  
               personer[index].fornavn, 
               personer[index].efternavn, 
               personer[index].pap, 
               personer[index].metal, 
               personer[index].plastik, 
               personer[index].dato);
    } else {
        printf("\nID ikke fundet\n");
    }

    return 0;
}

int check_for_id(Person personer[], int total, char *id_input) {
    for (int i = 0; i < total; i++) {
        if (strcmp(personer[i].RFID, id_input) == 0) {
            return i;
        }
    }
    return -1;
}
