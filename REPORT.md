# Report Questions — Feature 2: ls-v1.1.0

## Question 1: What is the crucial difference between `stat()` and `lstat()`? In the context of the `ls` command, when is it more appropriate to use `lstat()`?

Both `stat()` and `lstat()` are system calls used to retrieve file metadata, including file size, permissions, owner ID, group ID, and modification time.

The crucial difference is how they handle symbolic links. The `stat()` function follows a symbolic link and returns the metadata of the file to which the link points. In contrast, `lstat()` returns the metadata of the symbolic link itself.

In the context of the `ls` command, `lstat()` is more appropriate when listing directory entries because it allows the program to identify symbolic links and display their own file type and permissions. This helps the program distinguish a symbolic link from a regular file or directory.

## Question 2: The `st_mode` field in `struct stat` is an integer that contains both the file type and permission bits. Explain how bitwise operators and predefined macros can extract this information.

The `st_mode` field in the `struct stat` structure stores information about the file type and its permissions. Bitwise operators and predefined macros can be used to examine this information.

The bitwise AND operator (`&`) checks whether a particular bit is set. For example, `st.st_mode & S_IRUSR` checks whether the owner has read permission. Similarly, `S_IWUSR` checks owner write permission, and `S_IXUSR` checks owner execute permission. The same approach applies to group permissions (`S_IRGRP`, `S_IWGRP`, `S_IXGRP`) and permissions for others (`S_IROTH`, `S_IWOTH`, `S_IXOTH`).

File type information can be checked using macros such as `S_IFDIR` and `S_IFREG` with the appropriate file-type mask, `S_IFMT`. For example, `(st.st_mode & S_IFMT) == S_IFDIR` checks whether the file is a directory. The more convenient macros `S_ISDIR(st.st_mode)` and `S_ISREG(st.st_mode)` can also be used.

By checking these bits, the program constructs a permission string such as `-rwxr-xr--`, where the first character represents the file type and the remaining nine characters represent read, write, and execute permissions for the owner, group, and others. Special bits such as `S_ISUID`, `S_ISGID`, and `S_ISVTX` can also be checked to display set-user-ID, set-group-ID, and sticky-bit permissions.

# Report Questions — Feature 3: ls-v1.2.0

## Question 1: Explain the general logic for printing items in a "down then across" columnar format. Why is a simple single loop through the list of filenames insufficient for this task?

To print filenames in a down-then-across format, the program first reads all visible directory entries into a dynamically allocated array. While collecting the filenames, it keeps track of the total number of entries and the length of the longest filename.

After collecting the filenames, the program obtains the terminal width using `ioctl()` with `TIOCGWINSZ`. It calculates the maximum column width by adding spacing to the longest filename. The terminal width and column width are then used to calculate the number of columns and rows.

A simple loop through the array prints each filename on the next line, producing a single column. To print down then across, the program uses the index `row + column * rows`. This prints the first item of each column across a row while keeping the items vertically ordered within each column. The program also adds spaces after filenames to align the columns.

## Question 2: What is the purpose of the ioctl system call in this context? What would be the limitations of your program if you only used a fixed-width fallback (e.g., 80 columns) instead of detecting the terminal size?

The `ioctl()` system call is used to obtain information about a terminal. In this program, it is called with `TIOCGWINSZ` to retrieve the terminal window size, including its number of columns. The program uses this width to decide how many filename columns can fit on the screen. If the terminal width cannot be obtained, the program uses a fallback width of 80 characters.

Using only a fixed width would make the output less adaptable. On a narrow terminal, the filenames could extend beyond the screen and wrap onto additional lines. On a wide terminal, the program might use too few columns and leave unnecessary empty space. Detecting the actual terminal width allows the output to adjust to different terminal sizes and more closely resemble the standard `ls` utility.

# Report Questions — Feature 4: ls-v1.3.0

## Question 1: Compare the implementation complexity of the "down then across" (vertical) printing logic versus the "across" (horizontal) printing logic. Which one requires more pre-calculation and why?

The down-then-across printing logic is more complex because it requires calculating the number of columns and rows before printing. The program uses the terminal width and the longest filename to determine how many columns fit on the screen. It then uses the index `row + column * rows` to print the filenames vertically within each column before moving across.

The horizontal printing logic is simpler because the program processes the filenames sequentially from left to right. It keeps track of the current horizontal position and starts a new line when the next filename would exceed the terminal width. Both formats require the filenames and maximum filename length to be collected first, but the vertical format requires additional calculations for rows, columns, and filename indexes.

## Question 2: Describe the strategy you used in your code to manage the different display modes. How did your program decide which function to call for printing?

The program uses two integer flags, `long_format` and `horizontal_format`, to keep track of the selected display mode. The `getopt()` function recognizes the `-l` and `-x` options. When `-l` is provided, `long_format` is set to 1. When `-x` is provided, `horizontal_format` is set to 1.

In the main function, the program checks these flags to decide which printing function to call. If `-l` is selected, it calls `do_long_ls()`. Otherwise, it calls `do_ls()` with the horizontal-format flag. Inside `do_ls()`, the program calls `print_horizontal()` when `-x` is selected. If neither option is provided, it uses the original down-then-across printing logic. This allows the program to support all three display modes while preserving the previous functionality.

## Feature 5: Alphabetical Sorting (v1.4.0)

### Implementation

The program now sorts filenames alphabetically using the `qsort()` function from the C standard library. A comparison function named `compare_names()` compares two filenames using `strcmp()`.

The filenames are first collected into a dynamically allocated array. The `qsort()` function sorts this array before the filenames are displayed. Alphabetical sorting is supported in the default column display, horizontal display (`-x`), and long listing (`-l`).

### Testing

The program was compiled using `make clean` and `make` without errors or warnings. Sorting was tested by creating three files in an unsorted order: `zebra.txt`, `apple.txt`, and `mango.txt`.

The output showed the filenames in alphabetical order in all three display modes.

### Question 1: Why are all directory entries read into memory before sorting? What are the drawbacks for directories containing millions of files?

All directory entries are read into memory because `qsort()` sorts an array of elements. The program must collect the filenames before sorting them and displaying the results.

For directories containing millions of files, storing all filenames in memory can consume a large amount of RAM. It also requires additional time to allocate memory and copy filenames. If memory is insufficient, allocation may fail. Sorting a large number of filenames also takes additional processing time.

### Question 2: Explain the purpose and signature of the comparison function used by `qsort()`. How does it work, and why are its arguments `const void *`?

The comparison function determines the order of elements during sorting. Its signature is:

`int compare_names(const void *a, const void *b)`

The arguments are `const void *` because `qsort()` is a general-purpose function that can sort arrays containing different data types. The function receives pointers to two array elements. In this program, each element is a `char *`, so the arguments are converted to pointers to filename pointers. The filenames are then compared using `strcmp()`.

The function returns a negative value if the first filename comes before the second, zero if they are equal, and a positive value if the first comes after the second. The `const` qualifier indicates that the comparison function does not modify the elements being compared.