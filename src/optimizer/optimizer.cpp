#include "optimizer/optimizer.h"
#include <optional>
#include "common/util/string_util.h"
#include "execution/plans/abstract_plan.h"

namespace bustub {

auto Optimizer::Optimize(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef {
  if (force_starter_rule_) {
    // Use starter rules when `force_starter_rule_` is set to true.
    auto p = plan;
    p = OptimizeMergeProjection(p); // 合并多个 Projection 节点
    p = OptimizeMergeFilterNLJ(p); // 合并 Filter 和 Nested Loop Join (NLJ)
    p = OptimizeNLJAsIndexJoin(p); // 将 Nested Loop Join 转换为索引连接 (Index Join)
    p = OptimizeOrderByAsIndexScan(p); // 将 ORDER BY 转换为 Index Scan，如果有索引利用索引来排序
    p = OptimizeMergeFilterScan(p); // 合并 Filter 和 SeqScan（顺序扫描）
    p = OptimizeSeqScanAsIndexScan(p); // 将 SeqScan 转换为 IndexScan
    return p;
  }
  return OptimizeCustom(plan);
}

auto Optimizer::EstimatedCardinality(const std::string &table_name) -> std::optional<size_t> {
  if (StringUtil::EndsWith(table_name, "_1m")) {
    return std::make_optional(1000000);
  }
  if (StringUtil::EndsWith(table_name, "_100k")) {
    return std::make_optional(100000);
  }
  if (StringUtil::EndsWith(table_name, "_50k")) {
    return std::make_optional(50000);
  }
  if (StringUtil::EndsWith(table_name, "_10k")) {
    return std::make_optional(10000);
  }
  if (StringUtil::EndsWith(table_name, "_1k")) {
    return std::make_optional(1000);
  }
  if (StringUtil::EndsWith(table_name, "_100")) {
    return std::make_optional(100);
  }
  return std::nullopt;
}

}  // namespace bustub
