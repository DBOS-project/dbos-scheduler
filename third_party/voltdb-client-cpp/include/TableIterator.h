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

#ifndef VOLTDB_TABLEITERATOR_H_
#define VOLTDB_TABLEITERATOR_H_

#include "ByteBuffer.hpp"
#include "Column.hpp"
#include <boost/shared_ptr.hpp>
#include "Row.hpp"
#include "Exception.hpp"

namespace voltdb {
class Table;

/*
 * Iterator for retrieving rows from a table. Retains a shared pointer to the buffer backing the table. Watch out
 * for unwanted memory retention.
 */
class TableIterator {
public:

    /*
     * Construct an iterator for the table rows with the specified column schema and row count
     */
#ifdef SWIG
%ignore TableIterator(voltdb::SharedByteBuffer rows,
            boost::shared_ptr<std::vector<voltdb::Column> > columns,
            int32_t rowCount);
#endif
    TableIterator(
            voltdb::SharedByteBuffer rows,
            boost::shared_ptr<std::vector<voltdb::Column> > columns,
            int32_t rowCount) :
        m_buffer(rows), m_columns(columns), m_rowCount(rowCount), m_currentRow(0) {}

    /*
     * Returns true if the table has more rows that can be retrieved via invoking next and false otherwise.
     */
    bool hasNext() {
        if (m_currentRow < m_rowCount) {
            assert(m_buffer.hasRemaining());
            return true;
        }
        return false;
    }

    /*
     * Returns the next row in the table if there are more rows or NoMoreRowsException if there are no
     * more rows. OverflowUnderflowException and IndexOutOfBoundsException only result if there are bugs
     * and are not expected normally.
     */
    voltdb::Row next() throw (NoMoreRowsException, OverflowUnderflowException, IndexOutOfBoundsException) {
        if (m_rowCount <= m_currentRow) {
            throw NoMoreRowsException();
        }
        int32_t rowLength = m_buffer.getInt32();
        int32_t oldLimit = m_buffer.limit();
        m_buffer.limit(m_buffer.position() + rowLength);
        SharedByteBuffer buffer = m_buffer.slice();
        m_buffer.limit(oldLimit);
        m_currentRow++;
        return voltdb::Row(buffer, m_columns);
    }

private:
    voltdb::SharedByteBuffer m_buffer;
    boost::shared_ptr<std::vector<voltdb::Column> > m_columns;
    int32_t m_rowCount;
    int32_t m_currentRow;
};
}
#endif /* VOLTDB_TABLEITERATOR_H_ */
