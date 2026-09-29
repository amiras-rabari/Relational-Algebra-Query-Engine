//
// Created by amira on 2026-09-29.
//

#include <iostream>
#include <vector>
#include <string>
#include <exception>

// Assume your existing headers are included here:
#include "../Token.h"
#include "../Tokenizer.h"
#include "../Parser.h"
#include "../ASTPrinter.h"
#include "../SemanticValidator.h"
#include "../Executioner.h"

class QueryTester {
private:
    query::ASTPrinter printer;

    void printHeader(const std::string& category, const std::string& title) {
        std::cout << "\n======================================================================\n";
        std::cout << "[" << category << "] " << title << "\n";
        std::cout << "======================================================================\n";
    }

public:
    // Executes in treeOnlyMode: No database relations provided.
    void runTreeTest(const std::string& category, const std::string& testName, const std::string& testQuery, bool expectException) {
        printHeader(category, testName);
        std::cout << "Query: " << testQuery << "\n\n";

        try {
            query::Tokenizer lexer(testQuery);
            auto tokens = lexer.tokenizeAll();

            query::Parser my_parser{tokens};
            // Force treeOnlyMode = true since no relations are provided
            query::Parser::ParsedProgram program = my_parser.parseProgram(true);

            if (expectException) {
                std::cout << "[FAIL] Expected an exception but the test parsed successfully.\n";
            } else {
                std::cout << "[OK] Parsed successfully.\n\nAST Tree:\n";
                if (program.query) {
                    printer.printQuery(*program.query);
                }
            }
        }
        catch (const std::exception& e) {
            if (expectException) {
                std::cout << "[OK] Caught expected error:\n" << "Error Output By Prgram:\n" << e.what() << "\n";
            } else {
                std::cout << "[FAIL] Unexpected error:\n" << e.what() << "\n";
            }
        }
    }

    // Executes in normal mode: Database definition provided followed by the query.
    void runExecutionTest(const std::string& category, const std::string& testName, const std::string& dbDefinition, const std::string& testQuery, bool expectException) {
        printHeader(category, testName);

        // Combine relation and query directly as required by your program
        std::string fullInput = dbDefinition + "\n" + testQuery;
        std::cout << "Input:\n" << fullInput << "\n";

        try {
            query::Tokenizer lexer(fullInput);
            auto tokens = lexer.tokenizeAll();

            query::Parser my_parser{tokens};
            // Force treeOnlyMode = false since relations are provided
            query::Parser::ParsedProgram program = my_parser.parseProgram(false);

            query::SemanticValidator semanticValidator;
            query::Database validatedDatabase;

            if (program.database.has_value()) {
                validatedDatabase = semanticValidator.validateDatabase(program.database.value());
                std::cout << "[OK] Semantic relation validation passed.\n";
            }

            if (program.query != nullptr) {
                query::RelationSchema outputSchema = semanticValidator.validateQuery(*program.query);
                std::cout << "[OK] Semantic query validation passed.\n";

                query::Executioner executioner(validatedDatabase);
                query::Table result = executioner.execute(*program.query);

                if (expectException) {
                    std::cout << "[FAIL] Expected an exception but execution succeeded.\n";
                } else {
                    std::cout << "\nResult Table:\n------------------------\n";
                    printer.printTable(result);
                    std::cout << "\nOutput Tuples: " << result.rows.size() << "\n";
                }
            }
        }
        catch (const std::exception& e) {
            if (expectException) {
                std::cout << "[OK] Caught expected error:\n" << e.what() << "\n";
            } else {
                std::cout << "[FAIL] Unexpected error:\n" << e.what() << "\n";
            }
        }
    }
};

int main(int argc, char* argv[]) {
    QueryTester tester;

    // Standard database definitions to reuse across execution tests
    std::string empDeptDB = R"(
Emp (EID, Name, Age, DID, MgrID) = {
  E1, Alice, 30, D1, E3
  E2, Bob, 25, D2, E3
  E3, Carol, 45, D1, E5
  E2, Bob, 25, D2, E3
}
Dept (DID, DName, Budget) = {
  D1, Sales, 100
  D2, Eng, 2000
}
)";

    std::string simpleR = "R (a, b) = { 1, 2 \n 3, 3 }";
    std::string setMathDB = "A (k) = { 1 } \n B (k) = { 1 } \n C (k) = { 1 }";
    std::string mismatchDB = "R (a, b) = { 1, 2 } \n S (c, d, e) = { 1, 2, 3 }";

    // ==========================================================
    // 7.1 Tokenizer (Tree Only Mode - Syntax & Lexing checks)
    // ==========================================================
    tester.runTreeTest("7.1 Tokenizer", "1. No whitespace", "select[x1=3](R)", false);
    tester.runTreeTest("7.1 Tokenizer", "2. Whitespace padded", "select[ x1 = 3 ](R)", false);
    tester.runTreeTest("7.1 Tokenizer", "3. >= operator", "select[Age>=30](R)", false);
    tester.runTreeTest("7.1 Tokenizer", "4. > and - operators", "select[Age>-30](R)", false);
    tester.runTreeTest("7.1 Tokenizer", "5. Parenthesis inside string", "select[Name='Bob)'](R)", false);
    tester.runTreeTest("7.1 Tokenizer", "6. Comma inside string", "select[Name='a,b'](R)", false);
    tester.runTreeTest("7.1 Tokenizer", "7. Doubled quote inside string", "select[Name='O''Brien'](R)", false);
    tester.runTreeTest("7.1 Tokenizer", "8. Attribute spelled like keyword", "select[union=3](R)", false);
    tester.runTreeTest("7.1 Tokenizer", "9. Unclosed string (Lexical Error)", "select[Name='Bob](R)", true);

    // ==========================================================
    // 7.2 Grammar and Precedence (Tree Only Mode)
    // ==========================================================
    tester.runTreeTest("7.2 Grammar", "10. A union B minus C", "A union B minus C", false);
    tester.runTreeTest("7.2 Grammar", "11. A minus B minus C (Associativity)", "A minus B minus C", false);
    tester.runTreeTest("7.2 Grammar", "12. Logical precedence (not, and, or)", "select[not (a=1 and b=2) or c>3](R)", false);
    tester.runTreeTest("7.2 Grammar", "13. Logical precedence (and before or)", "select[a=1 and b=2 or c=3](R)", false);
    tester.runTreeTest("7.2 Grammar", "14. Deep nesting", "project[Name](select[Age>30](select[DID='D1'](Employees)))", false);
    tester.runTreeTest("7.2 Grammar", "15. Explicit parentheses", "(A union B) minus (C intersect D)", false);
    tester.runTreeTest("7.2 Grammar", "16. Missing parenthesis (Syntax Error)", "select[Age>30](R", true);
    tester.runTreeTest("7.2 Grammar", "17. Empty attribute list (Syntax Error)", "project[](R)", true);

    // ==========================================================
    // 7.3 Semantics (Normal Execution Mode - Requires DB)
    // ==========================================================

    tester.runExecutionTest("7.3 Semantics", "18. Compare two columns",
        simpleR, "select[a=b](R)", false);

    tester.runExecutionTest("7.3 Semantics", "19. Qualified Names / Join",
        empDeptDB, "Emp join[Emp.DID=Dept.DID] Dept", false);

    tester.runExecutionTest("7.3 Semantics", "20. Self Join with Rename",
        empDeptDB, "rename[E2](Emp) join[Emp.MgrID=E2.EID] Emp", false);

    tester.runExecutionTest("7.3 Semantics", "21. Union incompatible schemas (Schema Error)",
        mismatchDB, "R union S", true);

    tester.runExecutionTest("7.3 Semantics", "22. Type error (comparing Number to String)",
        empDeptDB, "select[Age>'30'](Emp)", true);

    tester.runExecutionTest("7.3 Semantics", "23. Projection duplicate removal",
        empDeptDB, "project[DID](Emp)", false);

    // Will either fail (duplicate column name in projection not allowed) or pass depending on your documented rule. Set expectException=true if you disallow it.
    tester.runExecutionTest("7.3 Semantics", "24. Duplicate projection columns",
        empDeptDB, "project[Name, Name](Emp)", true);

    tester.runExecutionTest("7.3 Semantics", "25. Query returning no tuples",
        empDeptDB, "select[Age>999](Emp)", false);

    // Bonus for #11: Execution test to prove (A - B) - C associativity data output
    tester.runExecutionTest("7.3 Semantics", "11 Data Proof. (A-B)-C",
        setMathDB, "A minus B minus C", false);

    return 0;
}