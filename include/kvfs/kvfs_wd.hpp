/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   kvfs_wd.hpp
 */

#ifndef KVFS_WD
#define KVFS_WD

#include <sys/types.h>

namespace kvfs {

    /**
     * @name Working directory
     * @brief This is used to resolve relative file names.
     *
     */
    class wd {
    public:
        // Constructor
        wd() = default;

        // Destructor
        virtual ~wd() = default;

        /**
         * @brief The getcwd function returns an absolute file name representing the current working directory,
         * storing it in the character array buffer that you provide.
         * The size argument is how you tell the system the allocation size of buffer.
         * @param buffer
         * @param size
         * @return The return value is buffer on success and a null pointer on failure.
         * The following errno error conditions are defined for this function:
         * EINVAL
         *      The size argument is zero and buffer is not a null pointer.
         * ERANGE
         *      The size argument is less than the length of the working directory name.
         *      You need to allocate a bigger array and try again.
         * EACCES
         *      Permission to read or search a component of the file name was denied.

         */
        virtual KVFS_WD char *fs_getcwd(char *buffer, size_t size) = 0;


        /**
         * @brief The get_current_dir_name function is basically equivalent to getcwd (NULL, 0),
         * except the value of the PWD environment variable is first examined,
         * and if it does in fact correspond to the current directory, that value is returned.
         * This is a subtle difference which is visible if the path described by the value in
         * PWD is using one or more symbolic links, in which case the value returned by getcwd
         * would resolve the symbolic links and therefore yield a different result.
         */
        virtual KVFS_WD char *fs_get_current_dir_name() = 0;

        /**
         * @brief This function is used to set the process’s working directory to filename.
         * @param filename
         * @return  The normal, successful return value from chdir is 0.
         * A value of -1 is returned to indicate an error. The errno error conditions defined
         * for this function are the usual file name syntax errors,
         * plus ENOTDIR if the file filename is not a directory.
         */
        virtual KVFS_WD int fs_chdir(const char *filename) = 0;

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
        virtual KVFS_WD int fs_fchdir(int filedes) = 0;
    };
}
#endif //KVFS_WD
