#ifndef QUERY_PROCESSOR_QUERY_PARSER_H
#define QUERY_PROCESSOR_QUERY_PARSER_H

#pragma once

#include "AST.h"
#include "ParserContext.h"

namespace query
{
    class QueryParser
    {
    public:
        explicit QueryParser(ParserContext& context);

        std::unique_ptr<QueryNode> parseQuery();

    private:
        ParserContext& context;

        // Expressions
        ExpressionPtr parseExpression();
        ExpressionPtr parseUnionMinusExpr();
        ExpressionPtr parseIntersectExpr();
        ExpressionPtr parseCombineExpr();
        ExpressionPtr parseUnaryExpr();
        ExpressionPtr parseSelect();
        ExpressionPtr parseProject();
        ExpressionPtr parseRename();
        ExpressionPtr parsePrimaryExpr();

        // Conditions
        ConditionPtr parseCondition();
        ConditionPtr parseOrCondition();
        ConditionPtr parseAndCondition();
        ConditionPtr parseNotCondition();
        ConditionPtr parseConditionPrimary();
        ConditionPtr parseComparison();
        OperandPtr parseOperand();

        // Query helpers
        std::string consumeRelationName(
            const std::string& errorMessage
        );

        std::unique_ptr<AttributeReferenceNode>
        parseAttributeReference();

        std::vector<std::unique_ptr<AttributeReferenceNode>>
        parseAttributeList();
    };
}

#endif // QUERY_PROCESSOR_QUERY_PARSER_H