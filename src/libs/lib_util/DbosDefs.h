// This file contains some common definitions for DBOS.
#ifndef DBOS_DEFS_H
#define DBOS_DEFS_H

#include <cstdint>

// Used for task_id, worker_id in DBOS.
// TODO: decide whether to use INT or STRING.
typedef int32_t DbosId;

// Used for status: true = succeeded, false = failed.
typedef bool DbosStatus;


#endif  // #ifndef DBOS_DEFS_H
