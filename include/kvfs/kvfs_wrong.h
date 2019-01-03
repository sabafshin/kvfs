// Copyright 2018 Afshin Sabahi. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.
/*
/**
 * @brief Header file for kvfs_wrong.cpp
 * 
 * @file kvfs_wrong.h
 * @author Afshin Sabahi
 */
#include <iostream>
#include <cassert>
#include <dirent.h>
#include <stdio.h>

// Rocksdb includes
#include <rocksdb/db.h>

#include <errno.h>


using namespace std;
/**
 * @brief Initilises FS, creates a DB and configures it with 
 * options.
 * 
 * @param db_path_ the path where the DB will be created.
 * @param db_options_path_ the path where the preconfigured options are written. 
 * This is a Rocks DB options file.
 */
void init(const string* db_path_, const string* db_options_path_);
/**
 * @brief Same init but uses default paths for DB and create a default options.
 * 
 */
void init();



typedef uint eqtype;
typedef char** val;
typedef bool mode;
typedef bool open_mode;
typedef vector<char> flags;
typedef bool stat;
typedef bool access_mode;

/**
 * @brief KVFS class
 * 
 */
class kvfs_wrong{
    eqtype uid;
    eqtype gid;
    eqtype file_desc;

    /**
     * @brief Convert between an abstract open file descriptor 
     * and the integer representation used by 
     * the operating system.
     * 
     * @param file_desc 
     * @return eqtype 
     */
    eqtype kvfs_fdToWord(eqtype* file_desc);
    eqtype kvfs_wordToFD(eqtype* i);
    
    /** 
     * @brief Opens the directory designated by dirName parameter 
     * and associates a directory stream with it. The directory stream
     * is positioned at the first entry. A directory stream is an 
     * ordered sequence of all the directory entries in a particular directory. 
     * 
     * @param dirName 
     */
    void kvfs_openDir(DIR dirName);

    /**
     * @brief Read the next filename in the directory stream d. 
     * Entries for "." (current directory) 
     * and ".." (parent directory) are never returned.
     * 
     * @param d 
     */
    void kvfs_readDir(DIR d);

    void kvfs_rewindDir(DIR d);

    void kvfs_closeDir(DIR d);

    void kvfs_chDir(DIR s);

    DIR kvfs_getcwd();

    struct kvfs_S{
        //include POSIX_FLAGS
        flags POSIX_FLAGS;

        mode irwxu;
        mode irusr;
        mode iwusr;
        mode ixusr;

        mode irwxg;
        mode irgrp;
        mode iwgrp;
        mode ixgrp;

        mode irwxo;
        mode iroth;
        mode iwoth;
        mode ixoth;

        mode isuid;
        mode isgid;
    };

    struct kvfs_O{
        //include POSIX_FLAGS
        flags POSIX_FLAGS;

        mode append;
        mode excl;
        mode noctty;
        mode nonblock;
        mode sync;

        mode trunct;
    };

    open_mode kvfs_open_mode;
    
    void kvfs_openf (val s, open_mode om, flags f);
    void kvfs_createf (val s, open_mode om, flags f, mode m);

    void kvfs_creat(val s, mode m);

    __cpu_mask kvfs_umask( __cpu_mask cmask);

    void kvfs_link(val _old, val _new);

    void kvfs_mkdir(val s, mode m);
    void kvfs_mkfifo(val s, mode m);

    void kvfs_unlink(val path);

    void kvfs_rmdir(val s);
    void kvfs_rename(val _old, val _new);

    void kvfs_symlink(val _old, val _new);
    void kvfs_readlink(val s);

    /**
     * @brief Device Identifier.
     * File serial number (I-node) and device ID uniquely identify a file
     */
    eqtype dev;

    eqtype kvfs_wordToDev(val i);
    val kvfs_devToWord (eqtype dev);
private:
    eqtype ino;
public:    
    eqtype kvfs_wordToIno(val i);
    val kvfs_inoToWord();
private:
    struct kvfs_ST{
        stat isDir;
        stat isChr;
        stat isBlk;
        stat isReg;
        stat isFIFO;
        stat isLink;
        stat isSock;
        mode st_mode;
        
        eqtype st_ino;
        eqtype st_dev;

        eqtype st_nlink;
        eqtype uid;
        eqtype gid;
        
        size_t size;

        uint atime;
        uint mtime;
        uint ctime;
    };

public:
    void kvfs_access(val s, access_mode l);

    void kvfs_chmod(val s, access_mode mode);
    void kvfs_fchmod(val fd, access_mode mode);
    
    void kvfs_chown(val s, eqtype uid, eqtype gid);
    void kvfs_fchown(val fd, eqtype uid, eqtype gid);

    void kvfs_utime(val s, access_mode a, uint m);
    void kvfs_utime(val s);

    void kvfs_ftruncate(val fd, uint n);
    void kvfs_pathconf(val s, bool property);

};