#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

#define SNAPSHOT_FILE "snapshot.txt"

// Obține și scrie metadatele unui fișier sau director într-un fișier de snapshot
void get_file_metadata(const char *path, int snapshot_fd) {
    
    struct stat file_stat;
    // Verifică dacă stat poate obține informații despre fișier/director
    if (stat(path, &file_stat) == 0) {
        // Buffer pentru stocarea permisiunilor fișierului
        char permissions[11];
        snprintf(permissions, sizeof(permissions), "%c%c%c%c%c%c%c%c%c%c",
                 (S_ISDIR(file_stat.st_mode)) ? 'd' : '-',
                 (file_stat.st_mode & S_IRUSR) ? 'r' : '-',
                 (file_stat.st_mode & S_IWUSR) ? 'w' : '-',
                 (file_stat.st_mode & S_IXUSR) ? 'x' : '-',
                 (file_stat.st_mode & S_IRGRP) ? 'r' : '-',
                 (file_stat.st_mode & S_IWGRP) ? 'w' : '-',
                 (file_stat.st_mode & S_IXGRP) ? 'x' : '-',
                 (file_stat.st_mode & S_IROTH) ? 'r' : '-',
                 (file_stat.st_mode & S_IXOTH) ? 'x' : '-',
                 (file_stat.st_mode & S_IWOTH) ? 'w' : '-');

        // Determină tipul elementului (fișier sau director)
        char type[10];
        if (S_ISDIR(file_stat.st_mode)) {
            strcpy(type, "Directory");
        } else {
            strcpy(type, "File");
        }

        // Buffer pentru compunerea și scrierea datelor în fișierul de snapshot
        char buffer[1024];
        int len = snprintf(buffer, sizeof(buffer), "Type: %s\nPath: %s\nSize: %ld bytes\nLast modified: %s"
                          "Permissions: %s\n\n", type, path, file_stat.st_size, ctime(&file_stat.st_mtime),
                          permissions);
        write(snapshot_fd, buffer, len);
    }
}

// Parcurge recursiv directoarele începând de la dir_path și generează snapshot-uri
void generate_snapshot(const char *dir_path, int snapshot_fd) {
    
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;

    // Încearcă să deschidă directorul specificat
    if ((dir = opendir(dir_path)) == NULL) {
        perror("Eroare la deschiderea directorului\n");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        // Ignoră directoarele curente și părinte
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Construiește calea completă a fiecărui fișier/director
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        if (stat(full_path, &file_stat) == 0) {
            // Dacă este director, obține metadatele și explorează mai departe
            if (S_ISDIR(file_stat.st_mode)) {
                get_file_metadata(full_path, snapshot_fd);
                generate_snapshot(full_path, snapshot_fd);
            } else {
                // Dacă este fișier, obține doar metadatele
                get_file_metadata(full_path, snapshot_fd);
            }
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    
    int status;
    pid_t p;

    // Verifică dacă sunt specificate argumentele necesare
    if (argc < 2) {
        perror("Eroare\n");
        return 1;
    }

    // Procesează fiecare argument primit (calea directorului)
    for (int i = 1; i < argc; i++) {
        pid_t pid = fork();
        // Gestionarea erorilor de fork
        if (pid == -1) {
            perror("Eroare\n");
            return 1;
        } else if (pid == 0) {
            // Procesul copil: deschide/crează fișierul snapshot
            int snapshotul = open(SNAPSHOT_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (snapshotul == -1) {
                perror("Eroare la deschiderea fisierului\n");
                return 1;
            }

            // Generează snapshot pentru directorul specificat
            generate_snapshot(argv[i], snapshotul);

            close(snapshotul);
            printf("Procesul de PID %d s-a incheiat cu codul 0\n", getpid());
            return 0;
        }
    }

    // Procesul părinte așteaptă terminarea tuturor proceselor copil
    while ((p = wait(&status)) > 0) {
        if (WIFEXITED(status)) {
            printf("Procesul de PID %d s-a incheiat cu codul %d\n", p, WEXITSTATUS(status));
        }
    }

    return 0;
}
