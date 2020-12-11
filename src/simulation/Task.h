#ifndef DBOS_SCHEDULER_TASK_H
#define DBOS_SCHEDULER_TASK_H

#include <grpcpp/grpcpp.h>

#include "frontend.grpc.pb.h"

struct Task {
  int targetData;
  int execTime;
};

static dbos_scheduler::SubmitTaskRequest taskToProtobuf(Task* task) {
  dbos_scheduler::SubmitTaskRequest st_request;
  st_request.set_targetdata(task->targetData);
  st_request.set_exectime(task->execTime);
  return st_request;
}

static Task protobufToTask(const dbos_scheduler::SubmitTaskRequest* request) {
  Task task;
  task.targetData = request->targetdata();
  task.execTime = request->exectime();
  return task;
}

#endif  // DBOS_SCHEDULER_TASK_H
