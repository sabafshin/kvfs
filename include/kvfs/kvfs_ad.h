/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   kvfs_ad.h
 */

#ifndef KVFS_AD
#define KVFS_AD

#include <sys/types.h>
#include <dirent.h>

namespace kvfs{
    class ad{

    private:
#ifdef __USE_LARGEFILE64
        struct dirent64 _dirent;
#else
        struct dirent _dirent;
#endif


    public:
        ad() = default;

        virtual ~ad() = default;

        /**
         * @brief The opendir function opens and returns a directory stream for reading the directory
         * whose file name is dirname. The stream has type DIR *.
         * @param dirname file name
         * @return If unsuccessful, opendir returns a null pointer.
         */
        virtual  DIR *fs_opendir (const char *dirname) = 0;

        /**
         * @brief The fdopendir function works just like opendir but instead of taking a file name and opening
         * a file descriptor for the directory the caller is required to provide a file descriptor.
         * This file descriptor is then used in subsequent uses of the returned directory stream object.
         * @param fd The caller must make sure the file descriptor is associated with a directory and it allows reading
         * @return If the fdopendir call returns successfully the file descriptor is now under the control of the system.
         * It can be used in the same way the descriptor implicitly created by opendir can be used but the program must
         * not close the descriptor. In case the function is unsuccessful it returns a null pointer and
         * the file descriptor remains to be usable by the program.
         */
        virtual DIR * fs_fdopendir (int fd) = 0;

        /**
         * @brief The function dirfd returns the file descriptor associated with the directory stream dirstream.
         * This descriptor can be used until the directory is closed with closedir.
         * @param dirstream
         * @return  If the directory stream implementation is not using file descriptors the return value is -1.
         */
        virtual int fs_dirfd (DIR *dirstream) = 0 ;
    };


}

#endif //KVFS_AD
