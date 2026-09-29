//
// Created by amira on 2026-09-22.
//

#ifndef QUERY_PROCESSOR_TOKENIZER_H
#define QUERY_PROCESSOR_TOKENIZER_H

#pragma once

#include "Token.h"
#include <string>
#include <vector>
#include <unordered_map>
namespace query
{
    class Tokenizer {
    private:
        std::string source;
        std::vector<Token> tokens;

        int start_index = 0;
        int current_index = 0;

        int line = 1;
        int current_column = 1;
        int start_column = 1;

        bool in_relation_data = false; // Toggled by { and }

        // Keyword lookup table
        const std::unordered_map<std::string, TokenType> keywords;

        // --- Core Navigation Helpers ---
        //A const keyword placed at the end of a C++
        //member function means that the method is a read-only function
        //that cannot modify the object's member variables
        bool isAtEnd() const;
        char peek() const;
        char peekNext() const;
        char advance();
        bool match(char expected);

        // --- Token Producers ---
        void addToken(TokenType type);
        void addToken(TokenType type, std::variant<std::monostate, std::string> literal);
        void throwLexicalError(const std::string& message);
        void skipWhitespaceAndComments();
        void parseString();
        void parseNumber(char sign);
        void parseDigitStartedToken();
        void parseIdentifierOrKeyword();
        void parseBareString();
        void parseRelationDataValue();

        // --- Main Loop ---
        void scanToken();

    public:
        Tokenizer(const std::string& input);
        std::vector<Token> tokenizeAll();
    };
} // query

#endif //QUERY_PROCESSOR_TOKENIZER_H
