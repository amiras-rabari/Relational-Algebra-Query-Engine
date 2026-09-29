#ifndef QUERY_PROCESSOR_PARSER_CONTEXT_H
#define QUERY_PROCESSOR_PARSER_CONTEXT_H

#pragma once

#include "Token.h"
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace query
{
    class ParserContext
    {
    public:
        explicit ParserContext(std::vector<Token> inputTokens);

        bool isAtEnd() const;

        const Token& peek() const;
        const Token& peekAhead(size_t offset) const;

        bool check(TokenType type) const;
        bool match(std::initializer_list<TokenType> types);

        Token advance();
        Token consume(
            TokenType type,
            const std::string& errorMessage
        );

        bool checkAttributeName() const;

        static bool isAttributeNameToken(TokenType type);

        std::string consumeAttributeName(
            const std::string& errorMessage
        );

    private:
        std::vector<Token> tokens;
        size_t current = 0;
    };

    [[noreturn]]
    void throwParseError(
        const Token& token,
        const std::string& message
    );
}

#endif // QUERY_PROCESSOR_PARSER_CONTEXT_H