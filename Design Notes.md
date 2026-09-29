# Design Notes

**Data Structure Architecture & Design Notes**

• **Stage-Specific Structs:** Most data structures are implemented as explicit struct data types tailored entirely to their respective compilation stages.

• **Token Representation (Standalone):** The `Token` struct exists as a completely standalone, flat entity. Because tokens represent individual lexical units in a sequential stream, they do not require structural nesting or hierarchy.

• **AST Representation (Hierarchical):** In contrast to flat tokens, Abstract Syntax Tree (**AST**) structs inherently implement strict **parent-child relationships**. This recursive, tree-like structure is required to naturally mirror and enforce the grammatical rules and operator precedence of the language syntax.

## Token Subsystem

```ebnf
query (Namespace)
│
├── Tokenizer Function
│    └── tokenize(const std::string& input) ──> std::vector<Token>
│
├── Token (Data Record)
│    ├── Position Metadata
│    │    ├── line: int
│    │    └── column: int
│    │
│    ├── Source Text
│    │    └── lexeme: std::string
│    │
│    ├── Type Classification
│    │    └── type: TokenType (Enum Class)
│    │
│    ├── Value Payload
│    │    └── value: std::variant<std::monostate, int, std::string>
│    │
│    └── Utilities
│         └── print(): void
│
└── TokenType (Enum Class Category Hierarchy)
     ├── Keywords
     │    ├── Relational: SELECT, PROJECT, RENAME, UNION, INTERSECT, MINUS, TIMES, JOIN
     │    └── Logical: AND, OR, NOT
     │
     ├── Dynamic Tokens
     │    ├── IDENTIFIER
     │    ├── INTEGER
     │    ├── QUOTED_STRING
     │    └── BARE_STRING
     │
     ├── Operators
     │    ├── EQUAL (=), NOT_EQUAL (!=)
     │    ├── LESS (<), LESS_EQUAL (<=)
     │    └── GREATER (>), GREATER_EQUAL (>=)
     │
     ├── Punctuation
     │    ├── Grouping: LPAREN ((), RPAREN ()), LBRACKET ([), RBRACKET (]), LBRACE ({), RBRACE (})
     │    └── Delimiters: DOT (.), COMMA (,)
     │
     └── Control Signals
          ├── NEWLINE
          └── END_OF_FILE
```

Token Sequence Representation For Input query  

```
select[Age >= 18]

std::vector<Token>
 ├── [0] Token
 │    ├── type:     TokenType::SELECT
 │    ├── lexeme:   "select"
 │    ├── value:    std::monostate
 │    └── pos:      Line 1, Col 1
 │
 ├── [1] Token
 │    ├── type:     TokenType::LBRACKET
 │    ├── lexeme:   "["
 │    ├── value:    std::monostate
 │    └── pos:      Line 1, Col 7
 │
 ├── [2] Token
 │    ├── type:     TokenType::IDENTIFIER
 │    ├── lexeme:   "Age"
 │    ├── value:    std::string("Age")
 │    └── pos:      Line 1, Col 8
 │
 ├── [3] Token
 │    ├── type:     TokenType::GREATER_EQUAL
 │    ├── lexeme:   ">="
 │    ├── value:    std::monostate
 │    └── pos:      Line 1, Col 12
 │
 ├── [4] Token
 │    ├── type:     TokenType::INTEGER
 │    ├── lexeme:   "18"
 │    ├── value:    int(18)
 │    └── pos:      Line 1, Col 15
 │
 ├── [5] Token
 │    ├── type:     TokenType::RBRACKET
 │    ├── lexeme:   "]"
 │    ├── value:    std::monostate
 │    └── pos:      Line 1, Col 17
 │
 └── [6] Token
      ├── type:     TokenType::END_OF_FILE
      ├── lexeme:   ""
      ├── value:    std::monostate
      └── pos:      Line 1, Col 18
```

Output of Tokenizer for a Query plus Relation definition

```
--- Source Query ---

        // 1. Relation Definition
        Employees (EID, Name, Age, DID) = {
            E1, John, 32, D1

            E2, Alice, 28, D2
        }

        // 2. Query
        project[Name]
(select

[Age>28](Employees))

--- Tokens ---
[Line 3, Col 9] STRING/ID: Employees (Val: Employees)
[Line 3, Col 19] SYMBOL/KEYWORD: (
[Line 3, Col 20] STRING/ID: EID (Val: EID)
[Line 3, Col 23] SYMBOL/KEYWORD: ,
[Line 3, Col 25] STRING/ID: Name (Val: Name)
[Line 3, Col 29] SYMBOL/KEYWORD: ,
[Line 3, Col 31] STRING/ID: Age (Val: Age)
[Line 3, Col 34] SYMBOL/KEYWORD: ,
[Line 3, Col 36] STRING/ID: DID (Val: DID)
[Line 3, Col 39] SYMBOL/KEYWORD: )
[Line 3, Col 41] SYMBOL/KEYWORD: =
[Line 3, Col 43] SYMBOL/KEYWORD: {
[Line 4, Col 13] STRING/ID: E1 (Val: E1)
[Line 4, Col 15] SYMBOL/KEYWORD: ,
[Line 4, Col 17] STRING/ID: John (Val: John)
[Line 4, Col 21] SYMBOL/KEYWORD: ,
[Line 4, Col 23] STRING/ID: 32 (Val: 32)
[Line 4, Col 25] SYMBOL/KEYWORD: ,
[Line 4, Col 27] STRING/ID: D1 (Val: D1)
[Line 4, Col 29] SYMBOL/KEYWORD:

[Line 8, Col 13] STRING/ID: E2 (Val: E2)
[Line 8, Col 15] SYMBOL/KEYWORD: ,
[Line 8, Col 17] STRING/ID: Alice (Val: Alice)
[Line 8, Col 22] SYMBOL/KEYWORD: ,
[Line 8, Col 24] STRING/ID: 28 (Val: 28)
[Line 8, Col 26] SYMBOL/KEYWORD: ,
[Line 8, Col 28] STRING/ID: D2 (Val: D2)
[Line 8, Col 30] SYMBOL/KEYWORD:

[Line 9, Col 9] SYMBOL/KEYWORD: }
[Line 12, Col 9] SYMBOL/KEYWORD: project
[Line 12, Col 16] SYMBOL/KEYWORD: [
[Line 12, Col 17] STRING/ID: Name (Val: Name)
[Line 12, Col 21] SYMBOL/KEYWORD: ]
[Line 13, Col 1] SYMBOL/KEYWORD: (
[Line 13, Col 2] SYMBOL/KEYWORD: select
[Line 15, Col 1] SYMBOL/KEYWORD: [
[Line 15, Col 2] STRING/ID: Age (Val: Age)
[Line 15, Col 5] SYMBOL/KEYWORD: >
[Line 15, Col 6] INTEGER: 28 (Val: 28)
[Line 15, Col 8] SYMBOL/KEYWORD: ]
[Line 15, Col 9] SYMBOL/KEYWORD: (
[Line 15, Col 10] STRING/ID: Employees (Val: Employees)
[Line 15, Col 19] SYMBOL/KEYWORD: )
[Line 15, Col 20] SYMBOL/KEYWORD: )
[Line 16, Col 6] SYMBOL/KEYWORD: EOF
Employees(EID,Name,Age,DID)={E1,John,32,D1
E2,Alice,28,D2
}project[Name](select[Age>28](Employees))EOF
```

## Abstract Syntax Tree (AST)

Used Smart pointer for parent child relationship and virtual destructor that takes care of destruction of child relations as well

**Strict Ownership Semantics (`std::unique_ptr`)**

- Using `std::unique_ptr` for child nodes enforces single-parent ownership.
- When a root node (`QueryNode` or `RelationFileNode`) goes out of scope, the entire sub-tree is destroyed automatically with zero risk of memory leaks.

```
ASTNode (Base Class)
│
├── QueryNode                              (Query Root)
├── RelationFileNode                       (Data File Container)
├── RelationDefinitionNode                 (Data Definition)
├── RelationRowNode                        (Data Row)
├── ValueNode                              (Literal Data Value)
├── AttributeReferenceNode                 (Attribute / Column Reference)
│
├── ExpressionNode                         (Base for Relational Expressions)
│   ├── RelationReferenceNode              (Base Relation Table Reference)
│   ├── BinaryExpressionNode               (Union, Minus, Intersect, Times)
│   ├── JoinNode                           (Theta Join)
│   ├── SelectNode                         (Selection σ)
│   ├── ProjectNode                        (Projection π)
│   └── RenameNode                         (Rename ρ)
│
├── ConditionNode                          (Base for Boolean Conditions)
│   ├── LogicalConditionNode               (And, Or)
│   ├── NotConditionNode                   (Not)
│   └── ComparisonNode                     (=, !=, <, <=, >, >=)
│
└── OperandNode                            (Base for Comparison Operands)
    ├── AttributeOperandNode               (Attribute reference inside condition)
    └── LiteralOperandNode                 (Constant value inside condition)
```

Diagram For Query 

$$
\text{project}_{[\text{Name, Age}]} ( \text{select}_{[\text{Age} \geq 18 \text{ and Status} = \text{'Active'}]} (\text{Employees}) )
$$

```latex
QueryNode
 └── expression
      └── ProjectNode
           ├── attributes
           │    ├── AttributeReferenceNode (name: "Name")
           │    └── AttributeReferenceNode (name: "Age")
           └── expression
                └── SelectNode
                     ├── condition
                     │    └── LogicalConditionNode (op: LogicalOperator::And)
                     │         ├── left
                     │         │    └── ComparisonNode (op: ComparisonOperator::GreaterEqual)
                     │         │         ├── left
                     │         │         │    └── AttributeOperandNode
                     │         │         │         └── reference: AttributeReferenceNode (name: "Age")
                     │         │         └── right
                     │         │              └── LiteralOperandNode (valueKind: Integer, text: "18")
                     │         └── right
                     │              └── ComparisonNode (op: ComparisonOperator::Equal)
                     │                   ├── left
                     │                   │    └── AttributeOperandNode
                     │                   │         └── reference: AttributeReferenceNode (name: "Status")
                     │                   └── right
                     │                        └── LiteralOperandNode (valueKind: QuotedString, text: "'Active'")
                     └── expression
                          └── RelationReferenceNode (name: "Employees")
```

New diagram as per the new standalone query AST remain the same her is one for example

```
(
project[Employees.Name, Departments.DeptName, Locations.City](

//another query for data
select[
        (Employees.Age >= '30' and Employees.Salary > 70000)
        or
        (Employees.Name = 'Alice' and Employees.Age < 30)
    ](
        Employees
        join[Employees.DeptID = Departments.DeptID]
        (
            Departments
            join[Departments.LocationID = Locations.LocationID]
            Locations
        )
    )
))join[e.id>5](
project[E.Name, D.DeptName, L.City, P.ProjectName](
    select[
        (
            (E.Age >= 30 and E.Age <= 45)
            and
            (
                E.DeptID = 10
                or
                E.DeptID = 20
            )
        )
        or
        (
            not (E.Age < 25)
            and
            (
                P.Budget > 12400000
                or
                A.Hours >= 40
            )
        )
        and
        (
            E.Name != Dian
        )
    ](
        (
            (
                (
                    rename[E](Employees)
                    join[E.DeptID = D.DeptID]
                    rename[D](Departments)
                )
                join[D.LocationID = L.LocationID]
                rename[L](Locations)
            )
            join[E.ID = A.EmployeeID]
            rename[A](Assignments)
        )
        join[A.ProjectID = P.ProjectID]

 rename[ih](Projects)
    )
)

) union (b)
```

Output

```
[OK] QueryNode present.
=========================

QueryNode
|____ BinaryExpression (UNION)
      |---- JoinNode
      |     |---- ProjectNode
      |     |     |---- AttributeReference: Employees.Name
      |     |     |---- AttributeReference: Departments.DeptName
      |     |     |---- AttributeReference: Locations.City
      |     |     |____ SelectNode
      |     |           |---- LogicalCondition (OR)
      |     |           |     |---- LogicalCondition (AND)
      |     |           |     |     |---- ComparisonNode (>=)
      |     |           |     |     |     |---- AttributeOperand
      |     |           |     |     |     |     |____ AttributeReference: Employees.Age
      |     |           |     |     |     |____ LiteralOperand (QuotedString): 30
      |     |           |     |     |____ ComparisonNode (>)
      |     |           |     |           |---- AttributeOperand
      |     |           |     |           |     |____ AttributeReference: Employees.Salary
      |     |           |     |           |____ LiteralOperand (Integer): 70000
      |     |           |     |____ LogicalCondition (AND)
      |     |           |           |---- ComparisonNode (=)
      |     |           |           |     |---- AttributeOperand
      |     |           |           |     |     |____ AttributeReference: Employees.Name
      |     |           |           |     |____ LiteralOperand (QuotedString): Alice
      |     |           |           |____ ComparisonNode (<)
      |     |           |                 |---- AttributeOperand
      |     |           |                 |     |____ AttributeReference: Employees.Age
      |     |           |                 |____ LiteralOperand (Integer): 30
      |     |           |____ JoinNode
      |     |                 |---- RelationReference: Employees
      |     |                 |---- ComparisonNode (=)
      |     |                 |     |---- AttributeOperand
      |     |                 |     |     |____ AttributeReference: Employees.DeptID
      |     |                 |     |____ AttributeOperand
      |     |                 |           |____ AttributeReference: Departments.DeptID
      |     |                 |____ JoinNode
      |     |                       |---- RelationReference: Departments
      |     |                       |---- ComparisonNode (=)
      |     |                       |     |---- AttributeOperand
      |     |                       |     |     |____ AttributeReference: Departments.LocationID
      |     |                       |     |____ AttributeOperand
      |     |                       |           |____ AttributeReference: Locations.LocationID
      |     |                       |____ RelationReference: Locations
      |     |---- ComparisonNode (>)
      |     |     |---- AttributeOperand
      |     |     |     |____ AttributeReference: e.id
      |     |     |____ LiteralOperand (Integer): 5
      |     |____ ProjectNode
      |           |---- AttributeReference: E.Name
      |           |---- AttributeReference: D.DeptName
      |           |---- AttributeReference: L.City
      |           |---- AttributeReference: P.ProjectName
      |           |____ SelectNode
      |                 |---- LogicalCondition (OR)
      |                 |     |---- LogicalCondition (AND)
      |                 |     |     |---- LogicalCondition (AND)
      |                 |     |     |     |---- ComparisonNode (>=)
      |                 |     |     |     |     |---- AttributeOperand
      |                 |     |     |     |     |     |____ AttributeReference: E.Age
      |                 |     |     |     |     |____ LiteralOperand (Integer): 30
      |                 |     |     |     |____ ComparisonNode (<=)
      |                 |     |     |           |---- AttributeOperand
      |                 |     |     |           |     |____ AttributeReference: E.Age
      |                 |     |     |           |____ LiteralOperand (Integer): 45
      |                 |     |     |____ LogicalCondition (OR)
      |                 |     |           |---- ComparisonNode (=)
      |                 |     |           |     |---- AttributeOperand
      |                 |     |           |     |     |____ AttributeReference: E.DeptID
      |                 |     |           |     |____ LiteralOperand (Integer): 10
      |                 |     |           |____ ComparisonNode (=)
      |                 |     |                 |---- AttributeOperand
      |                 |     |                 |     |____ AttributeReference: E.DeptID
      |                 |     |                 |____ LiteralOperand (Integer): 20
      |                 |     |____ LogicalCondition (AND)
      |                 |           |---- LogicalCondition (AND)
      |                 |           |     |---- NotConditionNode
      |                 |           |     |     |____ ComparisonNode (<)
      |                 |           |     |           |---- AttributeOperand
      |                 |           |     |           |     |____ AttributeReference: E.Age
      |                 |           |     |           |____ LiteralOperand (Integer): 25
      |                 |           |     |____ LogicalCondition (OR)
      |                 |           |           |---- ComparisonNode (>)
      |                 |           |           |     |---- AttributeOperand
      |                 |           |           |     |     |____ AttributeReference: P.Budget
      |                 |           |           |     |____ LiteralOperand (Integer): 12400000
      |                 |           |           |____ ComparisonNode (>=)
      |                 |           |                 |---- AttributeOperand
      |                 |           |                 |     |____ AttributeReference: A.Hours
      |                 |           |                 |____ LiteralOperand (Integer): 40
      |                 |           |____ ComparisonNode (!=)
      |                 |                 |---- AttributeOperand
      |                 |                 |     |____ AttributeReference: E.Name
      |                 |                 |____ AttributeOperand
      |                 |                       |____ AttributeReference: Dian
      |                 |____ JoinNode
      |                       |---- JoinNode
      |                       |     |---- JoinNode
      |                       |     |     |---- JoinNode
      |                       |     |     |     |---- RenameNode: New relationName = E
      |                       |     |     |     |     |____ RelationReference: Employees
      |                       |     |     |     |---- ComparisonNode (=)
      |                       |     |     |     |     |---- AttributeOperand
      |                       |     |     |     |     |     |____ AttributeReference: E.DeptID
      |                       |     |     |     |     |____ AttributeOperand
      |                       |     |     |     |           |____ AttributeReference: D.DeptID
      |                       |     |     |     |____ RenameNode: New relationName = D
      |                       |     |     |           |____ RelationReference: Departments
      |                       |     |     |---- ComparisonNode (=)
      |                       |     |     |     |---- AttributeOperand
      |                       |     |     |     |     |____ AttributeReference: D.LocationID
      |                       |     |     |     |____ AttributeOperand
      |                       |     |     |           |____ AttributeReference: L.LocationID
      |                       |     |     |____ RenameNode: New relationName = L
      |                       |     |           |____ RelationReference: Locations
      |                       |     |---- ComparisonNode (=)
      |                       |     |     |---- AttributeOperand
      |                       |     |     |     |____ AttributeReference: E.ID
      |                       |     |     |____ AttributeOperand
      |                       |     |           |____ AttributeReference: A.EmployeeID
      |                       |     |____ RenameNode: New relationName = A
      |                       |           |____ RelationReference: Assignments
      |                       |---- ComparisonNode (=)
      |                       |     |---- AttributeOperand
      |                       |     |     |____ AttributeReference: A.ProjectID
      |                       |     |____ AttributeOperand
      |                       |           |____ AttributeReference: P.ProjectID
      |                       |____ RenameNode: New relationName = ih
      |                             |____ RelationReference: Projects
      |____ RelationReference: b
```

## Change of  Design  on 27 Sept

as our program requires to resolve too many tuples like 64k i decide on changing my relation file to a proper database structure using basic struct and a list of tuples and it’s values  since our specification did not require AST for relation.

relation nodes do remain in AST while logic is backed up in case we ever need to implement AST for relation we do have the complete logic ready to go ahead this is the new relational database model and the printer result

```cpp
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
    };

}
#endif //QUERY_PROCESSOR_DATABASE_H

```

Printer

```cpp
   void ASTPrinter::printDatabase(const Database& db) {
        for (const auto& table : db.tables) {
            std::cout << table.name << " (";
            for (size_t i = 0; i < table.attributes.size(); ++i) {
                std::cout << table.attributes[i] << (i < table.attributes.size() - 1 ? ", " : "");
            }
            std::cout << ") =\n";

            for (const auto& row : table.rows) {
                for (const auto& val : row.values) {
                    std::cout << val.text << " ";
                }
                std::cout << "\n";
            }
            std::cout << "\n";
        }
    }
```

printed table with messed up query that passes all parser level cheks

```
Employees   (   ID ,   Name,Age   )   =   {
    1 ,   Research  , 25        // first employee
    2,'Bob'   ,30
    3   , 'Sara',   28        // lots of spaces
}
Departments
(
    DeptID,      DeptName,LocationID
)
=
{

    10, 'Engineering',100     // engineering
    20,Research  ,   500

    30 ,   'Finance' ,300     // finance }

}

Students(
fgff,
StudentName ,
Age,
DeptID )={
    1,'Amira',20,10
    2 , 'John' , 23 ,
20       // student 2
    3,'Sara',21,10
}
R(A) = {
1
2
}

S
(
    B
)
=
{
    2
}

Courses (
    CourseID ,
    CourseName,
    DeptID
) = {
    100 , 'Algorithms',10
    101,'OperatingSystems' , 10
    102 , 'Calculus' ,20
}
//hii
```

output

```
[OK] Database parsed successfully with 6 tables.
Employees (ID, Name, Age) =
1 Research 25
2 Bob 30
3 Sara 28

Departments (DeptID, DeptName, LocationID) =
10 Engineering 100
20 Research 500
30 Finance 300

Students (fgff, StudentName, Age, DeptID) =
1 Amira 20 10
2 John 23 20
3 Sara 21 10

R (A) =
1
2

S (B) =
2

Courses (CourseID, CourseName, DeptID) =
100 Algorithms 10
101 OperatingSystems 10
102 Calculus 20
```

## Importance of small imporvment

look at drastic difference in wall time between 2 tables as the faster had some improvement in code hence it ran faster then other 

new data 

| n | m | comparisons | wall time (s) | output tuples |
| --- | --- | --- | --- | --- |
| 1000 | 1000 | 1,000,000 | [MEASURED] | 1,000 |
| 2000 | 2000 | 4,000,000 | [MEASURED] | 2,000 |
| 4000 | 4000 | 16,000,000 | [MEASURED] | 4,000 |
| 8000 | 8000 | 64,000,000 | [MEASURED] | 8,000 |
| 16000 | 16000 | 256,000,000 | [MEASURED] | 16,000 |
| 32000 | 32000 | 1,024,000,000 | [MEASURED] | 32,000 |
| 64000 | 64000 | 4,096,000,000 | [MEASURED] | 64,000 |

old data 

| N | M | **comparisons** | **wall time (s)** | **output tuples** |
| --- | --- | --- | --- | --- |
| 1000 | 1000 | 1,000,000 | 1.39634 s | 1000 |
| 2000 | 2000 | 4,000,000 | 5.27074 s | 2000 |
| 4000 | 4000 | 16,000,000 | 20.6131 s | 4000 |
| 5000 | 5000 | 25000000 | 38.5787 s | 5000 |
| 8000 | 8000 | 64,000,000 | 1.362 minutes | 8000 |
| 16000 | 16000 | 256,000,000 | 5.904 minutes | 16000 |
| 32000 | 32000 | 1,024,000,000 |  |  |
| 64000 | 64000 | 4,096,000,000 |  |  |

```
Mix of old and new data produced by program

================ PERFORMANCE METRICS ================
Input N (Employees)  : 1000
Input M (Department) : 1000
JOIN Comparisons     : 1000000
SELECT Comparisons   : 0
Output Tuples        : 1000
Wall time (s): 0.624909 s
=====================================================

================ PERFORMANCE METRICS ================
Input N (Employees)  : 500
Input M (Department) : 500
JOIN Comparisons     : 250000
SELECT Comparisons   : 0
Output Tuples        : 500
Wall time (s): 0.337442 s
=====================================================

================ PERFORMANCE METRICS ================
Input N (Employees)  : 2000
Input M (Department) : 2000
JOIN Comparisons     : 4000000
SELECT Comparisons   : 0
Output Tuples        : 2000
Wall time (s): 2.50862 s
=====================================================

================ PERFORMANCE METRICS ================
Input N (Employees)  : 3000
Input M (Department) : 3000
JOIN Comparisons     : 9000000
SELECT Comparisons   : 0
Output Tuples        : 3000
Wall time (s): 5.62423 s
=====================================================

================ PERFORMANCE METRICS ================
Input N (Employees)  : 4000
Input M (Department) : 4000
JOIN Comparisons     : 16000000
SELECT Comparisons   : 0
Output Tuples        : 4000
Wall time (s): 10.6842 s
=====================================================

================ PERFORMANCE METRICS ================
Input N (Employees)  : 6000
Input M (Department) : 6000
JOIN Comparisons     : 36000000
SELECT Comparisons   : 0
Output Tuples        : 6000
Wall time (s): 22.0414 s
======================================================

================ PERFORMANCE METRICS ================
Input N (Employees)  : 8000
Input M (Department) : 8000
JOIN Comparisons     : 64000000
SELECT Comparisons   : 0
Output Tuples        : 8000
Wall time (s): 81.7355 s
=====================================================

================ PERFORMANCE METRICS ================
Input N (Employees)  : 16000
Input M (Department) : 16000
JOIN Comparisons     : 256000000
SELECT Comparisons   : 0
Output Tuples        : 16000
Wall time (s): 354.291 s
=====================================================

================ PERFORMANCE METRICS ================
Input N (Employees)  : 16000
Input M (Department) : 16000
JOIN Comparisons     : 256000000
SELECT Comparisons   : 0
Output Tuples        : 16000
Wall time (s): 340.888 s
=====================================================

old data of 32k
================ PERFORMANCE METRICS ================
Input N (Employees)  : 32000
Input M (Department) : 32000
JOIN Comparisons     : 1024000000
SELECT Comparisons   : 0
Output Tuples        : 32000
Wall time (s): 1513.64 s
=====================================================

new data of 32k

================ PERFORMANCE METRICS ================
Input N (Employees)  : 32000
Input M (Department) : 32000
JOIN Comparisons     : 1024000000
SELECT Comparisons   : 0
Output Tuples        : 32000
Wall time (s): 691.663 s
=====================================================

new data of 16k

================ PERFORMANCE METRICS ================
Input N (Employees)  : 16000
Input M (Department) : 16000
JOIN Comparisons     : 256000000
SELECT Comparisons   : 0
Output Tuples        : 16000
Wall time (s): 155.227 s
=====================================================

================ PERFORMANCE METRICS ================
Input N (Employees)  : 64000
Input M (Department) : 64000
JOIN Comparisons     : 4096000000
SELECT Comparisons   : 0
Output Tuples        : 64000
Wall time (s): 2857.81 s
=====================================================
```