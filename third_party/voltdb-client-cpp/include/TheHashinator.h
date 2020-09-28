/* This file is part of VoltDB.
 * Copyright (C) 2008-2018 VoltDB Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef THEHASHINATOR_H_
#define THEHASHINATOR_H_

//DEBUG ONLY
//#define __DEBUG

#ifdef __DEBUG
#include <iostream>
#include <fstream>

#define debug_msg(text)\
{\
    std::fstream fs;\
    fs.open ("/tmp/hashinator.log", std::fstream::out | std::fstream::app);\
    fs << __FILE__ << ":" << __LINE__ << " " << text << std::endl;\
    fs.close();\
}

#else

#define debug_msg(text)

#endif


#include <stdint.h>
namespace voltdb {


/**
 *  Abstract base class for hashing SQL values to partition ids
 */
class TheHashinator {
  public:

    virtual ~TheHashinator() {}

    TheHashinator() {}

  public:

    /**
     * Given a long value, pick a partition to store the data.
     *
     * @param value The value to hash.
     * @return A value between 0 and partitionCount-1, hopefully pretty evenly
     * distributed.
     */
    virtual int32_t hashinate(int64_t value) const = 0;

    /*
     * Given a piece of UTF-8 encoded character data OR binary data
     * pick a partition to store the data
     */
    virtual int32_t hashinate(const char *string, int32_t length) const = 0;
};

} // namespace voltdb

#endif // THEHASHINATOR_H_
