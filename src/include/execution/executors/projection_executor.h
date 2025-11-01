//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// projection_executor.h
//
// Identification: src/include/execution/executors/projection_executor.h
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <vector>

#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/plans/projection_plan.h"
#include "execution/plans/seq_scan_plan.h"
#include "storage/table/tuple.h"

namespace bustub {

class ProjectionExecutor : public AbstractExecutor {
 public:
  /**
   * Construct a new ProjectionExecutor instance.
   * @param exec_ctx The executor context
   * @param plan The projection plan to be executed
   */
  ProjectionExecutor(ExecutorContext *exec_ctx, const ProjectionPlanNode *plan,
                     std::unique_ptr<AbstractExecutor> &&child_executor);
  // &&表示右值引用，调用函数时资源会move进来

  void Init() override;


  auto Next(Tuple *tuple, RID *rid) -> bool override;

  auto GetOutputSchema() const -> const Schema & override { return plan_->OutputSchema(); }

 private:
  const ProjectionPlanNode *plan_;

  std::unique_ptr<AbstractExecutor> child_executor_;
};
}  // namespace bustub
