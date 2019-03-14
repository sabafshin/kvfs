/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   fs.h
 */

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>
#include <memory>
#include <dirent.h>
#include <sys/types.h>
#include <kvfs_config.h>
#include <kvfs/kvfs_dirent.h>

/**
 * Use this abstract class to create an instance of KVFS
 * Example:
 * std::unique_ptr<FS> fs_ = std::make_unique<KVFS>(// optional mount path );
 * or use shared ptr if necessary.
 * If no mount path is given, the filesystem is mounted at "/tmp/db/"
 */
class FS {
 public:

  FS() = default;
  ~FS() = default;

  /*############## POSIX Operations ##############*/

  /**
   * @brief The getcwd function returns an absolute file name representing the current working directory,
   * storing it in the character array buffer that you provide. The size argument is how you tell the system
   * the allocation size of buffer.
   * The GNU C Library version of this function also permits you to specify a null pointer for the buffer argument.
   * Then getcwd allocates a buffer automatically, as with malloc (see Unconstrained Allocation).
   * If the size is greater than zero, then the buffer is that large; otherwise, the buffer
   * is as large as necessary to hold the result.
   * @return The return value is buffer on success and a null pointer on failure.
   * The following errno error conditions are defined for this function:
   * EINVAL
   *    The size argument is zero and buffer is not a null pointer.
   * ERANGE
   *    The size argument is less than the length of the working directory name.
   *    You need to allocate a bigger array and try again.
   * EACCES
   *    Permission to read or search a component of the file name was denied.
   */
  virtual char *GetCWD(char *buffer, size_t size) = 0;
  /**
   * @brief The get_current_dir_name function is basically equivalent to getcwd (NULL, 0),
   * except the value of the PWD environment variable is first examined,
   * and if it does in fact correspond to the current directory, that value is returned.
   * This is a subtle difference which is visible if the path described by the value
   * in PWD is using one or more symbolic links, in which case the value returned by getcwd
   * would resolve the symbolic links and therefore yield a different result.
   */
  virtual char *GetCurrentDirName() = 0;
  /**
    * @brief This function is used to set the process’s working directory to directory
    * associated with the file descriptor filedes.
    * @param filedes
    * @return The normal, successful return value from fchdir is 0.
    * A value of -1 is returned to indicate an error.
    * EACCES
    *      Read permission is denied for the directory named by dirname.
    * EBADF
    *      The filedes argument is not a valid file descriptor.
    * ENOTDIR
    *      The file descriptor filedes is not associated with a directory.
    * EINTR
    *      The function call was interrupt by a signal.
    * EIO
    *      An I/O error occurred.
    */
  virtual int ChDir(const char *path) = 0;

  /**
    * @brief The opendir function opens and returns a directory stream for reading the directory
    * whose file name is dirname. The stream has type DIR *.
    * @param path file name
    * @return If unsuccessful, opendir returns a null pointer.
    */
  virtual kvfsDIR *OpenDir(const char *path) = 0;

  /**
   * This function reads the next entry from the directory.
   * It normally returns a pointer to a structure containing information about the file.
   * This structure is associated with the dirstream handle and can be rewritten by a subsequent call.
   * Portability Note: On some systems readdir may not return entries for . and ..,
   * even though these are always valid file names in any directory.
   * @param dirstream
   * @return The pointer returned by readdir points to a buffer within the DIR object.
   * The data in that buffer will be overwritten by the next call to readdir. You must take care,
   * for instance, to copy the d_name string if you need it later.
   */
  virtual kvfs_dirent *ReadDir(kvfsDIR *dirstream) = 0;

  /**
   * This function closes the directory stream dirstream. It returns 0 on success and -1 on failure.
   * The following errno error conditions are defined for this function:
   * EBADF
   *    The dirstream argument is not valid.
   * @param filedes
   */
  virtual int CloseDir(kvfsDIR *filedes) = 0;

  /**
   * The link function makes a new link to the existing file named by oldname, under the new name newname.
   * @param oldname
   * @param newname
   * @return This function returns a value of 0 if it is successful and -1 on failure.
   * In addition to the usual file name errors (see File Name Errors) for both oldname and newname,
   * the following errno error conditions are defined for this function:
   * EACCES
   *    You are not allowed to write to the directory in which the new link is to be written.
   * EEXIST
   *    There is already a file named newname. If you want to replace this link with a new link,
   *    you must remove the old link explicitly first.
   * EMLINK
   *    There are already too many links to the file named by oldname.
   *    (The maximum number of links to a file is LINK_MAX; see Limits for Files.)
   * ENOENT
   *    The file named by oldname doesn’t exist. You can’t make a link to a file that doesn’t exist.
   * ENOSPC
   *    The directory or file system that would contain the new link is full and cannot be extended.
   * EPERM
   *    On GNU/Linux and GNU/Hurd systems and some others, you cannot make links to directories.
   *    Many systems allow only privileged users to do so. This error is used to report the problem.
   * EROFS
   *    The directory containing the new link can’t be modified because it’s on a read-only file system.
   * EXDEV
   *    The directory specified in newname is on a different file system than the existing file.
   * EIO
   *    A hardware error occurred while trying to read or write the to filesystem.
   */
  virtual int Link(const char *oldname, const char *newname) = 0;

  /**
   * The symlink function makes a symbolic link to oldname named newname.
   * @param path1
   * @param path2
   * @return The normal return value from symlink is 0. A return value of -1 indicates an error.
   * In addition to the usual file name syntax errors (see File Name Errors),
   * the following errno error conditions are defined for this function:
   * EEXIST
   *    There is already an existing file named newname.
   * EROFS
   *    The file newname would exist on a read-only file system.
   * ENOSPC
   *    The directory or file system cannot be extended to make the new link.
   * EIO
   *    A hardware error occurred while reading or writing data on the disk.
   */
  virtual int SymLink(const char *path1, const char *path2) = 0;

  /**
   * The readlink function gets the value of the symbolic link filename.
   * The file name that the link points to is copied into buffer.
   * This file name string is not null-terminated; readlink normally returns the number of characters copied.
   * The size argument specifies the maximum number of characters to copy, usually the allocation size of buffer.
   * @param filename
   * @param buffer
   * @param size
   * @return A value of -1 is returned in case of error.
   * In addition to the usual file name errors (see File Name Errors),
   * the following errno error conditions are defined for this function:
   * EINVAL
   *    The named file is not a symbolic link.
   * EIO
   *    A hardware error occurred while reading or writing data on the disk.
   */
  virtual ssize_t ReadLink(const char *filename, char *buffer, size_t size) = 0;

  /**
   * The unlink function deletes the file name filename. If this is a file’s sole name,
   * the file itself is also deleted. (Actually, if any process has the file open when this happens,
   *    deletion is postponed until all processes have closed the file.)
   * @param filename
   * @return This function returns 0 on successful completion, and -1 on error.
   * In addition to the usual file name errors (see File Name Errors),
   * the following errno error conditions are defined for this function:
   * EACCES
   * Write permission is denied for the directory from which the file is to be removed,
   * or the directory has the sticky bit set and you do not own the file.
   * EBUSY
   * This error indicates that the file is being used by the system in such a way that it can’t be unlinked.
   * For example, you might see this error if the file name specifies the root directory
   * or a mount point for a file system.
   * ENOENT
   * The file name to be deleted doesn’t exist.
   * EPERM
   * On some systems unlink cannot be used to delete the name of a directory,
   * or at least can only be used this way by a privileged user. To avoid such problems,
   * use rmdir to delete directories. (On GNU/Linux and GNU/Hurd systems
   * unlink can never delete the name of a directory.)
   * EROFS
   * The directory containing the file name to be deleted is on a read-only file system and can’t be modified.
   */
  virtual int UnLink(const char *filename) = 0;

  /**
   * The rmdir function deletes a directory. The directory must be empty before it can be removed;
   * in other words, it can only contain entries for . and ...
   * In most other respects, rmdir behaves like unlink.
   * There are two additional errno error conditions defined for rmdir:
   * ENOTEMPTY
   * EEXIST
   * The directory to be deleted is not empty.
   * These two error codes are synonymous; some systems use one, and some use the other.
   * GNU/Linux and GNU/Hurd systems always use ENOTEMPTY.
   */
  virtual int RmDir(const char *filename) = 0;

  /**
   * This is the ISO C function to remove a file. It works like unlink for files and like rmdir for directories.
   */
  virtual int Remove(const char *filename) = 0;

  /**
   * The rename function renames the file oldname to newname. The file formerly accessible under
   * the name oldname is afterwards accessible as newname instead. (If the file had any other names aside from oldname,
   * it continues to have those names.)
   * The directory containing the name newname must be
   * on the same file system as the directory containing the name oldname.
   * @param oldname If oldname is not a directory, then any existing file named newname
   * is removed during the renaming operation. However, if newname is the name of a directory,
   * rename fails in this case.
   * If oldname is a directory, then either newname must not exist or it must name a directory that is empty.
   * In the latter case, the existing directory named newname is deleted first.
   * The name newname must not specify a subdirectory of the directory oldname which is being renamed.
   * One useful feature of rename is that the meaning of newname changes “atomically”
   * from any previously existing file by that name to its new meaning (i.e., the file that was called oldname).
   * There is no instant at which newname is non-existent “in between” the old meaning and the new meaning.
   * If there is a system crash during the operation, it is possible for both names to still exist;
   * but newname will always be intact if it exists at all.
   * @param newname
   * @return If rename fails, it returns -1. In addition to the usual file name errors (see File Name Errors),
   * the following errno error conditions are defined for this function:
   * EACCES
   * One of the directories containing newname or oldname refuses write permission;
   * or newname and oldname are directories and write permission is refused for one of them.
   * EBUSY
   * A directory named by oldname or newname is being used by the system in a way
   * that prevents the renaming from working. This includes directories that are mount points for filesystems,
   * and directories that are the current working directories of processes.
   * ENOTEMPTY
   * EEXIST
   * The directory newname isn’t empty. GNU/Linux and GNU/Hurd systems always return ENOTEMPTY for this,
   * but some other systems return EEXIST.
   * EINVAL
   * oldname is a directory that contains newname.
   * EISDIR
   * newname is a directory but the oldname isn’t.
   * EMLINK
   * The parent directory of newname would have too many links (entries).
   * ENOENT
   * The file oldname doesn’t exist.
   * ENOSPC
   * The directory that would contain newname has no room for another entry,
   * and there is no space left in the file system to expand it.
   * EROFS
   * The operation would involve writing to a directory on a read-only file system.
   * EXDEV
   * The two file names newname and oldname are on different file systems.
   */
  virtual int Rename(const char *oldname, const char *newname) = 0;

  /**
   * The mkdir function creates a new, empty directory with name filename.
   * @param filename
   * @param mode The argument mode specifies the file permissions for the new directory file.
   * @return A return value of 0 indicates successful completion, and -1 indicates failure.
   * In addition to the usual file name syntax errors (see File Name Errors),
   * the following errno error conditions are defined for this function:
   * EACCES
   * Write permission is denied for the parent directory in which the new directory is to be added.
   * EEXIST
   * A file named filename already exists.
   * EMLINK
   * The parent directory has too many links (entries).
   * Well-designed file systems never report this error, because they permit more links
   * than your disk could possibly hold. However, you must still take account of the possibility of this error,
   * as it could result from network access to a file system on another machine.
   * ENOSPC
   * The file system doesn’t have enough room to create the new directory.
   * EROFS
   * The parent directory of the directory being created is on a read-only file system and cannot be modified.
   */
  virtual int MkDir(const char *filename, mode_t mode) = 0;

  /**
   * The stat function returns information about the attributes of the file named by
   * filename in the structure pointed to by buf.
   * @param filename If filename is the name of a symbolic link, the attributes you get describe the
   * file that the link points to. If the link points to a nonexistent file name,
   * then stat fails reporting a nonexistent file.
   * @param buf
   * @return The return value is 0 if the operation is successful, or -1 on failure.
   * In addition to the usual file name errors (see File Name Errors,
   * the following errno error conditions are defined for this function:
   * ENOENT
   * The file named by filename doesn’t exist.
   */
  virtual int Stat(const char *filename, kvfs_stat *buf) = 0;

  /**
   * The chmod function sets the access permission bits for the file named by filename to mode.
   * @param filename If filename is a symbolic link, chmod changes the permissions of the file pointed to by the link,
   * not those of the link itself.
   * @param mode
   * @return This function returns 0 if successful and -1 if not.
   * In addition to the usual file name errors (see File Name Errors),
   * the following errno error conditions are defined for this function:
   * ENOENT
   * The named file doesn’t exist.
   * EPERM
   * This process does not have permission to change the access permissions of this file.
   * Only the file’s owner (as judged by the effective user ID of the process) or a privileged user can change them.
   * EROFS
   * The file resides on a read-only file system.
   * EFTYPE
   * mode has the S_ISVTX bit (the “sticky bit”) set, and the named file is not a directory.
   * Some systems do not allow setting the sticky bit on non-directory files,
   * and some do (and only some of those assign a useful meaning to the bit for non-directory files).
   * You only get EFTYPE on systems where the sticky bit has no useful meaning for non-directory files,
   * so it is always safe to just clear the bit in mode and call chmod again.
   */
  virtual int ChMod(const char *filename, mode_t mode) = 0;

  /**
   * The access function checks to see whether the file named by filename can be accessed
   * in the way specified by the how argument. The how argument either can be
   * the bitwise OR of the flags R_OK, W_OK, X_OK, or the existence test F_OK.
   * @param filename
   * @param how
   * @return The return value is 0 if the access is permitted, and -1 otherwise.
   * (In other words, treated as a predicate function, access returns true if the requested access is denied.)
   * In addition to the usual file name errors (see File Name Errors),
   * the following errno error conditions are defined for this function:
   * EACCES
   * The access specified by how is denied.
   * ENOENT
   * The file doesn’t exist.
   * EROFS
   * Write permission was requested for a file on a read-only file system.
   */
  virtual int Access(const char *filename, int how) = 0;

  /**
   * This function is used to modify the file times associated with the file named filename.
   * The attribute modification time for the file is set to the current time in either case
   * (since changing the time stamps is itself a modification of the file attributes).
   * @param filename
   * @param times If times is a null pointer, then the access and modification times
   * of the file are set to the current time. Otherwise, they are set to the values
   * from the actime and modtime members (respectively) of the utimbuf structure pointed to by times.
   * @return The utime function returns 0 if successful and -1 on failure.
   * In addition to the usual file name errors (see File Name Errors),
   * the following errno error conditions are defined for this function:
   * EACCES
   * There is a permission problem in the case where a null pointer was passed as the times argument.
   * In order to update the time stamp on the file, you must either be the owner of the file,
   * have write permission for the file, or be a privileged user.
   * ENOENT
   * The file doesn’t exist.
   * EPERM
   * If the times argument is not a null pointer, you must either be the owner of the file or be a privileged user.
   * EROFS
   * The file lives on a read-only file system.
   */
  virtual int UTime(const char *filename, const struct utimbuf *times) = 0;

  /**
   * The truncate function changes the size of filename to length. If length is shorter than the previous length, #
   * data at the end will be lost. The file must be writable by the user to perform this operation.
   * @param filename
   * @param length If length is longer, holes will be added to the end.
   * @return The return value is 0 for success, or -1 for an error.
   * In addition to the usual file name errors, the following errors may occur:
   * EACCES
   * The file is a directory or not writable.
   * EINVAL
   * length is negative.
   * EFBIG
   * The operation would extend the file beyond the limits of the operating system.
   * EIO
   * A hardware I/O error occurred.
   * EPERM
   * The file is "append-only" or "immutable".
   * EINTR
   * The operation was interrupted by a signal.
   */
  virtual int Truncate(const char *filename, off_t length) = 0;

  /**
   * The mknod function makes a special file with name filename. The mode specifies the mode of the file,
   * and may include the various special file bits,
   * such as S_IFCHR (for a character special file) or S_IFBLK (for a block special file).
   * @param filename
   * @param mode
   * @param dev The dev argument specifies which device the special file refers to.
   * Its exact interpretation depends on the kind of special file being created.
   * @return The return value is 0 on success and -1 on error.
   * In addition to the usual file name errors (see File Name Errors),
   * the following errno error conditions are defined for this function:
   * EPERM
   * The calling process is not privileged. Only the superuser can create special files.
   * ENOSPC
   * The directory or file system that would contain the new file is full and cannot be extended.
   * EROFS
   * The directory containing the new file can’t be modified because it’s on a read-only file system.
   * EEXIST
   * There is already a file named filename. If you want to replace this file,
   * you must remove the old file explicitly first.
   */
  virtual int Mknod(const char *filename, mode_t mode, dev_t dev) = 0;

  /**
   * Trigger compaction and tuning for the undelying key-value store.
   */
  virtual void TuneFS() = 0;

  /**
   * The open function creates and returns a new file descriptor for the file named by filename.
   * Initially, the file position indicator for the file is at the beginning of the file.
   * @param filename
   * @param flags The flags argument controls how the file is to be opened.
   * This is a bit mask; you create the value by the bitwise OR of the appropriate parameters (using the ‘|’ operator in C).
   * @return The normal return value from open is a non-negative integer file descriptor.
   * In the case of an error, a value of -1 is returned instead. In addition to the usual file name errors,
   * the following errno error conditions are defined for this function:
   *
EACCES

    The file exists but is not readable/writable as requested by the flags argument, or the file does not exist and the directory is unwritable so it cannot be created.
EEXIST

    Both O_CREAT and O_EXCL are set, and the named file already exists.
EINTR

    The open operation was interrupted by a signal. See Interrupted Primitives.
EISDIR

    The flags argument specified write access, and the file is a directory.
EMFILE

    The process has too many files open. The maximum number of file descriptors is controlled by the RLIMIT_NOFILE resource limit; see Limits on Resources.
ENFILE

    The entire system, or perhaps the file system which contains the directory, cannot support any additional open files at the moment. (This problem cannot happen on GNU/Hurd systems.)
ENOENT

    The named file does not exist, and O_CREAT is not specified.
ENOSPC

    The directory or file system that would contain the new file cannot be extended, because there is no disk space left.
ENXIO

    O_NONBLOCK and O_WRONLY are both set in the flags argument, the file named by filename is a FIFO (see Pipes and FIFOs), and no process has the file open for reading.
EROFS

    The file resides on a read-only file system and any of O_WRONLY, O_RDWR, and O_TRUNC are set in the flags argument, or O_CREAT is set and the file does not already exist.

   */
  virtual int Open(const char *filename, int flags, mode_t mode) = 0;

  /**
   * The function close closes the file descriptor filedes. Closing a file has the following consequences:

    The file descriptor is deallocated.
    Any record locks owned by the process on the file are unlocked.
    When all file descriptors associated with a pipe or FIFO have been closed, any unread data is discarded.
    This function is a cancellation point in multi-threaded programs. This is a problem if the thread allocates some resources (like memory, file descriptors, semaphores or whatever) at the time close is called. If the thread gets canceled these resources stay allocated until the program ends. To avoid this, calls to close should be protected using cancellation handlers.
   * @param filedes
   * @return The normal return value from close is 0; a value of -1 is returned in case of failure. The following errno error conditions are defined for this function:

EBADF

    The filedes argument is not a valid file descriptor.
EINTR

    The close call was interrupted by a signal. See Interrupted Primitives. Here is an example of how to handle EINTR properly:

    TEMP_FAILURE_RETRY (close (desc));

ENOSPC
EIO
EDQUOT
   */
  virtual int Close(int filedes) = 0;

  /**
   * The read function reads up to size bytes from the file with descriptor filedes, storing the results in the buffer. (This is not necessarily a character string, and no terminating null character is added.)
   * @param filedes
   * @param buffer
   * @param size
   * @return
   * The return value is the number of bytes actually read. This might be less than size; for example, if there aren’t that many bytes left in the file or if there aren’t that many bytes immediately available. The exact behavior depends on what kind of file it is. Note that reading less than size bytes is not an error.

A value of zero indicates end-of-file (except if the value of the size argument is also zero). This is not considered an error. If you keep calling read while at end-of-file, it will keep returning zero and doing nothing else.

If read returns at least one character, there is no way you can tell whether end-of-file was reached. But if you did reach the end, the next read will return zero.

In case of an error, read returns -1. The following errno error conditions are defined for this function:

EAGAIN

    Normally, when no input is immediately available, read waits for some input. But if the O_NONBLOCK flag is set for the file (see File Status Flags), read returns immediately without reading any data, and reports this error.

    Compatibility Note: Most versions of BSD Unix use a different error code for this: EWOULDBLOCK. In the GNU C Library, EWOULDBLOCK is an alias for EAGAIN, so it doesn’t matter which name you use.

    On some systems, reading a large amount of data from a character special file can also fail with EAGAIN if the kernel cannot find enough physical memory to lock down the user’s pages. This is limited to devices that transfer with direct memory access into the user’s memory, which means it does not include terminals, since they always use separate buffers inside the kernel. This problem never happens on GNU/Hurd systems.

    Any condition that could result in EAGAIN can instead result in a successful read which returns fewer bytes than requested. Calling read again immediately would result in EAGAIN.
EBADF

    The filedes argument is not a valid file descriptor, or is not open for reading.
EINTR

    read was interrupted by a signal while it was waiting for input. See Interrupted Primitives. A signal will not necessarily cause read to return EINTR; it may instead result in a successful read which returns fewer bytes than requested.
EIO

    For many devices, and for disk files, this error code indicates a hardware error.

    EIO also occurs when a background process tries to read from the controlling terminal, and the normal action of stopping the process by sending it a SIGTTIN signal isn’t working. This might happen if the signal is being blocked or ignored, or because the process group is orphaned. See Job Control, for more information about job control, and Signal Handling, for information about signals.
EINVAL

    In some systems, when reading from a character or block device, position and size offsets must be aligned to a particular block size. This error indicates that the offsets were not properly aligned.

   */
  virtual ssize_t Read(int filedes, void *buffer, size_t size) = 0;

  /**
   * The write function writes up to size bytes from buffer to the file with descriptor filedes. The data in buffer is not necessarily a character string and a null character is output like any other character.
   * The return value is the number of bytes actually written. This may be size, but can always be smaller. Your program should always call write in a loop, iterating until all the data is written.

Once write returns, the data is enqueued to be written and can be read back right away, but it is not necessarily written out to permanent storage immediately. You can use fsync when you need to be sure your data has been permanently stored before continuing.
   * @param filedes
   * @param buffer
   * @param size
   * @return
   * In the case of an error, write returns -1. The following errno error conditions are defined for this function:

EAGAIN

    Normally, write blocks until the write operation is complete. But if the O_NONBLOCK flag is set for the file (see Control Operations), it returns immediately without writing any data and reports this error. An example of a situation that might cause the process to block on output is writing to a terminal device that supports flow control, where output has been suspended by receipt of a STOP character.

    Compatibility Note: Most versions of BSD Unix use a different error code for this: EWOULDBLOCK. In the GNU C Library, EWOULDBLOCK is an alias for EAGAIN, so it doesn’t matter which name you use.

    On some systems, writing a large amount of data from a character special file can also fail with EAGAIN if the kernel cannot find enough physical memory to lock down the user’s pages. This is limited to devices that transfer with direct memory access into the user’s memory, which means it does not include terminals, since they always use separate buffers inside the kernel. This problem does not arise on GNU/Hurd systems.
EBADF

    The filedes argument is not a valid file descriptor, or is not open for writing.
EFBIG

    The size of the file would become larger than the implementation can support.
EINTR

    The write operation was interrupted by a signal while it was blocked waiting for completion. A signal will not necessarily cause write to return EINTR; it may instead result in a successful write which writes fewer bytes than requested. See Interrupted Primitives.
EIO

    For many devices, and for disk files, this error code indicates a hardware error.
ENOSPC

    The device containing the file is full.
EPIPE

    This error is returned when you try to write to a pipe or FIFO that isn’t open for reading by any process. When this happens, a SIGPIPE signal is also sent to the process; see Signal Handling.
EINVAL

    In some systems, when writing to a character or block device, position and size offsets must be aligned to a particular block size. This error indicates that the offsets were not properly aligned.

   */
  virtual ssize_t Write(int filedes, const void *buffer, size_t size) = 0;

  /**
   * The lseek function is used to change the file position of the file with descriptor filedes.
   * @param filedes
   * @param offset
   * @param whence The whence argument specifies how the offset should be interpreted, in the same way as for the fseek function, and it must be one of the symbolic constants SEEK_SET, SEEK_CUR, or SEEK_END.

SEEK_SET

    Specifies that offset is a count of characters from the beginning of the file.
SEEK_CUR

    Specifies that offset is a count of characters from the current file position. This count may be positive or negative.
SEEK_END

    Specifies that offset is a count of characters from the end of the file. A negative count specifies a position within the current extent of the file; a positive count specifies a position past the current end. If you set the position past the current end, and actually write data, you will extend the file with zeros up to that position.

   * @return
   * The return value from lseek is normally the resulting file position, measured in bytes from the beginning of the file. You can use this feature together with SEEK_CUR to read the current file position.

If you want to append to the file, setting the file position to the current end of file with SEEK_END is not sufficient. Another process may write more data after you seek but before you write, extending the file so the position you write onto clobbers their data. Instead, use the O_APPEND operating mode; see Operating Modes.

You can set the file position past the current end of the file. This does not by itself make the file longer; lseek never changes the file. But subsequent output at that position will extend the file. Characters between the previous end of file and the new position are filled with zeros. Extending the file in this way can create a “hole”: the blocks of zeros are not actually allocated on disk, so the file takes up less space than it appears to; it is then called a “sparse file”.

If the file position cannot be changed, or the operation is in some way invalid, lseek returns a value of -1. The following errno error conditions are defined for this function:

EBADF

    The filedes is not a valid file descriptor.
EINVAL

    The whence argument value is not valid, or the resulting file offset is not valid. A file offset is invalid.
ESPIPE

    The filedes corresponds to an object that cannot be positioned, such as a pipe, FIFO or terminal device. (POSIX.1 specifies this error only for pipes and FIFOs, but on GNU systems, you always get ESPIPE if the object is not seekable.)

   */
  virtual off_t LSeek(int filedes, off_t offset, int whence) = 0;

  /**
   * This function copies up to length bytes from the file descriptor inputfd to the file descriptor outputfd.

The function can operate on both the current file position (like read and write) . If the inputpos pointer is null, the file position of inputfd is used as the starting point of the copy operation, and the file position is advanced during it. If inputpos is not null, then *inputpos is used as the starting point of the copy operation, and *inputpos is incremented by the number of copied bytes, but the file position remains unchanged. Similar rules apply to outputfd and outputpos for the output file position.
   * @param inputfd
   * @param inputpos
   * @param outputfd
   * @param outputpos
   * @param length
   * @param flags
   * @return
   * The copy_file_range function returns the number of bytes copied. This can be less than the specified length in case the input file contains fewer remaining bytes than length, or if a read or write failure occurs. The return value is zero if the end of the input file is encountered immediately.

If no bytes can be copied, to report an error, copy_file_range returns the value -1 and sets errno. The following errno error conditions are specific to this function:

EISDIR

    At least one of the descriptors inputfd or outputfd refers to a directory.
EINVAL

    At least one of the descriptors inputfd or outputfd refers to a non-regular, non-directory file (such as a socket or a FIFO).

    The input or output positions before are after the copy operations are outside of an implementation-defined limit.

    The flags argument is not zero.
EFBIG

    The new file size would exceed the process file size limit. See Limits on Resources.

    The input or output positions before are after the copy operations are outside of an implementation-defined limit. This can happen if the file was not opened with large file support (LFS) on 32-bit machines, and the copy operation would create a file which is larger than what off_t could represent.
EBADF

    The argument inputfd is not a valid file descriptor open for reading.

    The argument outputfd is not a valid file descriptor open for writing, or outputfd has been opened with O_APPEND.
EXDEV

    The input and output files reside on different file systems.
In addition, copy_file_range can fail with the error codes which are used by read, pread, write, and pwrite.
   */
  virtual ssize_t CopyFileRange(int inputfd,
                                off64_t *inputpos,
                                int outputfd,
                                off64_t *outputpos,
                                ssize_t length,
                                unsigned int flags) = 0;

  /**
   * A call to this function will not return as long as there is data which has not been written to the device.
   */
  virtual void Sync() = 0;

  /**
   * The fsync function can be used to make sure all data associated with the open file fildes is written to the device associated with the descriptor. The function call does not return unless all actions have finished.
   * @param filedes
   * @return
   * The return value of the function is zero if no error occurred. Otherwise it is -1 and the global variable errno is set to the following values:

EBADF

    The descriptor fildes is not valid.
EINVAL

    No synchronization is possible since the system does not implement this.

   */
  virtual int FSync(int filedes) = 0;
  /**
   * The pread function is similar to the read function. The first three arguments are identical, and the return values and error codes also correspond.

The difference is the fourth argument and its handling. The data block is not read from the current position of the file descriptor filedes. Instead the data is read from the file starting at position offset. The position of the file descriptor itself is not affected by the operation. The value is the same as before the call.

When the source file is compiled with _FILE_OFFSET_BITS == 64 the pread function is in fact pread64 and the type off_t has 64 bits, which makes it possible to handle files up to 2^63 bytes in length.


   * @param filedes
   * @param buffer
   * @param size
   * @param offset
   * @return
   * The return value of pread describes the number of bytes read. In the error case it returns -1 like read does and the error codes are also the same, with these additions:

EINVAL

    The value given for offset is negative and therefore illegal.
ESPIPE

    The file descriptor filedes is associated with a pipe or a FIFO and this device does not allow positioning of the file pointer.
   */
  virtual ssize_t PRead(int filedes, void *buffer, size_t size, off_t offset) = 0;

  /**
   * The pwrite function is similar to the write function. The first three arguments are identical, and the return values and error codes also correspond.

The difference is the fourth argument and its handling. The data block is not written to the current position of the file descriptor filedes. Instead the data is written to the file starting at position offset. The position of the file descriptor itself is not affected by the operation. The value is the same as before the call.
   * @param filedes
   * @param buffer
   * @param size
   * @param offset
   * @return The return value of pwrite describes the number of written bytes. In the error case it returns -1 like write does and the error codes are also the same, with these additions:

EINVAL

    The value given for offset is negative and therefore illegal.
ESPIPE

    The file descriptor filedes is associated with a pipe or a FIFO and this device does not allow positioning of the file pointer.

   */
  virtual ssize_t PWrite(int filedes, const void *buffer, size_t size, off_t offset) = 0;

  virtual void DestroyFS() = 0;
};

#endif //FILESYSTEM_H