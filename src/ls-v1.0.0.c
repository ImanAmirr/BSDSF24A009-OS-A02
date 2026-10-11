
/*
 * Programming Assignment 02: ls v1.5.0
 * Features:
 * 1. Default column display (down-then-across)
 * 2. Long listing (-l)
 * 3. Horizontal display (-x)
 * 4. Alphabetical sorting
 * 5. Colorized output based on file type
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <limits.h>

/* ANSI color escape codes */
#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[0;34m"
#define COLOR_GREEN   "\033[0;32m"
#define COLOR_RED     "\033[0;31m"
#define COLOR_PINK    "\033[0;95m"
#define COLOR_REVERSE "\033[7m"

void do_ls(const char *dir, int horizontal_format);
void do_long_ls(const char *dir);

void print_horizontal(char **filenames, size_t count,
                      size_t column_width, int terminal_width);

void print_long_entry(const char *dir, const char *name);
void get_permissions(mode_t mode, char permissions[]);
void print_colored_name(const char *dir, const char *name);
void print_colored_name_from_mode(const char *name, mode_t mode);

int get_terminal_width(void);
int compare_names(const void *a, const void *b);
int is_archive(const char *name);

/* Compare filenames alphabetically */
int compare_names(const void *a, const void *b)
{
    const char *name1 = *(const char **)a;
    const char *name2 = *(const char **)b;

    return strcmp(name1, name2);
}

/* Check whether a filename has an archive extension */
int is_archive(const char *name)
{
    const char *extension = strrchr(name, '.');

    if (extension == NULL)
    {
        return 0;
    }

    if (strcmp(extension, ".tar") == 0 ||
        strcmp(extension, ".gz") == 0 ||
        strcmp(extension, ".zip") == 0)
    {
        return 1;
    }

    return 0;
}

/* Print a filename using a color based on its file type */

void print_colored_name_from_mode(const char *name, mode_t mode)
{
    const char *color = COLOR_RESET;
    const char *extension = strrchr(name, '.');

    if (S_ISLNK(mode))
    {
        color = COLOR_PINK;
    }
    else if (!S_ISDIR(mode) && !S_ISREG(mode))
    {
        color = COLOR_REVERSE;
    }
    else if (S_ISDIR(mode))
    {
        color = COLOR_BLUE;
    }
    else if (is_archive(name))
    {
        color = COLOR_RED;
    }
    else if (extension != NULL && strcmp(extension, ".sh") == 0)
    {
        color = COLOR_GREEN;
    }
    else
    {
        /*
         * Windows-mounted filesystems may report every file
         * as executable. Keep ordinary files in the default
         * color on this filesystem.
         */
        color = COLOR_RESET;
    }

    if (isatty(STDOUT_FILENO))
    {
        printf("%s%s%s", color, name, COLOR_RESET);
    }
    else
    {
        printf("%s", name);
    }
}


/* Get file information and print a colored filename */
void print_colored_name(const char *dir, const char *name)
{
    char path[PATH_MAX];
    struct stat st;
    int result;

    if (strcmp(dir, ".") == 0)
    {
        result = snprintf(path, sizeof(path), "%s", name);
    }
    else
    {
        result = snprintf(path, sizeof(path), "%s/%s", dir, name);
    }

    if (result < 0 || (size_t)result >= sizeof(path))
    {
        printf("%s", name);
        return;
    }

    /* lstat identifies symbolic links without following them */
    if (lstat(path, &st) == -1)
    {
        printf("%s", name);
        return;
    }

    print_colored_name_from_mode(name, st.st_mode);
}

/* Get terminal width; use 80 columns if unavailable */
int get_terminal_width(void)
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 &&
        ws.ws_col > 0)
    {
        return ws.ws_col;
    }

    return 80;
}

/* Get file type and permission characters */
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

    /* Handle special permission bits */
    if (mode & S_ISUID)
        permissions[3] = (mode & S_IXUSR) ? 's' : 'S';

    if (mode & S_ISGID)
        permissions[6] = (mode & S_IXGRP) ? 's' : 'S';

    if (mode & S_ISVTX)
        permissions[9] = (mode & S_IXOTH) ? 't' : 'T';

    permissions[10] = '\0';
}

/* Print filenames left-to-right, wrapping when necessary */
void print_horizontal(char **filenames, size_t count,
                      size_t column_width, int terminal_width)
{
    size_t i;
    size_t current_width = 0;

    for (i = 0; i < count; i++)
    {
        size_t name_length = strlen(filenames[i]);
        size_t needed_width = name_length;

        if (current_width > 0)
        {
            needed_width += 2;
        }

        if (current_width > 0 &&
            current_width + needed_width > (size_t)terminal_width)
        {
            printf("\n");
            current_width = 0;
        }

        if (current_width > 0)
        {
            printf("  ");
            current_width += 2;
        }

        print_colored_name(".", filenames[i]);
        current_width += name_length;
    }

    if (count > 0)
    {
        printf("\n");
    }

    (void)column_width;
}

/* Display directory entries in columns or horizontally */
void do_ls(const char *dir, int horizontal_format)
{
    DIR *dp;
    struct dirent *entry;

    char **filenames = NULL;
    size_t count = 0;
    size_t capacity = 10;
    size_t max_length = 0;
    size_t i;

    int terminal_width;
    size_t column_width;
    size_t columns;
    size_t rows;

    dp = opendir(dir);

    if (dp == NULL)
    {
        perror(dir);
        return;
    }

    filenames = malloc(capacity * sizeof(char *));

    if (filenames == NULL)
    {
        perror("malloc");
        closedir(dp);
        return;
    }

    /* Read visible filenames */
    while ((entry = readdir(dp)) != NULL)
    {
        size_t name_length;

        if (entry->d_name[0] == '.')
        {
            continue;
        }

        if (count == capacity)
        {
            size_t new_capacity = capacity * 2;
            char **temp = realloc(filenames,
                                  new_capacity * sizeof(char *));

            if (temp == NULL)
            {
                perror("realloc");

                for (i = 0; i < count; i++)
                {
                    free(filenames[i]);
                }

                free(filenames);
                closedir(dp);
                return;
            }

            filenames = temp;
            capacity = new_capacity;
        }

        filenames[count] = malloc(strlen(entry->d_name) + 1);

        if (filenames[count] == NULL)
        {
            perror("malloc");

            for (i = 0; i < count; i++)
            {
                free(filenames[i]);
            }

            free(filenames);
            closedir(dp);
            return;
        }

        strcpy(filenames[count], entry->d_name);

        name_length = strlen(entry->d_name);

        if (name_length > max_length)
        {
            max_length = name_length;
        }

        count++;
    }

    closedir(dp);

    /* Sort filenames alphabetically */
    qsort(filenames, count, sizeof(char *), compare_names);

    if (count == 0)
    {
        free(filenames);
        return;
    }

    terminal_width = get_terminal_width();
    column_width = max_length + 2;

    if (horizontal_format)
    {
        print_horizontal(filenames, count,
                         column_width, terminal_width);
    }
    else
    {
        /* Default display: down-then-across */
        columns = (size_t)terminal_width / column_width;

        if (columns == 0)
        {
            columns = 1;
        }

        if (columns > count)
        {
            columns = count;
        }

        rows = (count + columns - 1) / columns;

        for (i = 0; i < rows; i++)
        {
            size_t col;

            for (col = 0; col < columns; col++)
            {
                size_t index = col * rows + i;

                if (index < count)
                {
                    size_t name_length = strlen(filenames[index]);

                    print_colored_name(dir, filenames[index]);

                    if (col < columns - 1 &&
                        index + rows < count)
                    {
                        size_t spaces = column_width - name_length;

                        while (spaces > 0)
                        {
                            printf(" ");
                            spaces--;
                        }
                    }
                }
            }

            printf("\n");
        }
    }

    /* Free allocated memory */
    for (i = 0; i < count; i++)
    {
        free(filenames[i]);
    }

    free(filenames);
}

/* Print one entry in long-listing format */
void print_long_entry(const char *dir, const char *name)
{
    char path[PATH_MAX];
    char permissions[11];
    char time_string[20];

    struct stat st;
    struct passwd *owner;
    struct group *group;
    struct tm *time_info;

    int result;

    if (strcmp(dir, ".") == 0)
    {
        result = snprintf(path, sizeof(path), "%s", name);
    }
    else
    {
        result = snprintf(path, sizeof(path), "%s/%s", dir, name);
    }

    if (result < 0 || (size_t)result >= sizeof(path))
    {
        fprintf(stderr, "Path is too long: %s\n", name);
        return;
    }

    if (lstat(path, &st) == -1)
    {
        perror(path);
        return;
    }

    get_permissions(st.st_mode, permissions);

    owner = getpwuid(st.st_uid);
    group = getgrgid(st.st_gid);

    time_info = localtime(&st.st_mtime);

    if (time_info != NULL)
    {
        strftime(time_string, sizeof(time_string),
                 "%b %d %H:%M", time_info);
    }
    else
    {
        strcpy(time_string, "Unknown time");
    }

    printf("%s %2lu %-8s %-8s %8ld %s ",
           permissions,
           (unsigned long)st.st_nlink,
           owner ? owner->pw_name : "unknown",
           group ? group->gr_name : "unknown",
           (long)st.st_size,
           time_string);

    /* Colorize the filename using its existing stat information */
    print_colored_name_from_mode(name, st.st_mode);

    printf("\n");
}

/* Display a directory in long-listing format */
void do_long_ls(const char *dir)
{
    DIR *dp;
    struct dirent *entry;
    struct stat st;

    char **filenames = NULL;
    size_t count = 0;
    size_t capacity = 10;
    size_t i;

    if (lstat(dir, &st) == -1)
    {
        perror(dir);
        return;
    }

    /* If the path is not a directory, list that file */
    if (!S_ISDIR(st.st_mode))
    {
        print_long_entry(".", dir);
        return;
    }

    dp = opendir(dir);

    if (dp == NULL)
    {
        perror(dir);
        return;
    }

    filenames = malloc(capacity * sizeof(char *));

    if (filenames == NULL)
    {
        perror("malloc");
        closedir(dp);
        return;
    }

    /* Read visible filenames */
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
        {
            continue;
        }

        if (count == capacity)
        {
            size_t new_capacity = capacity * 2;
            char **temp = realloc(filenames,
                                  new_capacity * sizeof(char *));

            if (temp == NULL)
            {
                perror("realloc");

                for (i = 0; i < count; i++)
                {
                    free(filenames[i]);
                }

                free(filenames);
                closedir(dp);
                return;
            }

            filenames = temp;
            capacity = new_capacity;
        }

        filenames[count] = malloc(strlen(entry->d_name) + 1);

        if (filenames[count] == NULL)
        {
            perror("malloc");

            for (i = 0; i < count; i++)
            {
                free(filenames[i]);
            }

            free(filenames);
            closedir(dp);
            return;
        }

        strcpy(filenames[count], entry->d_name);
        count++;
    }

    closedir(dp);

    /* Sort before displaying */
    qsort(filenames, count, sizeof(char *), compare_names);

    for (i = 0; i < count; i++)
    {
        print_long_entry(dir, filenames[i]);
        free(filenames[i]);
    }

    free(filenames);
}

/* Main function */
int main(int argc, char *argv[])
{
    int option;
    int long_format = 0;
    int horizontal_format = 0;
    int i;
    int directory_count = 0;

    while ((option = getopt(argc, argv, "lx")) != -1)
    {
        switch (option)
        {
            case 'l':
                long_format = 1;
                break;

            case 'x':
                horizontal_format = 1;
                break;

            default:
                fprintf(stderr,
                        "Usage: %s [-l] [-x] [directory ...]\n",
                        argv[0]);
                return 1;
        }
    }

    if (optind == argc)
    {
        if (long_format)
        {
            do_long_ls(".");
        }
        else
        {
            do_ls(".", horizontal_format);
        }

        return 0;
    }

    for (i = optind; i < argc; i++)
    {
        if (directory_count > 0)
        {
            printf("\n");
        }

        if (argc - optind > 1)
        {
            printf("Directory listing of %s:\n", argv[i]);
        }

        if (long_format)
        {
            do_long_ls(argv[i]);
        }
        else
        {
            do_ls(argv[i], horizontal_format);
        }

        directory_count++;
    }

    return 0;
}
