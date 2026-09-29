//
// Created by amira on 2026-09-22.
//

#include "Tokenizer.h"
#include <cctype>

namespace query
{
    // Constructor
    Tokenizer::Tokenizer(const std::string& input) : source(input), keywords({
                                                         {"select", TokenType::SELECT}, {"project", TokenType::PROJECT},
                                                         {"rename", TokenType::RENAME}, {"union", TokenType::UNION},
                                                         {"intersect", TokenType::INTERSECT},
                                                         {"minus", TokenType::MINUS},
                                                         {"times", TokenType::TIMES}, {"join", TokenType::JOIN},
                                                         {"and", TokenType::AND}, {"or", TokenType::OR},
                                                         {"not", TokenType::NOT}
                                                     })
    {
    }

    // --- Public API ---
    //This is the main engine driver.
    //It loops until the file is consumed, calling scanToken() to extract one token at a time.
    //Once the loop finishes, it pushes a synthetic END_OF_FILE token.
    //This is crucial for the Parser later; it needs a definitive signal that the token stream has ended.
    std::vector<Token> Tokenizer::tokenizeAll()
    {
        while (!isAtEnd())
        {
            scanToken();
        }
        tokens.push_back({TokenType::END_OF_FILE, "EOF", std::monostate{}, line, current_column});
        return tokens;
    }

    // --- Core Navigation Helpers ---
    //The const keyword at the end of the method guarantees
    //to the C++ compiler that calling this method will never mutate internal class variables.
    bool Tokenizer::isAtEnd() const
    {
        return current_index >= source.length();
    }

    //peek looks at the current character without consuming it. peekNext looks one character ahead.
    //This is required for a LL(1) / LL(2) Lexer, meaning it needs 1 or 2 characters
    //of lookahead to make a decision

    //same principle followed as the dragon book theory
    char Tokenizer::peek() const
    {
        if (isAtEnd()) return '\0';
        return source[current_index];
    }

    char Tokenizer::peekNext() const
    {
        if (current_index + 1 >= source.length()) return '\0';
        return source[current_index + 1];
    }

    char Tokenizer::advance()
    {
        current_column++;
        return source[current_index++];
    }

    // Maximal Munch
    //This method tries to consume the longest possible token.
    //If the scout sees <, it calls match('='). If = is there, it consumes it
    //and returns true (forming <=). If not, it safely returns false without advancing the pointer,
    //leaving just <

    bool Tokenizer::match(char expected)
    {
        if (isAtEnd() || source[current_index] != expected) return false;
        current_index++;
        current_column++;
        return true;
    }

    // --- Token Producers ---

    void Tokenizer::addToken(TokenType type)
    {
        tokens.push_back({
            type, source.substr(start_index, current_index - start_index), std::monostate{}, line, start_column
        });
    }

    void Tokenizer::addToken(TokenType type, std::variant<std::monostate, std::string> literal)
    {
        tokens.push_back({type, source.substr(start_index, current_index - start_index), literal, line, start_column});
    }

    void Tokenizer::throwLexicalError(const std::string& message)
    {
        throw std::runtime_error("Lexical Error at Line " + std::to_string(line) +
            ", Col " + std::to_string(start_column) + ": " + message);
    }

    void Tokenizer::skipWhitespaceAndComments()
    {
        while (true)
        {
            char c = peek();
            if (c == ' ' || c == '\r' || c == '\t')
            {
                advance();
            }
            else if (c == '\n')
            {
                if (in_relation_data) return;
                advance();
                line++;
                current_column = 1;
            }
            else if (c == '/')
            {
                if (peekNext() == '/')
                {
                    while (peek() != '\n' && !isAtEnd()) advance();
                }
                else
                {
                    return;
                }
            }
            else
            {
                return;
            }
        }
    }

    //Quote Escaping: If it sees a quote inside a string, it peeks ahead.
    //If the next char is also a quote (''), it treats it as a literal single quote character
    //inside the string, consumes both, and continues. Otherwise, it breaks the loop because
    //the string is finished.
    void Tokenizer::parseString()
    {
        std::string value = "";
        while (!isAtEnd())
        {
            if (peek() == '\n')
            {
                throwLexicalError("Unterminated string (newline found before closing quote).");
            }
            if (peek() == '\'')
            {
                if (peekNext() == '\'')
                {
                    value += '\'';
                    advance();
                    advance();
                }
                else
                {
                    break;
                }
            }
            else
            {
                value += advance();
            }
        }

        if (isAtEnd()) throwLexicalError("Unterminated string (reached end of file).");

        // Consume closing quote
        advance();
        addToken(TokenType::QUOTED_STRING, value);
    }

    void Tokenizer::parseNumber(char sign)
    {
        // In normal query mode the sign/digit that started the token has
        // already been consumed. In relation-data mode the cursor may have
        // been rewound to the sign, so consume it here when necessary.
        const bool signWasRewound =
            (sign == '+' || sign == '-') && peek() == sign;

        if (signWasRewound)
        {
            advance();
        }

        const bool firstCharacterWasSign =
            source[start_index] == '+' || source[start_index] == '-';

        if (firstCharacterWasSign &&
            !std::isdigit(static_cast<unsigned char>(peek())))
        {
            throwLexicalError("Expected at least one digit after integer sign.");
        }

        while (std::isdigit(static_cast<unsigned char>(peek())))
        {
            advance();
        }

        std::string lexeme = source.substr(start_index, current_index - start_index);
        addToken(TokenType::INTEGER, lexeme);
    }

    // Normal query mode only.
    //
    // This fixes the bad case:
    //
    //     123abc
    //
    // Without this method, the tokenizer could produce:
    //
    //     INTEGER("123") IDENTIFIER("abc")
    //
    // But outside relation data { ... }, a token like 123abc is not valid.
    // It is a digit-starting identifier-like word, so reject it.
    //
    // Valid:
    //
    //     123
    //
    // Invalid outside { ... }:
    //
    //     123abc
    //     123_abc
    //     123Employees
    void Tokenizer::parseDigitStartedToken()
    {
        bool invalidIdentifierStartedWithDigit = false;

        while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')
        {
            if (std::isalpha(static_cast<unsigned char>(peek())) || peek() == '_')
            {
                invalidIdentifierStartedWithDigit = true;
            }

            advance();
        }

        if (invalidIdentifierStartedWithDigit)
        {
            throwLexicalError("Identifier must start with a letter.");
        }

        std::string lexeme = source.substr(start_index, current_index - start_index);
        addToken(TokenType::INTEGER, lexeme);
    }

    void Tokenizer::parseIdentifierOrKeyword()
    {
        //std::isalnum is a built-in function used to check if a character is alphanumeric
        while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')
        {
            advance();
        }
        std::string text = source.substr(start_index, current_index - start_index);

        // auto  keyword that tells the compiler to automatically deduce the data type
        auto it = keywords.find(text);
        if (it != keywords.end())
        {
            //If it is in the map, it->second extracts the TokenType (e.g., SELECT) and emits that instead.
            addToken(it->second);
        }
        else
        {
            addToken(TokenType::IDENTIFIER, text);
        }
    }

    //This is a highly specialized parser used only when in_relation_data == true.
    //It scoops up raw text (like Bob or Smith) without requiring quotes around them,
    //stopping only when it hits a space, comma, or bracket

    void Tokenizer::parseBareString()
    {
        std::string value = "";
        while (!isAtEnd() && !std::isspace(static_cast<unsigned char>(peek())) &&
            peek() != ',' && peek() != '(' && peek() != ')' &&
            peek() != '\'' && peek() != '{' && peek() != '}')
        {
            value += advance();
        }
        addToken(TokenType::BARE_STRING, value);
    }

    // Relation body mode only.
    //
    // This is used only after the tokenizer has entered { ... }.
    //
    // Inside relation rows, bare values are allowed.
    // Therefore these are valid row values:
    //
    //     123abc
    //     123_abc
    //     Employee_123
    //
    // But outside { ... }, 123abc is rejected by parseDigitStartedToken().
    //
    // This method keeps the full relation value as ONE token.
    // It does not split 123abc into INTEGER("123") and BARE_STRING("abc").
    void Tokenizer::parseRelationDataValue()
    {
        std::string value = "";

        while (!isAtEnd() && !std::isspace(static_cast<unsigned char>(peek())) &&
            peek() != ',' && peek() != '(' && peek() != ')' &&
            peek() != '\'' && peek() != '{' && peek() != '}')
        {
            value += advance();
        }

        bool isInteger = !value.empty();
        size_t index = 0;

        if (index < value.length() && (value[index] == '+' || value[index] == '-'))
        {
            index++;
        }

        if (index >= value.length())
        {
            isInteger = false;
        }

        while (index < value.length())
        {
            if (!std::isdigit(static_cast<unsigned char>(value[index])))
            {
                isInteger = false;
                break;
            }

            index++;
        }

        if (isInteger)
        {
            addToken(TokenType::INTEGER, value);
        }
        else
        {
            addToken(TokenType::BARE_STRING, value);
        }
    }

    // --- Main Loop ---

    void Tokenizer::scanToken()
    {
        skipWhitespaceAndComments();
        if (isAtEnd()) return;

        start_index = current_index;
        start_column = current_column;
        char c = advance();

        // MODE 1: Inside Relation Data { ... }
        //
        // This mode is the only place where digit-starting bare strings
        // like 123abc or 123_abc are allowed.
        if (in_relation_data)
        {
            if (c == '\n')
            {
                // Only emit NEWLINE if the previous token wasn't already a NEWLINE or '{'
                if (!tokens.empty() &&
                    tokens.back().type != TokenType::NEWLINE &&
                    tokens.back().type != TokenType::LBRACE)
                {
                    addToken(TokenType::NEWLINE);
                }
                line++;
                current_column = 1;
                return;
            }

            switch (c)
            {
            case '}':
                addToken(TokenType::RBRACE);
                in_relation_data = false;
                return;

            case '{':
                throwLexicalError("Nested { brackets are not allowed.");

            case ',':
                addToken(TokenType::COMMA);
                return;

            case '(':
                addToken(TokenType::LPAREN);
                return;

            case ')':
                addToken(TokenType::RPAREN);
                return;

            case '\'':
                parseString();
                return;
            }

            // Restore cursor before scanning the complete relation row value.
            //
            // Example inside { ... }:
            //
            //     123abc
            //
            // should become:
            //
            //     BARE_STRING("123abc")
            //
            // not:
            //
            //     INTEGER("123") BARE_STRING("abc")
            current_index--;
            current_column--;

            parseRelationDataValue();
            return;
        }

        // MODE 2: Normal Query Language
        //
        // In this mode, identifiers must start with a letter.
        // Therefore 123abc, 123Name, and 123_abc are invalid here.
        switch (c)
        {
        case '(':
            addToken(TokenType::LPAREN);
            break;

        case ')':
            addToken(TokenType::RPAREN);
            break;

        case '[':
            addToken(TokenType::LBRACKET);
            break;

        case ']':
            addToken(TokenType::RBRACKET);
            break;

        // { opens relation body mode.
        // The parser grammar decides whether this { is valid.
        // The tokenizer only changes scanning behavior after seeing it.
        case '{':
            addToken(TokenType::LBRACE);
            in_relation_data = true;
            break;

        case '}':
            addToken(TokenType::RBRACE);
            break;

        case ',':
            addToken(TokenType::COMMA);
            break;

        case '.':
            addToken(TokenType::DOT);
            break;

        case '=':
            addToken(TokenType::EQUAL);
            break;

        case '!':
            //ternary operator used same as if..else
            if (match('=')) addToken(TokenType::NOT_EQUAL);
            else throwLexicalError("Expected '!=' but found '!'");
            break;

        case '<':
            addToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
            break;

        case '>':
            addToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
            break;

        case '\'':
            parseString();
            break;

        case '+':
        case '-':
            if (std::isdigit(static_cast<unsigned char>(peek()))) parseNumber(c);
            else throwLexicalError("Standalone sign '+' or '-' is not allowed.");
            break;

        default:
            if (std::isdigit(static_cast<unsigned char>(c)))
            {
                parseDigitStartedToken();
            }
            else if (std::isalpha(static_cast<unsigned char>(c)))
            {
                parseIdentifierOrKeyword();
            }
            else
            {
                throwLexicalError(std::string("Unexpected character: ") + c);
            }
            break;
        }
    }
} // query