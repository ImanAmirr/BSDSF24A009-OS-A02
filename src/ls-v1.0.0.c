
/*
 * Programming Assignment 02: ls v1.1.0
 * Long listing format using the -l option
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

void do_ls(const char *dir);
void do_long_ls(const char *dir);
void print_long_entry(const char *dir, const char *name);
void get_permissions(mode_t mode, char permissions[]);

int main(int argc, char *argv[])
{
    int opt;
    int long_format = 0;

    while ((opt = getopt(argc, argv, "l")) != -1)
    {
        if (opt == 'l')
            long_format = 1;
        else
        {
            fprintf(stderr, "Usage: %s [-l] [directory ...]\n", argv[0]);
            return 1;
        }
    }

    if (optind == argc)
    {
        if (long_format)
            do_long_ls(".");
        else
            do_ls(".");
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            if (argc - optind > 1)
                printf("Directory listing of %s:\n", argv[i]);

            if (long_format)
                do_long_ls(argv[i]);
            else
                do_ls(argv[i]);

            if (argc - optind > 1)
                puts("");
        }
    }

    return 0;
}

void do_ls(const char *dir)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);

    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory %s: ", dir);
        perror("");
        return;
    }

    errno = 0;

    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        printf("%s\n", entry->d_name);
    }

    if (errno != 0)
        perror("readdir failed");

    closedir(dp);
}

void do_long_ls(const char *dir)
{
    struct stat st;
    struct dirent *entry;
    DIR *dp;

    if (lstat(dir, &st) == -1)
    {
        fprintf(stderr, "Cannot access %s: ", dir);
        perror("");
        return;
    }

    if (!S_ISDIR(st.st_mode))
    {
        print_long_entry(".", dir);
        return;
    }

    dp = opendir(dir);

    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory %s: ", dir);
        perror("");
        return;
    }

    errno = 0;

    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        print_long_entry(dir, entry->d_name);
    }

    if (errno != 0)
        perror("readdir failed");

    closedir(dp);
}

void print_long_entry(const char *dir, const char *name)
{
    char path[4096];
    char permissions[11];
    struct stat st;
    struct passwd *pw;
    struct group *gr;
    char time_buffer[32];
    char *time_string;

    if (strcmp(dir, ".") == 0)
        snprintf(path, sizeof(path), "%s", name);
    else if (snprintf(path, sizeof(path), "%s/%s", dir, name)
             >= (int)sizeof(path))
    {
        fprintf(stderr, "Path too long: %s/%s\n", dir, name);
        return;
    }

    if (lstat(path, &st) == -1)
    {
        fprintf(stderr, "Cannot access %s: ", path);
        perror("");
        return;
    }

    get_permissions(st.st_mode, permissions);

    pw = getpwuid(st.st_uid);
    gr = getgrgid(st.st_gid);

    time_string = ctime(&st.st_mtime);

    if (time_string != NULL)
    {
        snprintf(time_buffer, sizeof(time_buffer), "%s", time_string);
        time_buffer[strlen(time_buffer) - 1] = '\0';
    }
    else
    {
        snprintf(time_buffer, sizeof(time_buffer), "Unknown time");
    }

    printf("%s %2lu %-8s %-8s %8ld %s %s\n",
           permissions,
           (unsigned long)st.st_nlink,
           pw ? pw->pw_name : "unknown",
           gr ? gr->gr_name : "unknown",
           (long)st.st_size,
           time_buffer,
           name);
}

void get_permissions(mode_t mode, char permissions[])
{
    if (S_ISDIR(mode))
        permissions[0] = 'd';
    else if (S_ISLNK(mode))
        permissions[0] = 'l';
    else if (S_ISCHR(mode))
        permissions[0] = 'c';
    else if (S_ISBLK(mode))
        permissions[0] = 'b';
    else if (S_ISFIFO(mode))
        permissions[0] = 'p';
    else if (S_ISSOCK(mode))
        permissions[0] = 's';
    else
        permissions[0] = '-';

    permissions[1] = (mode & S_IRUSR) ? 'r' : '-';
    permissions[2] = (mode & S_IWUSR) ? 'w' : '-';
    permissions[3] = (mode & S_IXUSR) ? 'x' : '-';

    permissions[4] = (mode & S_IRGRP) ? 'r' : '-';
    permissions[5] = (mode & S_IWGRP) ? 'w' : '-';
    permissions[6] = (mode & S_IXGRP) ? 'x' : '-';

    permissions[7] = (mode & S_IROTH) ? 'r' : '-';
    permissions[8] = (mode & S_IWOTH) ? 'w' : '-';
    permissions[9] = (mode & S_IXOTH) ? 'x' : '-';

    if (mode & S_ISUID)
        permissions[3] = (mode & S_IXUSR) ? 's' : 'S';

    if (mode & S_ISGID)
        permissions[6] = (mode & S_IXGRP) ? 's' : 'S';

    if (mode & S_ISVTX)
        permissions[9] = (mode & S_IXOTH) ? 't' : 'T';

    permissions[10] = '\0';
}

