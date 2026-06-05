#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include "options.h"
#include "unistd.h"
#include "fcntl.h"
#include "time.h"

#define MAX_SIZE 10

void FatalError(char c, const char* msg, int exit_status);
void PrintCopymasterOptions(struct CopymasterOptions* cpm_options);

mode_t get_modifier(char options[4]){
    if (!(options[1] == '+' || options[1] == '-'))
        return 0;

    if (options[0] == 'u'){
        if (options[2] == 'r') return S_IRUSR;
        else if (options[2] == 'w') return S_IWUSR;
        else if (options[2] == 'x') return S_IXUSR;
        else return 0;
    }
    else if (options[0] == 'g'){
        if (options[2] == 'r') return S_IRGRP;
        else if (options[2] == 'w') return S_IWGRP;
        else if (options[2] == 'x') return S_IXGRP;
        else return 0;
    }
    else if (options[0] == 'o'){
        if (options[2] == 'r') return S_IROTH;
        else if (options[2] == 'w') return S_IWOTH;
        else if (options[2] == 'x') return S_IXOTH;
        else return 0;
    }
    else return 0;
}

int main(int argc, char* argv[]){
    // Kontrola hodnot prepinacov
    struct CopymasterOptions cpm_options = ParseCopymasterOptions(argc, argv);

    // Vypis hodnot prepinacov odstrante z finalnej verzie
    PrintCopymasterOptions(&cpm_options);

    // Osetrenie prepinacov pred kopirovanim
    if (cpm_options.fast && cpm_options.slow) {
        fprintf(stderr, "CHYBA PREPINACOV\n");
        exit(EXIT_FAILURE);
    }
//    if (cpm_options.overwrite && cpm_options.append) {
//        fprintf(stderr, "CHYBA PREPINACOV\n");
//        exit(EXIT_FAILURE);
//    }

    // TODO Nezabudnut dalsie kontroly kombinacii prepinacov ...

    if (cpm_options.append && cpm_options.overwrite)
        cpm_options.append = 0;
    if (cpm_options.create && cpm_options.append)
        cpm_options.append = 0;
//    if (cpm_options.directory && argv[4] != NULL) {
//        fprintf(stderr, "CHYBA PREPINACOV\n");
//        exit(EXIT_FAILURE);
//    }

    //-------------------------------------------------------------------
    // Kopirovanie suborov
    //-------------------------------------------------------------------

    // TODO Implementovat kopirovanie suborov

    int return_error = 21;
    int infile = open(cpm_options.infile, O_RDONLY);

    if (cpm_options.link){
        return_error = 30;
        if (infile == -1) {
            fprintf(stderr, "VSTUPNY SUBOR NEEXISTUJE\n");
            return return_error;
        }
        if (link(cpm_options.infile, cpm_options.outfile) == -1){
            fprintf(stderr, "VYSTUPNY SUBOR UZ EXISTUJE\n");
            return return_error;
        }
    }
    else if (infile == -1){
        fprintf(stderr, "SUBOR NEEXISTUJE\n");
        return return_error;
    }

    int flags = O_CREAT|O_WRONLY|O_TRUNC;
    struct stat infile_stats;
    fstat(infile, &infile_stats);
    mode_t modes = infile_stats.st_mode;

    if (cpm_options.create){
        return_error = 23;
        flags = O_CREAT|O_WRONLY|O_EXCL;
        modes = cpm_options.create_mode;
    }
    if (cpm_options.overwrite){
        return_error = 24;
        flags = O_WRONLY|O_TRUNC;
    }
    if (cpm_options.append){
        return_error = 22;
        flags = O_APPEND|O_WRONLY;
    }
    if (cpm_options.lseek){
        return_error = 33;
        flags = O_WRONLY;
        off_t pos1 = lseek(infile, cpm_options.lseek_options.pos1, SEEK_SET);
        if (pos1 == -1){
            fprintf(stderr, "CHYBA POZICIE infile\n");
            return return_error;
        }
    }
    if (cpm_options.chmod) return_error = 34;
    if (cpm_options.inode){
        return_error = 27;
        if (!S_ISREG(infile_stats.st_mode)) {
            fprintf(stderr, "ZLY TYP VSTUPNEHO SUBORU\n");
            return return_error;
        }
        else if (infile_stats.st_ino != cpm_options.inode_number){
            fprintf(stderr, "ZLY INODE\n");
            return return_error;
        }
    }
    if (cpm_options.sparse) return_error = 41;

    //    umask(S_IWGRP | S_IWOTH | S_IROTH);
    if (cpm_options.umask) {
        mode_t mask = umask(0);
        return_error = 32;
        mode_t modifier;

        for (int i = 0; i < MAX_SIZE; ++i) {
            if (!cpm_options.umask_options[i][0]) break;

            modifier = get_modifier(cpm_options.umask_options[i]);
            if (modifier == 0){
                fprintf(stderr, "ZLA MASKA\n");
                return return_error;
            }

            if (cpm_options.umask_options[i][1] == '+') mask = mask & ~modifier;
            else mask |= modifier;
        }
        umask(mask);
    }

    int outfile = open(cpm_options.outfile, flags, modes);
    if (outfile == -1) {
        if (cpm_options.create) fprintf(stderr, "SUBOR EXISTUJE\n");
        else fprintf(stderr, "SUBOR NEEXISTUJE\n");
        return return_error;
    }

    int n = 0; int num = 1;
    if (cpm_options.sparse) {
        num = infile_stats.st_blksize;
        char buffer[num];

        while ((n = read(infile, buffer, num)) > 0) {
            int sparsable = 1;
            for (int i = 0; i < num; ++i) {
                if (buffer[i] != 0){
                    sparsable = 0;
                    break;
                }
            }

            if (sparsable) {
                if (lseek(outfile, n, SEEK_CUR) == -1){
                    close(infile);
                    close(outfile);
                    fprintf(stderr, "INA CHYBA\n");
                    return return_error;
                }
            }
            else{
                if (write(outfile, buffer, n) != n){
                    close(infile);
                    close(outfile);
                    fprintf(stderr, "INA CHYBA\n");
                    return return_error;
                }
            }
        }

        struct stat outfile_stats;
        fstat(outfile, &outfile_stats);
        close(infile);
        close(outfile);

        if (n<0){
            fprintf(stderr, "INA CHYBA\n");
            return return_error;
        }

        if (outfile_stats.st_blocks * outfile_stats.st_blksize >= outfile_stats.st_size){
            fprintf(stderr, "RIEDKY SUBOR NEVYTVORENY\n");
            return return_error;
        }
        return 0;
    }
    else{
        if (cpm_options.fast) num = infile_stats.st_size;
        char buffer[num];

        if (cpm_options.lseek){
            off_t pos2 = lseek(outfile, cpm_options.lseek_options.pos2, cpm_options.lseek_options.x);
            if (pos2 == -1){
                fprintf(stderr, "CHYBA POZICIE outfile\n");
                return return_error;
            }
        }

        int lseek_num = cpm_options.lseek_options.num;
        int loop = 0;
        while ((n = read(infile, buffer, num)) > 0) {
            loop++;
            if (cpm_options.lseek && loop > lseek_num) break;
            if (write(outfile, buffer, n) != n) {
                close(infile);
                close(outfile);
                fprintf(stderr, "INA CHYBA\n");
                return return_error;
            }
            if (cpm_options.fast && cpm_options.lseek) break;
        }
    }

    if (cpm_options.truncate && S_ISREG(infile_stats.st_mode)){
        return_error = 31;
        if (cpm_options.truncate_size < 0){
            fprintf(stderr, "ZAPORNA VELKOST\n");
            return return_error;
        }
        if (truncate(cpm_options.infile, cpm_options.truncate_size) == -1){
            fprintf(stderr, "INA CHYBA\n");
            return return_error;
        }
    }

    if (cpm_options.chmod) {
        if (chmod(cpm_options.outfile, cpm_options.chmod_mode) == -1) {
            fprintf(stderr, "ZLE PRAVA\n");
            return return_error;
        }
    }

    if (cpm_options.delete_opt && S_ISREG(infile_stats.st_mode)) {
        if (unlink(cpm_options.infile) == -1) {
            fprintf(stderr, "SUBOR NEBOL ZMAZANY\n"); // neviem kedy nastane taka situacia // jaj ked sa zapne este chmod
            return 26;
        }
    }

// - sparse
// while(read(,,1) == 0)
//    if (buf == '\0') lseek(out,1,seek_cur)
//    write()

//    /copymaster aa bb -c 0660 -l b,0,5,5 -- napisalo mi subor neexistuje

    close(infile);
    close(outfile);

    // TODO Implementovat vypis adresara

    if (cpm_options.directory) {
        return_error = 28;

        DIR *infile = opendir(cpm_options.infile);
        if (!infile) {
            fprintf(stderr, "VSTUPNY SUBOR NIE JE ADRESAR\n");
            return return_error;
        }

        FILE *outfile = fopen(cpm_options.outfile, "w");
        if (!outfile) {
            fprintf(stderr, "VYSTUPNY SUBOR - CHYBA\n");
            return return_error;
        }

        errno = 0;

        struct dirent *dirent = readdir(infile);
        while(dirent) {
            if (!strcmp(dirent->d_name, ".") || !strcmp(dirent->d_name, "..")) {
                dirent = readdir(infile);
                continue;
            }
            char buffer[500];
            snprintf(buffer, sizeof(buffer), "%s/%s", cpm_options.infile, dirent->d_name);
            stat(buffer, &infile_stats);

            char rights[11];
            for (int i = 0; rights[i] != '\0'; ++i) rights[i] = '-';

            if (S_ISDIR(infile_stats.st_mode)) rights[0] = 'd';
            if (infile_stats.st_mode & S_IRUSR) rights[1] = 'r';
            if (infile_stats.st_mode & S_IWUSR) rights[2] = 'w';
            if (infile_stats.st_mode & S_IXUSR) rights[3] = 'x';
            if (infile_stats.st_mode & S_IRGRP) rights[4] = 'r';
            if (infile_stats.st_mode & S_IWGRP) rights[5] = 'w';
            if (infile_stats.st_mode & S_IXGRP) rights[6] = 'x';
            if (infile_stats.st_mode & S_IROTH) rights[7] = 'r';
            if (infile_stats.st_mode & S_IWOTH) rights[8] = 'w';
            if (infile_stats.st_mode & S_IXOTH) rights[9] = 'x';
            rights[10] = '\0';

            char date[11];
            strftime(date, sizeof(date), "%d-%m-%Y", localtime(&infile_stats.st_mtime));
            date[10] = '\0';

            fprintf(outfile, "%s %lu %u %u %ld %s %s\n",
                    rights,
                    infile_stats.st_nlink,
                    infile_stats.st_uid,
                    infile_stats.st_gid,
                    infile_stats.st_size,
                    date,
                    dirent->d_name);

            dirent = readdir(infile);
        }

        closedir(infile);
        fclose(outfile);
        return 0;
    }

    //-------------------------------------------------------------------
    // Osetrenie prepinacov po kopirovani
    //-------------------------------------------------------------------

    // TODO Implementovat osetrenie prepinacov po kopirovani

    return 0;
}


void FatalError(char c, const char* msg, int exit_status){
    fprintf(stderr, "%c:%d:%s:%s\n", c, errno, strerror(errno), msg);
    exit(exit_status);
}

void PrintCopymasterOptions(struct CopymasterOptions* cpm_options){
    if (cpm_options == 0) return;

    printf("infile:        %s\n", cpm_options->infile);
    printf("outfile:       %s\n", cpm_options->outfile);

    printf("fast:          %d\n", cpm_options->fast);
    printf("slow:          %d\n", cpm_options->slow);
    printf("create:        %d\n", cpm_options->create);
    printf("create_mode:   %o\n", (unsigned int)cpm_options->create_mode);
    printf("overwrite:     %d\n", cpm_options->overwrite);
    printf("append:        %d\n", cpm_options->append);
    printf("lseek:         %d\n", cpm_options->lseek);

    printf("lseek_options.x:    %d\n", cpm_options->lseek_options.x);
    printf("lseek_options.pos1: %ld\n", cpm_options->lseek_options.pos1);
    printf("lseek_options.pos2: %ld\n", cpm_options->lseek_options.pos2);
    printf("lseek_options.num:  %lu\n", cpm_options->lseek_options.num);

    printf("directory:     %d\n", cpm_options->directory);
    printf("delete_opt:    %d\n", cpm_options->delete_opt);
    printf("chmod:         %d\n", cpm_options->chmod);
    printf("chmod_mode:    %o\n", (unsigned int)cpm_options->chmod_mode);
    printf("inode:         %d\n", cpm_options->inode);
    printf("inode_number:  %lu\n", cpm_options->inode_number);

    printf("umask:\t%d\n", cpm_options->umask);
    for(unsigned int i=0; i<kUMASK_OPTIONS_MAX_SZ; ++i) {
        if (cpm_options->umask_options[i][0] == 0) break;
        printf("umask_options[%u]: %s\n", i, cpm_options->umask_options[i]);
    }

    printf("link:          %d\n", cpm_options->link);
    printf("truncate:      %d\n", cpm_options->truncate);
    printf("truncate_size: %ld\n", cpm_options->truncate_size);
    printf("sparse:        %d\n", cpm_options->sparse);
}