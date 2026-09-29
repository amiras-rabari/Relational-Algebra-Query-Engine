//
// Created by amira on 2026-09-22.
//

#ifndef QUERY_PROCESSOR_TOKEN_H
#define QUERY_PROCESSOR_TOKEN_H

#pragma once // Prevents the header from being included multiple times

#include <iostream>
#include <string>
#include <variant>
#include <vector>

namespace query
{
    // 1. Strongly typed enum for fast switch statements and checks
    enum class TokenType
    {
        // Keywords
        SELECT, PROJECT, RENAME, UNION, INTERSECT, MINUS, TIMES, JOIN,
        AND, OR, NOT,

        // Dynamic Tokens
        IDENTIFIER,
        INTEGER,
        QUOTED_STRING,
        BARE_STRING,

        // Operators
        EQUAL, // =
        NOT_EQUAL, // !=
        LESS, // <
        LESS_EQUAL, // <=
        GREATER, // >
        GREATER_EQUAL, // >=

        // Punctuation
        LPAREN, RPAREN, // ( )
        LBRACKET, RBRACKET, // [ ]
        LBRACE, RBRACE, // { }
        DOT, COMMA, // . ,

        // Control
        NEWLINE,
        END_OF_FILE
    };

    // 2. Token record structure
    struct Token
    {
        TokenType type; // What kind of token is this?
        std::string lexeme; // Exact text from input (e.g., "-30")

        // The Payload (handles your variations):
        // std::monostate means "no value" (used for keywords/operators).
        // std::string stores decoded strings and the complete integer text,
        // so integer literals are not limited by the range of int.
        std::variant<std::monostate, std::string> value;
        int line; // For error reporting
        int column; // For error reporting


        // The print function must be inside the struct
        void print() const
        {
            std::cout << "[Line " << line << ", Col " << column << "] ";
            if (type == TokenType::IDENTIFIER)
            {
                std::cout << "IDENTIFIER: " << lexeme << "\n";
            }
            if (type == TokenType::INTEGER)
            {
                std::cout << "INTEGER: " << lexeme << "\n";
            }
            else if (std::holds_alternative<std::string>(value))
            {
                if (type == TokenType::QUOTED_STRING)
                {
                    std::cout << "QOUTED STRING/ID: " << lexeme << " (Val: " << std::get<std::string>(value) << ")\n";
                }
                if (type == TokenType::BARE_STRING)
                {
                    std::cout << "Bare STRING/ID: " << lexeme << " (Val: " << std::get<std::string>(value) << ")\n";
                }
            }
            else
            {
                std::cout << "SYMBOL/KEYWORD: " << lexeme << "\n";
            }
        }
    };

    // Function declaration to tokenize an input string
    std::vector<Token> tokenize(const std::string& input);
} // query

#endif //QUERY_PROCESSOR_TOKEN_H
