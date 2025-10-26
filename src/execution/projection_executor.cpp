#include "execution/executors/projection_executor.h"
#include "storage/table/tuple.h"

namespace bustub {

ProjectionExecutor::ProjectionExecutor(ExecutorContext *exec_ctx, const ProjectionPlanNode *plan,
                                       std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

void ProjectionExecutor::Init() {
  child_executor_->Init();
}

auto ProjectionExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  Tuple child_tuple{};
  const auto status = child_executor_->Next(&child_tuple, rid);
  if (!status) {
    return false;
  }

  // 在向量中预先分配内存空间，以容纳输出模式中列的数量
  std::vector<Value> values{};
  values.reserve(GetOutputSchema().GetColumnCount());
  // Expression是每个表达式代表要输出的一列，如1，colA，colB*2
  // Evaluate: 从给定的 Tuple（当前输入行）和输入表的 Schema 中，计算出当前表达式的值。
  for (const auto &expr : plan_->GetExpressions()) {
    values.push_back(expr->Evaluate(&child_tuple, child_executor_->GetOutputSchema()));
  }
  //使用计算出的值和输出模式构造一个新的 Tuple 对象
  *tuple = Tuple{values, &GetOutputSchema()};
  return true;
}
}  // namespace bustub
