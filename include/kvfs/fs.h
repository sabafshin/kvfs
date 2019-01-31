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

/**
 * Use this abstract class to create an instance of KVFS
 * Example:
 * std::unique_ptr<FS> fs_ = std::make_unique<KVFS>(// optional mount path );
 * or use shared ptr if necessary.
 * If no mount path is given, the filesystem is mounted at "/tmp/db/"
 */
class FS {
 public:

  FS() : root_path_("tmp/db/") {};
  explicit FS(std::string mount_path) : root_path_(std::move(mount_path)) {};
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
  virtual int ChDir(const char *filename) = 0;

  /**
    * @brief The opendir function opens and returns a directory stream for reading the directory
    * whose file name is dirname. The stream has type DIR *.
    * @param dirname file name
    * @return If unsuccessful, opendir returns a null pointer.
    */
  virtual DIR *OpenDir(const char *dirname) = 0;

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
  virtual struct dirent *ReadDir(DIR *dirstream) = 0;
  virtual struct dirent64 *ReadDir64(DIR *dirstream) = 0;

  /**
   * This function closes the directory stream dirstream. It returns 0 on success and -1 on failure.
   * The following errno error conditions are defined for this function:
   * EBADF
   *    The dirstream argument is not valid.
   * @param dirstream
   */
  virtual int CloseDir(DIR *dirstream) = 0;

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
   * @param oldname
   * @param newname
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
  virtual int SymLink(const char *oldname, const char *newname) = 0;

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
  virtual int Stat(const char *filename, struct stat *buf) = 0;
  virtual int Stat64(const char *filename, struct stat64 *buf) = 0;

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
  virtual int Truncate64(const char *name, off64_t length) = 0;

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

 protected:
  const std::string root_path_;
};

#endif //FILESYSTEM_H