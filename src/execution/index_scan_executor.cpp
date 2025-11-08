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
  table_info_ = catalog->GetTable(plan_->table_oid_);
  b_plus_tree_index_ = dynamic_cast<BPlusTreeIndexForTwoIntegerColumn *>(index_info->index_.get());

  if (!plan_->pred_keys_.empty()) {
    is_point_scan_ = false;
    index_iter_ = std::make_unique<BPlusTreeIndexIteratorForTwoIntegerColumn>(b_plus_tree_index_->GetBeginIterator());
  }
}


auto IndexScanExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  // 点查询 测试中不会有重复键，如果存在就返回一条，否则返回空。  
  // SELECT * FROM table WHERE indexed_col = constant
  if (is_point_scan_) {
    Tuple key_tuple;
    // 构建b+树查找用的key tuple
    std::vector<Value> key_values{};
    key_values.reserve(GetOutputSchema().GetColumnCount());
    for (const auto &this_expression : plan_->pred_keys_) {
      // 常数表达式，不需要tuple和schema
      key_values.push_back(this_expression->Evaluate(&key_tuple, GetOutputSchema()));
    }
    // 获取key schema
    const auto &key_schema = b_plus_tree_index_->GetKeySchema();
    key_tuple = Tuple(key_values, key_schema);
    // 查找索引
    std::vector<RID> result_rids;
    b_plus_tree_index_->ScanKey(key_tuple, &result_rids, exec_ctx_->GetTransaction());
    if (!result_rids.empty()) {
      *rid = result_rids[0];
      auto [meta, found_tuple] = table_info_->table_->GetTuple(*rid);
      // 忽略删除的tuple
      if (meta.is_deleted_) {
        return false;
      }
      // 根据过滤条件过滤
      if (plan_->filter_predicate_) {
        auto value = plan_->filter_predicate_->Evaluate(&found_tuple, plan_->OutputSchema());
        if (!value.GetAs<bool>()) {
          return false;
        }
      }
      *tuple = std::move(found_tuple);
      return true;
    }
  } else {
    // 用 index 的迭代器（index iterator）来遍历。
    // SELECT * FROM table ORDER  BY indexed_col
    while (!index_iter_->IsEnd()) {
      auto [key, rid_value] = **index_iter_;
      ++(*index_iter_);
      *rid = rid_value;
      auto [meta, found_tuple] = table_info_->table_->GetTuple(*rid);
      // 忽略删除的tuple
      if (meta.is_deleted_) {
        continue;
      }
      // 根据过滤条件过滤
      if (plan_->filter_predicate_) {
        auto value = plan_->filter_predicate_->Evaluate(&found_tuple, plan_->OutputSchema());
        if (!value.GetAs<bool>()) {
          continue;
        }
      }
      *tuple = std::move(found_tuple);
      return true;
    }
  }
  return false;
}

}  // namespace bustub
