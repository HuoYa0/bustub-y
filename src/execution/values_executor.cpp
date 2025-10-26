#include "execution/executors/values_executor.h"

namespace bustub {

ValuesExecutor::ValuesExecutor(ExecutorContext *exec_ctx, const ValuesPlanNode *plan)
    : AbstractExecutor(exec_ctx), plan_(plan), dummy_schema_(Schema({})) {}

void ValuesExecutor::Init() { cursor_ = 0; }

// VALUES (1, 2), (3, 4);
// 第一次 Next()	cursor=0 → Evaluate(1,2) → 生成 tuple (1,2)
// 第二次 Next()	cursor=1 → Evaluate(3,4) → 生成 tuple (3,4)
// 第三次 Next()	cursor=2 ≥ size → return false
auto ValuesExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  const auto &expressions = plan_->GetValues();
  if (cursor_ >= expressions.size()) {
    return false;
  }
  // ValuePlanNode的expressions是二维数组，最小单元通常是constantExpression，直接返回他的值，所以是col->Evaluate(nullptr,
  // dummy_schema_));
  std::vector<Value> values{};
  values.reserve(GetOutputSchema().GetColumnCount());

  const auto &row_expr = expressions[cursor_];
  for (const auto &col : row_expr) {
    values.push_back(col->Evaluate(nullptr, dummy_schema_));
  }
  *tuple = Tuple{values, &GetOutputSchema()};
  cursor_ += 1;
  return true;
}

}  // namespace bustub
