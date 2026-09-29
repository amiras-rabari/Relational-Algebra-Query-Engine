#ifndef QUERY_PROCESSOR_PARSER_H
#define QUERY_PROCESSOR_PARSER_H

#pragma once

#include "AST.h"
#include "Database.h"
#include "ParserContext.h"

#include <memory>
#include <optional>
#include <vector>

namespace query
{
    class Parser
    {
    public:
        explicit Parser(std::vector<Token> tokens);

        struct ParsedProgram
        {
            std::optional<Database> database;
            std::unique_ptr<QueryNode> query;
        };

        void runTests();

        ParsedProgram parseProgram(bool treeOnlyMode);

        // Compatibility wrappers for existing callers.
        Database parseRelationFile();
        std::unique_ptr<QueryNode> parseQuery();

    private:
        ParserContext context;

        void printAST(ParsedProgram& program);
    };
}

#endif // QUERY_PROCESSOR_PARSER_H