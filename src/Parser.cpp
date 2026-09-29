#include "Parser.h"

#include "ASTPrinter.h"
#include "QueryParser.h"
#include "RelationParser.h"

#include <iostream>
#include <utility>

namespace query
{
    Parser::Parser(std::vector<Token> inputTokens)
        : context(std::move(inputTokens))
    {
    }



    Parser::ParsedProgram Parser::parseProgram(bool treeOnlyMode)
    {
        ParsedProgram program;

        // ----------------------------------------
        // 1. Relations
        // ----------------------------------------
        if (!treeOnlyMode)
        {
            program.database = parseRelationFile();

            // std::cout
            //     << "After parseRelationFile(): "
            //     << context.peek().lexeme
            //     << '\n';
        }

        // ----------------------------------------
        // 2. Query
        // ----------------------------------------
        program.query = parseQuery();

        // std::cout
        //     << "After parseQuery(): "
        //     << context.peek().lexeme
        //     << '\n';

        // ----------------------------------------
        // 3. Printing / status
        // ----------------------------------------


        return program;
    }

    Database Parser::parseRelationFile()
    {
        RelationParser relationParser(context);

        return relationParser.parseRelationFile();
    }

    std::unique_ptr<QueryNode> Parser::parseQuery()
    {
        QueryParser queryParser(context);

        return queryParser.parseQuery();
    }

    void Parser::printAST(ParsedProgram& program)
    {
        ASTPrinter printer(std::cout);
        std::cout << "=== AST Status Checks ===\n";

        if (program.database.has_value())
        {
            std::cout
                << "[OK] Database parsed successfully with "
                << program.database->tables.size()
                << " tables.\n";

            // printer.printDatabase(*program.database);
        }
        else
        {
            std::cout
                << "[Info] No relation file present (Tree-only mode).\n";
        }

        if (program.query != nullptr)
        {
            if (program.query->kind == NodeKind::Query)
            {
                std::cout
                    << "[OK] QueryNode present.\n";
            }

            if (program.query->expression == nullptr)
            {
                std::cout
                    << "[Warning] QueryNode exists but expression is empty!\n";
            }
        }
        else
        {
            std::cout
                << "[Info] No QueryNode present.\n";
        }

        std::cout
            << "=========================\n\n";



        // printer.printQuery(*program.query);

    }
}