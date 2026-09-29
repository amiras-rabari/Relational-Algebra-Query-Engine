#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include "ASTPrinter.h"
#include "Executioner.h"
#include "Parser.h"
#include "SemanticValidator.h"
#include "Tokenizer.h"
#include <exception>
#include "Token.h"
#include "AST.h"




int main(int argc, char* argv[])
{
    bool treeOnlyMode = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--tree")
        {
            treeOnlyMode = true;
        }
    }

    std::cout << "======================================================================\n";
    std::cout << " Mode: " << (treeOnlyMode ? "Tree-Only Mode" : "Normal Mode (Database + Query)") << "\n";
    std::cout << "======================================================================\n";
    std::cout << (treeOnlyMode ? "Enter Query Only" :"Enter your input (Relations + Query). You can use multiple lines.")<<"\n";
    std::cout << "Enter and then Type'END' on newline or put a ';' at the end of query when finished either should work:\n";
    std::cout << "----------------------------------------------------------------------\n\n";

    std::string full_input;
    std::string line;

    while (std::getline(std::cin, line))
    {
        // Trim trailing spaces/carriage returns
        std::string trimmed = line;
        trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);

        // Check for keyword "END" / "end" on a new line
        if (trimmed == "END" || trimmed == "end")
        {
            break;
        }

        full_input += line + "\n";

        // Check if the current line ends with a semicolon ';'
        if (!trimmed.empty() && trimmed.back() == ';')
        {
            // Remove the trailing semicolon if your grammar doesn't expect it
            full_input.pop_back(); // removes '\n'
            if (!full_input.empty() && full_input.back() == ';')
            {
                full_input.pop_back();
            }
            full_input += "\n";
            break;
        }
    }

    if (full_input.empty())
    {
        std::cerr << "Error: No input provided.\n";
        return 1;
    }

    try
    {
        query::Tokenizer lexer(full_input);
        std::vector<query::Token> tokens = lexer.tokenizeAll();

        query::ASTPrinter printer;
        query::Parser my_parser{tokens};
        query::Parser::ParsedProgram program = my_parser.parseProgram(treeOnlyMode);

        // --- TREE ONLY MODE ---
        if (treeOnlyMode)
        {
            if (program.query == nullptr)
            {
                std::cerr << "Error: No valid query found to display AST.\n";
                return 1;
            }

            std::cout <<
                "-----------------------------------------------------------------------------------------------------------------\n";
            std::cout << "AST tree  \n";
            printer.printQuery(*program.query);
            return 0;
        }

        // --- NORMAL EXECUTION MODE ---
        query::SemanticValidator semanticValidator;
        query::Database validatedDatabase;

        if (program.database.has_value())
        {
            validatedDatabase = semanticValidator.validateDatabase(
                program.database.value()
            );

            std::cout << "[OK] Semantic relation validation passed.\n";
            std::cout << "[OK] Duplicate tuples removed using set semantics.\n";
        }

        if (program.query != nullptr)
        {
            query::RelationSchema outputSchema =
                semanticValidator.validateQuery(*program.query);

            std::cout << "[OK] Semantic query validation passed.\n";
            std::cout <<
                "-----------------------------------------------------------------------------------------------------------------\n";

            std::cout << "Output schema: "
                << outputSchema.relationName
                << " (";

            for (size_t i = 0; i < outputSchema.attributes.size(); ++i)
            {
                const query::SemanticAttribute& attribute =
                    outputSchema.attributes[i];

                std::cout << attribute.relationName
                    << "."
                    << attribute.name
                    << ":";

                if (attribute.type == query::SemanticValueType::Number)
                {
                    std::cout << "number";
                }
                else
                {
                    std::cout << "string";
                }

                if (i + 1 < outputSchema.attributes.size())
                {
                    std::cout << ", ";
                }
            }
            std::cout << ")\n";
        }
        else
        {
            std::cerr << "Error: No query supplied for execution.\n";
            return 1;
        }

        query::Executioner executioner(validatedDatabase);
        query::Table result = executioner.execute(*program.query);

        std::cout << "\nResult of the Query\n";
        std::cout << "------------------------\n";
        printer.printTable(result);
        std::cout << "JOIN Comparisons     : " << executioner.getJoinComparisonCount() << "\n";
        std::cout << "SELECT Comparisons   : " << executioner.getSelectTupleCount() << "\n";
        std::cout << "Output Tuples        : " << result.rows.size() << "\n";
        std::cout <<
            "-----------------------------------------------------------------------------------------------------------------\n";
        std::cout << "AST tree  \n";
        std::cout << "-------------\n";
        printer.printQuery(*program.query);

        std::cout <<
            "-----------------------------------------------------------------------------------------------------------------\n";
        std::cout << "Database Schema \n";
        std::cout << "----------------\n";
        printer.printDatabase(validatedDatabase);
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n" << e.what() << "\n";
        return 1;
    }

    return 0;
}
