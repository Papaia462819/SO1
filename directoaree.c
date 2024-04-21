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
            char timeBuffer[128];
            strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", localtime(&fileStat.st_mtime));
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
    // Verifica nr de argumente
    if (argc < 3 || argc > 12) {
        fprintf(stderr, "Usage: %s <output_directory> <directory1> ... <directoryN> (max 10 directories)\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Primul arg este output directory pentru fisierul de snapshot
    char *outputDirectory = argv[1];
    char outputPath[512];
    sprintf(outputPath, "%s/Snapshot.txt", outputDirectory);
    
    // creaza fisierul de snapshot
    int snapshotFile = open(outputPath, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
    if (snapshotFile == -1) {
        perror("Error creating or opening the snapshot file");
        return EXIT_FAILURE;
    }
    
    // Scriem metadatele în fișier pentru directorul principal și subdirectoarele sale
    for(int i = 2; i < argc; i++) {
    char header[512];
    sprintf(header, "Snapshot pentru directorul: %s\n", directory);
    write(snapshotFile, header, strlen(header));
    traverseDirectory(directory, snapshotFile);
    
    // Închidem fișierul de snapshot
    close(snapshotFile);
    
    return EXIT_SUCCESS;
}
