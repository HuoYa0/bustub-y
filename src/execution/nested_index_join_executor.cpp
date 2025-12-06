//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// nested_index_join_executor.cpp
//
// Identification: src/execution/nested_index_join_executor.cpp
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/nested_index_join_executor.h"
#include <cstddef>
#include <vector>
#include "binder/table_ref/bound_join_ref.h"
#include "catalog/catalog.h"
#include "type/value.h"
#include "type/value_factory.h"

namespace bustub {

NestIndexJoinExecutor::NestIndexJoinExecutor(ExecutorContext *exec_ctx, const NestedIndexJoinPlanNode *plan,
                                             std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void NestIndexJoinExecutor::Init() {
  child_executor_->Init();
  auto catalog = exec_ctx_->GetCatalog();
  right_index_info_ = catalog->GetIndex(plan_->index_oid_);
  right_table_info_ = catalog->GetTable(plan_->GetInnerTableOid());
}

auto NestIndexJoinExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  // 优化器识别出右表用来查询的列上有索引，这意味着对于左侧的每个元组，系统可以使用键去查右表索引来生成连接结果。
  Tuple left_tuple;
  RID left_rid;
  while (child_executor_->Next(&left_tuple, &left_rid)) {
    // 具体用哪个列在plan_->key_predicate_中
    std::vector<Value> left_key_values;
    left_key_values.emplace_back(plan_->key_predicate_->Evaluate(&left_tuple, plan_->InnerTableSchema()));
    Tuple left_join_key(left_key_values, right_index_info_->index_->GetKeySchema());
    auto &index = right_index_info_->index_;
    // 查index
    std::vector<RID> right_rids;
    index->ScanKey(left_join_key, &right_rids, exec_ctx_->GetTransaction());

    if (right_rids.empty() && plan_->join_type_ == JoinType::LEFT) {
      std::vector<Value> values;
      auto &left_schema = child_executor_->GetOutputSchema();
      auto &right_schema = plan_->InnerTableSchema();
      for (size_t i = 0; i < left_schema.GetColumnCount(); i++) {
        values.emplace_back(left_tuple.GetValue(&left_schema, i));
      }
      for (size_t i = 0; i < right_schema.GetColumnCount(); i++) {
        values.emplace_back(ValueFactory::GetNullValueByType(right_schema.GetColumn(i).GetType()));
      }
      *tuple = Tuple(values, &GetOutputSchema());
      return true;
    }
    if (!right_rids.empty()) {
      auto [meta, right_tuple] = right_table_info_->table_->GetTuple(right_rids[0]);
      if (meta.is_deleted_) {
        continue;
      }
      std::vector<Value> values;
      for (size_t i = 0; i < child_executor_->GetOutputSchema().GetColumnCount(); i++) {
        values.emplace_back(left_tuple.GetValue(&child_executor_->GetOutputSchema(), i));
      }
      for (size_t i = 0; i < plan_->InnerTableSchema().GetColumnCount(); i++) {
        values.emplace_back(right_tuple.GetValue(&plan_->InnerTableSchema(), i));
      }
      *tuple = Tuple(values, &GetOutputSchema());
      return true;
    }
  }
  return false;
}

}  // namespace bustub