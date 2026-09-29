# GRAMMAR

# Grammar and Language Design

## 1. Scope

This document defines the concrete syntax and parsing rules for the relational-algebra query language used by the project. The parser must implement these rules directly; precedence and associativity are not special cases hidden inside the parser.

There are two logical entry points:

- `relation_file` for loading one or more relation definitions.
- `query` for parsing a relational-algebra query expression.

This component intentionally does not add SQL syntax, arithmetic expressions, NULLs, three-valued logic, or query optimization.

## 2. Lexical Conventions

### 2.1 Whitespace and comments

Outside relation data rows, ordinary whitespace separates tokens and is ignored. Blank lines are ignored.

A line beginning with `//` is a comment and produces no tokens.

Inside a relation definition, newline separates tuple lines, so the tokenizer preserves the significant newline between non-empty tuple rows.

### 2.2 Identifiers

An identifier is a name beginning with an ASCII letter, followed by zero or more ASCII letters, digits, or underscores.

EBNF

```ebnf
identifier ::= letter { letter | digit | "_" }
```

where:

EBNF

```ebnf
letter ::= "A".."Z" | "a".."z"
digit  ::= "0".."9"
```

Examples: `Age`, `Employees`, `x1`, `E2`, `employee_2`. An identifier cannot begin with a digit, and `.` is not part of an identifier.

### 2.3 Keywords

The following lowercase words are language keywords:

`select`, `project`, `rename`, `union`, `intersect`, `minus`, `times`, `join`, `and`, `or`, `not`

Keywords are recognized by the tokenizer as keyword tokens. They are reserved in positions where the grammar expects an operator or relation name, but a keyword spelling is allowed as an attribute name because the specification explicitly requires cases such as:

`select[union=3](R)`

Therefore, when an attribute name is expected, a keyword token is accepted as an attribute name. Relation names and new relation names use ordinary identifiers and may not be keyword spellings. Keyword spellings are lowercase and case-sensitive. Identifiers are also case-sensitive.

### 2.4 Integer literals

An integer consists of an optional leading sign followed by one or more digits:

EBNF

```ebnf
integer ::= [ "+" | "-" ] digit { digit }
```

There is no language-level limit on the number of digits.

Examples: `0`, `30`, `+30`, `1000000`, `5467792783`, `-30`, `-7`.

A leading `+` does not change the value, so `+30` is treated as the integer value `30`.

A leading `-` changes the value and must be handled explicitly by the tokenizer and value representation.

The scanner must recognize `-30` as one integer token. Thus `Age>-30` must tokenize as:

`Age`, `>`, `-30`

and not as a nonexistent `>-` token.

The same rule applies to `+30`: `Age>+30` tokenizes as `Age`, `>`, `+30` with the integer value `30`.

A standalone `+` or `-` is not an integer and is not an operator in the query language. Therefore, after tokenization, an expression requiring an operand cannot use a bare sign.

#### Number inside ‘ ‘:

anywhere in program if a number is inside  ‘ ‘ like ‘1’ it is treated as a quoted string instead of a number because that enables user to store a number as string as well or if a  number is a string then query it in such a way  , if want to declare and int simply just do 1 and it will be saved as integer

### 2.5 Quoted strings

A quoted string begins and ends with a single quote. Two consecutive single quotes inside a quoted string represent one literal quote character.

Conceptually:

EBNF

```ebnf
quoted_string ::= "'" { string_char | "''" } "'"

string_char = any character except a single quote or line terminator

''          = one literal single quote
'           = closes the string
```

where `string_char` is any character other than a single quote or a line terminator.

Examples: `'Bob'`, `'Bob)'`, `'a,b'`, `'O''Brien'` (evaluates to `O'Brien`).

While scanning a quoted string, the tokenizer processes quotes left-to-right. When it sees `'` inside the string, it consumes `''` as one literal quote if another `'` immediately follows; otherwise the single quote closes the string.

An unterminated quoted string is a lexical error reported with its input position.

Characters such as `)`, `,`, `[`, `]`, and operator characters inside a quoted string are string contents, not syntax.

### 2.6 Bare strings in relation definitions

The relation-definition format permits strings to be written without quotes. A bare string is a non-empty sequence of data characters that does not contain whitespace, a comma, a parenthesis, a quote, a line terminator, or the relation-body delimiter `}`. Values containing those structural characters must use the quoted form.

```ebnf
bare_string ::= bare_char { bare_char }

```

#### **Here, bare_char denotes any input character other than whitespace,**

#### **,, (, ), ', {, }, or a line terminator.**

Bare strings are valid **only as relation-data values**. Their interpretation is therefore determined by the grammatical context.

In relation-data context, a word is treated as a string value even when its spelling matches one of the language keywords. For example:

```ebnf
R(Name) = {
    Bob
    select
    union
}
```

Here, `Bob`, `select`, and `union` are all string values in the relation `R`.

In query context, however, an unquoted name-shaped token is an attribute reference rather than a string literal. For example:

```
select[Name=Bob](R)
```

means that attribute `Name` is compared with attribute `Bob`, whereas:

```
select[Name='Bob'](R)
```

compares `Name` with the string literal `Bob`.

Similarly, a keyword spelling can be used as an attribute name when an attribute is expected:

```
select[union=3](R)
```

Here `union` is an attribute name, not the `union` operator.

Thus, the same spelling may have different meanings depending on the grammatical context:

```
Relation data:       union       → string value "union"
Attribute context:   union       → attribute named union
Operator context:    union       → UNION operator
```

Bare strings are a relation-data syntax only. In a query condition, **an unquoted name-shaped token is an attribute reference, not a string literal.** This is necessary so that `A = B` means attribute `A` compared with attribute `B`, as required by the specification.

This contextual interpretation keeps keyword handling unambiguous without requiring separate syntax for keyword-shaped data values.

### 2.7 Comparison operators and maximal munch

The only comparison operators are:

`=`, `!=`, `<`, `<=`, `>`, `>=`

The tokenizer uses maximal munch for multi-character operators. For example, after reading `>` it checks the next character to decide whether the token is `>` or `>=`. The same applies to `<`/`<=` and `!`/`!=`.

Basically it follows the principle of  computer science where a compiler or parser consumes as many input characters as possible to form a single valid token before moving on to the next one

A character sequence that does not form a defined token is a lexical error rather than an invented operator.

### 2.8 Punctuation

The following punctuation symbols are individual tokens:

`(`, `)`, `[`, `]`, `.`, `,`, `{`, `}`

`(` and `)` group expressions and delimit unary-instruction inputs. `[` and `]` delimit instruction parameters. `.` separates the relation qualifier from an attribute name. `,` separates list elements and tuple values. `{` and `}` delimit relation data.

## 3. Complete EBNF

### 3.1 Entry points

EBNF

```ebnf
input ::= [ relation_file ] query

input_tree_mode ::= query

relation_file ::= relation_definition { relation_definition }

query ::= expr EOF
```

A program may load relation definitions separately from parsing a query; therefore the grammar exposes them as separate entry points rather than assuming one combined input format.

### 3.2 Relation definitions

EBNF

```ebnf
relation_definition
    ::= relation_name format_newlines
        "(" format_newlines relation_attribute_list format_newlines ")"
        format_newlines "=" format_newlines
        "{" format_newlines [ relation_rows ] format_newlines "}"

relation_name
    ::= identifier

relation_attribute_list
    ::= attribute_name
        { format_newlines "," format_newlines attribute_name }

relation_rows
    ::= tuple_line { row_separator tuple_line } [ row_separator ]

row_separator
    ::= NEWLINE { NEWLINE }

tuple_line
    ::= value
        { format_newlines "," format_newlines value }

value
    ::= integer
     |  quoted_string
     |  bare_string

bare_string
    ::= bare_char { bare_char }

format_newlines
    ::= { NEWLINE }
```

*(where `bare_char` represents any non-whitespace ASCII character except `,`, `(`, `)`, `'`, `{`, and `}`)*

The grammar requires a non-empty schema and a non-empty tuple line. The semantic loader checks that every tuple has exactly as many values as the relation has attributes. Duplicate tuples collapse because relations are sets.

**the change:** `format_newlines` allows users to freely place newlines around structural parts such as `(`, `)`, `=`, `{`, and `,`, and also between a comma and the next tuple value. This makes formatting flexible without changing the meaning.

**What remains strict:** the actual structure is still enforced. A relation must have a relation name, attribute list, `=`, `{}`, and valid values separated by commas. A newline is still significant when it separates completed tuples through `row_separator`, so tuples cannot simply run together without a separator.

### 3.3 Attribute names and references

EBNF

```ebnf
attribute_name
    ::= identifier
     |  keyword_name

keyword_name
    ::= "select"
     |  "project"
     |  "rename"
     |  "union"
     |  "intersect"
     |  "minus"
     |  "times"
     |  "join"
     |  "and"
     |  "or"
     |  "not"

attribute_reference
    ::= attribute_name
     |  relation_name "." attribute_name
```

Examples: `DID`, `Emp.DID`, `union`, `Emp.union`.

The semantic layer, not the grammar, decides whether a referenced relation or attribute actually exists.

### 3.4 Projection lists

EBNF

```ebnf
attribute_list
    ::= attribute_reference { "," attribute_reference }
```

Because at least one `attribute_reference` is required, this grammar rejects `project[](R)`.

Duplicate attribute names in a projection are handled semantically. This implementation chooses to reject duplicates with a clear semantic error rather than silently repeating an output column.

### 3.5 Relational expressions and precedence

The relational-expression grammar is stratified into precedence levels. The highest-level rule is deliberately not left-recursive so that it can be implemented directly with recursive descent.

EBNF

```ebnf
query ::= expr EOF

expr
    ::= union_expr

union_expr
    ::= intersect_expr
        { ( "union" | "minus" ) intersect_expr }

intersect_expr
    ::= combine_expr
        { "intersect" combine_expr }

combine_expr
    ::= unary_expr
        { ( "times" | join_operator ) unary_expr }

join_operator
    ::= "join" "[" condition "]"

unary_expr
    ::= select_expr
     |  project_expr
     |  rename_expr
     |  primary_expr

select_expr
    ::= "select" "[" condition "]" "(" expr ")"

project_expr
    ::= "project" "[" attribute_list "]" "(" expr ")"

rename_expr
    ::= "rename" "[" relation_name "]" "(" expr ")"

primary_expr
    ::= relation_name
     |  "(" expr ")"
```

### 3.6 Conditions

EBNF

```ebnf
condition
    ::= or_condition

or_condition
    ::= and_condition { "or" and_condition }

and_condition
    ::= not_condition { "and" not_condition }

not_condition
    ::= "not" not_condition
     |  condition_primary

condition_primary
    ::= comparison
     |  "(" condition ")"

comparison
    ::= operand comparison_operator operand

comparison_operator
    ::= "=" | "!=" | "<" | "<=" | ">" | ">="

operand
    ::= integer
     |  quoted_string
     |  attribute_reference
```

A comparison contains exactly one comparison operator and two operands. Therefore a chained comparison such as `A < B < C` is not generated by the grammar and must be rejected as a syntax error.

## Operator Precedence and Associativity Overview

The language has two separate precedence hierarchies: **condition precedence** and **relational-expression (instruction) precedence**.

### 1. Condition Precedence

From highest to lowest:

| Precedence | Operator(s) | Associativity |
| --- | --- | --- |
| Highest | Parentheses `( ... )` | — |
| 2 | `not` | Right |
| 3 | `and` | Left |
| Lowest | `or` | Left |

A comparison has the form:

```
operand comparison_operator operand
```

where the comparison operators are:

```
=   !=   <   <=   >   >=
```

Comparisons are treated as atomic conditions when applying `not`, `and`, and `or` precedence.

For example:

```
a=1 and b=2 or c=3
```

is parsed as:

```
(a=1 and b=2) or c=3
```

and:

```
not (a=1 and b=2) or c=3
```

is parsed as:

```
(not (a=1 and b=2)) or c=3
```

### 2. Relational-Expression / Instruction Precedence

From highest to lowest:

| Precedence | Operator(s) | Associativity |
| --- | --- | --- |
| Highest | `select[C](E)`, `project[A](E)`, `rename[R](E)` | Prefix; operand explicitly delimited by `( ... )` |
| 2 | `times`, `join[C]` | Left |
| 3 | `intersect` | Left |
| Lowest | `union`, `minus` | Left |

Thus:

```
A union B intersect C
```

is parsed as:

```
A union (B intersect C)
```

and:

```
A union B minus C
```

is parsed as:

```
(A union B) minus C
```

while:

```
A minus B minus C
```

is parsed as:

```
(A minus B) minus C
```

The unary relational operators explicitly delimit their operand with parentheses, so nested expressions such as:

```
project[Name](select[Age>30](Employees))
```

are unambiguous. Explicit parentheses always override the precedence rules.

### Summary

```
CONDITIONS (highest → lowest)

( ... )
   ↓
not
   ↓
and
   ↓
or

RELATIONAL EXPRESSIONS (highest → lowest)

select / project / rename
          ↓
     times / join
          ↓
       intersect
          ↓
     union / minus
```

## 4. Precedence and Associativity

### 4.1 Relational-expression precedence

| **Level** | **Construct / Operator** | **Associativity** | **Enforcing Grammar Rule** |
| --- | --- | --- | --- |
| **1** | `select[C](E)`, `project[A](E)`, `rename[R](E)`, parenthesized expressions | Explicitly delimited / atomic | `unary_expr`, `primary_expr` |
| **2** | `times`, `join[C]` | Left | `combine_expr` |
| **3** | `intersect` | Left | `intersect_expr` |
| **4** | `union`, `minus` | Left | `union_expr` |

This gives:

```
select / project / rename / ( ... )
                ↓
          times / join
                ↓
            intersect
                ↓
          union / minus
```

The `select`, `project`, and `rename` inputs are explicitly enclosed in `( ... )`, so their operands are already delimited. Their nesting does not depend on associativity; for example, `project[Name](select[Age>30](Employees))` has an unambiguous nested structure.

For binary operators, left associativity is enforced by the EBNF repetition at each level. For example, `A minus B minus C` parses as `(A minus B) minus C`, and `A union B minus C` parses as `(A union B) minus C`.

`intersect` binds more tightly than `union` and `minus`, so `A union B intersect C` parses as `A union (B intersect C)`.

The `times`/`join` level is a design choice for this algebra. SQL does not put `JOIN` and set operations into one flat operator-precedence table because joins belong to the table-expression structure while `UNION`/`INTERSECT`/`EXCEPT` combine query expressions. Placing `times` and `join` above the set operations preserves that same structural separation in this algebra.

### 4.2 Condition precedence

| **Level** | **Construct / Operator** | **Associativity** | **Enforcing Grammar Rule** |
| --- | --- | --- | --- |
| **1** | Comparison (`=`, `!=`, `<`, `<=`, `>`, `>=`) | Not applicable (exactly 1 operator) in between 2 operands and joined using nested and, not , or  | `comparison` |
| **2** | `not` | Right | `not_condition` |
| **3** | `and` | Left | `and_condition` |
| **4** | `or` | Left | `or_condition` |

Parentheses override these levels. Thus:

`not a=1 and b=2 or c=3` parses as `((not (a=1)) and (b=2)) or (c=3)`

and `not (a=1 and b=2) or c=3` parses as `(not ((a=1) and (b=2))) or (c=3)`.

The right recursion in `not_condition` enforces right-nesting for repeated `not` operators: `not not A=1` becomes `not (not (A=1))`.

## 5. Why This Grammar Is Unambiguous

### 5.1 The deliberately ambiguous grammar

The specification gives this intentionally naive grammar:

EBNF

```
Expr ::= Expr "union" Expr
       | Expr "minus" Expr
       | "(" Expr ")"
       | IDENT
```

For `A union B minus C`, there are at least two valid parse trees.

### 5.2 First parse tree: (A union B) minus C

```
        minus
       /     \
    union      C
   /     \
  A       B
```

### 5.3 Second parse tree: A union (B minus C)

```
         union
        /     \
       A      minus
             /     \
            B       C
```

Therefore the naive grammar is ambiguous.

### 5.4 Concrete data showing the ambiguity matters

Use the same one-column schema for all three relations:

```
A(x) = { 1 }
B(x) = { 2 }
C(x) = { 1 }
```

First grouping:

```
(A union B) minus C
= {1,2} minus {1}
= {2}
```

Second grouping:

```
A union (B minus C)
= {1} union ({2} minus {1})
= {1,2}
```

The results differ (`{2}` versus `{1,2}`), so the ambiguity is semantically significant.

### 5.5 How the final grammar removes the ambiguity

The final grammar separates precedence levels:

EBNF

```ebnf
union_expr
    ::= intersect_expr
        { ( "union" | "minus" ) intersect_expr }

intersect_expr
    ::= combine_expr
        { "intersect" combine_expr }

combine_expr
    ::= unary_expr
        { ( "times" | join_operator ) unary_expr }
```

Because `union` and `minus` occur only in the outer `union_expr` repetition, and `intersect` occurs in the tighter `intersect_expr` level, the grammar forces one unique structure. It therefore chooses `A union B minus C` to mean `(A union B) minus C`.

### 5.6 Concrete data showing why left-associativity matters for repeated MINUS (Test Case 11)

Use:

```
A(x) = { 1, 2 }
B(x) = { 2 }
C(x) = { 1 }
```

Left-associative grouping:

```
(A minus B) minus C
= ({1,2} minus {2}) minus {1}
= {1} minus {1}
= {}
```

Right-associative grouping:

```
A minus (B minus C)
= {1,2} minus ({2} minus {1})
= {1,2} minus {2}
= {1}
```

The results differ (`{}` versus `{1}`), demonstrating that associativity for `minus` is semantically significant.

The `union_expr` rule enforces left-associativity.

### 5.7 Parentheses override the grammar levels

The expression `(A union B) minus (C intersect D)` forces this tree:

Plaintext

```
          minus
         /     \
      union   intersect
      /  \     /   \
     A    B   C     D
```

This is independent of the unparenthesized precedence rules.

## 6. Parsing Strategy

The parser will use handwritten **Top-Down Predictive Recursive Descent (LL1)**.

the simple rule that this stratgy follows are 

- first `L`: read input left to right
- second `L`: construct a leftmost derivation
- `1`: use one token of lookahead.

Two alternatives must not compete for the same lookahead token.

The final grammar is intentionally free of left recursion. A naive rule such as:

EBNF

```ebnf
Expr ::= Expr "union" Term | Term
```

cannot be used directly with recursive descent because `parseExpr()` would call `parseExpr()` again before consuming any input, causing infinite recursion.

Instead, each left-associative precedence level uses EBNF repetition. For example:

EBNF

```ebnf
union_expr
    ::= intersect_expr
        { ( "union" | "minus" ) intersect_expr }
```

A recursive-descent implementation of that rule conceptually does this:

Python

```
left = parse_intersect_expr()
while next_token in ("union", "minus"):
    op = consume_operator()
    right = parse_intersect_expr()
    left = Binary(op, left, right)
return left
```

This produces left associativity without left recursion.
As demonstrated above, accumulating AST nodes inside a left-to-right `while` loop automatically forces binary operations into a **left-deep tree structure**. For instance, evaluating `A minus B minus C` parses `(A minus B)` first and binds it as the left child of the outer `minus` node, yielding `((A minus B) minus C)` as required 

The same pattern is used for `intersect`, `times`, and `join`.

`not` is deliberately right-recursive:

EBNF

```
not_condition
    ::= "not" not_condition
     |  condition_primary
```

so repeated `not` expressions nest naturally from the right.

Parenthesized expressions call the full `expr` rule recursively. The parser returns from a nested expression when its matching `)` is consumed, not merely when a relation name is encountered.

The parser therefore mirrors the grammar hierarchy directly:

```
parse_expr
    -> parse_union_expr
        -> parse_intersect_expr
            -> parse_combine_expr
                -> parse_unary_expr
                    -> parse_select/project/rename/primary
```

Conditions have their own hierarchy:

```
parse_condition
    -> parse_or_condition
        -> parse_and_condition
            -> parse_not_condition
                -> parse_comparison / parenthesized condition
```

## 7. Syntax vs Semantic Validation

The grammar answers whether the query has the correct structure. Semantic validation happens after parsing.

### Case of

```
project[Name, Name](R)

Result: Clear error. Duplicate attributes are not allowed in the projection list.

This is taken care by our semantics validator 

```

Examples:

`R union S` is syntactically valid even if `R` and `S` later turn out to have incompatible schemas.

`select[A=B](R)` is syntactically valid even if `A` or `B` does not exist in `R`.

Semantic validation is responsible for:

- unknown relation names;
- unknown attributes;
- ambiguous unqualified attribute references after a join;
- union incompatibility;
- incompatible operand types in comparisons;
- duplicate projection attributes (this design chooses a clear error as stated above);
- duplicate/conflicting output attribute names where the operator requires an error.

For comparisons, numbers may be compared with numbers and strings with strings. A number compared with a string is a type error rather than a silent false.

The `join` operator is semantically defined as `times` followed by `select[condition]`; it is a theta join, not a natural join.

## 8. Required Error Behavior

The grammar and parser must support useful errors rather than stack traces , and All have been implemented as such 

Examples required by the specification include:

- `select[](R)`: Syntax error because a condition is required.
- `project[](R)`: Syntax error because the attribute list must contain at least one attribute.
- `select[Age>30](R`: Syntax error identifying the missing `)` and its position.
- `select[Name='Bob](R)`: Lexical error identifying the unterminated string and its position.
- `R union S`: Semantic schema error if the relations are not union-compatible.
- `select[Age>'30'](R)`: Semantic type error because a number is being compared with a string.
- `select[*](R)` is not accepted as an alternative to a condition, it gives lexical error directly as invalid character.

## 9. Sanity Checks Against Required Test Cases

The following required cases are directly determined by this grammar:

| Test | Expected result |
| --- | --- |
| `A union B minus C` | `(A union B) minus C` |
| `A minus B minus C` | `(A minus B) minus C` |
| `not (a=1 and b=2) or c>3` | `(not ((a=1) and (b=2))) or (c>3)` |
| `a=1 and b=2 or c=3` | `((a=1) and (b=2)) or (c=3)` |
| `project[Name](select[Age>30](select[DID='D1'](Employees)))` | `Project -> Select -> Select -> Relation` |
| `(A union B) minus (C intersect D)` | Explicit parentheses determine both children of `MINUS` |
| `select[Age>30](R` | Missing closing parenthesis error |
| `project[](R)` | Empty projection rejected |

The tokenizer rules also determine the required cases:

- `select[Age>=30](R)` -> `>=` is one token.
- `select[Age>-30](R)` -> `>` and `-30` are separate tokens.
- `select[Name='Bob)'](R)` -> `)` is part of the string.
- `select[Name='a,b'](R)` -> `,` is part of the string.
- `select[Name='O''Brien'](R)` -> doubled quote is one literal quote.
- `select[union=3](R)` -> `union` is accepted as an attribute name in attribute context.

## 10. Sources and Design Notes

### Foundational References

- Aho, A. V., Lam, M. S., Sethi, R., & Ullman, J. D. *Compilers: Principles, Techniques, and Tools* (Dragon Book, 2nd ed.), Sections 2.2–2.4, 4.4. Consulted for formal EBNF grammar design, precedence stratification, and left-recursion elimination techniques.
- Wikipedia articles: "Extended Backus–Naur form", "Recursive descent parser", "Maximal munch", and "Operator-precedence parser".
- AI assistance was taken for refinement and clean document writing based on general best practices However the grammar is heavily influenced by the textbook listed above

### SQL Precedence References

The set-operation precedence was chosen with SQL compatibility as a design goal. PostgreSQL 18 documents that `INTERSECT` binds more tightly than `UNION` and `EXCEPT`, while `UNION` and `EXCEPT` associate left-to-right. Microsoft SQL Server documents the same precedence relationship for `INTERSECT`, `EXCEPT`, and `UNION`.

- PostgreSQL 18 Documentation, Section 7.4, "Combining Queries (UNION, INTERSECT, EXCEPT)".
- Microsoft Learn, "EXCEPT and INTERSECT (Transact-SQL)".

### AI-Assisted Design Correction

An earlier AI suggestion proposed a precedence ordering with `UNION` above `INTERSECT` and gave `MINUS` its own separate level. That was corrected after checking the official SQL documentation. The final rule follows the documented SQL set-operation relationship: `INTERSECT` binds more tightly, while `UNION` and `EXCEPT` are peers evaluated left-to-right.

## 11. Final Precedence Summary

### Conditions

```
comparison
    ↓
not
    ↓
and
    ↓
or
```

*(Parentheses override all condition precedence.)*

### Relational expressions

```
select / project / rename / ( ... )
                ↓
          times / join
                ↓
            intersect
                ↓
          union / minus
```

All binary relational operators are left-associative. Unary instruction operands are explicitly delimited by parentheses, so their nesting is determined by their syntax rather than by binary associativity.