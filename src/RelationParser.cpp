#include "RelationParser.h"

#include <string>
#include <utility>

namespace query
{
    RelationParser::RelationParser(ParserContext& context)
        : context(context)
    {
    }

    // ============================================================
    // Exact relation start
    // ============================================================
    //
    // relation_definition
    //     ::= relation_name format_newlines
    //         "(" ...
    //
    // relation_name
    //     ::= identifier
    //
    // Therefore a relation begins exactly with:
    //
    //     IDENTIFIER LPAREN
    //
    // We do NOT try to guess malformed relations here.
    //
    bool RelationParser::isRelationDefinitionStart() const
    {
        if (!context.check(TokenType::IDENTIFIER))
            return false;

        size_t offset = 1;

        while (context.peekAhead(offset).type == TokenType::NEWLINE)
            ++offset;

        return context.peekAhead(offset).type == TokenType::LPAREN;
    }


    bool RelationParser::isUnnamedRelationDefinitionStart() const
    {
        if (!context.check(TokenType::LPAREN))
            return false;

        size_t offset = 1;

        while (!context.isAtEnd())
        {
            TokenType type = context.peekAhead(offset).type;

            if (type == TokenType::RPAREN)
            {
                offset++;
                break;
            }

            if (type == TokenType::LBRACE ||
                type == TokenType::RBRACE ||
                type == TokenType::EQUAL ||
                type == TokenType::END_OF_FILE)
            {
                return false;
            }

            offset++;
        }

        if (context.peekAhead(offset - 1).type != TokenType::RPAREN)
            return false;

        while (context.peekAhead(offset).type == TokenType::NEWLINE)
            offset++;

        if (context.peekAhead(offset).type != TokenType::EQUAL)
            return false;

        offset++;

        while (context.peekAhead(offset).type == TokenType::NEWLINE)
            offset++;

        return context.peekAhead(offset).type == TokenType::LBRACE;
    }

    // ============================================================
    // Value start
    // ============================================================
    //
    // value
    //     ::= integer
    //      |  quoted_string
    //      |  bare_string
    //
    bool RelationParser::isValueStart() const
    {
        return context.check(TokenType::INTEGER) ||
               context.check(TokenType::QUOTED_STRING) ||
               context.check(TokenType::BARE_STRING);
    }

    // ============================================================
    // Formatting newlines
    // ============================================================
    //
    // format_newlines ::= { NEWLINE }
    //
    // These newlines are purely formatting.
    //
    void RelationParser::skipNewlines()
    {
        while (context.match({TokenType::NEWLINE}))
        {
            // Consume formatting newlines.
        }
    }

    // ============================================================
    // Detect comma after formatting newlines
    // ============================================================
    //
    // This is the important ambiguity inside tuple_line.
    //
    // Because we allow:
    //
    //     value
    //     ,
    //     value
    //
    // we cannot simply consume NEWLINE after every value.
    //
    // Example:
    //
    //     1,
    //     2
    //
    // The newline belongs to tuple formatting.
    //
    // But:
    //
    //     1
    //     2
    //
    // The newline is a row separator.
    //
    // Therefore we look ahead before consuming the newline.
    //
    bool RelationParser::hasCommaAfterNewlines() const
    {
        size_t offset = 0;

        while (context.peekAhead(offset).type == TokenType::NEWLINE)
        {
            ++offset;
        }

        return context.peekAhead(offset).type == TokenType::COMMA;
    }

    bool RelationParser::parseRelationRowEnd()
    {
        // ------------------------------------------------------------
        // Case 1:
        //
        // The row is followed directly by the closing relation body.
        //
        // Example:
        //
        //     1, Bob, 30}
        //
        // or normally:
        //
        //     1, Bob, 30
        //     }
        //
        // The actual RBRACE is not consumed here. Stage 14 consumes it.
        // ------------------------------------------------------------
        if (context.check(TokenType::RBRACE))
        {
            return true;
        }

        // ------------------------------------------------------------
        // Case 2:
        //
        // Another value starts immediately after the tuple_line.
        //
        // Example:
        //
        //     1, Amira Ali, 25
        //
        // Token-wise this becomes:
        //
        //     INTEGER(1), COMMA, BARE_STRING(Amira), BARE_STRING(Ali), ...
        //
        // That is not an arity problem first. It is a syntax problem:
        // missing comma, or a bare string with whitespace that should
        // have been quoted.
        // ------------------------------------------------------------
        if (isValueStart())
        {
            throwParseError(
                context.peek(),
                "Expected ',' between tuple values or newline between tuple rows. "
                "Bare strings cannot contain whitespace; use single quotes for strings with spaces."
            );
        }

        // ------------------------------------------------------------
        // Case 3:
        //
        // A row_separator is required if the relation did not close.
        //
        // row_separator ::= NEWLINE { NEWLINE }
        // ------------------------------------------------------------
        if (!context.check(TokenType::NEWLINE))
        {
            throwParseError(
                context.peek(),
                "Expected newline after tuple row or '}' to close relation."
            );
        }

        // Consume row_separator.
        skipNewlines();

        // ------------------------------------------------------------
        // Case 4:
        //
        // Trailing row_separator before '}' is allowed.
        //
        // relation_rows ::= tuple_line { row_separator tuple_line }
        //                   [ row_separator ]
        // ------------------------------------------------------------
        if (context.check(TokenType::RBRACE))
        {
            return true;
        }

        // ------------------------------------------------------------
        // Case 5:
        //
        // After a row separator, either another tuple_line must start
        // or the relation body must close. Since '}' was already checked,
        // the only valid option left is another value.
        // ------------------------------------------------------------
        if (!isValueStart())
        {
            throwParseError(
                context.peek(),
                "Expected tuple row or '}' after row separator."
            );
        }

        return false;
    }

    // ============================================================
    // relation_file
    // ============================================================
    //
    // relation_file
    //     ::= relation_definition { relation_definition }
    //
    // At least one relation is required when RelationParser is
    // called.
    //
    // After a complete relation:
    //
    //     IDENTIFIER LPAREN -> another relation
    //
    // otherwise:
    //
    //     stop and leave the token for QueryParser.
    //
    Database RelationParser::parseRelationFile()
    {
        Database db;

        // --------------------------------------------------------
        // Formatting before first relation
        // --------------------------------------------------------
        skipNewlines();

        // --------------------------------------------------------
        // At least one relation is required
        // --------------------------------------------------------
        if (context.isAtEnd())
        {
            throwParseError(
                context.peek(),
                "Expected at least one relation definition."
            );
        }

        // A relation MUST begin with a relation name.
        // Therefore '(' cannot be the first token of a relation.
        if (context.check(TokenType::LPAREN))
        {
            throwParseError(
                context.peek(),
                "Expected relation name before '('."
            );
        }

        if (!isRelationDefinitionStart())
        {
            throwParseError(
                context.peek(),
                "Expected relation definition beginning with "
                "relation_name '('."
            );
        }

        // --------------------------------------------------------
        // Parse consecutive relations
        // --------------------------------------------------------
        while (isRelationDefinitionStart())
        {
            db.tables.push_back(
                parseRelationDefinition()
            );

            // Formatting newlines between relations.
            skipNewlines();

            if (context.check(TokenType::RBRACE))
            {
                throwParseError(
                    context.peek(),
                    "Unexpected extra closing brace '}'."
                );
            }

            if (isUnnamedRelationDefinitionStart())
            {
                throwParseError(
                    context.peek(),
                    "Expected relation name before attribute list."
                );
            }
        }

        // Do NOT consume anything belonging to the query.
        return db;
    }

    // ============================================================
    // relation_definition
    // ============================================================
    //
    // relation_definition
    //     ::= relation_name format_newlines
    //         "(" format_newlines
    //         relation_attribute_list format_newlines
    //         ")"
    //         format_newlines "=" format_newlines
    //         "{"
    //         format_newlines
    //         [ relation_rows ]
    //         format_newlines "}"
    //
    Table RelationParser::parseRelationDefinition()
    {
        Table table;

        // ========================================================
        // Stage 1: relation_name
        // ========================================================
        //
        // relation_name ::= identifier
        //
        Token relationNameToken = context.consume(
            TokenType::IDENTIFIER,
            "Expected relation name identifier."
        );

        table.name = relationNameToken.lexeme;

        // ========================================================
        // Stage 2: formatting newline before "("
        // ========================================================
        skipNewlines();

        // ========================================================
        // Stage 3: "("
        // ========================================================
        context.consume(
            TokenType::LPAREN,
            "Expected '(' after relation name '" +
            table.name +
            "'."
        );

        // ========================================================
        // Stage 4: formatting newline after "("
        // ========================================================
        skipNewlines();

        // ========================================================
        // Stage 5: first attribute
        // ========================================================
        //
        // relation_attribute_list requires at least one attribute.
        //
        if (!context.checkAttributeName())
        {
            throwParseError(
                context.peek(),
                "Expected attribute name after '(' it must be a string with no '' starting with char always eg.abc_123."
            );
        }

        table.attributes.push_back(
            context.consumeAttributeName(
                "Expected attribute name after '('."
            )
        );

        // ========================================================
        // Stage 6: remaining attributes / closing ")"
        // ========================================================
        //
        // After an attribute:
        //
        //     formatting newlines + "," + attribute
        //
        // or:
        //
        //     formatting newlines + ")"
        //
        while (true)
        {
            // Newlines between an attribute and the next structural
            // token are formatting.
            skipNewlines();

            // ----------------------------------------------------
            // Another attribute
            // ----------------------------------------------------
            if (context.match({TokenType::COMMA}))
            {
                skipNewlines();

                if (!context.checkAttributeName())
                {
                    throwParseError(
                        context.peek(),
                        "Expected attribute name after ','."
                    );
                }

                table.attributes.push_back(
                    context.consumeAttributeName(
                        "Expected attribute name after ','."
                    )
                );

                continue;
            }

            // ----------------------------------------------------
            // Attribute list finished
            // ----------------------------------------------------
            if (context.check(TokenType::RPAREN))
            {
                break;
            }

            throwParseError(
                context.peek(),
                "Expected ',' or ')' after attribute '" +
                table.attributes.back() +
                "'."
            );
        }

        // ========================================================
        // Stage 7: ")"
        // ========================================================
        context.consume(
            TokenType::RPAREN,
            "Expected ')' after relation attribute list."
        );

        // ========================================================
        // Stage 8: formatting newline before "="
        // ========================================================
        skipNewlines();

        // ========================================================
        // Stage 9: "="
        // ========================================================
        context.consume(
            TokenType::EQUAL,
            "Expected '=' after relation attribute list."
        );

        // ========================================================
        // Stage 10: formatting newline before "{"
        // ========================================================
        skipNewlines();

        // ========================================================
        // Stage 11: "{"
        // ========================================================
        context.consume(
            TokenType::LBRACE,
            "Expected '{' after '=' to open relation body."
        );

        // ========================================================
        // Stage 12: formatting newlines after "{"
        // ========================================================
        skipNewlines();

        // ========================================================
        // Empty relation
        // ========================================================
        //
        // This is valid because relation_rows is optional.
        //
        //     R(A, B) = {
        //     }
        //
        if (context.check(TokenType::RBRACE))
        {
            context.consume(
                TokenType::RBRACE,
                "Expected '}' after relation body."
            );

            return table;
        }

        // ========================================================
        // Stage 13: first tuple
        // ========================================================
        if (!isValueStart())
        {
            throwParseError(
                context.peek(),
                "Expected tuple row or '}' after opening relation body."
            );
        }

        while (true)
        {
            // ----------------------------------------------------
            // Stage 13a: parse tuple_line
            // ----------------------------------------------------
            Token rowStartToken = context.peek();

            Tuple row = parseRelationRow();

            // ----------------------------------------------------
            // Stage 13b: validate row boundary BEFORE arity
            // ----------------------------------------------------
            //
            // This order is important.
            //
            // Example:
            //
            //     1, Amira Ali, 25
            //
            // should report a syntax error about a missing comma
            // or whitespace inside a bare string, not an arity error.
            //
            bool relationEndsAfterThisRow = parseRelationRowEnd();

            // ----------------------------------------------------
            // Stage 13c: arity validation
            // ----------------------------------------------------
            //
            // This is NOT grammar syntax.
            //
            // It verifies:
            //
            //     number of values == number of attributes
            //
            // It must happen only after the row boundary is known
            // to be syntactically valid.
            //
            if (row.values.size() != table.attributes.size())
            {
                throwParseError(
                    rowStartToken,
                    "Tuple value count (" +
                    std::to_string(row.values.size()) +
                    ") does not match schema attribute count (" +
                    std::to_string(table.attributes.size()) +
                    ") for relation '" +
                    table.name +
                    "'."
                );
            }

            table.rows.push_back(
                std::move(row)
            );

            // ----------------------------------------------------
            // Stage 13d: relation body finished or next row
            // ----------------------------------------------------
            if (relationEndsAfterThisRow)
            {
                break;
            }

            // Otherwise parse the next tuple_line.
            // parseRelationRowEnd() already verified that the next
            // token is a valid value start.
        }

        // ========================================================
        // Stage 14: "}"
        // ========================================================
        context.consume(
            TokenType::RBRACE,
            "Expected '}' after relation body."
        );

        return table;
    }

    // ============================================================
    // tuple_line
    // ============================================================
    //
    // tuple_line
    //     ::= value
    //         { format_newlines "," format_newlines value }
    //
    // The important rule is:
    //
    //     newline + comma => same tuple
    //
    //     newline + value => next tuple
    //
    // We determine this using hasCommaAfterNewlines().
    //
    Tuple RelationParser::parseRelationRow()
    {
        Tuple row;

        // --------------------------------------------------------
        // First value is mandatory.
        // --------------------------------------------------------
        if (!isValueStart())
        {
            throwParseError(
                context.peek(),
                "Expected value at beginning of tuple row."
            );
        }

        row.values.push_back(
            parseValue()
        );

        // --------------------------------------------------------
        // Additional values
        // --------------------------------------------------------
        while (true)
        {
            // ----------------------------------------------------
            // Case 1:
            //
            // Current token is comma.
            //
            // Example:
            //
            //     1, 2
            // ----------------------------------------------------
            //
            // Case 2:
            //
            // Current token is newline(s), and after them there
            // is a comma.
            //
            // Example:
            //
            //     1
            //     ,
            //     2
            //
            // Both mean the tuple is continuing.
            // ----------------------------------------------------

            if (context.check(TokenType::COMMA))
            {
                context.consume(
                    TokenType::COMMA,
                    "Expected ',' between tuple values."
                );
            }




            else
            {
                if (!hasCommaAfterNewlines())
                {
                    // No comma after the newline(s).
                    //
                    // Therefore those NEWLINEs are the row
                    // separator and MUST remain unconsumed.
                    break;
                }

                // Consume formatting newlines before comma.
                skipNewlines();

                context.consume(
                    TokenType::COMMA,
                    "Expected ',' between tuple values."
                );
            }

            // ----------------------------------------------------
            // Formatting newlines after comma are allowed.
            // ----------------------------------------------------
            skipNewlines();

            // ----------------------------------------------------
            // A comma requires another value.
            // ----------------------------------------------------
            if (!isValueStart())
            {
                throwParseError(
                    context.peek(),
                    "Expected value after ','."
                );
            }

            row.values.push_back(
                parseValue()
            );
        }

        return row;
    }

    // ============================================================
    // value
    // ============================================================
    //
    // value
    //     ::= integer
    //      |  quoted_string
    //      |  bare_string
    //
    DataValue RelationParser::parseValue()
    {
        if (!isValueStart())
        {
            throwParseError(
                context.peek(),
                "Expected value "
                "(integer, quoted string, or bare string)."
            );
        }

        Token token = context.advance();

        if (token.type == TokenType::INTEGER)
        {
            return DataValue{
                dbValueKind::Integer,
                token.lexeme
            };
        }

        if (token.type == TokenType::QUOTED_STRING)
        {
            return DataValue{
                dbValueKind::QuotedString,
                std::get<std::string>(token.value)
            };
        }

        // TokenType::BARE_STRING
        return DataValue{
            dbValueKind::BareString,
            std::get<std::string>(token.value)
        };
    }
}