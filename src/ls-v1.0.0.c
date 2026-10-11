
/*
 * Programming Assignment 02: ls v1.4.0
 * Default columns, horizontal display (-x), long listing (-l),
 * and alphabetical sorting using qsort()
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

void do_ls(const char *dir, int horizontal_format);
void do_long_ls(const char *dir);
void print_horizontal(char **filenames, size_t count,
                      size_t column_width, int terminal_width);
void print_long_entry(const char *dir, const char *name);
void get_permissions(mode_t mode, char permissions[]);
int get_terminal_width(void);
int compare_names(const void *a, const void *b);

int compare_names(const void *a, const void *b)
{
    const char *name1 = *(const char **)a;
    const char *name2 = *(const char **)b;

    return strcmp(name1, name2);
}

int main(int argc, char *argv[])
{
    int opt;
    int long_format = 0;
    int horizontal_format = 0;

    while ((opt = getopt(argc, argv, "lx")) != -1)
    {
        if (opt == 'l')
            long_format = 1;
        else if (opt == 'x')
            horizontal_format = 1;
        else
        {
            fprintf(stderr, "Usage: %s [-l] [-x] [directory ...]\n",
                    argv[0]);
            return 1;
        }
    }

    if (optind == argc)
    {
        if (long_format)
            do_long_ls(".");
        else
            do_ls(".", horizontal_format);
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
                do_ls(argv[i], horizontal_format);

            if (argc - optind > 1)
                puts("");
        }
    }

    return 0;
}

int get_terminal_width(void)
{
    struct winsize window;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &window) == 0 &&
        window.ws_col > 0)
        return window.ws_col;

    return 80;
}

void do_ls(const char *dir, int horizontal_format)
{
    DIR *dp;
    struct dirent *entry;
    char **filenames = NULL;
    size_t count = 0;
    size_t capacity = 16;
    size_t max_length = 0;
    size_t rows;
    size_t columns;
    size_t i;
    int terminal_width;
    size_t column_width;
    size_t spacing = 2;

    dp = opendir(dir);

    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory %s: ", dir);
        perror("");
        return;
    }

    filenames = malloc(capacity * sizeof(char *));

    if (filenames == NULL)
    {
        perror("malloc");
        closedir(dp);
        return;
    }

    errno = 0;

    while ((entry = readdir(dp)) != NULL)
    {
        char *copy;
        size_t length;

        if (entry->d_name[0] == '.')
            continue;

        length = strlen(entry->d_name);

        if (count == capacity)
        {
            size_t new_capacity = capacity * 2;
            char **new_filenames =
                realloc(filenames, new_capacity * sizeof(char *));

            if (new_filenames == NULL)
            {
                perror("realloc");

                for (i = 0; i < count; i++)
                    free(filenames[i]);

                free(filenames);
                closedir(dp);
                return;
            }

            filenames = new_filenames;
            capacity = new_capacity;
        }

        copy = malloc(length + 1);

        if (copy == NULL)
        {
            perror("malloc");

            for (i = 0; i < count; i++)
                free(filenames[i]);

            free(filenames);
            closedir(dp);
            return;
        }

        strcpy(copy, entry->d_name);
        filenames[count] = copy;
        count++;

        if (length > max_length)
            max_length = length;
    }

    if (errno != 0)
        perror("readdir failed");

    closedir(dp);

    /* Sort filenames alphabetically */
    qsort(filenames, count, sizeof(char *), compare_names);

    if (count == 0)
    {
        free(filenames);
        return;
    }

    terminal_width = get_terminal_width();
    column_width = max_length + spacing;

    if (horizontal_format)
    {
        print_horizontal(filenames, count, column_width, terminal_width);
    }
    else
    {
        columns = (size_t)terminal_width / column_width;

        if (columns == 0)
            columns = 1;

        if (columns > count)
            columns = count;

        rows = (count + columns - 1) / columns;

        for (i = 0; i < rows; i++)
        {
            size_t column;

            for (column = 0; column < columns; column++)
            {
                size_t index = i + column * rows;

                if (index < count)
                {
                    printf("%s", filenames[index]);

                    if (column + 1 < columns && index + rows < count)
                    {
                        size_t length = strlen(filenames[index]);
                        size_t padding = column_width - length;

                        while (padding > 0)
                        {
                            putchar(' ');
                            padding--;
                        }
                    }
                }
            }

            putchar('\n');
        }
    }

    for (i = 0; i < count; i++)
        free(filenames[i]);

    free(filenames);
}

void print_horizontal(char **filenames, size_t count,
                      size_t column_width, int terminal_width)
{
    size_t i;
    size_t current_position = 0;

    for (i = 0; i < count; i++)
    {
        size_t length = strlen(filenames[i]);
        size_t padding = column_width - length;

        if (current_position > 0 &&
            current_position + column_width > (size_t)terminal_width)
        {
            putchar('\n');
            current_position = 0;
        }

        printf("%s", filenames[i]);
        current_position += length;

        if (i + 1 < count)
        {
            if (current_position + padding <= (size_t)terminal_width)
            {
                while (padding > 0)
                {
                    putchar(' ');
                    padding--;
                }

                current_position += column_width - length;
            }
            else
            {
                putchar('\n');
                current_position = 0;
            }
        }
        else
        {
            putchar('\n');
        }
    }
}

void do_long_ls(const char *dir)
{
    struct stat st;
    struct dirent *entry;
    DIR *dp;
    char **filenames = NULL;
    size_t count = 0;
    size_t capacity = 16;
    size_t i;

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

    filenames = malloc(capacity * sizeof(char *));

    if (filenames == NULL)
    {
        perror("malloc");
        closedir(dp);
        return;
    }

    errno = 0;

    while ((entry = readdir(dp)) != NULL)
    {
        char *copy;
        size_t length;

        if (entry->d_name[0] == '.')
            continue;

        length = strlen(entry->d_name);

        if (count == capacity)
        {
            size_t new_capacity = capacity * 2;
            char **new_filenames =
                realloc(filenames, new_capacity * sizeof(char *));

            if (new_filenames == NULL)
            {
                perror("realloc");

                for (i = 0; i < count; i++)
                    free(filenames[i]);

                free(filenames);
                closedir(dp);
                return;
            }

            filenames = new_filenames;
            capacity = new_capacity;
        }

        copy = malloc(length + 1);

        if (copy == NULL)
        {
            perror("malloc");

            for (i = 0; i < count; i++)
                free(filenames[i]);

            free(filenames);
            closedir(dp);
            return;
        }

        strcpy(copy, entry->d_name);
        filenames[count] = copy;
        count++;
    }

    if (errno != 0)
        perror("readdir failed");

    closedir(dp);

    /* Sort filenames before printing long listings */
    qsort(filenames, count, sizeof(char *), compare_names);

    for (i = 0; i < count; i++)
        print_long_entry(dir, filenames[i]);

    for (i = 0; i < count; i++)
        free(filenames[i]);

    free(filenames);
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
