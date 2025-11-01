//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// update_executor.cpp
//
// Identification: src/execution/update_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include <memory>

#include "concurrency/transaction.h"
#include "execution/executors/update_executor.h"

namespace bustub {

UpdateExecutor::UpdateExecutor(ExecutorContext *exec_ctx, const UpdatePlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void UpdateExecutor::Init() {
  if (child_executor_ != nullptr) {
    child_executor_->Init();
  }
  auto catelog = exec_ctx_->GetCatalog();
  table_info_ = catelog->GetTable(plan_->GetTableOid());
  table_indexes_ = catelog->GetTableIndexes(catelog->GetTable(plan_->GetTableOid())->name_);
}
// 返回 tuple of integer , 表示更新的行数
// Hint: To implement an update, first delete the affected tuple and then insert a new tuple.
auto UpdateExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) -> bool {
  int count = 0;
  while (true) {
    // 获取子节点tuple
    Tuple old_child_tuple{};
    RID old_child_rid;
    const auto status = child_executor_->Next(&old_child_tuple, &old_child_rid);
    if (!status) {
      break;
    }
    count++;
    // 逻辑上删除，更新TupleMeta 的删除标记
    table_info_->table_->UpdateTupleMeta(TupleMeta{0, true}, old_child_rid);
    // 计算更新后的值，构建并插入新的tuple
    Tuple new_tuple{};
    std::vector<Value> new_values{};
    new_values.reserve(GetOutputSchema().GetColumnCount());
    for (auto &target_expr : plan_->target_expressions_) {
      new_values.push_back(target_expr->Evaluate(&old_child_tuple, table_info_->schema_));
    }
    auto new_rid_opt = table_info_->table_->InsertTuple(TupleMeta{0, false}, new_tuple);
    if (!new_rid_opt.has_value()) {
      throw Exception("UpdateExecutor: failed to insert updated tuple");
      continue;
    }
    // 删除并增加相关索引
    // InsertExecutor 不需要额外判断“哪些索引被影响”，因为所有索引都需要被更新
    for (const auto &index_info : table_indexes_) {
      auto key_attrs = index_info->index_->GetMetadata()->GetKeyAttrs();    // 哪些列是索引的列
      auto key_schema = index_info->index_->GetMetadata()->GetKeySchema();  // 索引键的schema
      //从child_tuple中的各种值中，提取索引列，构造索引键
      Tuple old_key = old_child_tuple.KeyFromTuple(table_info_->schema_, *key_schema, key_attrs);
      Tuple new_key = new_tuple.KeyFromTuple(table_info_->schema_, *key_schema, key_attrs);
      index_info->index_->DeleteEntry(old_key, old_child_rid, exec_ctx_->GetTransaction());
      index_info->index_->InsertEntry(new_key, new_rid_opt.value(), exec_ctx_->GetTransaction());
    }
  }
  // 初始化返回tuple
  std::vector<Value> values{};
  values.emplace_back(TypeId::INTEGER, count);
  *tuple = Tuple{values, &GetOutputSchema()};
  return true;
}

}  // namespace bustub
