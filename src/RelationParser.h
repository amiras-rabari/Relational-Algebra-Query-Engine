#ifndef QUERY_PROCESSOR_RELATION_PARSER_H
#define QUERY_PROCESSOR_RELATION_PARSER_H

#pragma once

#include "Database.h"
#include "ParserContext.h"

namespace query
{
    class RelationParser
    {
    public:
        explicit RelationParser(ParserContext& context);

        Database parseRelationFile();

    private:
        ParserContext& context;

        // ------------------------------------------------------------
        // Relation grammar
        // ------------------------------------------------------------

        Table parseRelationDefinition();
        Tuple parseRelationRow();
        DataValue parseValue();

        // Validates what comes after a tuple_line.
        //
        // Returns true if the relation body is closed by '}'.
        // Returns false if a valid row_separator was consumed and
        // another tuple row should follow.
        bool parseRelationRowEnd();

        // ------------------------------------------------------------
        // Relation-specific checks/helpers
        // ------------------------------------------------------------

        // Exact beginning of a relation:
        //
        //     IDENTIFIER LPAREN
        //
        bool isRelationDefinitionStart() const;

        bool isUnnamedRelationDefinitionStart() const;
        // value ::= integer | quoted_string | bare_string
        bool isValueStart() const;

        // Formatting newlines are allowed around structural syntax.
        void skipNewlines();

        // Used only while parsing a tuple.
        //
        // If the current token is a NEWLINE, determine whether,
        // after some number of NEWLINEs, a COMMA appears.
        //
        // This is necessary because:
        //
        //     1,\n25      -> same tuple
        //
        // while:
        //
        //     1\n25       -> two tuples
        //
        bool hasCommaAfterNewlines() const;
    };
}

#endif // QUERY_PROCESSOR_RELATION_PARSER_H