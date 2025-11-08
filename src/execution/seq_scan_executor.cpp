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
  auto filter = plan_->filter_predicate_;
  while (!iter_->IsEnd()) {
    auto [tup_meta, tup] = iter_->GetTuple();
    *rid = iter_->GetRID();
    *tuple = std::move(tup);
    ++(*iter_);
    if (tup_meta.is_deleted_) {
      continue;
    }
    // 根据过滤条件过滤
    if (filter) {
      auto value = filter->Evaluate(tuple, plan_->OutputSchema());
      if (!value.GetAs<bool>()) {
        continue;
      }
    }
    return true;
  }
  return false;
}

}  // namespace bustub
