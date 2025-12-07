//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hash_join_executor.cpp
//
// Identification: src/execution/hash_join_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/hash_join_executor.h"
#include <asm-generic/errno.h>
#include <cstddef>
#include <utility>
#include <vector>
#include "binder/table_ref/bound_join_ref.h"
#include "common/rid.h"
#include "execution/plans/hash_join_plan.h"
#include "type/value.h"
#include "type/value_factory.h"

namespace bustub {

HashJoinExecutor::HashJoinExecutor(ExecutorContext *exec_ctx, const HashJoinPlanNode *plan,
                                   std::unique_ptr<AbstractExecutor> &&left_child,
                                   std::unique_ptr<AbstractExecutor> &&right_child)
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      left_child_(std::move(left_child)),
      right_child_(std::move(right_child)) {}

void HashJoinExecutor::Init() {
  left_child_->Init();
  if (!map_.empty()) {
    iter_ = map_.cbegin();
    return;
  }
  right_child_->Init();
  Tuple right_tup;
  RID rid;
  // 考虑到左连接，所以用右表作为hashtable
  while (right_child_->Next(&right_tup, &rid)) {
    JoinKey join_key;
    for (auto &expr : plan_->RightJoinKeyExpressions()) {
      join_key.join_keys_.emplace_back(expr->Evaluate(&right_tup, right_child_->GetOutputSchema()));
    }
    if (map_.count(join_key) == 0) {
      // 新map key，插入map
      map_.emplace(std::move(join_key), std::vector<JoinValue>{MakeJoinValue(right_tup, right_child_->GetOutputSchema())});
    } else {
      // 对应key的value数组中插入新元素
      map_[join_key].emplace_back(MakeJoinValue(right_tup, right_child_->GetOutputSchema()));
    }
  }
  iter_ = map_.cbegin();
}

auto HashJoinExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  RID left_rid;

  // 处理一个左表对应多个右表的情况
  if (value_iter_valid_ && value_idx_ < right_join_values_->size()) {
    const auto &jvs = (*right_join_values_)[value_idx_].values_;
    std::vector<Value> values;
    for (size_t i = 0; i < left_child_->GetOutputSchema().GetColumnCount(); i++) {
      values.emplace_back(left_tuple_.GetValue(&left_child_->GetOutputSchema(), i));
    }
    values.insert(values.end(), jvs.begin(), jvs.end());
    *tuple = Tuple(values, &GetOutputSchema());
    value_idx_++;
    return true;
  }

  while (left_child_->Next(&left_tuple_, &left_rid)) {
    JoinKey left_join_key;
    for (auto &expr : plan_->LeftJoinKeyExpressions()) {
      left_join_key.join_keys_.emplace_back(expr->Evaluate(&left_tuple_, left_child_->GetOutputSchema()));
    }
    auto it = map_.find(left_join_key);
    if (it != map_.end()) {
      auto &join_values = it->second;
      auto &jv = join_values[0];
      std::vector<Value> values;
      // jv是hash表中对应的value的第一个，先处理它
      // 左表值拼jv
      for (size_t i = 0; i < left_child_->GetOutputSchema().GetColumnCount(); i++) {
        values.emplace_back(left_tuple_.GetValue(&left_child_->GetOutputSchema(), i));
      }
      values.insert(values.end(), jv.values_.begin(), jv.values_.end());
      *tuple = Tuple(values, &GetOutputSchema());

      if (join_values.size() > 1) {
        // 为调用下一个Next用下一个value元素做准备
        value_idx_ = 1;
        right_join_values_ = &it->second;
        value_iter_valid_ = true;
      }
      return true;
    }
    // 处理左表值在hash中没查到的情况
    if (plan_->GetJoinType() == JoinType::LEFT) {
      std::vector<Value> values;
      for (size_t i = 0; i < left_child_->GetOutputSchema().GetColumnCount(); i++) {
        values.emplace_back(left_tuple_.GetValue(&left_child_->GetOutputSchema(), i));
      }
      for (auto &col : right_child_->GetOutputSchema().GetColumns()) {
        values.emplace_back(ValueFactory::GetNullValueByType(col.GetType()));
      }
      *tuple = Tuple(values, &GetOutputSchema());
      return true;
    }
  }
  return false;
}

}  // namespace bustub