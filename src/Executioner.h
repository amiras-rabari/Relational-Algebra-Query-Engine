//
// Created by amira on 2026-09-28.
//

#pragma once

#include "AST.h"
#include "Database.h"

#include <cstddef>

namespace query
{
    class Executioner
    {
    public:
        // Creates an executor over a semantically validated database.
        explicit Executioner(const Database& database);

        // Executes the complete query AST and returns the final result table.
        Table execute(const QueryNode& query);

        // Resets execution instrumentation counters.
        void resetCounters();

        // Returns the number of tuple pairs examined by join conditions.
        std::size_t getJoinComparisonCount() const;

        // Returns the number of tuples examined by select conditions.
        std::size_t getSelectTupleCount() const;

    private:
        const Database& database;

        // ============================================================
        // AST & Operator Execution
        // ============================================================

        Table executeExpression(const ExpressionNode* node);
        Table executeSelect(const SelectNode* node);
        Table executeProject(const ProjectNode* node);
        Table executeJoin(const JoinNode* node);
        Table executeRename(const RenameNode* node);

        // Executes a relation reference.
        Table executeRelationReference(const RelationReferenceNode* node);

        // Executes UNION, MINUS, INTERSECT, or TIMES.
        Table executeBinaryExpression(const BinaryExpressionNode* node);

        // Core Relational Algebra Operators
        Table executeTimes(const Table& left, const Table& right);
        Table executeUnion(const Table& left, const Table& right);
        Table executeMinus(const Table& left, const Table& right);
        Table executeIntersect(const Table& left, const Table& right);

        // ============================================================
        // Condition & Expression Evaluation
        // ============================================================

        // Evaluates a condition node against a specific tuple.
        bool evaluateCondition(
            const ConditionNode* condition,
            const Table& table,
            const Tuple& tuple
        );

        // Evaluates an operand (attribute reference or literal) to a runtime value.
        DataValue evaluateOperand(
            const OperandNode* operand,
            const Table& table,
            const Tuple& tuple
        );

        // Compares two runtime values using a comparison operator (=, !=, <, <=, >, >=).
        bool compareValues(
            const DataValue& left,
            const DataValue& right,
            ComparisonOperator operation
        ) const;
        bool evaluateJoinCondition(const ConditionNode* condition, const Table& leftTable, const Table& rightTable,
                                   const Tuple& leftTuple, const Tuple& rightTuple);
        DataValue evaluateJoinOperand(const OperandNode* operand, const Table& leftTable, const Table& rightTable,
                                      const Tuple& leftTuple, const Tuple& rightTuple);

        // ============================================================
        // Helpers & Instrumentation
        // ============================================================

        std::size_t findColumn(const AttributeReferenceNode* reference, const Table& table) const;
        Table combineTables(const Table& left, const Table& right) const;
        Tuple combineTuples(const Tuple& left, const Tuple& right) const;
        Tuple projectTuple(const Tuple& tuple, const Table& table, const ProjectNode* node) const;
        bool containsTuple(const Table& table, const Tuple& tuple) const;

        // Instrumentation counters
        std::size_t joinComparisonCount = 0;
        std::size_t selectTupleCount = 0;


    };

} // namespace query