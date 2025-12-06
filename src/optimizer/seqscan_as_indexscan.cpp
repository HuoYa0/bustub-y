//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// seqscan_as_indexscan.cpp
//
// Identification: src/optimizer/seqscan_as_indexscan.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/expressions/column_value_expression.h"
#include "execution/expressions/logic_expression.h"
#include "execution/plans/index_scan_plan.h"
#include "execution/plans/seq_scan_plan.h"
#include "optimizer/optimizer.h"

namespace bustub {

/**
 * @brief Optimizes seq scan as index scan if there's an index on a table
 */

auto Optimizer::IsNeedIndexOptimise(const SeqScanPlanNode &plan, const AbstractExpressionRef &expression,
                                    std::vector<AbstractExpressionRef> &pred_keys, index_oid_t &index_oid) -> bool {
  //  WHERE v1 = 1 or v1 = 4 or v1 = 7 or v1 =10 且全是针对一个列
  auto logic_expr = std::dynamic_pointer_cast<LogicExpression>(expression);
  if (logic_expr && logic_expr->logic_type_ == LogicType::Or) {
    for (auto &child_expression : logic_expr->GetChildren()) {
      bool sub_need = IsNeedIndexOptimise(plan, child_expression, pred_keys, index_oid);
      if (!sub_need) {
        return false;
      }
    }
    return true;
  }
  // （WHERE 1 = v1）；和 （WHERE v1 = 1）；
  auto comparison_expr = std::dynamic_pointer_cast<ComparisonExpression>(expression);
  if (comparison_expr && comparison_expr->comp_type_ == ComparisonType::Equal) {
    std::shared_ptr<ColumnValueExpression> column_expr = nullptr;
    bool is_left_column = true;
    // 拿到相关列表达式col；
    for (auto &child_expression : comparison_expr->GetChildren()) {
      auto sub_column_expr = std::dynamic_pointer_cast<ColumnValueExpression>(child_expression);
      if (sub_column_expr) {
        column_expr = sub_column_expr;
        break;
      }
      is_left_column = false;
    }
    if (!column_expr) {
      return false;
    }
    // 根据plan获得全部索引，查看col是否与它相关，相关则返回true，并设置pred_keys和index_oid；
    auto indexs = catalog_.GetTableIndexes(plan.table_name_);
    for (auto &index : indexs) {
      auto key_attrs = index->index_->GetKeyAttrs();
      if (std::find(key_attrs.begin(), key_attrs.end(), column_expr->GetColIdx()) != key_attrs.end()) {
        index_oid = index->index_oid_;
        pred_keys.push_back(comparison_expr->GetChildren()[is_left_column ? 1 : 0]);
        return true;  
      }
    }
  }
  return false;
}

auto Optimizer::OptimizeSeqScanAsIndexScan(const bustub::AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef {
  // 递归处理子节点
  std::vector<AbstractPlanNodeRef> children;
  for (const auto &child : plan->GetChildren()) {
    children.emplace_back(OptimizeSeqScanAsIndexScan(child));
  }
  auto this_plan = plan->CloneWithChildren(std::move(children));

  if (this_plan->GetType() == PlanType::SeqScan) {
    const auto &seq_scan_plan = dynamic_cast<const SeqScanPlanNode &>(*this_plan);
    //  当前优化在OptimizeMergeFilterScan之后，子节点filterNode的筛选已转移到SeqScanPlanNode里
    if (seq_scan_plan.filter_predicate_ != nullptr) {
      // 需要用filter_predicate_判断是否有相关索引，有则得到下面两个变量去创建IndexScanPlanNode
      index_oid_t relative_index_oid;
      std::vector<AbstractExpressionRef> pred_keys;
      bool use_index =
          IsNeedIndexOptimise(seq_scan_plan, seq_scan_plan.filter_predicate_, pred_keys, relative_index_oid);
      if (use_index) {
        auto index_scan_plan = std::make_shared<IndexScanPlanNode>(
            seq_scan_plan.output_schema_, seq_scan_plan.table_oid_, relative_index_oid, seq_scan_plan.filter_predicate_,
            std::move(pred_keys));
        return index_scan_plan;
      }
    }
  }
  return this_plan;
}

}  // namespace bustub