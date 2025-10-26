//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// insert_executor.cpp
//
// Identification: src/execution/insert_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <memory>

#include "execution/executors/insert_executor.h"

namespace bustub {

InsertExecutor::InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), child_executor_(std::move(child_executor)) {}

// look up information about the table being inserted into.TODO
void InsertExecutor::Init() {
  if (child_executor_ != nullptr) {
    child_executor_->Init();
  }
  auto catelog = exec_ctx_->GetCatalog();
  table_info_ = catelog->GetTable(plan_->GetTableOid());
  table_indexes_ = catelog->GetTableIndexes(catelog->GetTable(plan_->GetTableOid())->name_);
}

// 插入到表后面，更新相关index
// 返回 tuple of integer , 表示插入的行数
auto InsertExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) -> bool {
  int count = 0;
  while (true) {
    // 获取子节点tuple
    Tuple child_tuple{};
    RID child_rid;
    const auto status = child_executor_->Next(&child_tuple, &child_rid);
    if (!status) {
      break;
    }
    // 插入表
    if (table_info_->table_->InsertTuple(TupleMeta{0, false}, child_tuple)) {
      // 更新相关索引
      // InsertExecutor 不需要额外判断“哪些索引被影响”，因为所有索引都需要被更新
      for (const auto &index_info : table_indexes_) {
        auto key_attrs = index_info->index_->GetMetadata()->GetKeyAttrs();    // 哪些列是索引的列
        auto key_schema = index_info->index_->GetMetadata()->GetKeySchema();  // 索引键的schema
        //从child_tuple中的各种值中，提取索引列，构造索引键
        Tuple key = child_tuple.KeyFromTuple(table_info_->schema_, *key_schema, key_attrs);
        index_info->index_->InsertEntry(key, child_rid, exec_ctx_->GetTransaction());
      }
      count++;
    }
  }
  // 初始化返回tuple
  std::vector<Value> values{};
  values.emplace_back(TypeId::INTEGER, count);
  *tuple = Tuple{values, &GetOutputSchema()};
  return true;
}
}  // namespace bustub
