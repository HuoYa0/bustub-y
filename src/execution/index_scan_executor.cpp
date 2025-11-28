//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// index_scan_executor.cpp
//
// Identification: src/execution/index_scan_executor.cpp
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include "execution/executors/index_scan_executor.h"
#include <memory>
#include <vector>
#include "catalog/catalog.h"
#include "storage/index/b_plus_tree_index.h"

namespace bustub {
IndexScanExecutor::IndexScanExecutor(ExecutorContext *exec_ctx, const IndexScanPlanNode *plan)
    : AbstractExecutor(exec_ctx), plan_(plan) {}

void IndexScanExecutor::Init() {
  auto catalog = exec_ctx_->GetCatalog();
  auto index_info = catalog->GetIndex(plan_->index_oid_);
  const auto &index = index_info->index_;
  b_plus_tree_index_ = dynamic_cast<BPlusTreeIndexForTwoIntegerColumn *>(index.get());

  // ORDER BY 查询，需用到迭代器
  if (plan_->pred_keys_.empty()) {
    is_point_scan_ = false;
    index_iter_ = std::make_unique<BPlusTreeIndexIteratorForTwoIntegerColumn>(b_plus_tree_index_->GetBeginIterator());
  }
  // 否则是点查询
}

auto IndexScanExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  if (is_point_scan_) {
    // 点查询，pred_keys[]可能查询多个value 
    while (current_idx_ < plan_->pred_keys_.size()) {
      // 构造索引键的tuple
      const auto &val_exp = plan_->pred_keys_[current_idx_++];
      auto val = val_exp->Evaluate(nullptr, GetOutputSchema());  // 不需要tuple和schema
      std::vector<Value> key_values{val};
      Tuple key_tuple = Tuple(key_values, b_plus_tree_index_->GetKeySchema());
      // 查找索引
      std::vector<RID> result;
      b_plus_tree_index_->ScanKey(key_tuple, &result, exec_ctx_->GetTransaction());
      if (!result.empty()) {
        *rid = result[0];
        auto table_info = exec_ctx_->GetCatalog()->GetTable(plan_->table_oid_);
        auto [meta, tup] = table_info->table_->GetTuple(*rid);
        if (!meta.is_deleted_) {
          *tuple = std::move(tup);
          return true;
        }
      }
    }
    return false;
  }
  //  ORDER BY 查询
  while (!index_iter_->IsEnd()) {
    auto [key, rid_value] = **index_iter_;
    ++(*index_iter_);
    *rid = rid_value;
    auto table_info = exec_ctx_->GetCatalog()->GetTable(plan_->table_oid_);
    auto [found, tup] = table_info->table_->GetTuple(*rid);
    if (found.is_deleted_) {
      continue;
    }
    *tuple = std::move(tup);
    return true;
  }
  return false;
}

}  // namespace bustub