/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   store_result.cpp
 */


#include "store_result.h"

namespace {
void freeString(void * /* buffer */, void *userData) {
  auto str = static_cast<std::string *>(userData);
  delete str;
}
} // namespace