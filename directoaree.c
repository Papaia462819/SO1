#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

// Structura pentru a stoca metadatele fiecărei intrări
struct EntryMetadata {
    char name[256];
    int size;
    time_t lastModified;
    // Alte metadate cum ar fi permisiunile pot fi adăugate aici
};

// Funcție pentru a parcurge directorul și subdirectoarele și a scrie metadatele în fișier
void traverseDirectory(char *path, int snapshotFile) {
    DIR *dir;
    struct dirent *entry;
    struct stat fileStat;
    
    if ((dir = opendir(path)) != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            // Ignorăm . și ..
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            
            char filePath[512];
            sprintf(filePath, "%s/%s", path, entry->d_name);
            
            // Obținem metadatele fișierului/directorului
            if (stat(filePath, &fileStat) < 0)
                continue;
            
            // Salvăm metadatele în fișier
            char metadata[512];
            sprintf(metadata, "Nume: %s, Dimensiune: %ld bytes, Ultima modificare: %s", entry->d_name, fileStat.st_size, ctime(&fileStat.st_mtime));
            write(snapshotFile, metadata, strlen(metadata));
            
            // Verificăm dacă este director și parcurgem recursiv
            if (S_ISDIR(fileStat.st_mode))
                traverseDirectory(filePath, snapshotFile);
        }
        closedir(dir);
    } else {
        perror("Eroare la deschiderea directorului");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Utilizare: %s <director>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Directorul de monitorizat este dat ca argument în linia de comandă
    char *directory = argv[1];
    
    // Deschidem fișierul de snapshot pentru scriere (în modul adăugare pentru a actualiza la fiecare rulare)
    int snapshotFile = open("Snapshot.txt", O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
    if (snapshotFile == -1) {
        perror("Eroare la crearea sau deschiderea fișierului de snapshot");
        return EXIT_FAILURE;
    }
    
    // Scriem metadatele în fișier pentru directorul principal și subdirectoarele sale
    char header[512];
    sprintf(header, "Snapshot pentru directorul: %s\n", directory);
    write(snapshotFile, header, strlen(header));
    traverseDirectory(directory, snapshotFile);
    
    // Închidem fișierul de snapshot
    close(snapshotFile);
    
    return EXIT_SUCCESS;
}
