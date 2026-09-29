# Relational Algebra Query Processor

A relational algebra interpreter written in C++. It reads relation definitions and one query, checks that the query is valid, and evaluates it against the relations held in memory. It can also print the parse tree of a query without running it.

The formal grammar, precedence table and the ambiguity demonstration are in `GRAMMAR.md`. The performance study is in `REPORT.md`. This file explains how to use the program and what input it accepts.

## Contents

1. Build
2. Running the program
3. Regular mode: input structure
4. Relation definitions
5. Queries
6. Semantics and output schemas
7. Tree-only mode (`-tree`)
8. Errors
9. Tokenizer cheat sheet
10. Worked examples
11. Data generator and instrumentation
12. Why a self join needs `rename`
13. Known limitations

---

## 1. Build

The project is built with CMake (CLion works directly). Build in **Release** mode. A Debug build is many times slower and will distort every timing in `REPORT.md`.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Or If using any Linux system simply copy all *.cpp and *.h files into local folder and make sure you are inside that folder and  run this command in terminal to compile and build 

```latex
Note: DataGenerator is a Tool used only for Report as Current main() of src takes user
input and not an input file hence Modiy main() to accept datagenarator text file if needed

PROJECT STRUCTURE
=================

/home/student/Project/
|
|-- Makefile
|
|-- src/
|   |-- main.cpp
|   |-- Token.h
|   |-- Tokenizer.cpp
|   |-- Tokenizer.h
|   |-- AST.h
|   |-- Parser.cpp
|   |-- Parser.h
|   |-- ASTPrinter.cpp
|   |-- ASTPrinter.h
|   |-- Database.h
|   |-- QueryParser.cpp
|   |-- QueryParser.h
|   |-- RelationParser.cpp
|   |-- RelationParser.h
|   |-- ParserContext.cpp
|   |-- ParserContext.h
|   |-- SemanticValidator.cpp
|   |-- SemanticValidator.h
|   |-- Executioner.cpp
|   |-- Executioner.h
|   |
|   `-- Test/
|       `-- QueryTester.cpp
|
`-- DataGenerator/
    |-- main.cpp
    |-- R.txt
    `-- S.txt

WHAT EACH PART DOES
===================

src/
    Contains the main Query Processor source code.

src/main.cpp
    Entry point for the Query Processor program.

src/Test/QueryTester.cpp
    Entry point for the test program.

DataGenerator/
    Contains a separate program used to generate the input data files.

DataGenerator/*.txt
    Data files used by the programs.

Makefile
    Tells GNU Make how to compile and link the programs.
```

```latex
BUILDING THE PROJECT
====================

1. Go to the project root:

    cd /home/student/Project

2. Build everything:

    make

This builds three executables:

    Query_Processor
    Query_Processor_Test
    DataGenerator

-- After this simply run the program for treeonly mode with query 
either paste or type no "" needed

./Query_Processor --tree

--output 
======================================================================
 Mode: Tree-Only Mode
======================================================================
Enter Query Only
Type 'END' or put a ';' at the end when finished:
----------------------------------------------------------------------

For regular mode 

 ./Query_Processor
 
 -- ouput 
 ======================================================================
 Mode: Normal Mode (Database + Query)
======================================================================
Enter your input (Relations + Query). You can use multiple lines.
Type 'END' or put a ';' at the end when finished:
----------------------------------------------------------------------
```

## 2. Running the program

The program has two modes.

| Mode | How to start it | What it expects | What it does |
| --- | --- | --- | --- |
| **Regular** | `Query_Processor <input>` | One or more relation definitions, then exactly one query no “” required | Parses, validates, executes, prints the result relation |
| **Tree-only** | `Query_Processor --tree <input>` | Exactly one query and nothing else no “” required | Parses and prints the parse tree. Nothing is validated against data or executed |

How `<input>` is supplied : Via Terminal Input a valid text defined below

An input has this shape, in this order:

```
[ comment lines and blank lines ]
relation definition          (one or more)
[ comment lines and blank lines ]
query                        (exactly one)
[ comment lines and blank lines ] ;

OR ALTERNATE

[ comment lines and blank lines ]
relation definition          (one or more)
[ comment lines and blank lines ]
query                        (exactly one)
[ comment lines and blank lines ] 
END
```

### Case 1: Normal Mode (Database + Query)

Execute without flags to pass both relation defintions and a query: The Format must be valid multiline and spaces for format are permitted 

```latex
Employees(EID, Name, Age) = {
  E1, 'Alice Smith', 28
  E2, 'Bob Jones', 35
}

PROJECTName);

Note: Instead of ; the word END can be used also but it must start at newline

Employees(EID, Name, Age) = {
  E1, 'Alice Smith', 28
  E2, 'Bob Jones', 35
}

PROJECTName)
END
```

### Case 2: Tree-Only Mode (`-tree`)

Execute with the `--tree` flag when you only want to inspect the generated Abstract Syntax Tree (AST) without defining a database or executing the query:

```
PROJECTDName) MINUS PROJECTDName)
END

-- OR

PROJECTDName) MINUS PROJECTDName);
```

## 3. Regular mode: input structure

Rules:

- Nested Queries are allowed and it can run any  nesting level as long as  query format is valid
- **At least one relation definition, then exactly one query.** A query with no definitions will not parse until and unless in tree mode
- **Comments and blank lines are allowed** before, between and after the parts above, and between tuple lines inside a relation body. A comment is a line that starts with `//`. Keep each comment on its own line.
- **Nothing else may appear** between or around the definitions and the query. Stray text, a second query, or text after the query is a syntax error.
- **Definitions come before the query.** The query is always the last thing in the input.
- Outside relation bodies, newlines and spaces are ordinary whitespace, so a query may span several lines or contain no whitespace at all.

## 4. Relation definitions

### 4.1 Shape

```
Employees (EID, Name, Age, DID) = {
  E1, John, 32, D1
  E2, Alice, 28, D2
  E3, Bob, 29, D1
}

Or Use Spaces in String
Employees(EID, Name, Age) = {
    E1, 'Alice Smith', 28
    E2, 'Bob Jones', 35
}
```

- A relation name, then an attribute list in parentheses, then `=`, then the tuples inside `{ }`.
- **One tuple per line.** Values in a tuple are separated by commas. Tuples are separated by at least one newline. Two tuples cannot share a line , tuple values can be formatted using newline or spaces in that case as long as , in between and input tuple matches attribute nums should be no problem .
- Tuple ending is decide by simply seeing value Newline Value meaning one end value and newline and one start value indicates a valid new tuple start hence formatting newlines and spaces are allowed as long as tuple values are properly provided
- **Newlines are free** around `(`, `)`, `=`, `{`, `}` and commas, so you may lay a definition out over many lines. A newline after a comma continues the same tuple.
- The attribute list must contain at least one attribute, and each tuple line must contain at least one value.
- An empty body, `{ }`, is allowed (an empty relation).

### 4.2 Names

- A name starts with an ASCII letter and continues with letters, digits or underscores: `Age`, `Employees`, `x1`, `employee_2`.
- Names are case-sensitive.
- **Relation names cannot be keyword spellings.** Attribute names can (see 5.4).
- Every attribute name inside one relation must be unique, and every relation name must be unique.

### 4.3 Values

There are three ways to write a value, but only **two types**: Number and String.

| Written as | Example | Type | Stored value |
| --- | --- | --- | --- |
| Integer: optional `+` or `-`, then digits | `30`, `-7`, `+30`, `5467792783` | Number | the integer (`+30` is `30`) |
| Quoted string: in single quotes | `'Bob'`, `'a,b'`, `'O''Brien'` | String | the text without the quotes (`O'Brien`) |
| Bare string: no quotes | `Bob`, `D1`, `E1`, `30abc` | String | the text which does not have whitespace, `,`, `(`, `)`, `'`, `{`, `}` and line breaks |

Details:

- **Integers have no size limit.** They are kept as text and compared numerically, so very large numbers work.
- **A number in quotes is a string.** `'1'` is the string `1`, and `1` is the number one. To store a number as text, quote it.
- **A bare string** is one or more characters other than whitespace, `,`, `(`, `)`, `'`, `{`, `}` and line breaks. A value with any of those characters must be quoted.
- **A bare string that looks like a number is a number.** A token is an integer only if the whole token is an optional sign followed by digits. `D1`, `E1` and `30abc` are bare strings.
- **Keyword spellings are ordinary data here.** In a relation body, `select`, `union` and `join` are just strings.
- **Inside quotes, `''` means one literal quote**, and `'` alone ends the string. Commas, parentheses and brackets inside quotes are plain text.
- **Bare and quoted spellings are the same string.** `Bob` and `'Bob'` are the same value, so `E1, Bob` and `E1, 'Bob'` are one tuple.

### 4.4 Rules checked when relations are loaded

| Rule | Error category |
| --- | --- |
| Every tuple has exactly as many values as the relation has attributes | Schema |
| No two relations share a name | Name |
| No two attributes in a relation share a name | Schema |
| Each column keeps one type. The first tuple decides Number or String, and every later tuple must match | Type |
| **Duplicate tuples collapse to one.** A relation is a set | (not an error) |

## 5. Queries

### 5.1 Operators

Unary operators take a parameter in `[ ]` and their input in `( )`:

```
select condition 
project attribute-list 
rename new-relation-name 
```

Binary operators are written between two expressions:

```
expr union expr
expr intersect expr
expr minus expr
expr times expr
expr join[ condition ] expr

If Nested binary op make sure to include valid parenthesis such as 

(expr union expr) union (expr intersect expr)
This is also valid as long as valid query
```

- `project` needs at least one attribute. `project[](R)` is a syntax error.
- `select[](R)` is a syntax error: a condition is required.
- `join[c]` is a **theta join**: `times` followed by `select[c]`. It is not a natural join.
- Parentheses `( expr )` group expressions and override every precedence rule.
- A query is a single expression. Everything after it must be blank or a comment.

### 5.2 Precedence and associativity of relational operators

| Level | Operators | Associativity |
| --- | --- | --- |
| 1 (tightest) | `select`, `project`, `rename`, `( ... )` | delimited by their own parentheses |
| 2 | `times`, `join[c]` | left |
| 3 | `intersect` | left |
| 4 (loosest) | `union`, `minus` | left |

This gives:

| Query | Parsed as |
| --- | --- |
| `A union B minus C` | `(A union B) minus C` |
| `A minus B minus C` | `(A minus B) minus C` |
| `A union B intersect C` | `A union (B intersect C)` |
| `A times B intersect C` | `(A times B) intersect C` |

These groupings change results. For example, with `A = {1}`, `B = {2}`, `C = {1}`, the query `A union B minus C` gives `{2}`, while `A union (B minus C)` would give `{1,2}`. See `GRAMMAR.md` section 5.

### 5.3 Conditions

```
condition   ::= comparison
             |  not condition
             |  condition and condition
             |  condition or condition
             |  ( condition )

comparison  ::= operand  op  operand
op          ::= =  |  !=  |  <  |  <=  |  >  |  >=
```

| Level | Operator | Associativity |
| --- | --- | --- |
| 1 (tightest) | comparison, `( ... )` | not applicable |
| 2 | `not` | right |
| 3 | `and` | left |
| 4 (loosest) | `or` | left |

| Condition | Parsed as |
| --- | --- |
| `a=1 and b=2 or c=3` | `(a=1 and b=2) or c=3` |
| `not (a=1 and b=2) or c>3` | `(not (a=1 and b=2)) or c>3` |
| `not not a=1` | `not (not (a=1))` |

A comparison has exactly one operator and two operands, so `A < B < C` is a syntax error.

### 5.4 Operands (the most important rule)

An operand is one of:

| Operand | Written as | Meaning |
| --- | --- | --- |
| Number | `30`, `-30`, `+30` | a number |
| String | `'Bob'` (quotes required) | a string |
| Attribute | `Age` or `Emp.Age` | the value of that column |

**In a query, an unquoted name is always an attribute, never a string.** This is what lets `A=B` mean "column A equals column B". To compare with text, quote it:

```
selectName='Bob'     // Name equals the string Bob
selectName=Bob       // Name equals the value of column Bob (error if there is no such column)
selectAge>'30'       // type error: number vs string
```

An attribute name may be spelled like a keyword when an attribute is expected: `selectunion=3` compares an attribute called `union` with 3.

### 5.5 Comparison rules

- Number with number: compared numerically, with no size limit.
- String with string: compared character by character, case-sensitive. Uppercase sorts before lowercase, so `'Banana' < 'apple'` is true.
- Number with string: **type error**, never a silent false.

### 5.6 Keywords

```
select  project  rename  union  intersect  minus  times  join  and  or  not
```

Keywords are lowercase and case-sensitive. They can be attribute names, but not relation names or `rename` targets.

## 6. Semantics and output schemas

| Operator | Result |
| --- | --- |
| Relation name | The stored relation. Its attributes are qualified by the relation name (`Employees.Age`) |
| `selectc` | Same schema as `e`. Keeps tuples where `c` holds. `c` may compare two columns  |
| `projecta1, a2` | The listed attributes in the listed order. Duplicate tuples are removed afterwards |
| `renameN` | Same attributes, requalified with the new name `N` |
| `a times b` | All attributes of both sides, each keeping its own qualifier |
| `a join[c] b` | `a times b`, then `select[c]` |
| `a union b`, `a intersect b`, `a minus b` | Schema of the left side. Both sides must be union-compatible |

Rules that follow from this:

- **Union compatibility:** the same number of attributes, the same attribute names in the same order (qualifiers are ignored), and the same type in each position.
- **Qualified names.** `Emp.DID` refers to the column `DID` that came from `Emp`. After `project` a column keeps the qualifier it came from. After `renameE` every column is qualified by `E`.
- **Unqualified names** work only when they match exactly one column. If several columns match (for example `DID` after joining two relations that both have `DID`), it is an ambiguity error and you must qualify it.
- **Collisions.** If `times` or `join` would produce two columns with the same qualified name, it is an error. Use `rename` to give one side a different name.
- **Duplicate projection.** `projectName, Name` is rejected with a clear error. This is a deliberate design choice. It also applies when two references point at the same column (`Emp.Name` and `Name`).
    
    ```latex
    ======================================================================
     Mode: Normal Mode (Database + Query)
    ======================================================================
    Enter your input (Relations + Query). You can use multiple lines.
    Enter and then Type'END' on newline or put a ';' at the end of query when finished either should work:
    ----------------------------------------------------------------------
    
    Employees(EID, Name, Age) = { 
        1E, 'Alice Smith', 28 
        E2, 'Bob Jones', 35 
    } 
    projectName, Employees.Name);
    
    [OK] Semantic relation validation passed.
    [OK] Duplicate tuples removed using set semantics.
    
    [Semantic Schema Error] Duplicate projection attribute 'Employees.Name'.
    ```
    
- **Set semantics everywhere.** Every intermediate result is a set. `project` removes duplicates. `union`, `intersect` and `minus` never produce duplicates.
- **Empty results.** A query with no matching tuples prints its schema and an empty body.
- **Execution order.** The tree is executed exactly as written, bottom-up. There is no optimisation or rewriting.

## 7. Tree-only mode (`-tree`)

```
Query_Processor --tree 
======================================================================
 Mode: Tree-Only Mode
======================================================================
Enter Query Only
Type 'END' or put a ';' at the end when finished:
----------------------------------------------------------------------
---Terminal Input ----
Query
```

- **Input is one query only.** No relation definitions, and no data of any kind. Anything other than a query (including a relation definition) is a syntax error.
- **Nothing is executed and no data is needed.** The output is the abstract syntax tree of the query.
- **Only lexical and syntax errors can occur** in this mode. Relation names and attributes are never looked up, so `-tree "Nothing"` succeeds and prints a single relation node. Name, ambiguity, schema and type errors need the relation definitions and appear only in regular mode.
- The tree shows the grouping the parser chose, so it is the quickest way to check precedence and associativity.

Example:

```
student @ comp2401-headless : 14:31:50
~/Project # ./my_program --tree
======================================================================
 Mode: Tree-Only Mode
======================================================================
Enter Query Only
Type 'END' or put a ';' at the end when finished:
----------------------------------------------------------------------

( projectEmployees.Name, Departments.DeptName, Locations.City
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
) )
joine.id>5
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
                        renameE
                        join[E.DeptID = D.DeptID]
                        renameD
                    )
                    join[D.LocationID = L.LocationID]
                    renameL
                )
                join[E.ID = A.EmployeeID]
                renameA
            )
            join[A.ProjectID = P.ProjectID]
            renameP
        )
    )
) union (b);
-----------------------------------------------------------------------------------------------------------------
AST tree
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
      |                       |____ RenameNode: New relationName = P
      |                             |____ RelationReference: Projects
      |____ RelationReference: b

```

Checking precedence:

```
student @ comp2401-headless : 14:32:47
~/Project # ./my_program --tree
======================================================================
 Mode: Tree-Only Mode
======================================================================
Enter Query Only
Type 'END' or put a ';' at the end when finished:
----------------------------------------------------------------------

A union B minus C
END
-----------------------------------------------------------------------------------------------------------------
AST tree
QueryNode
|____ BinaryExpression (MINUS)
      |---- BinaryExpression (UNION)
      |     |---- RelationReference: A
      |     |____ RelationReference: B
      |____ RelationReference: C
```

The tree for `A union B minus C` shows `(A union B) minus C`: the `union` is the left child of the `minus`.

## 8. Errors

Every error is reported as a message. The program never shows a stack trace. There are six categories.

| Category | Meaning | Reported with position? |
| --- | --- | --- |
| Lexical | A character sequence that is not a token, such as an unterminated string or an unknown symbol | yes |
| Syntax | The tokens do not fit the grammar, such as unbalanced parentheses, a missing operand, `project[](R)` | yes |
| Name | Unknown relation, unknown attribute, unknown qualified attribute, duplicate relation definition | no |
| Ambiguity | An unqualified attribute matches more than one column | no |
| Schema | Incompatible union, column collisions, wrong number of values in a tuple, duplicate attributes, duplicate projection attribute | no |
| Type | Comparing a number with a string, or mixing types in one column | no |

**[CONFIRM]** The exact wording and position format of lexical and syntax errors, and paste one real example of each below.

Examples of messages from the validation stage (prefixed with `[Semantic <Category> Error]`):

| Input | Message |
| --- | --- |
| `Foo` | `Unknown relation 'Foo'.` |
| `selectNope=1` | `Unknown attribute 'Nope'.` |
| `Employees join[DID=1] Dept` | `Ambiguous unqualified attribute 'DID'. Use a relation qualifier.` |
| `Employees times Employees` | `Conflicting qualified output attributes after 'times'.` |
| `Employees union Dept` | `Relation schemas are not union-compatible for 'union': attribute counts differ.` |
| `selectAge>'30'` | `Incompatible comparison operand types: number and string.` |
| `projectName, Name` | `Duplicate projection attribute 'Employees.Name'.` |

Lexical and syntax examples:

| Input | Category |
| --- | --- |
| `selectName='Bob` | Lexical, unterminated string |
| `select*` | Lexical, invalid character |
| `selectAge>30` | Syntax, empty attribute list |
| `select[](R)` | Syntax, missing condition |
| `selecta<b<c` | Syntax, chained comparison |

## 9. Tokenizer cheat sheet

The scanner reads character by character and works without any whitespace.

| Input | What happens |
| --- | --- |
| `selectx1=3` | Same tree as `select x1 = 3 ` |
| `Age>=30` | one `>=` token, not `>` then `=` |
| `Age>-30` | tokens `>` and `-30`. No `>-` operator exists |
| `Age>+30` | tokens `>` and `+30` (value 30) |
| `Name='Bob)'` | the `)` is inside the string, not syntax |
| `Name='a,b'` | the comma is inside the string |
| `Name='O''Brien'` | the value is `O'Brien` |
| `selectunion=3` | `union` is an attribute name here |
| `Name='Bob` | lexical error, the string is never closed |
| `!` alone | lexical error. Only `!=` exists |
| bare `+` or `-` | not a token unless followed by digits |

The scanner uses maximal munch: it takes the longest valid token. After `>`, `<` or `!` it checks the next character before deciding which token it has.

## 10. Worked examples

Use these relations for all examples below:

```
// employees and their departments
Employees (EID, Name, Age, DID) = {
  E1, John, 32, D1
  E2, Alice, 28, D2
  E3, Bob, 29, D1
}

Dept (DID, DName) = {
  D1, Sales
  D2, Engineering
}

Sample Input and Output:

// employees and their departments
Employees (EID, Name, Age, DID) = {
  E1, John, 32, D1
  E2, Alice, 28, D2
  E3, Bob, 29, D1
}

Dept (DID, DName) = {
  D1, Sales
  D2, Engineering
}
projectName);

[OK] Semantic relation validation passed.
[OK] Duplicate tuples removed using set semantics.
[OK] Semantic query validation passed.
-----------------------------------------------------------------------------------------------------------------
Output schema: Employees (Employees.Name:string)

Result of the Query
------------------------
Employees (Employees.Name) =
John

JOIN Comparisons     : 0
SELECT Comparisons   : 3
Output Tuples        : 1
-----------------------------------------------------------------------------------------------------------------
AST tree
-------------
QueryNode
|____ ProjectNode
      |---- AttributeReference: Name
      |____ SelectNode
            |---- ComparisonNode (>)
            |     |---- AttributeOperand
            |     |     |____ AttributeReference: Age
            |     |____ LiteralOperand (Integer): 30
            |____ RelationReference: Employees
-----------------------------------------------------------------------------------------------------------------
Database Schema
----------------
Employees (EID, Name, Age, DID) =
E1 John 32 D1
E2 Alice 28 D2
E3 Bob 29 D1

Dept (DID, DName) =
D1 Sales
D2 Engineering
```

Each query goes after the definitions, as the last item in the input.

| Query | Result |
| --- | --- |
| `projectName)` | `John` |
| `projectDID` | `D1`, `D2` (two tuples, not three) |
| `selectEID='E2' or Age>30` | E1 John 32 D1, and E2 Alice 28 D2 |
| `Employees join[Employees.DID=Dept.DID] Dept` | three tuples, e.g. `E1, John, 32, D1, D1, Sales`. Both `DID` columns stay distinguishable as `Employees.DID` and `Dept.DID` |
| `projectEmployees.Name, Dept.DName` | (John, Sales), (Alice, Engineering), (Bob, Sales) |
| `selectAge>100` | empty result: the schema, then an empty body |
| `projectDID minus projectDID)` | `D2` |

A self join (see section 12):

```
renameE2 join[Employees.DID=E2.DID and Employees.EID!=E2.EID] Employees
```

returns two tuples: (E1, John, 32, D1) paired with (E3, Bob, 29, D1), and the same pair in the opposite order.

## 11. Data generator and instrumentation

### Generator

Writes `R(a, b)` and `S(b, c)` in the relation-definition format above.

```
generator <numR> <numS> <matchRate> [outputDirectory]
```

| Argument | Meaning |
| --- | --- |
| `numR` | number of tuples in `R` (positive) |
| `numS` | number of tuples in `S` (positive) |
| `matchRate` | on average, how many `S` tuples each `R` tuple joins with on `R.b = S.b` (at least 0) |
| `outputDirectory` | where `R.txt` and `S.txt` are written (default `..`) |
- `R.b` values are unique, so each matching `S` tuple joins exactly one `R` tuple. The join output size is `numR x matchRate`, rounded.
- `matchRate` must satisfy `numR x matchRate <= numS`. With `numR = numS` the largest rate is `1.0`. A larger rate is rejected with an error, and **no files are written**, so old files from an earlier run stay on disk.
- Output is shuffled, uses a fixed seed (42), and contains no duplicate tuples.

Example: `generator 1000 1000 1.0 .` writes 1000 tuples per relation, all of which match.

### Counters

The engine counts work inside the operators (counted, not estimated):

- **Join counter:** incremented once for every pair of tuples whose join condition is evaluated, whether or not the pair matches. It always equals (left size) x (right size) for each join.
- **Select counter:** incremented once for every tuple the selection condition is evaluated on.

Both are reset at the start of every query.

## 12. Why a self join needs `rename`

Every column is identified by its relation name plus its attribute name. In a self join both sides are the same relation, so both copies of every column would carry the same qualified name.

`Employees join[Employees.MgrID=Employees.EID] Employees` cannot work for two reasons. The combined schema would contain `Employees.EID` twice, which is a collision error. And even if that were allowed, the condition could not tell which copy of `Employees.EID` it means, since there is no way to say "the manager's row" versus "the employee's row".

`renameE2` gives one copy a different relation name, so its columns become `E2.EID`, `E2.MgrID` and so on. Now `E2.EID` and `Employees.MgrID` are different columns, and the join condition can relate the two roles:

```
renameE2 join[Employees.MgrID=E2.EID] Employees
```

## 13. Known limitations

- **Nested-loop join only.** Every join compares every pair of tuples, so it costs n x m. There are no hash joins, sort-merge joins or indexes. This is intentional and is what the performance study measures.
- **No optimisation.** The query runs exactly as written.
- **Duplicate removal is a linear scan.** `project` (and `union`) check each tuple against the output so far, so their cost grows with the number of distinct output tuples, not just the input size.
- **Everything is in memory.** Relations are loaded completely before the query runs, and each relation reference copies its stored table.
- **Load-time cost.** Deduplicating a relation on load is also a linear scan per tuple, so loading very large relations (tens of thousands of tuples) is slow.
- **Empty relations have untyped columns.** A relation with no tuples treats all its columns as strings, because the type of a column is decided by its first tuple. Comparing such a column with a number, or using it in a union with a numeric relation, can report a type error.
- **No NULLs and no three-valued logic.** Every attribute has a value.
- **Single query per run,** and one query per input.
- Accepts Terminal Input can be turned into accept file but for simplicity it is terminal input
- **Case-sensitive** keywords, relation names and attribute names.
- **Only the ASCII form** of the syntax is supported.
