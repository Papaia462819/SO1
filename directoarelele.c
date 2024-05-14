#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define SNAPSHOT_FILE "snapshot.txt"
#define PIPE_READ_END  0
#define PIPE_WRITE_END 1

// Funcție pentru a verifica dacă un fișier are toate drepturile lipsă
int has_all_permissions_missing(mode_t mode) {
    return !(mode & (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH));
}

// Funcție pentru a executa scriptul pentru verificarea fișierelor malitioase
void check_for_malicious(const char *file_path, int pipe_fd) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("Eroare la fork");
        exit(1);
    } else if (pid == 0) {
        execl("/bin/sh", "sh", "verify_for_malicious.sh", file_path, NULL);
        perror("Eroare la exec");
        exit(1);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status != 0) {
                // Trimite numele fișierului periculos prin pipe către procesul părinte
                if(write(pipe_fd, file_path, strlen(file_path) + 1) == -1) {
                    perror("Eroare la scrierea în pipe");
                    exit(1);
                }  // +1 pentru a include terminatorul null
            } else {
                // Trimite un mesaj "SAFE" prin pipe către procesul părinte
                char safe_message[] = "SAFE";
                if (write(pipe_fd, safe_message, strlen(safe_message) + 1) == -1) { 
                    perror("Eroare la scrierea în pipe");
                    exit(1); // +1 pentru a include terminatorul null
                }
            }
        }
    }
}


// Obține și scrie metadatele unui fișier sau director într-un fișier de snapshot
void get_file_metadata(const char *path, int snapshot_fd, int pipe_fd) {
    
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
        if (write(snapshot_fd, buffer, len) == -1) { 
            perror("Eroare la scrierea în fișierul de snapshot");
            exit(1);
        }

        if (has_all_permissions_missing(file_stat.st_mode)) {
                check_for_malicious(path, pipe_fd);
        }
    }
}

// Parcurge recursiv directoarele începând de la dir_path și generează snapshot-uri
void generate_snapshot(const char *dir_path, int snapshot_fd, int pipe_fd) {
    
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
                get_file_metadata(full_path, snapshot_fd, pipe_fd);
                generate_snapshot(full_path, snapshot_fd, pipe_fd);
            } else {
                // Dacă este fișier, obține doar metadatele
                get_file_metadata(full_path, snapshot_fd, pipe_fd);
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

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("Eroare la crearea pipe-ului\n");
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
            close(pipe_fd[PIPE_READ_END]);
            // Procesul copil: deschide/crează fișierul snapshot
            int snapshotul = open(SNAPSHOT_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (snapshotul == -1) {
                perror("Eroare la deschiderea fisierului\n"); 
                return 1;
            }

            // Generează snapshot pentru directorul specificat
            generate_snapshot(argv[i], snapshotul, pipe_fd[PIPE_WRITE_END]);

            close(snapshotul);
            close(pipe_fd[PIPE_WRITE_END]);
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
