#include "QueryParser.h"

#include <string>
#include <utility>

namespace query
{
    QueryParser::QueryParser(ParserContext& context)
        : context(context)
    {
    }

    std::unique_ptr<QueryNode> QueryParser::parseQuery()
    {
        while (context.match({TokenType::NEWLINE}))
        {
            // Ignore leading blank lines.
        }

        if (context.isAtEnd())
        {
            throwParseError(
                context.peek(),
                "Empty input stream. Expected query expression."
            );
        }

        auto queryNode = std::make_unique<QueryNode>();

        queryNode->expression = parseExpression();

        while (context.match({TokenType::NEWLINE}))
        {
            // Consume trailing newlines.
        }

        context.consume(
            TokenType::END_OF_FILE,
            "Expected EOF after query expression, Make sure query starts with (relation_name) binaryop or unary op and relation defination must be completed before"
        );

        return queryNode;
    }

    ExpressionPtr QueryParser::parseExpression()
    {
        if (
            context.check(TokenType::IDENTIFIER) &&
            context.peekAhead(1).type == TokenType::IDENTIFIER
        )
        {
            throwParseError(
                context.peekAhead(1),
                "Expected binary operator between expressions"
            );
        }

        return parseUnionMinusExpr();
    }

    ExpressionPtr QueryParser::parseUnionMinusExpr()
    {
        auto left = parseIntersectExpr();

        while (
            context.check(TokenType::UNION) ||
            context.check(TokenType::MINUS)
        )
        {
            Token opToken = context.advance();

            auto right = parseIntersectExpr();

            BinaryOperator op =
                (opToken.type == TokenType::UNION)
                    ? BinaryOperator::Union
                    : BinaryOperator::Minus;

            left = std::make_unique<BinaryExpressionNode>(
                op,
                std::move(left),
                std::move(right)
            );
        }

        return left;
    }

    ExpressionPtr QueryParser::parseIntersectExpr()
    {
        auto left = parseCombineExpr();

        while (context.check(TokenType::INTERSECT))
        {
            context.consume(
                TokenType::INTERSECT,
                "Expected 'intersect'."
            );

            auto right = parseCombineExpr();

            left = std::make_unique<BinaryExpressionNode>(
                BinaryOperator::Intersect,
                std::move(left),
                std::move(right)
            );
        }

        return left;
    }

    ExpressionPtr QueryParser::parseCombineExpr()
    {
        auto left = parseUnaryExpr();

        while (
            context.check(TokenType::TIMES) ||
            context.check(TokenType::JOIN)
        )
        {
            Token opToken = context.advance();

            if (opToken.type == TokenType::TIMES)
            {
                auto right = parseUnaryExpr();

                left = std::make_unique<BinaryExpressionNode>(
                    BinaryOperator::Times,
                    std::move(left),
                    std::move(right)
                );
            }
            else
            {
                context.consume(
                    TokenType::LBRACKET,
                    "Expected '[' after JOIN"
                );

                auto condition = parseCondition();

                context.consume(
                    TokenType::RBRACKET,
                    "Expected ']' after join condition"
                );

                auto right = parseUnaryExpr();

                left = std::make_unique<JoinNode>(
                    std::move(left),
                    std::move(condition),
                    std::move(right)
                );
            }
        }

        return left;
    }

    ExpressionPtr QueryParser::parseUnaryExpr()
    {
        if (context.check(TokenType::SELECT))
            return parseSelect();

        if (context.check(TokenType::PROJECT))
            return parseProject();

        if (context.check(TokenType::RENAME))
            return parseRename();

        return parsePrimaryExpr();
    }

    ExpressionPtr QueryParser::parseSelect()
    {
        context.consume(
            TokenType::SELECT,
            "Expected 'select'."
        );

        context.consume(
            TokenType::LBRACKET,
            "Expected '[' and condtion statment after 'select'."
        );

        auto condition = parseCondition();

        context.consume(
            TokenType::RBRACKET,
            "Expected ']' after select condition."
        );

        context.consume(
            TokenType::LPAREN,
            "Expected '(' after select condition."
        );

        auto expression = parseExpression();

        context.consume(
            TokenType::RPAREN,
            "Expected ')' after select expression check relationname must be inside ()."
        );

        return std::make_unique<SelectNode>(
            std::move(condition),
            std::move(expression)
        );
    }

    ExpressionPtr QueryParser::parseProject()
    {
        context.consume(
            TokenType::PROJECT,
            "Expected 'project'."
        );

        context.consume(
            TokenType::LBRACKET,
            "Expected '[' after 'project'."
        );

        if (context.check(TokenType::RBRACKET))
        {
            throwParseError(
                context.peek(),
                "An empty attribute list is not a valid projection."
            );
        }

        auto attributes = parseAttributeList();

        context.consume(
            TokenType::RBRACKET,
            "Expected ']' after projection attribute list."
        );

        context.consume(
            TokenType::LPAREN,
            "Expected '(' after projection attribute list."
        );

        auto expression = parseExpression();

        context.consume(
            TokenType::RPAREN,
            "Expected ')' after project expression."
        );

        return std::make_unique<ProjectNode>(
            std::move(attributes),
            std::move(expression)
        );
    }

    ExpressionPtr QueryParser::parseRename()
    {
        context.consume(
            TokenType::RENAME,
            "Expected 'rename'."
        );

        context.consume(
            TokenType::LBRACKET,
            "Expected '[' after 'rename'."
        );

        std::string relationName =
            consumeRelationName(
                "Expected relation name inside rename."
            );

        context.consume(
            TokenType::RBRACKET,
            "Expected ']' after rename relation name."
        );

        context.consume(
            TokenType::LPAREN,
            "Expected '(' after rename relation name."
        );

        auto expression = parseExpression();

        context.consume(
            TokenType::RPAREN,
            "Expected ')' after rename expression."
        );

        return std::make_unique<RenameNode>(
            std::move(relationName),
            std::move(expression)
        );
    }

    ExpressionPtr QueryParser::parsePrimaryExpr()
    {
        if (context.check(TokenType::LPAREN))
        {
            context.consume(
                TokenType::LPAREN,
                "Expected '('."
            );

            auto expression = parseExpression();

            context.consume(
                TokenType::RPAREN,
                "Expected ')' after expression."
            );

            return expression;
        }

        std::string relationName =
            consumeRelationName(
                "Expected relation name or '('."
            );

        return std::make_unique<RelationReferenceNode>(
            std::move(relationName)
        );
    }

    ConditionPtr QueryParser::parseCondition()
    {
        return parseOrCondition();
    }

    ConditionPtr QueryParser::parseOrCondition()
    {
        auto left = parseAndCondition();

        while (context.match({TokenType::OR}))
        {
            auto right = parseAndCondition();

            left = std::make_unique<LogicalConditionNode>(
                LogicalOperator::Or,
                std::move(left),
                std::move(right)
            );
        }

        return left;
    }

    ConditionPtr QueryParser::parseAndCondition()
    {
        auto left = parseNotCondition();

        while (context.match({TokenType::AND}))
        {
            auto right = parseNotCondition();

            left = std::make_unique<LogicalConditionNode>(
                LogicalOperator::And,
                std::move(left),
                std::move(right)
            );
        }

        return left;
    }

    ConditionPtr QueryParser::parseNotCondition()
    {
        if (context.match({TokenType::NOT}))
        {
            auto operand = parseNotCondition();

            return std::make_unique<NotConditionNode>(
                std::move(operand)
            );
        }

        return parseConditionPrimary();
    }

    ConditionPtr QueryParser::parseConditionPrimary()
    {
        if (context.match({TokenType::LPAREN}))
        {
            auto condition = parseCondition();

            context.consume(
                TokenType::RPAREN,
                "Expected ')' after condition."
            );

            return condition;
        }

        return parseComparison();
    }

    ConditionPtr QueryParser::parseComparison()
    {
        auto left = parseOperand();

        ComparisonOperator op;

        if (context.match({TokenType::EQUAL}))
        {
            op = ComparisonOperator::Equal;
        }
        else if (context.match({TokenType::NOT_EQUAL}))
        {
            op = ComparisonOperator::NotEqual;
        }
        else if (context.match({TokenType::LESS}))
        {
            op = ComparisonOperator::Less;
        }
        else if (context.match({TokenType::LESS_EQUAL}))
        {
            op = ComparisonOperator::LessEqual;
        }
        else if (context.match({TokenType::GREATER}))
        {
            op = ComparisonOperator::Greater;
        }
        else if (context.match({TokenType::GREATER_EQUAL}))
        {
            op = ComparisonOperator::GreaterEqual;
        }
        else
        {
            throwParseError(
                context.peek(),
                "Expected comparison operator."
            );
        }

        auto right = parseOperand();

        return std::make_unique<ComparisonNode>(
            op,
            std::move(left),
            std::move(right)
        );
    }

    OperandPtr QueryParser::parseOperand()
    {
        if (context.check(TokenType::INTEGER))
        {
            Token token = context.advance();

            return std::make_unique<LiteralOperandNode>(
                ValueKind::Integer,
                token.lexeme
            );
        }

        if (context.check(TokenType::QUOTED_STRING))
        {
            Token token = context.advance();

            return std::make_unique<LiteralOperandNode>(
                ValueKind::QuotedString,
                std::get<std::string>(token.value)
            );
        }

        if (context.checkAttributeName())
        {
            auto attribute = parseAttributeReference();

            return std::make_unique<AttributeOperandNode>(
                std::move(attribute)
            );
        }

        throwParseError(
            context.peek(),
            "Expected operand (integer, quoted string, or attribute reference)."
        );
    }

    std::string QueryParser::consumeRelationName(
        const std::string& errorMessage)
    {
        if (context.check(TokenType::IDENTIFIER))
            return context.advance().lexeme;

        throwParseError(
            context.peek(),
            errorMessage
        );
    }

    std::unique_ptr<AttributeReferenceNode>
    QueryParser::parseAttributeReference()
    {
        if (
            context.check(TokenType::IDENTIFIER) &&
            context.peekAhead(1).type == TokenType::DOT
        )
        {
            std::string relName = context.advance().lexeme;

            context.consume(
                TokenType::DOT,
                "Expected '.' in qualified attribute reference"
            );

            std::string attrName =
                context.consumeAttributeName(
                    "Expected attribute name after '.'"
                );

            auto node =
                std::make_unique<AttributeReferenceNode>();

            node->relationQualifier = relName;
            node->name = attrName;

            return node;
        }

        std::string attrName =
            context.consumeAttributeName(
                "Expected attribute reference"
            );

        auto node =
            std::make_unique<AttributeReferenceNode>();

        node->name = attrName;

        return node;
    }

    std::vector<std::unique_ptr<AttributeReferenceNode>>
    QueryParser::parseAttributeList()
    {
        std::vector<std::unique_ptr<AttributeReferenceNode>> list;

        list.push_back(parseAttributeReference());

        while (context.match({TokenType::COMMA}))
        {
            list.push_back(parseAttributeReference());
        }

        return list;
    }
}