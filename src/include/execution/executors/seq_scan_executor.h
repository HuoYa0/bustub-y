#pragma once
#include <memory>
#include <vector>
#include "catalog/catalog.h"
#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/plans/seq_scan_plan.h"
#include "storage/table/table_iterator.h"
#include "storage/table/tuple.h"

namespace bustub {

class SeqScanExecutor : public AbstractExecutor {
 public:
  SeqScanExecutor(ExecutorContext *exec_ctx, const SeqScanPlanNode *plan);

  void Init() override;

  auto Next(Tuple *tuple, RID *rid) -> bool override;

  auto GetOutputSchema() const -> const Schema & override { return plan_->OutputSchema(); }

 private:
  const SeqScanPlanNode *plan_;
  std::unique_ptr<TableIterator> iter_;
};
}  // namespace bustub
