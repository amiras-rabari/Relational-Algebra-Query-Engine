//
// Created by amira on 2026-09-27.
//

#include "ParserContext.h"

#include "ParserContext.h"

#include <stdexcept>

namespace query
{
    ParserContext::ParserContext(std::vector<Token> inputTokens)
        : tokens(std::move(inputTokens)), current(0)
    {
    }

    bool ParserContext::isAtEnd() const
    {
        return current >= tokens.size() ||
               tokens[current].type == TokenType::END_OF_FILE;
    }

    const Token& ParserContext::peek() const
    {
        if (current >= tokens.size())
            return tokens.back();

        return tokens[current];
    }

    const Token& ParserContext::peekAhead(size_t offset) const
    {
        if (current + offset >= tokens.size())
            return tokens.back();

        return tokens[current + offset];
    }

    bool ParserContext::check(TokenType type) const
    {
        return peek().type == type;
    }

    bool ParserContext::match(std::initializer_list<TokenType> types)
    {
        for (TokenType type : types)
        {
            if (check(type))
            {
                advance();
                return true;
            }
        }

        return false;
    }

    Token ParserContext::advance()
    {
        if (!isAtEnd())
            current++;

        return tokens[current - 1];
    }

    Token ParserContext::consume(
        TokenType type,
        const std::string& errorMessage)
    {
        if (check(type))
            return advance();

        throwParseError(peek(), errorMessage);
    }

    bool ParserContext::checkAttributeName() const
    {
        return isAttributeNameToken(peek().type);
    }

    bool ParserContext::isAttributeNameToken(TokenType type)
    {
        switch (type)
        {
        case TokenType::IDENTIFIER:
        case TokenType::SELECT:
        case TokenType::PROJECT:
        case TokenType::RENAME:
        case TokenType::UNION:
        case TokenType::INTERSECT:
        case TokenType::MINUS:
        case TokenType::TIMES:
        case TokenType::JOIN:
        case TokenType::AND:
        case TokenType::OR:
        case TokenType::NOT:
            return true;

        default:
            return false;
        }
    }

    std::string ParserContext::consumeAttributeName(
        const std::string& errorMessage)
    {
        if (checkAttributeName())
            return advance().lexeme;

        throwParseError(peek(), errorMessage);
    }

    [[noreturn]]
    void throwParseError(
        const Token& token,
        const std::string& message)
    {
        throw std::runtime_error(
            "[Parse Error] Line " + std::to_string(token.line) +
            ", Col " + std::to_string(token.column) +
            ": " + message +
            " (Got char '" + token.lexeme + "')"
        );
    }
}