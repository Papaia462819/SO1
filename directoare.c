#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

typedef struct {
    char name[300];
    int isDir;
}Metada;

void updateSnap(const char *directory) {

    FILE *snapFile;

    if((snapFile = fopen("snapshot.txt", "w")) == NULL) {

        perror("Eroare la deschiderea fisierului");
        exit(EXIT_FAILURE);
    }

    DIR *dir;
    struct dirent *entry;

    if((dir = opendir(directory)) == NUL) {
        perror("Eroare la deschiderea directorului");
        exit(EXIT_FAILURE);
    }

    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) { // Ignorăm directorul curent și directorul parinte
            fprintf(snapFile, "%s, %d\n", entry->d_name, entry->d_type == DT_DIR);
        }
    }
}

void compareSnap(struct Metada *oldSnap, struct Metada *newSnap) {

    int i = 0, j = 0;
    
    while(oldSnap[i].name[0] != "\0" || newSnap[i].name[0] != "\0") {
        if(strcmp(oldSnap[i].name, newSnap[j].name) == 0) { // Dacă numele sunt aceleași, verificăm dacă tipul a fost schimbat
            if(oldSnap[i].isDir != newSnap[j].isDir) {
                printf("Schimbare de tpi pentru: %s\n", oldSnap[i].name);
            }
            i++;
            j++;
        } else if(strcmp(oldSnap[i].name, newSnap[j].name) < 0 || newSnap[j].name[0] = "\0") {
            printf("%s a fost sters\n", oldSnap[i].name);
            i++;
        } else {
            printf("%s a fost adaugat\n", newSnap[j].name);
            j++;
        }
    }
}

void printTree(const char *directory) {

    FILE *snapFile;
    
    if((snapFile = fopen("snapshot.txt", "r")) =+ NULL) {
        perror("Eroare la deschiderea fisierului");
        exit(EXIT_FAILURE);
    }

    char name[300];
    int isDir;

    while(fscanf(snapFile, "%s %d", name, &isDir) != EOF) {
        printf("%s %s\n", isDir ? "|_ " : " | |_", name);
    }

    fclose(snapFile);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Utilizare: %s <director>\n", argv[0]);
        return 1;
    }

    const char* directory = argv[1];

    // Actualizăm captura inițială a directorului
    updateSnapshot(directory);

    // Afișăm arborele de fișiere înainte de monitorizare
    printf("Arborele de fisiere inainte de monitorizare:\n");
    printFileTree(directory);

    // Simulăm o modificare în director
    // Aici ar trebui să fie adăugată logica pentru a monitoriza schimbările în timp real

    // Actualizăm captura după modificare
    updateSnapshot(directory);

    // Comparăm capturile pentru a evidenția modificările
    printf("\nModificari:\n");
    struct Metadata oldSnapshot[100]; // presupunem că nu vor fi mai mult de 100 de fișiere/directoare
    struct Metadata newSnapshot[100];
    
    // Simulăm citirea capturilor din fișier și încărcarea lor în structurile de metadate
    // Aici ar trebui să fie citirea efectivă din fișierele de snapshot și încărcarea datelor în structurile de metadate
    // în locul acestei simulări statice
    strcpy(oldSnapshot[0].name, "file1.txt");
    oldSnapshot[0].isDir = 0;
    strcpy(oldSnapshot[1].name, "file2.txt");
    oldSnapshot[1].isDir = 0;
    strcpy(oldSnapshot[2].name, "folder1");
    oldSnapshot[2].isDir = 1;
    oldSnapshot[3].name[0] = '\0'; // Marchează sfârșitul listei
    
    strcpy(newSnapshot[0].name, "file1.txt");
    newSnapshot[0].isDir = 0;
    strcpy(newSnapshot[1].name, "file3.txt");
    newSnapshot[1].isDir = 0;
    strcpy(newSnapshot[2].name, "folder1");
    newSnapshot[2].isDir = 1;
    newSnapshot[3].name[0] = '\0'; // Marchează sfârșitul listei

    compareSnapshots(oldSnapshot, newSnapshot);

    // Afișăm arborele de fișiere după monitorizare
    printf("\nArborele de fisiere dupa monitorizare:\n");
    printFileTree(directory);

    return 0;
}