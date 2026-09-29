
#include "Executioner.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace query
{
    namespace
    {
        constexpr std::size_t NOT_FOUND = static_cast<std::size_t>(-1);

        // Cap for speculative reserve() so huge products never hit bad_alloc.
        constexpr std::size_t MAX_RESERVE = 4'000'000;

        // "Emp.DID" -> {Emp, DID, true}     "DID" -> {, DID, false}
        struct AttributeParts
        {
            std::string_view qualifier;
            std::string_view name;
            bool hasQualifier;
        };

        AttributeParts splitAttribute(const std::string& stored)
        {
            const std::size_t dot = stored.find_last_of('.');
            if (dot == std::string::npos)
            {
                return { std::string_view{}, std::string_view(stored), false };
            }
            return {
                std::string_view(stored).substr(0, dot),
                std::string_view(stored).substr(dot + 1),
                true
            };
        }

        // Integer text compare without stoll: no overflow, no exceptions,
        // no allocation. Input is a valid integer (semantics guarantees it).
        struct IntegerText
        {
            bool negative;
            std::string_view digits; // no sign, no redundant leading zeros
        };

        IntegerText splitInteger(const std::string& text)
        {
            std::size_t i = 0;
            bool negative = false;

            if (text[0] == '-' || text[0] == '+')
            {
                negative = (text[0] == '-');
                i = 1;
            }
            while (i + 1 < text.size() && text[i] == '0')
            {
                ++i;
            }

            std::string_view digits = std::string_view(text).substr(i);
            if (digits == "0")
            {
                negative = false; // "-0" == "0"
            }
            return { negative, digits };
        }

        // Returns -1 / 0 / 1
        int compareIntegers(const std::string& a, const std::string& b)
        {
            const IntegerText x = splitInteger(a);
            const IntegerText y = splitInteger(b);

            if (x.negative != y.negative)
            {
                return x.negative ? -1 : 1;
            }

            int magnitude;
            if (x.digits.size() != y.digits.size())
            {
                magnitude = x.digits.size() < y.digits.size() ? -1 : 1;
            }
            else
            {
                const int c = x.digits.compare(y.digits);
                magnitude = (c < 0) ? -1 : (c > 0 ? 1 : 0);
            }

            return x.negative ? -magnitude : magnitude;
        }

        bool applyOperator(int c, ComparisonOperator operation)
        {
            switch (operation)
            {
            case ComparisonOperator::Equal:        return c == 0;
            case ComparisonOperator::NotEqual:     return c != 0;
            case ComparisonOperator::Less:         return c < 0;
            case ComparisonOperator::LessEqual:    return c <= 0;
            case ComparisonOperator::Greater:      return c > 0;
            case ComparisonOperator::GreaterEqual: return c >= 0;
            }
            return false;
        }

        // Tuple-equality for set semantics.
        // Numbers compare numerically (007 == 7); bare and quoted strings
        // are the same kind of value, so Bob == 'Bob' (compare text only).
        bool valuesEqual(const DataValue& a, const DataValue& b)
        {
            if (a.kind == dbValueKind::Integer)
            {
                return compareIntegers(a.text, b.text) == 0;
            }
            return a.text == b.text;
        }

        DataValue makeLiteral(const LiteralOperandNode* literalNode)
        {
            DataValue value;
            value.text = literalNode->text;

            switch (literalNode->valueKind)
            {
            case ValueKind::Integer:
                value.kind = dbValueKind::Integer;
                break;
            case ValueKind::QuotedString:
                value.kind = dbValueKind::QuotedString;
                break;
            case ValueKind::BareString:
                value.kind = dbValueKind::BareString;
                break;
            }
            return value;
        }
    } // anonymous namespace


    // Constructor: Binds the database reference using a member initializer list
    // create one Executioner and reuse it effortlessly across all queries kept in mind future use cases
    Executioner::Executioner(const Database& database)
        : database(database), joinComparisonCount(0), selectTupleCount(0)
    {
    }


    // Resets all benchmark instrumentation counters to zero
    void Executioner::resetCounters()
    {
        joinComparisonCount = 0;
        selectTupleCount = 0;
    }

    std::size_t Executioner::getJoinComparisonCount() const
    {
        return joinComparisonCount;
    }

    std::size_t Executioner::getSelectTupleCount() const
    {
        return selectTupleCount;
    }

    // Main execution entry point
    Table Executioner::execute(const QueryNode& query)
    {
        resetCounters();

        // .get passes the root node of the expression tree but keeps the ownership of the pointer
        return executeExpression(query.expression.get());
    }

    // Every column of a base relation is stored fully qualified ("R.b") so that
    // after times/join the columns R.b and S.b can always be told apart.
    // Already-qualified columns are left untouched.
    Table Executioner::executeRelationReference(const RelationReferenceNode* node)
    {
        Table table = *database.getTable(node->name);

        for (auto& attribute : table.attributes)
        {
            if (attribute.find('.') == std::string::npos)
            {
                attribute = table.name + "." + attribute;
            }
        }

        return table;
    }

    Table Executioner::executeExpression(const ExpressionNode* node)
    {
        switch (node->kind)
        {
        case NodeKind::RelationReference:
            return executeRelationReference(
                static_cast<const RelationReferenceNode*>(node)
            );

        case NodeKind::Times:
        case NodeKind::Union:
        case NodeKind::Intersect:
        case NodeKind::Minus:
            return executeBinaryExpression(
                static_cast<const BinaryExpressionNode*>(node)
            );

        case NodeKind::Join:
            return executeJoin(
                static_cast<const JoinNode*>(node)
            );

        case NodeKind::Select:
            return executeSelect(
                static_cast<const SelectNode*>(node)
            );

        case NodeKind::Project:
            return executeProject(
                static_cast<const ProjectNode*>(node)
            );

        case NodeKind::Rename:
            return executeRename(
                static_cast<const RenameNode*>(node)
            );

        default:
            return Table{};
        }
    }


    Table Executioner::executeBinaryExpression(const BinaryExpressionNode* node)
    {
        // Evaluate left and right sub-trees bottom-up first
        Table leftResult = executeExpression(node->left.get());
        Table rightResult = executeExpression(node->right.get());

        switch (node->kind)
        {
        case NodeKind::Times:
            return executeTimes(leftResult, rightResult);

        case NodeKind::Union:
            return executeUnion(leftResult, rightResult);

        case NodeKind::Minus:
            return executeMinus(leftResult, rightResult);

        case NodeKind::Intersect:
            return executeIntersect(leftResult, rightResult);

        default:
            return Table{};
        }
    }


    Table Executioner::executeUnion(const Table& left, const Table& right)
    {
        Table resultTable;
        resultTable.name = left.name;
        resultTable.attributes = left.attributes;

        // Invariant: every table reaching this point is already a set
        // (base tables are deduplicated by the semantic layer; project and
        // union deduplicate their own output; select/times/join/rename cannot
        // create duplicates). So left is copied as is, and a right tuple only
        // needs checking against left, because right has no duplicates itself.
        resultTable.rows = left.rows;

        for (const auto& tuple : right.rows)
        {
            if (!containsTuple(left, tuple))
            {
                resultTable.rows.push_back(tuple);
            }
        }

        return resultTable;
    }


    Table Executioner::executeIntersect(const Table& left, const Table& right)
    {
        Table resultTable;
        resultTable.name = left.name;
        resultTable.attributes = left.attributes;

        for (const auto& leftTuple : left.rows)
        {
            // Left is a set, so the output cannot contain duplicates
            if (containsTuple(right, leftTuple))
            {
                resultTable.rows.push_back(leftTuple);
            }
        }

        return resultTable;
    }

    Table Executioner::executeTimes(const Table& left, const Table& right)
    {
        Table resultTable = combineTables(left, right);

        // Only pre-allocate when the product is reasonable (avoids bad_alloc).
        const std::size_t leftCount = left.rows.size();
        const std::size_t rightCount = right.rows.size();
        if (leftCount > 0 && rightCount <= MAX_RESERVE / leftCount)
        {
            resultTable.rows.reserve(leftCount * rightCount);
        }

        // Cartesian Product: Pair every left row with every right row
        for (const auto& leftTuple : left.rows)
        {
            for (const auto& rightTuple : right.rows)
            {
                resultTable.rows.push_back(combineTuples(leftTuple, rightTuple));
            }
        }

        return resultTable;
    }


    Table Executioner::executeMinus(const Table& left, const Table& right)
    {
        Table resultTable;
        resultTable.name = left.name;
        resultTable.attributes = left.attributes;

        for (const auto& leftTuple : left.rows)
        {
            // Set difference A - B. Left is a set, so no duplicates can appear.
            if (!containsTuple(right, leftTuple))
            {
                resultTable.rows.push_back(leftTuple);
            }
        }

        return resultTable;
    }


    Table Executioner::executeJoin(const JoinNode* node)
    {
        Table leftTable = executeExpression(node->left.get());
        Table rightTable = executeExpression(node->right.get());

        Table result = combineTables(leftTable, rightTable);

        // Nested-loop join
        for (const Tuple& leftTuple : leftTable.rows)
        {
            for (const Tuple& rightTuple : rightTable.rows)
            {
                // Instrumentation: count every pair whose condition is evaluated
                ++joinComparisonCount;

                // Evaluate BEFORE allocating a combined tuple
                if (evaluateJoinCondition(
                        node->condition.get(),
                        leftTable, rightTable,
                        leftTuple, rightTuple))
                {
                    result.rows.push_back(combineTuples(leftTuple, rightTuple));
                }
            }
        }

        return result;
    }


    Table Executioner::executeSelect(const SelectNode* node)
    {
        Table inputTable = executeExpression(node->expression.get());

        Table outputTable;
        outputTable.name = inputTable.name;
        outputTable.attributes = inputTable.attributes;

        for (const auto& tuple : inputTable.rows)
        {
            // Instrumentation: count every tuple examined by selection
            ++selectTupleCount;

            if (evaluateCondition(node->condition.get(), inputTable, tuple))
            {
                outputTable.rows.push_back(tuple);
            }
        }

        return outputTable;
    }


    Table Executioner::executeProject(const ProjectNode* node)
    {
        Table inputTable = executeExpression(node->expression.get());

        Table outputTable;
        outputTable.name = inputTable.name;

        // 1. Output schema: the projected attributes in the listed order
        for (const auto& attribute : node->attributes)
        {
            outputTable.attributes.push_back(
                inputTable.attributes[findColumn(attribute.get(), inputTable)]
            );
        }

        // 2. Project each row and enforce set semantics (deduplication)
        for (const auto& tuple : inputTable.rows)
        {
            Tuple projectedTuple = projectTuple(tuple, inputTable, node);

            if (!containsTuple(outputTable, projectedTuple))
            {
                outputTable.rows.push_back(std::move(projectedTuple));
            }
        }

        return outputTable;
    }


    Table Executioner::executeRename(const RenameNode* node)
    {
        Table inputTable = executeExpression(node->expression.get());

        Table outputTable;
        outputTable.name = node->relationName;

        // Re-qualify each attribute with the new relation name (e.g. "Emp.EID" -> "E.EID")
        for (const auto& attribute : inputTable.attributes)
        {
            const std::size_t dotPos = attribute.find_last_of('.');
            const std::string bare =
                (dotPos == std::string::npos) ? attribute : attribute.substr(dotPos + 1);

            outputTable.attributes.push_back(node->relationName + "." + bare);
        }

        // Tuples remain unchanged during rename (schema change only)
        outputTable.rows = inputTable.rows;

        return outputTable;
    }


    // Strict column resolution.
    //  - Qualified reference (S.b): the column's OWN qualifier must equal "S"
    //    and its name must equal "b". A bare-name match is never accepted.
    //  - Unqualified reference (b): matches by attribute name.
    // Returns NOT_FOUND if this table doesn't own the column (used by the
    // join operand lookup to fall through from the left schema to the right).
    std::size_t Executioner::findColumn(
        const AttributeReferenceNode* reference,
        const Table& table
    ) const
    {
        const bool qualified = reference->relationQualifier.has_value();

        for (std::size_t i = 0; i < table.attributes.size(); ++i)
        {
            const AttributeParts parts = splitAttribute(table.attributes[i]);

            if (parts.name != std::string_view(reference->name))
            {
                continue;
            }

            if (!qualified)
            {
                return i;
            }

            if (parts.hasQualifier &&
                parts.qualifier == std::string_view(reference->relationQualifier.value()))
            {
                return i;
            }
        }

        return NOT_FOUND;
    }


    bool Executioner::containsTuple(const Table& table, const Tuple& tuple) const
    {
        for (const auto& existingRow : table.rows)
        {
            bool match = true;
            for (std::size_t i = 0; i < tuple.values.size(); ++i)
            {
                if (!valuesEqual(existingRow.values[i], tuple.values[i]))
                {
                    match = false;
                    break;
                }
            }

            if (match)
            {
                return true;
            }
        }
        return false;
    }

    Table Executioner::combineTables(const Table& left, const Table& right) const
    {
        Table result;
        result.name = left.name + "_times_" + right.name;
        result.attributes.reserve(left.attributes.size() + right.attributes.size());

        for (const auto& attr : left.attributes)
        {
            result.attributes.push_back(attr);
        }
        for (const auto& attr : right.attributes)
        {
            result.attributes.push_back(attr);
        }

        return result;
    }

    Tuple Executioner::combineTuples(const Tuple& left, const Tuple& right) const
    {
        Tuple combinedRow;
        combinedRow.values.reserve(left.values.size() + right.values.size());

        for (const auto& val : left.values)
        {
            combinedRow.values.push_back(val);
        }
        for (const auto& val : right.values)
        {
            combinedRow.values.push_back(val);
        }

        return combinedRow;
    }

    Tuple Executioner::projectTuple(
        const Tuple& tuple,
        const Table& table,
        const ProjectNode* node
    ) const
    {
        Tuple projected;
        projected.values.reserve(node->attributes.size());

        for (const auto& attrRef : node->attributes)
        {
            projected.values.push_back(tuple.values[findColumn(attrRef.get(), table)]);
        }

        return projected;
    }


    bool Executioner::evaluateCondition(
        const ConditionNode* condition,
        const Table& table,
        const Tuple& tuple
    )
    {
        switch (condition->kind)
        {
        case NodeKind::Not:
            {
                auto notNode = static_cast<const NotConditionNode*>(condition);
                return !evaluateCondition(notNode->operand.get(), table, tuple);
            }

        case NodeKind::And:
        case NodeKind::Or:
            {
                auto logicalNode = static_cast<const LogicalConditionNode*>(condition);

                const bool leftResult =
                    evaluateCondition(logicalNode->left.get(), table, tuple);

                // Short-circuit: false AND x == false, true OR x == true
                if (logicalNode->op == LogicalOperator::And)
                {
                    return leftResult && evaluateCondition(logicalNode->right.get(), table, tuple);
                }
                return leftResult || evaluateCondition(logicalNode->right.get(), table, tuple);
            }

        case NodeKind::Comparison:
            {
                auto compNode = static_cast<const ComparisonNode*>(condition);

                DataValue leftVal = evaluateOperand(compNode->left.get(), table, tuple);
                DataValue rightVal = evaluateOperand(compNode->right.get(), table, tuple);

                return compareValues(leftVal, rightVal, compNode->op);
            }

        default:
            return false;
        }
    }


    DataValue Executioner::evaluateOperand(
        const OperandNode* operand,
        const Table& table,
        const Tuple& tuple
    )
    {
        if (operand->kind == NodeKind::LiteralOperand)
        {
            return makeLiteral(static_cast<const LiteralOperandNode*>(operand));
        }

        auto attrNode = static_cast<const AttributeOperandNode*>(operand);
        return tuple.values[findColumn(attrNode->reference.get(), table)];
    }


    bool Executioner::compareValues(
        const DataValue& left,
        const DataValue& right,
        ComparisonOperator operation
    ) const
    {
        // Numeric comparison for integers (avoids lexicographical errors like "10" < "2")
        if (left.kind == dbValueKind::Integer)
        {
            return applyOperator(compareIntegers(left.text, right.text), operation);
        }

        // String comparison (lexicographical on raw text)
        const int raw = left.text.compare(right.text);
        return applyOperator((raw < 0) ? -1 : (raw > 0 ? 1 : 0), operation);
    }


    bool Executioner::evaluateJoinCondition(
        const ConditionNode* condition,
        const Table& leftTable,
        const Table& rightTable,
        const Tuple& leftTuple,
        const Tuple& rightTuple
    )
    {
        switch (condition->kind)
        {
        case NodeKind::Not:
            {
                auto notNode = static_cast<const NotConditionNode*>(condition);
                return !evaluateJoinCondition(
                    notNode->operand.get(), leftTable, rightTable, leftTuple, rightTuple);
            }

        case NodeKind::And:
        case NodeKind::Or:
            {
                auto logicalNode = static_cast<const LogicalConditionNode*>(condition);

                const bool leftResult = evaluateJoinCondition(
                    logicalNode->left.get(), leftTable, rightTable, leftTuple, rightTuple);

                if (logicalNode->op == LogicalOperator::And)
                {
                    return leftResult && evaluateJoinCondition(
                        logicalNode->right.get(), leftTable, rightTable, leftTuple, rightTuple);
                }
                return leftResult || evaluateJoinCondition(
                    logicalNode->right.get(), leftTable, rightTable, leftTuple, rightTuple);
            }

        case NodeKind::Comparison:
            {
                auto compNode = static_cast<const ComparisonNode*>(condition);

                DataValue leftVal = evaluateJoinOperand(
                    compNode->left.get(), leftTable, rightTable, leftTuple, rightTuple);
                DataValue rightVal = evaluateJoinOperand(
                    compNode->right.get(), leftTable, rightTable, leftTuple, rightTuple);

                return compareValues(leftVal, rightVal, compNode->op);
            }

        default:
            return false;
        }
    }


    // The side is found by searching each side's schema with the strict
    // findColumn (which checks every column's own qualifier), NOT by comparing
    // the qualifier to leftTable.name. After a join the left table is called
    // "R_times_S", so name-based lookup breaks nested joins.
    DataValue Executioner::evaluateJoinOperand(
        const OperandNode* operand,
        const Table& leftTable,
        const Table& rightTable,
        const Tuple& leftTuple,
        const Tuple& rightTuple
    )
    {
        if (operand->kind == NodeKind::LiteralOperand)
        {
            return makeLiteral(static_cast<const LiteralOperandNode*>(operand));
        }

        auto attrNode = static_cast<const AttributeOperandNode*>(operand);
        const AttributeReferenceNode* reference = attrNode->reference.get();

        const std::size_t leftColIndex = findColumn(reference, leftTable);
        if (leftColIndex != NOT_FOUND)
        {
            return leftTuple.values[leftColIndex];
        }

        return rightTuple.values[findColumn(reference, rightTable)];
    }

} // namespace query