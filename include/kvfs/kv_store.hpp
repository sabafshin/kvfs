/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   kv_store.hpp
 */

#include <iostream>

#ifndef KVFS_KV_STORE_HPP
#define KVFS_KV_STORE_HPP


class kvfs_kv_store {
public:
    kvfs_kv_store() = default;

    virtual ~kvfs_kv_store() = default;
};

class kvfs_kv_store_init {
public:
    virtual kvfs_kv_store *create_kv_store_instance() = 0;

    virtual ~kvfs_kv_store_init() = default;
};

#endif //KVFS_KV_STORE_HPP
