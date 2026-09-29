//
// Created by amira on 2026-09-27.
//

#ifndef QUERY_PROCESSOR_DATABASE_H
#define QUERY_PROCESSOR_DATABASE_H
#include <string>
#include <vector>

#pragma once
namespace query
{

    // ============================================================
    // High-Performance Runtime Data Model
    // ============================================================
    // These replace the old AST data nodes. They use contiguous
    // memory and avoid pointer/virtual overhead for max performance.

    enum class dbValueKind
    {
        Integer,
        QuotedString,
        BareString
    };


    struct DataValue {
        dbValueKind kind;
        std::string text;
    };

    struct Tuple {
        std::vector<DataValue> values;
    };
    // for future use if needed to define the type of attributes beforehand
     struct attribute {
         std::string name;
         dbValueKind kind;
     };

    struct Table {
        std::string name;
        std::vector<std::string> attributes;
        std::vector<Tuple> rows;
    };

    struct Database {
        std::vector<Table> tables;

        // Helper to find a table by name
        const Table* getTable(const std::string& tableName) const {
            for (const auto& table : tables) {
                if (table.name == tableName) {
                    return &table;
                }
            }
            return nullptr;
        }

        const double getTableRowCount(const std::string& tableName) const {
            const Table* table = getTable(tableName);
            if (table != nullptr) {
                return table->rows.size();
            }
            return 0;
        }
    };


}
#endif //QUERY_PROCESSOR_DATABASE_H
