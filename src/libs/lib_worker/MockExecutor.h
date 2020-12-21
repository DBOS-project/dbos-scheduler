#ifndef MOCK_EXECUTOR_H
#define MOCK_EXECUTOR_H

#include "Executor.h"

class MockExecutor : public Executor {
public:
  MockExecutor() : Executor() {};

  DbosStatus executeTask();

  ~MockExecutor() {/* placeholder for now. */
  };

private:
};

#endif  // #ifndef MOCK_EXECUTOR_H
