//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// seq_scan_executor.cpp
//
// Identification: src/execution/seq_scan_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/seq_scan_executor.h"
#include <memory>
#include "catalog/catalog.h"
#include "common/macros.h"
#include "concurrency/transaction.h"
#include "storage/table/table_iterator.h"
#include "type/type_id.h"

namespace bustub {

SeqScanExecutor::SeqScanExecutor(ExecutorContext *exec_ctx, const SeqScanPlanNode *plan)
    : AbstractExecutor(exec_ctx), plan_(plan) {}

// 从 exec_ctx 获取 Catalog、找到 TableInfo，初始化表迭代器（table_heap_->Begin()）
void SeqScanExecutor::Init() {
  auto catelog = exec_ctx_->GetCatalog();
  auto table_info = catelog->GetTable(plan_->GetTableOid());
  auto table_heap = table_info->table_.get();
  iter_ = std::make_unique<TableIterator>(table_heap->MakeIterator());
}

auto SeqScanExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  // 在向量中预先分配内存空间，以容纳输出模式中列的数量
  while (!iter_->IsEnd()) {
    auto [current_meta, current_tuple] = iter_->GetTuple();
    if (current_meta.is_deleted_) {
      ++(*iter_);
      continue;
    }
    // 如果有过滤条件，进行过滤
    if (plan_->filter_predicate_ != nullptr) {
      auto eval_result = plan_->filter_predicate_->Evaluate(&current_tuple, plan_->OutputSchema());
      if (!eval_result.IsNull() && eval_result.GetAs<bool>()) {
        ++(*iter_);
        *tuple = std::move(current_tuple);
        *rid = iter_->GetRID();
        return true;
      }
      ++(*iter_);
      continue;
    }
  }
  return false;
}

}  // namespace bustub
