#include <algorithm>
#include <memory>
#include <vector>
#include "catalog/column.h"
#include "catalog/schema.h"
#include "common/exception.h"
#include "common/macros.h"
#include "execution/expressions/abstract_expression.h"
#include "execution/expressions/column_value_expression.h"
#include "execution/expressions/comparison_expression.h"
#include "execution/expressions/constant_value_expression.h"
#include "execution/expressions/logic_expression.h"
#include "execution/plans/abstract_plan.h"
#include "execution/plans/filter_plan.h"
#include "execution/plans/hash_join_plan.h"
#include "execution/plans/nested_loop_join_plan.h"
#include "execution/plans/projection_plan.h"
#include "optimizer/optimizer.h"
#include "type/type_id.h"

namespace bustub {

auto IsHashExpression(const AbstractExpressionRef &expr, std::vector<AbstractExpressionRef> &l_exprs,
                      std::vector<AbstractExpressionRef> &r_exprs) -> bool {
  if (expr == nullptr) {
    return false;
  }
  // AND表达式，需要一直往下
  if (auto lg = dynamic_cast<const LogicExpression *>(expr.get())) {
    if (lg->logic_type_ == LogicType::And) {
      return IsHashExpression(lg->GetChildAt(0), l_exprs, r_exprs) &&
             IsHashExpression(lg->GetChildAt(1), l_exprs, r_exprs);
    }
    return false;
  }

  // 最基础的等值表达式
  if (auto cmp = dynamic_cast<const ComparisonExpression *>(expr.get())) {
    if (cmp->comp_type_ != ComparisonType::Equal) {
      return false;
    }
    AbstractExpressionRef l_expr = cmp->GetChildAt(0);
    AbstractExpressionRef r_expr = cmp->GetChildAt(1);

    const ColumnValueExpression *l_col = nullptr;
    const ColumnValueExpression *r_col = nullptr;
    // 判断等号两边都是列值表达式
    if ((l_col = dynamic_cast<const ColumnValueExpression *>(l_expr.get())) != nullptr &&
        (r_col = dynamic_cast<const ColumnValueExpression *>(r_expr.get())) != nullptr) {
      (void)r_col;
    } else {
      return false;
    }        

    if (l_col->GetTupleIdx() == 0) {
      l_exprs.emplace_back(l_expr);
      r_exprs.emplace_back(r_expr);
    } else {
      l_exprs.emplace_back(r_expr);
      r_exprs.emplace_back(l_expr);
    }
    return true;
  }
  return false;
}

 
auto Optimizer::OptimizeNLJAsHashJoin(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef {
  // 当连接谓词是两列之间多个等价条件的合取时，可以使用哈希连接算法。在本项目中，您应该能够处理由 AND 连接的多个等价条件。
  std::vector<AbstractPlanNodeRef> children;
  for (const auto &child : plan->GetChildren()) {
    children.emplace_back(OptimizeNLJAsHashJoin(child));
  }
  auto optimized_plan = plan->CloneWithChildren(children);

  // 只考虑NestedLoopJoin
  if (optimized_plan->GetType() == PlanType::NestedLoopJoin) {
    const auto &nlj_plan_node = dynamic_cast<const NestedLoopJoinPlanNode &>(*optimized_plan);

    //需根据predicate_得到两个表达式列表，创建HashJoinPlanNode
    std::vector<AbstractExpressionRef> l_exprs;
    std::vector<AbstractExpressionRef> r_exprs;
    if (IsHashExpression(nlj_plan_node.predicate_, l_exprs, r_exprs)) {
      return std::make_shared<HashJoinPlanNode>(nlj_plan_node.output_schema_, nlj_plan_node.GetLeftPlan(),
                                                nlj_plan_node.GetRightPlan(), l_exprs, r_exprs, nlj_plan_node.GetJoinType());
    }
  }
  return optimized_plan;
}

}  // namespace bustub