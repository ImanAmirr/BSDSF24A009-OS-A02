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
