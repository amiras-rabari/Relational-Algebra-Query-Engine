//
// Created by amira on 2026-09-27.
//

#ifndef QUERY_PROCESSOR_SEMANTICVALIDATOR_H
#define QUERY_PROCESSOR_SEMANTICVALIDATOR_H

#pragma once

#include "AST.h"
#include "Database.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace query
{
    enum class SemanticValueType
    {
        Number,
        String
    };

    enum class SemanticErrorCategory
    {
        Name,
        Ambiguity,
        Schema,
        Type
    };

    struct SemanticError
    {
        SemanticErrorCategory category;
        std::string message;
    };

    struct SemanticAttribute
    {
        std::string relationName;
        std::string name;
        SemanticValueType type;
    };

    struct RelationSchema
    {
        std::string relationName;
        std::vector<SemanticAttribute> attributes;
    };

    struct SemanticValidationResult
    {
        bool ok = true;
        std::vector<SemanticError> errors;

        bool hasErrors() const
        {
            return !ok || !errors.empty();
        }
    };

    class SemanticValidator
    {
    public:
        SemanticValidator() = default;

        // ------------------------------------------------------------
        // Relation validation
        // ------------------------------------------------------------
        //
        // Validates parsed relation tables, builds the schema catalog,
        // and returns a validated database where duplicate tuples are
        // removed using set semantics.
        Database validateDatabase(const Database& database);

        // Validates relations and stores all errors instead of throwing.
        SemanticValidationResult validateDatabaseCollectErrors(
            const Database& database
        );

        // ------------------------------------------------------------
        // Query validation
        // ------------------------------------------------------------
        //
        // Requires validateDatabase(...) or validateDatabaseCollectErrors(...)
        // to be called first when relation definitions exist.
        RelationSchema validateQuery(const QueryNode& query);

        // Validates query and stores all errors instead of throwing.
        SemanticValidationResult validateQueryCollectErrors(
            const QueryNode& query
        );

        // ------------------------------------------------------------
        // Schema access
        // ------------------------------------------------------------
        const std::unordered_map<std::string, RelationSchema>& schemas() const
        {
            return schemasByRelationName;
        }

        bool hasSchema(const std::string& relationName) const;

        const RelationSchema& getSchema(const std::string& relationName) const;

        void clear();

    private:
        std::unordered_map<std::string, RelationSchema> schemasByRelationName;
        std::vector<SemanticError> errors;

        // ------------------------------------------------------------
        // Error helpers
        // ------------------------------------------------------------
        void addError(
            SemanticErrorCategory category,
            const std::string& message
        );

        [[noreturn]]
        void throwSemanticError(
            SemanticErrorCategory category,
            const std::string& message
        ) const;

        static std::string categoryToString(
            SemanticErrorCategory category
        );

        static std::string typeToString(
            SemanticValueType type
        );

        // ------------------------------------------------------------
        // Relation validation helpers
        // ------------------------------------------------------------
        RelationSchema validateTableSchema(
            const Table& table
        );

        void validateUniqueRelationName(
            const std::string& relationName
        );

        static void validateUniqueAttributeNames(
            const Table& table
        );

        static SemanticValueType typeFromDataValue(
            const DataValue& value
        );

        static bool sameTuple(
            const Tuple& left,
            const Tuple& right
        );

        static bool sameDataValue(
            const DataValue& left,
            const DataValue& right
        );

        static Table deduplicateTuples(
            const Table& table
        );

        // ------------------------------------------------------------
        // Query expression validation helpers
        // ------------------------------------------------------------
        RelationSchema validateExpression(
            const ExpressionNode* expression
        );

        RelationSchema validateRelationReference(
            const RelationReferenceNode* node
        );

        RelationSchema validateBinaryExpression(
            const BinaryExpressionNode* node
        );

        RelationSchema validateJoin(
            const JoinNode* node
        );

        RelationSchema validateSelect(
            const SelectNode* node
        );

        RelationSchema validateProject(
            const ProjectNode* node
        );

        RelationSchema validateRename(
            const RenameNode* node
        );

        // ------------------------------------------------------------
        // Condition validation helpers
        // ------------------------------------------------------------
        void validateCondition(
            const ConditionNode* condition,
            const RelationSchema& scope
        );

        SemanticValueType validateOperand(
            const OperandNode* operand,
            const RelationSchema& scope
        );

        SemanticValueType validateLiteralOperand(
            const LiteralOperandNode* literal
        );

        SemanticValueType validateAttributeOperand(
            const AttributeOperandNode* operand,
            const RelationSchema& scope
        );

        const SemanticAttribute& resolveAttribute(
            const AttributeReferenceNode* reference,
            const RelationSchema& scope
        );

        // ------------------------------------------------------------
        // Schema operation helpers
        // ------------------------------------------------------------
        static RelationSchema combineSchemas(
            const RelationSchema& left,
            const RelationSchema& right,
            const std::string& operationName
        );

        static void validateNoQualifiedCollisions(
            const RelationSchema& schema,
            const std::string& operationName
        );

        static void validateUnionCompatible(
            const RelationSchema& left,
            const RelationSchema& right,
            const std::string& operationName
        );

        static std::string qualifiedAttributeName(
            const SemanticAttribute& attribute
        );

        static bool schemaHasRelationQualifier(
            const RelationSchema& schema,
            const std::string& relationName
        );

        static bool schemaHasDuplicateQualifiedAttributes(
            const RelationSchema& schema
        );
    };
} // query

#endif //QUERY_PROCESSOR_SEMANTICVALIDATOR_H