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

#ifndef ELASTICHASHINATOR_H_
#define ELASTICHASHINATOR_H_

#include <string>

#include "TheHashinator.h"
#include "MurmurHash3.h"
#include <map>

namespace voltdb {

/*
 * Concrete implementation of TheHashinator that uses MurmurHash3_x64_128 to hash values
 * onto a consistent hash ring.
 */
class ElasticHashinator : public TheHashinator {

public:

    ElasticHashinator(const char *tokens){
        /* Construct the hashinator from a binary description of the ring.
        * followed by the token values where each token value consists of the 8-byte position on the ring
        * and and the 4-byte partition id. All values are signed.
        */
        m_tokenCount = 0;
        memcpy( &m_tokenCount, tokens, 4);
        m_tokenCount = ntohl(m_tokenCount);
        debug_msg("ElasticHashinator: tokenCount " << m_tokenCount);
        const uint32_t tokenCountsOffset = 4;
        const uint32_t partitionOffset = 8;
        const uint32_t int32size = sizeof(int32_t);

        m_tokens.reset(new int32_t[2*m_tokenCount]);

        for (uint32_t ii = 0; ii < m_tokenCount; ii++) {
            int32_t hash;
            memcpy( &hash, tokens + ii*8 + tokenCountsOffset, int32size);

            int32_t partitionId;
            memcpy( &partitionId, tokens + ii*8 + partitionOffset, int32size);

            debug_msg("ElasticHashinator: hash " << std::hex << ntohl(hash) <<" partition " << std::dec << ntohl(partitionId));
            m_tokens[ii * 2] = ntohl(hash);
            m_tokens[ii * 2 + 1] =  ntohl(partitionId);
        }
    }


    virtual ~ElasticHashinator() { }

    /**
     * Given a long value, pick a partition to store the data.
     *
     * @param value The value to hash.
     * @param partitionCount The number of partitions to choose from.
     * @return A value between 0 and partitionCount-1, hopefully pretty evenly
     * distributed.
     */
    int32_t hashinate(int64_t value) const {
        // special case this hard to hash value to 0 (in both c++ and java)
        if (value == INT64_MIN) return 0;
        const int32_t hash = MurmurHash3_x64_128(value);
        return partitionForToken(hash);
    }

    /*
     * Given a piece of UTF-8 encoded character data OR binary data
     * pick a partition to store the data
     */
    int32_t hashinate(const char *string, int32_t length) const {
        const int32_t hash = MurmurHash3_x64_128(string, length, 0);
        debug_msg("ElasticHashinator: hashinate " <<  string <<" hash " <<std::hex<< hash);
        return partitionForToken(hash);
    }

private:
    boost::scoped_array<int32_t> m_tokens;
    uint32_t m_tokenCount;

    int32_t partitionForToken(int32_t hash) const {
        int32_t min = 0;
        int32_t max = m_tokenCount - 1;

        while (min <= max) {
            assert(min >= 0);
            assert(max >= 0);
            uint32_t mid = (min + max) >> 1;
            int32_t midval = m_tokens[mid * 2];

            if (midval < hash) {
                min = mid + 1;
            } else if (midval > hash) {
                max = mid - 1;
            } else {
                return m_tokens[mid * 2 + 1];
            }
        }
        return m_tokens[(min - 1) * 2 + 1];
    }
};
}
#endif /* ELASTICHASHINATOR_H_ */
