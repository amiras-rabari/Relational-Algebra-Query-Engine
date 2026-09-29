#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#pragma once

#include "AST.h"
#include "Database.h"
#include <iostream>
#include <string>

namespace query
{
    class ASTPrinter
    {
    public:
        // Binds output stream (defaults to std::cout)
        // Accepts a reference to any C++ output stream (like std::cout, a file stream std::ofstream, or a string stream std::ostringstream)
        explicit ASTPrinter(std::ostream& out = std::cout);

        // Master entry point: prints both relation schema/data and query AST
        void printAll(const RelationFileNode* relFile, const QueryNode* query);

        // Individual entry points
        void printQuery(const QueryNode& query);
        void printDatabase(const Database& db);
        void printTable(Table table);
        void printRelationFile(const RelationFileNode& file);

    private:
        std::ostream& out_;

        // Recursive tree traversal dispatcher
        void printNode(const ASTNode* node, const std::string& prefix, bool isLast);

        // Specialized helpers for schema and raw data formatted views
        void printRelationDefinition(const RelationDefinitionNode& def, const std::string& prefix, bool isLast);
        void printRelationRow(const RelationRowNode& row, const std::string& prefix, bool isLast);

        // Enum-to-String converters
        static std::string binaryOpToString(BinaryOperator op);
        static std::string logicalOpToString(LogicalOperator op);
        static std::string comparisonOpToString(ComparisonOperator op);
        static std::string valueKindToString(ValueKind kind);
    };
} // namespace query

#endif // AST_PRINTER_H