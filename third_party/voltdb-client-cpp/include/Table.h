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

#ifndef VOLTDB_TABLE_H_
#define VOLTDB_TABLE_H_

#include "ByteBuffer.hpp"
#include <boost/shared_ptr.hpp>
#include <vector>
#include "Column.hpp"
#include <sstream>
#include <iostream>

namespace voltdb {
class TableIterator;
class RowBuilder;

/*
 * Reprentation of result tables returns by VoltDB.
 */
class Table {
    friend class TableTest;
public:
    /*
     * Construct a table from a shared buffer. The table retains a reference
     * to the shared buffer indefinitely so watch out for unwanted memory retension.
     */
    Table(SharedByteBuffer buffer);
    Table(const std::vector<Column> &columns) throw (TableException);
    Table() {}

    ~Table() {
    }

    /*
     * Returns an iterator for iterating over the rows of this table
     */
    TableIterator iterator() const;

    /*
     * Returns the status code associated with this table that was set by the stored procedure.
     * Default value if not set is -128
     */
    int8_t getStatusCode() const;

    /*
     * Retrieve the number of rows contained in this table
     */
    int32_t rowCount() const;

    /*
     * Adds the row specified in RowBuilder to table.
     * Precondition: Row and Table schema should be same
     * and all the row columns should be init'ed
     */
    void addRow(RowBuilder& row) throw (TableException, UninitializedColumnException, InCompatibleSchemaException);

    /*
     * Retrieve a copy of the column metadata.
     */
    std::vector<voltdb::Column> columns() const;

    /*
     * Retrieve the number of columns in this table's schema.
     */
    int32_t columnCount() const;

    /*
     * Returns a string representation of this table and all of its rows.
     */
    std::string toString() const;

    /*
     * Serialize Volt table to specified byte buffer. The specified buffer should
     * have ample space available to serialize table data. The needed space can
     * be queried using getSerializedSize()
     */
    int32_t serializeTo(ByteBuffer& buffer) throw (TableException);

    /**
     * Returns the size in bytes needed to serialize the current table data
     * including 4 byte size meta-data
     */
    int32_t getSerializedSize() const;

    /*
     * Returns a string representation of this table and all of its rows with
     * the specified level of indentation before each line.
     */
    void toString(std::ostringstream &ostream, std::string indent) const;

    void operator >> (std::ostream &ostream) const;

    bool operator==(const Table& rhs) const;
    bool operator!=(const Table& rhs) const;

    Table(std::istream &istream);

    const static int32_t MAX_TUPLE_LENGTH;
    const static int8_t DEFAULT_STATUS_CODE;
private:
    void validateRowScehma(const std::vector<Column>& schema) const throw (InCompatibleSchemaException);
    boost::shared_ptr<std::vector<voltdb::Column> > m_columns;
    int32_t m_rowCountPosition;
    int32_t m_rowCount;
    mutable voltdb::SharedByteBuffer m_buffer;
};
}

#endif /* VOLTDB_TABLE_H_ */
