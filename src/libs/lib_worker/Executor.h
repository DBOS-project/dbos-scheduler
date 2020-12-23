#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "DbosDefs.h"

class Executor {
public:
  // Task state.
  Executor(){};

  // TODO: Use Task as an argument here.
  virtual DbosStatus executeTask() = 0;

  // Virtual destructor so that derived classes can be freed.
  virtual ~Executor() = 0;
};

#endif  // #ifndef EXECUTOR_H
