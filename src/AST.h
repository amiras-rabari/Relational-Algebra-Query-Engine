//
// Created by amira on 2026-09-23.
//

#ifndef QUERY_PROCESSOR_AST_H
#define QUERY_PROCESSOR_AST_H


#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <utility>

// ============================================================================
// AST overview
// ============================================================================
//
// This AST represents the semantic structure needed by the parser,
// semantic analyser, and query executor.
//
// Grammar rules such as expr, union_expr, intersect_expr, combine_expr,
// row_separator, relation_attribute_list, etc. are parsing structure and do
// not need one AST node each. Their meaning is represented by the actual
// expression/condition/data nodes below.
//
// ============================================================================

namespace query
{
    // ============================================================
    // 1. What kind of AST node is this?
    // ============================================================
    //
    // The lexer has TokenType.
    // The AST has NodeKind.
    //
    // TokenType answers:
    //     "What did the lexer see?"
    //
    // NodeKind answers:
    //     "What syntactic construct did the parser build?"
    //
    // These two enums are intentionally separate.
    // ============================================================

    enum class NodeKind
    {
        // Entry points
        Query,
        RelationFile,

        // Relation definitions / data
        RelationDefinition,
        RelationRow,
        Value,

        // Relational expressions
        RelationReference,
        Union,
        Minus,
        Intersect,
        Times,
        Join,
        Select,
        Project,
        Rename,

        // Attributes
        AttributeReference,

        // Conditions
        And,
        Or,
        Not,
        Comparison,

        // Condition operands
        AttributeOperand,
        LiteralOperand
    };


    // ============================================================
    // 2. Base AST node
    // ============================================================
    //
    // Every real AST node derives from ASTNode.
    //
    // Example:
    //
    //     SelectNode
    //          ↓
    //     ExpressionNode
    //          ↓
    //       ASTNode
    //
    // This gives us a common type for the whole tree.
    // ============================================================

    struct ASTNode
    {
        explicit ASTNode(NodeKind kind)
            : kind(kind)
        {
        }

        // IMPORTANT C++ concept:
        // virtual destructor.
        //
        // We will store derived nodes through pointers such as:
        //
        //     std::unique_ptr<ASTNode>
        //
        // Because ASTNode is a base class, its destructor must be
        // virtual so the correct derived destructor is called.
        virtual ~ASTNode() = default;

        NodeKind kind;
    };


    // ============================================================
    // 3. Ownership of AST nodes
    // ============================================================
    //
    // unique_ptr means:
    //
    //     "This parent owns this child."
    //
    // For example:
    //
    //             UNION
    //            /     \
    //           A       B
    //
    // The UNION node owns both children.
    //
    // When the UNION node is destroyed, its children are
    // automatically destroyed.
    //
    // This is much safer than manually using new/delete.
    // ============================================================

    using ASTPtr = std::unique_ptr<ASTNode>;


    // ============================================================
    // 4. Categories of AST nodes
    // ============================================================
    //
    // These are intermediate base classes.
    //
    // ExpressionNode means:
    //     "This node represents a relational expression."
    //
    // ConditionNode means:
    //     "This node represents a condition."
    //
    // OperandNode means:
    //     "This node can appear as an operand in a comparison."
    // ============================================================

    struct ExpressionNode : ASTNode
    {
        explicit ExpressionNode(NodeKind kind)
            : ASTNode(kind)
        {
        }
    };

    struct ConditionNode : ASTNode
    {
        explicit ConditionNode(NodeKind kind)
            : ASTNode(kind)
        {
        }
    };

    struct OperandNode : ASTNode
    {
        explicit OperandNode(NodeKind kind)
            : ASTNode(kind)
        {
        }
    };


    // Convenient pointer types.
    //
    // These make the code much easier to read:
    //
    //     ExpressionPtr
    //
    // instead of:
    //
    //     std::unique_ptr<ExpressionNode>
    using ExpressionPtr = std::unique_ptr<ExpressionNode>;
    using ConditionPtr = std::unique_ptr<ConditionNode>;
    using OperandPtr = std::unique_ptr<OperandNode>;


    // ============================================================
    // 5. Relation DATA representation
    // ============================================================
    //
    // These correspond to:
    //
    //     value
    //     tuple_line
    //     relation_definition
    //     relation_file
    //
    // They are AST nodes because relation files are also parsed,
    // but they are separate from query-expression nodes.
    // ============================================================


    enum class ValueKind
    {
        Integer,
        QuotedString,
        BareString
    };


    // Represents one value inside relation data.
    //
    // Examples:
    //
    //     42
    //     'Bob'
    //     Bob
    //
    // We keep the text as a string.
    //
    // Your grammar allows an integer to have arbitrarily many
    // digits, so we should NOT assume it fits into a C++ int.
    struct ValueNode : ASTNode
    {
        ValueNode(ValueKind kind, std::string text)
            : ASTNode(NodeKind::Value),
              kind(kind),
              text(std::move(text))
        {
        }

        ValueKind kind;

        // Original textual representation of the value.
        //
        // Example:
        //     "-999999999999999999999999"
        //
        // We preserve it rather than forcing it into int.
        std::string text;
    };


    struct RelationRowNode : ASTNode
    {
        RelationRowNode()
            : ASTNode(NodeKind::RelationRow)
        {
        }

        // A tuple_line contains one or more values.
        //
        // Example:
        //
        //     1, 'Bob', 30
        //
        // becomes:
        //
        //     RelationRow
        //       ├── 1
        //       ├── 'Bob'
        //       └── 30
        //
        std::vector<ValueNode> values;
    };


    struct RelationDefinitionNode : ASTNode
    {
        RelationDefinitionNode()
            : ASTNode(NodeKind::RelationDefinition)
        {
        }

        // relation_name
        //
        // Example:
        //
        //     Employees
        //
        std::string name;

        // relation_attribute_list
        //
        // Example:
        //
        //     ID, Name, Age
        //
        std::vector<std::string> attributes;

        // relation_rows
        //
        // Zero or more rows.
        //
        // unique_ptr means this relation-definition node owns
        // its row nodes.
        std::vector<std::unique_ptr<RelationRowNode>> rows;
    };


    struct RelationFileNode : ASTNode
    {
        RelationFileNode()
            : ASTNode(NodeKind::RelationFile)
        {
        }

        // relation_file ::= relation_definition
        //                   { relation_definition }
        //
        // Therefore a file can contain many definitions.
        std::vector<std::unique_ptr<RelationDefinitionNode>> definitions;
    };

    // ============================================================
    // 8. Relation reference
    // ============================================================
    //
    // Grammar:
    //
    //     primary_expr ::= relation_name
    //
    // Example:
    //
    //     Employees
    //
    // IMPORTANT:
    // This does NOT contain the actual Relation object.
    //
    // It only means:
    //
    //     "The query refers to a relation called Employees."
    //
    // The semantic/execution layer resolves the name later.
    // ============================================================

    struct RelationReferenceNode : ExpressionNode
    {
        explicit RelationReferenceNode(std::string name)
            : ExpressionNode(NodeKind::RelationReference),
              name(std::move(name))
        {
        }

        std::string name;
    };


    // ============================================================
    // 6. Attribute references
    // ============================================================
    //
    // Grammar:
    //
    //     attribute_reference
    //         ::= attribute_name
    //          |  relation_name "." attribute_name
    //
    // So both are valid:
    //
    //     Age
    //     Employees.Age
    // ============================================================

    struct AttributeReferenceNode : ASTNode
    {
        AttributeReferenceNode()
            : ASTNode(NodeKind::AttributeReference)
        {
        }

        // C++ concept:
        //
        // std::optional<T>
        //
        // means:
        //
        //     "A value may exist, or may not exist."
        //
        // For:
        //
        //     Age
        //
        // relationQualifier = nothing
        //
        // For:
        //
        //     Employees.Age
        //
        // relationQualifier = "Employees"
        std::optional<std::string> relationQualifier;

        // Attribute name.
        //
        // Example:
        //
        //     Age
        //
        // or:
        //
        //     Employees.Age
        //
        // name = "Age"
        std::string name;
    };


    // ============================================================
    // 7. Query root
    // ============================================================
    //
    // Grammar:
    //
    //     query ::= expr EOF
    //
    // The QueryNode is the root of the query AST.
    // ============================================================

    struct QueryNode : ASTNode
    {
        QueryNode()
            : ASTNode(NodeKind::Query)
        {
        }

        // The actual relational expression.
        ExpressionPtr expression;
    };


    // ============================================================
    // 9. Binary relational operators
    // ============================================================
    //
    // Grammar:
    //
    //     union
    //     minus
    //     intersect
    //     times
    //
    // All of these have:
    //
    //             OP
    //            /  \
    //           A    B
    //
    // Therefore we can reuse ONE node type.
    // ============================================================

    enum class BinaryOperator
    {
        Union,
        Minus,
        Intersect,
        Times
    };


    struct BinaryExpressionNode : ExpressionNode
    {
        BinaryExpressionNode(
            BinaryOperator op,
            ExpressionPtr left,
            ExpressionPtr right)
            : ExpressionNode(kindFor(op)),
              op(op),
              left(std::move(left)),
              right(std::move(right))
        {
        }

        BinaryOperator op;

        // Left child.
        ExpressionPtr left;

        // Right child.
        ExpressionPtr right;

    private:
        // Convert the operator-specific enum into the general
        // AST node kind.
        static NodeKind kindFor(BinaryOperator op)
        {
            switch (op)
            {
            case BinaryOperator::Union:
                return NodeKind::Union;

            case BinaryOperator::Minus:
                return NodeKind::Minus;

            case BinaryOperator::Intersect:
                return NodeKind::Intersect;

            case BinaryOperator::Times:
                return NodeKind::Times;
            }

            // Defensive fallback.
            return NodeKind::Union;
        }
    };


    // ============================================================
    // 10. JOIN
    // ============================================================
    //
    // Grammar:
    //
    //     join_operator ::= "join" "[" condition "]"
    //
    // But JOIN is a binary relational operation, so the AST needs:
    //
    //             JOIN
    //            /    \
    //           A      B
    //              +
    //           condition
    //
    // ============================================================

    struct JoinNode : ExpressionNode
    {
        JoinNode(
            ExpressionPtr left,
            ConditionPtr condition,
            ExpressionPtr right)
            : ExpressionNode(NodeKind::Join),
              left(std::move(left)),
              condition(std::move(condition)),
              right(std::move(right))
        {
        }

        ExpressionPtr left;

        ConditionPtr condition;

        ExpressionPtr right;
    };


    // ============================================================
    // 11. SELECT
    // ============================================================
    //
    // Grammar:
    //
    //     select_expr ::=
    //         "select" "[" condition "]" "(" expr ")"
    //
    // Example:
    //
    //     select[Age > 30](Employees)
    //
    // AST:
    //
    //             SELECT
    //            /      \
    //       condition   Employees
    // ============================================================

    struct SelectNode : ExpressionNode
    {
        SelectNode(
            ConditionPtr condition,
            ExpressionPtr expression)
            : ExpressionNode(NodeKind::Select),
              condition(std::move(condition)),
              expression(std::move(expression))
        {
        }

        ConditionPtr condition;

        ExpressionPtr expression;
    };


    // ============================================================
    // 12. PROJECT
    // ============================================================
    //
    // Grammar:
    //
    //     project_expr ::=
    //         "project" "[" attribute_list "]" "(" expr ")"
    //
    // Example:
    //
    //     project[Name, Age](Employees)
    //
    // AST:
    //
    //              PROJECT
    //             /       \
    //       attributes   Employees
    // ============================================================

    struct ProjectNode : ExpressionNode
    {
        ProjectNode(
            std::vector<std::unique_ptr<AttributeReferenceNode>> attributes,
            ExpressionPtr expression)
            : ExpressionNode(NodeKind::Project),
              attributes(std::move(attributes)),
              expression(std::move(expression))
        {
        }

        // One or more attributes.
        std::vector<std::unique_ptr<AttributeReferenceNode>> attributes;

        ExpressionPtr expression;
    };


    // ============================================================
    // 13. RENAME
    // ============================================================
    //
    // Grammar:
    //
    //     rename_expr ::=
    //         "rename" "[" relation_name "]" "(" expr ")"
    //
    // Example:
    //
    //     rename[R](Employees)
    //
    // AST:
    //
    //            RENAME
    //            /    \
    //           R    Employees
    // ============================================================

    struct RenameNode : ExpressionNode
    {
        RenameNode(
            std::string relationName,
            ExpressionPtr expression)
            : ExpressionNode(NodeKind::Rename),
              relationName(std::move(relationName)),
              expression(std::move(expression))
        {
        }

        std::string relationName;

        ExpressionPtr expression;
    };


    // ============================================================
    // 14. Logical conditions
    // ============================================================
    //
    // Grammar:
    //
    //     and_condition ::=
    //         not_condition { "and" not_condition }
    //
    //     or_condition ::=
    //         and_condition { "or" and_condition }
    //
    // These are binary operations.
    // ============================================================

    enum class LogicalOperator
    {
        And,
        Or
    };


    struct LogicalConditionNode : ConditionNode
    {
        LogicalConditionNode(
            LogicalOperator op,
            ConditionPtr left,
            ConditionPtr right)
            : ConditionNode(kindFor(op)),
              op(op),
              left(std::move(left)),
              right(std::move(right))
        {
        }

        LogicalOperator op;

        ConditionPtr left;
        ConditionPtr right;

    private:
        static NodeKind kindFor(LogicalOperator op)
        {
            switch (op)
            {
            case LogicalOperator::And:
                return NodeKind::And;

            case LogicalOperator::Or:
                return NodeKind::Or;
            }

            return NodeKind::And;
        }
    };


    // ============================================================
    // 15. NOT
    // ============================================================
    //
    // Grammar:
    //
    //     not_condition ::=
    //         "not" not_condition
    //      |  condition_primary
    //
    // NOT has ONE child because it is unary.
    //
    // Example:
    //
    //     not Age > 30
    //
    // AST:
    //
    //          NOT
    //           |
    //        Age > 30
    // ============================================================

    struct NotConditionNode : ConditionNode
    {
        explicit NotConditionNode(ConditionPtr operand)
            : ConditionNode(NodeKind::Not),
              operand(std::move(operand))
        {
        }

        ConditionPtr operand;
    };


    // ============================================================
    // 16. Comparison operators
    // ============================================================
    //
    // Grammar:
    //
    //     comparison_operator ::=
    //         "="
    //       | "!="
    //       | "<"
    //       | "<="
    //       | ">"
    //       | ">="
    // ============================================================

    enum class ComparisonOperator
    {
        Equal,
        NotEqual,
        Less,
        LessEqual,
        Greater,
        GreaterEqual
    };


    struct ComparisonNode : ConditionNode
    {
        ComparisonNode(
            ComparisonOperator op,
            OperandPtr left,
            OperandPtr right)
            : ConditionNode(NodeKind::Comparison),
              op(op),
              left(std::move(left)),
              right(std::move(right))
        {
        }

        ComparisonOperator op;

        OperandPtr left;
        OperandPtr right;
    };


    // ============================================================
    // 17. Attribute operand
    // ============================================================
    //
    // Grammar:
    //
    //     operand ::=
    //         integer
    //       | quoted_string
    //       | attribute_reference
    //
    // Example:
    //
    //     Age > 30
    //
    // "Age" is an attribute operand.
    //
    // It wraps an AttributeReferenceNode because the same
    // attribute-reference structure is also used elsewhere,
    // such as projection lists.
    // ============================================================

    struct AttributeOperandNode : OperandNode
    {
        explicit AttributeOperandNode(
            std::unique_ptr<AttributeReferenceNode> reference)
            : OperandNode(NodeKind::AttributeOperand),
              reference(std::move(reference))
        {
        }

        std::unique_ptr<AttributeReferenceNode> reference;
    };


    // ============================================================
    // 18. Literal operand
    // ============================================================
    //
    // Query conditions allow:
    //
    //     integer
    //     quoted_string
    //
    // They DO NOT allow bare_string.
    //
    // Example:
    //
    //     Age > 30
    //     Name = 'Bob'
    // ============================================================

    struct LiteralOperandNode : OperandNode
    {
        LiteralOperandNode(
            ValueKind kind,
            std::string text)
            : OperandNode(NodeKind::LiteralOperand),
              valueKind(kind),
              text(std::move(text))
        {
        }

        ValueKind valueKind;

        // Again, preserve the text rather than assuming every
        // integer fits inside a C++ int.
        std::string text;
    };
} // query

#endif //QUERY_PROCESSOR_AST_H
