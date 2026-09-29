# Semantics Validator

## Table (Relation)

```

(
    (
        project[
            Employees.Name,
            Departments.DeptName,
            Locations.City
        ](
            select[
                (
                    Employees.Age >= 30
                    and
                    Employees.Salary > 70000
                )
                or
                (
                    Employees.Name = 'Alice'
                    and
                    Employees.Age < 30
                )
            ](
                Employees
                join[
                    Employees.DeptID = Departments.DeptID
                ](
                    Departments
                    join[
                        Departments.LocationID = Locations.LocationID
                    ]
                    Locations
                )
            )
        )
    )
    join[
        Employees.Name = E.Name
    ]
    (
        project[
            E.Name,
            D.DeptName,
            L.City,
            P.ProjectName
        ](
            select[
                (
                    (
                        (
                            E.Age >= 30
                            and
                            E.Age <= 45
                        )
                        and
                        (
                            E.DeptID = 10
                            or
                            E.DeptID = 20
                        )
                    )
                    or
                    (
                        (
                            not (E.Age < 25)
                        )
                        and
                        (
                            P.Budget > 12400000
                            or
                            A.Hours >= 40
                        )
                        and
                        (
                            E.Name != 'Dian'
                        )
                    )
                )
            ](
                (
                    (
                        (
                            rename[E](Employees)
                            join[
                                E.DeptID = D.DeptID
                            ]
                            rename[D](Departments)
                        )
                        join[
                            D.LocationID = L.LocationID
                        ]
                        rename[L](Locations)
                    )
                    join[
                        E.ID = A.EmployeeID
                    ]
                    rename[A](Assignments)
                )
                join[
                    A.ProjectID = P.ProjectID
                ]
                rename[P](Projects)
            )
        )
    )
)
union
(
    (
        project[
            Employees.Name,
            Departments.DeptName,
            Locations.City
        ](
            select[
                (
                    Employees.Age >= 30
                    and
                    Employees.Salary > 70000
                )
                or
                (
                    Employees.Name = 'Alice'
                    and
                    Employees.Age < 30
                )
            ](
                Employees
                join[
                    Employees.DeptID = Departments.DeptID
                ](
                    Departments
                    join[
                        Departments.LocationID = Locations.LocationID
                    ]
                    Locations
                )
            )
        )
    )
    join[
        Employees.Name = E.Name
    ]
    (
        project[
            E.Name,
            D.DeptName,
            L.City,
            P.ProjectName
        ](
            select[
                (
                    (
                        (
                            E.Age >= 30
                            and
                            E.Age <= 45
                        )
                        and
                        (
                            E.DeptID = 10
                            or
                            E.DeptID = 20
                        )
                    )
                    or
                    (
                        (
                            not (E.Age < 25)
                        )
                        and
                        (
                            P.Budget > 12400000
                            or
                            A.Hours >= 40
                        )
                        and
                        (
                            E.Name != 'Dian'
                        )
                    )
                )
            ](
                (
                    (
                        (
                            rename[E](Employees)
                            join[
                                E.DeptID = D.DeptID
                            ]
                            rename[D](Departments)
                        )
                        join[
                            D.LocationID = L.LocationID
                        ]
                        rename[L](Locations)
                    )
                    join[
                        E.ID = A.EmployeeID
                    ]
                    rename[A](Assignments)
                )
                join[
                    A.ProjectID = P.ProjectID
                ]
                rename[P](Projects)
            )
        )
    )
)

```

## 1. Relation Semantic Validation

- **Unique relation names:** Each relation definition must have a unique relation name. For example, defining `Student` twice is a semantic error.
- **Unique attribute names:** Each relation's attribute list must contain unique attribute names. For example, `Student(StudentID, Name, Name)` is a semantic error.
- **Attribute typing:** The **first tuple row** determines the type of each attribute from the value stored at that position (`number` or `string`). This establishes the relation's schema types. Every subsequent tuple row must use the same type for the corresponding attribute position. This rule is applied independently to every relation.
    - Example:
        
        ```
        Student(StudentID, Name, Age) = {
          1, 'Alice', 20
          2, 'Bob', 21
        }
        ```
        
        The first row establishes `StudentID:number`, `Name:string`, and `Age:number`; every later row must follow those types. A row such as `3, 'Charlie', '22'` is a semantic type error.
        
- **Duplicate tuples:** Relations use **set semantics**; identical tuples are collapsed to one tuple.
- **Schema construction:** After a relation definition passes semantic validation, construct and store its schema containing the relation name, attribute names, and inferred attribute types. This schema is then used for query semantic validation.
- **Value representation:** Unquoted numeric values are numbers; quoted values are strings. A quoted numeric value such as `'20'` remains a string, which suggests bare strings and quoted strings are both strings but are just written differently because of spaces, etc. Therefore, `bob` and `'bob'` should be treated equally, as we know a query cannot have any attribute value as a bare string; bare strings are reserved for attribute reference only inside the query part, while in a tuple it can be a value for a relation attribute.
- **Data-context keywords:** Keyword spellings used as relation-data values are treated as strings .

## 2. Query Semantic Validation

After relation schemas have been constructed successfully, validate the query AST against those schemas to ensure that every relation, attribute reference, operator, and comparison is semantically valid.

The parser already produces the complete query AST, so semantic validation operates directly on the AST and does **not** need to re-parse tokens.

The query root is:

```
QueryNode
└── expression
```

`QueryNode` contains the complete relational expression in its `expression` child.

### Expression Nodes

The semantic analyser can encounter these relational-expression nodes:

```
RelationReferenceNode
BinaryExpressionNode
JoinNode
SelectNode
ProjectNode
RenameNode
```

`BinaryExpressionNode` stores the operator (`Union`, `Minus`, `Intersect`, `Times`) and its `left` and `right` expression children.

`JoinNode` stores:

```
left expression
condition
right expression
```

so the join condition can be validated against the combined schemas of its two children.

`SelectNode`, `ProjectNode`, and `RenameNode` directly store their parameters together with their input expression.

### Condition and Operand Nodes

Conditions are represented by:

```
LogicalConditionNode
NotConditionNode
ComparisonNode
```

`ComparisonNode` contains:

```
operator
left OperandNode
right OperandNode
```

The two possible operand forms are already represented explicitly:

```
AttributeOperandNode
LiteralOperandNode
```

`LiteralOperandNode` already stores the `ValueKind` and value text, so semantic analysis can directly determine whether the literal is a number or string without reinterpreting the source text. `AttributeOperandNode` contains an `AttributeReferenceNode`, which supplies the attribute name and optional relation qualifier for schema lookup.

### Name and Attribute Resolution

- **Unknown relation:** Every relation referenced by a query must exist.
    - Example: `project[Name](Student)` is valid only if `Student` has been defined.
- **Unknown attribute:** Every referenced attribute must exist in the schema of the relevant input expression.
    - Example: `project[StudentName](Student)` is invalid if `StudentName` is not an attribute of `Student`.
- **Qualified reference:** `Department.DeptID` is valid only when the current expression contains a relation named `Department` with attribute `DeptID`.
- **Unqualified reference:** An attribute such as `DeptID` must resolve to exactly one attribute in the current schema.
    - Example: `Student join[Student.DeptID=Department.DeptID] Department` uses qualified references because both inputs contain `DeptID`.
- **Attribute-vs-attribute comparison:** `Age=MaxAge` means comparison of two attributes, not comparison with the string `"MaxAge"`. A quoted string must be used for a string literal, such as `Name='Alice'`.

### Type Validation

- **Comparison compatibility:** The two operands of every comparison must have compatible types.
- **Valid:** number ↔ number or string ↔ string.
    - Example: `select[Age>20](Student)`
    - Example: `select[Name='Alice'](Student)`
- **Invalid:** number ↔ string.
    - Example: `select[Age>'20'](Student)` is a type error.

### `select`

- The input expression must be semantically valid.
- Every attribute referenced by the condition must exist in the input schema.
- Every comparison must satisfy the type rules.
- The output schema is identical to the input schema.
- Example:
    
    ```
    select[Age>20](Student)
    ```
    

### `project`

- Every projected attribute must exist in the input schema.
- Attributes appear in the specified order.
- Duplicate projection attributes are a semantic error.
    - Example: `project[Name, Name](Student)` is rejected.
- Duplicate output tuples are removed after projection.
    - Example: `project[DeptID](Student)` returns one tuple for each distinct `DeptID`.

### `rename`

- The input expression must be semantically valid.
- The new relation name must not create an invalid/conflicting relation qualifier in the resulting expression.
- Attributes and their types remain unchanged; only the relation name changes.
- Example:
    
    ```
    rename[Student2](Student)
    ```
    
- This allows a self-join such as:
    
    ```
    rename[Student2](Student) join[Student.MentorID=Student2.StudentID] Student2
    ```
    
    where the two instances can be distinguished by their relation names.
    

### `times`

- Both input expressions must be semantically valid.
- The output contains all attributes from both inputs.
- Attributes are qualified by their relation names.
- If the resulting qualified attribute names still collide, report a schema error.
- Example:
    
    ```
    Student times Department
    ```
    
    produces attributes from both `Student` and `Department`, qualified by their relation names.
    

### `join`

- Both input expressions must be semantically valid.
- The join condition is resolved against the combined schema of both inputs.
- Attribute references must resolve unambiguously.
- Comparison operands must have compatible types.
- `join` is semantically equivalent to `times` followed by `select[condition]`; it is a theta join, not a natural join.
- Example:
    
    ```
    Student join[Student.DeptID=Department.DeptID] Department
    ```
    

### `union`, `intersect`, `minus`

- Both input expressions must be semantically valid.
- Both inputs must be **union compatible**:
    - same number of attributes;
    - same attribute names in the same order;
    - compatible types at each corresponding position.
- Example:
    
    ```
    UndergraduateStudent union GraduateStudent
    ```
    
    is valid only if both relations have compatible schemas.
    
- Example:
    
    ```
    Student union Department
    ```
    
    is a semantic schema error when their schemas are incompatible.
    
- The output schema is the left input schema.

### Semantic De-duplication

- tuple deduplication is handled by relation validators keep responsibly separated we assume that our query can just assume that each time  tuple will be a set in each relation

### Error Categories

- **Name error:** unknown relation or attribute.
- **Ambiguity error:** an unqualified attribute matches multiple attributes.
- **Schema error:** incompatible schemas or conflicting output attributes.
- **Type error:** incompatible comparison operand types.
- Every semantic error must produce a clear error message rather than a crash.