//
// Created by amira on 2026-09-27.
//


#include "SemanticValidator.h"

#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace query
{
    void SemanticValidator::clear()
    {
        schemasByRelationName.clear();
        errors.clear();
    }

    bool SemanticValidator::hasSchema(const std::string& relationName) const
    {
        return schemasByRelationName.find(relationName) != schemasByRelationName.end();
    }

    const RelationSchema& SemanticValidator::getSchema(const std::string& relationName) const
    {
        auto it = schemasByRelationName.find(relationName);

        if (it == schemasByRelationName.end())
        {
            throwSemanticError(
                SemanticErrorCategory::Name,
                "Unknown relation '" + relationName + "'."
            );
        }

        //n an unordered map, each item is a key-value pair:
        //it->first   // relation name
        //it->second  // relation schema

        // for unordered map
        //.first always gives you the Key..
        //second always gives you the Value.

        return it->second;
    }

    Database SemanticValidator::validateDatabase(const Database& database)
    {
        //Before validating a new database, the validator clears old schemas and old errors.
        clear();
        // return deduplicated database by using the actual parsed database
        Database validatedDatabase;

        for (const Table& table : database.tables)
        {
            validateUniqueRelationName(table.name);

            RelationSchema schema = validateTableSchema(table);

            //emplace needed here instead of insert because it is more efficient
            // Just like insert, emplace will only successfully add an item if the key does not already exist in the map.
            // If the key exists, the operation fails and the map remains unchanged.Return Value:
            // It returns a std::pair containing:An iterator pointing to the element
            // (either the newly inserted one or the existing one).
            // A boolean value (true if insertion succeeded, false if the key already existed)

            schemasByRelationName.emplace(table.name, schema);

            //This removes duplicate tuples from this table and stores the cleaned table
            validatedDatabase.tables.push_back(
                deduplicateTuples(table)
            );
        }

        return validatedDatabase;
    }

    SemanticValidationResult SemanticValidator::validateDatabaseCollectErrors(
        const Database& database )
    {
        clear();

        SemanticValidationResult result;

        try
        {
            validateDatabase(database);
        }
        catch (const std::exception&)
        {
            result.ok = false;
        }

        result.errors = errors;
        result.ok = errors.empty();

        return result;
    }

    RelationSchema SemanticValidator::validateQuery(const QueryNode& query)
    {
        if (!query.expression)
        {
            throwSemanticError(
                SemanticErrorCategory::Schema,
                "Query root does not contain an expression."
            );
        }

        return validateExpression(query.expression.get());
    }

    SemanticValidationResult SemanticValidator::validateQueryCollectErrors(
        const QueryNode& query
    )
    {
        errors.clear();

        SemanticValidationResult result;

        try
        {
            validateQuery(query);
        }
        catch (const std::exception&)
        {
            result.ok = false;
        }

        result.errors = errors;
        result.ok = errors.empty();

        return result;
    }

    void SemanticValidator::addError(
        SemanticErrorCategory category,
        const std::string& message
    )
    {
        errors.push_back(
            SemanticError{
                category,
                "[" + categoryToString(category) + " Error] " + message
            }
        );
    }

    [[noreturn]]
    void SemanticValidator::throwSemanticError(
        SemanticErrorCategory category,
        const std::string& message
    ) const
    {
        throw std::runtime_error(
            "[Semantic " + categoryToString(category) + " Error] " + message
        );
    }

    std::string SemanticValidator::categoryToString(
        SemanticErrorCategory category
    )
    {
        switch (category)
        {
        case SemanticErrorCategory::Name:
            return "Name";

        case SemanticErrorCategory::Ambiguity:
            return "Ambiguity";

        case SemanticErrorCategory::Schema:
            return "Schema";

        case SemanticErrorCategory::Type:
            return "Type";
        }

        return "Unknown";
    }

    std::string SemanticValidator::typeToString(
        SemanticValueType type
    )
    {
        switch (type)
        {
        case SemanticValueType::Number:
            return "number";

        case SemanticValueType::String:
            return "string";
        }

        return "unknown";
    }

    void SemanticValidator::validateUniqueRelationName(
        const std::string& relationName
    )
    {
        //schemasByRelationName.end(). represents the "past-the-end" element of the map. Think of it as a placeholder meaning
        //"out of bounds" or "end of the road". It signifies that the search came up empty
        //The find() function searches the map for the key relationName.If it finds the key,
        //it returns an iterator pointing directly to that key-value pair.
        //If it does not find the key, it returns a special iterator called schemasByRelationName.end().

        if (schemasByRelationName.find(relationName) != schemasByRelationName.end())
        {
            const std::string message =
                "Duplicate relation definition for '" + relationName + "'.";

            addError(SemanticErrorCategory::Name, message);
            throwSemanticError(SemanticErrorCategory::Name, message);
        }
    }

    void SemanticValidator::validateUniqueAttributeNames(
        const Table& table
    )
    {
        //This creates an empty hash set called seen. Hash sets only store unique values.
        //They are incredibly fast at checking whether an item exists or inserting a new one.
        std::unordered_set<std::string> seen;

        for (const std::string& attributeName : table.attributes)
        {
            /*
             * When you call .insert() on a std::unordered_set, it doesn't just insert the item;
             * it returns a std::pair containing two pieces of information:.first:
             * An iterator pointing to the item in the set..second: A boolean flag (true or false)
             * indicating whether the insertion actually happened.Returns true if
             * the item was new and successfully added.Returns false if the item was already in the set
             * (a duplicate).
             */


            if (!seen.insert(attributeName).second)
            {
                throw std::runtime_error(
                    "[Semantic Schema Error] Duplicate attribute '" +
                    attributeName +
                    "' in relation '" +
                    table.name +
                    "'."
                );
            }
        }
    }

    RelationSchema SemanticValidator::validateTableSchema(
        const Table& table
    )
    {
        try
        {
            validateUniqueAttributeNames(table);
        }
        catch (const std::exception& error)
        {
            //in built function fo std exception error type
            //When an error or exception occurs and is caught, .what() returns a null-terminated character string
            //(const char*) containing the explanatory text or error message associated with that exception.
            addError(SemanticErrorCategory::Schema, error.what());
            throw;
        }

        RelationSchema schema;
        schema.relationName = table.name;

        if (table.rows.empty())
        {
            for (const std::string& attributeName : table.attributes)
            {
                schema.attributes.push_back(
                    SemanticAttribute{
                        table.name,
                        attributeName,
                        SemanticValueType::String
                    }
                );
            }

            return schema;
        }

        //in built function for vector .front
        //front(): This grabs the first element inside that vector. Since the vector holds Tuple

        const Tuple& firstRow = table.rows.front();


        if (firstRow.values.size() != table.attributes.size())
        {
            const std::string message =
                "Tuple value count (" +
                std::to_string(firstRow.values.size()) +
                ") does not match attribute count (" +
                std::to_string(table.attributes.size()) +
                ") in relation '" +
                table.name +
                "'.";

            addError(SemanticErrorCategory::Schema, message);
            throwSemanticError(SemanticErrorCategory::Schema, message);
        }

        // Simply takes in the type of the first value in the tuple adn based on that assign the type to the attribute
        //this dynamically decides the type of each attribute simply meaning typeof each column in database
        // since we are not give the data type already in the table schema we do it this way

        for (size_t i = 0; i < table.attributes.size(); ++i)
        {
            schema.attributes.push_back(
                SemanticAttribute{
                    table.name,
                    table.attributes[i],
                    typeFromDataValue(firstRow.values[i])
                }
            );
        }

        for (size_t rowIndex = 1; rowIndex < table.rows.size(); ++rowIndex)
        {
            const Tuple& row = table.rows[rowIndex];

            if (row.values.size() != table.attributes.size())
            {
                const std::string message =
                    "Tuple value count (" +
                    std::to_string(row.values.size()) +
                    ") does not match attribute count (" +
                    std::to_string(table.attributes.size()) +
                    ") in relation '" +
                    table.name +
                    "' at row " +
                    std::to_string(rowIndex + 1) +
                    ".";

                addError(SemanticErrorCategory::Schema, message);
                throwSemanticError(SemanticErrorCategory::Schema, message);
            }

            for (size_t columnIndex = 0; columnIndex < row.values.size(); ++columnIndex)
            {
                SemanticValueType actualType = typeFromDataValue(row.values[columnIndex]);
                SemanticValueType expectedType = schema.attributes[columnIndex].type;

                if (actualType != expectedType)
                {
                    const std::string message =
                        "Type mismatch in relation '" +
                        table.name +
                        "', row " +
                        std::to_string(rowIndex + 1) +
                        ", attribute '" +
                        table.attributes[columnIndex] +
                        "'. Expected " +
                        typeToString(expectedType) +
                        " but got " +
                        typeToString(actualType) +
                        ".";

                    addError(SemanticErrorCategory::Type, message);
                    throwSemanticError(SemanticErrorCategory::Type, message);
                }
            }
        }

        return schema;
    }

    SemanticValueType SemanticValidator::typeFromDataValue(
        const DataValue& value
    )
    {
        if (value.kind == dbValueKind::Integer)
        {
            return SemanticValueType::Number;
        }

        return SemanticValueType::String;
    }

    bool SemanticValidator::sameDataValue(
        const DataValue& left,
        const DataValue& right
    )
    {
        return left.kind == right.kind && left.text == right.text;
    }

    /*
     * checks if two tuples are the same
     * cheks them value by value meaning if both values are the same
     * and then evry other value in tuple also matches that suggest 2 tuples are the same
     */
    bool SemanticValidator::sameTuple(
        const Tuple& left,
        const Tuple& right
    )
    {
        if (left.values.size() != right.values.size())
        {
            return false;
        }

        for (size_t i = 0; i < left.values.size(); ++i)
        {
            if (!sameDataValue(left.values[i], right.values[i]))
            {
                return false;
            }
        }

        return true;
    }


    /*
     * The Outer Loop (for (const Tuple& row : table.rows))This steps through your original table,
     * one row at a time (Row 1, Row 2, Row 3, etc.).The Inner Loop (for (const Tuple& existingRow : result.rows))
     * This steps through the result table, which starts out completely empty.It only compares the current row against
     * the rows that have already been approved and added to result.
     */

    Table SemanticValidator::deduplicateTuples(
        const Table& table
    )
    {
        Table result;
        result.name = table.name;
        result.attributes = table.attributes;

        for (const Tuple& row : table.rows)
        {
            // Bare vs quoted is only how a string was spelled in the file, not
            // a different value. Canonicalize to one string kind so that
            // "E1, Bob" and "E1, 'Bob'" are the same tuple and collapse to one.
            Tuple canonical = row;
            for (DataValue& value : canonical.values)
            {
                if (value.kind == dbValueKind::BareString)
                {
                    value.kind = dbValueKind::QuotedString;
                }
            }

            bool alreadyExists = false;

            for (const Tuple& existingRow : result.rows)
            {
                if (sameTuple(canonical, existingRow))
                {
                    alreadyExists = true;
                    break;
                }
            }

            if (!alreadyExists)
            {
                result.rows.push_back(std::move(canonical));
            }
        }

        return result;
    }

    RelationSchema SemanticValidator::validateExpression(
        const ExpressionNode* expression
    )
    {
        if (!expression)
        {
            throwSemanticError(
                SemanticErrorCategory::Schema,
                "Missing relational expression."
            );
        }

        switch (expression->kind)
        {
        case NodeKind::RelationReference:
            return validateRelationReference(
                static_cast<const RelationReferenceNode*>(expression)
            );

        case NodeKind::Union:
        case NodeKind::Minus:
        case NodeKind::Intersect:
        case NodeKind::Times:
            return validateBinaryExpression(
                static_cast<const BinaryExpressionNode*>(expression)
            );

        case NodeKind::Join:
            return validateJoin(
                static_cast<const JoinNode*>(expression)
            );

        case NodeKind::Select:
            return validateSelect(
                static_cast<const SelectNode*>(expression)
            );

        case NodeKind::Project:
            return validateProject(
                static_cast<const ProjectNode*>(expression)
            );

        case NodeKind::Rename:
            return validateRename(
                static_cast<const RenameNode*>(expression)
            );

        default:
            throwSemanticError(
                SemanticErrorCategory::Schema,
                "Invalid node found where relational expression was expected."
            );
        }
    }

    // 1. USING THE ARROW (->)
    // "Go straight to the treasure inside the safe box and read its name."
    // Use this 95% of the time when you just want to use your data.
    // std::string name = userPtr->name;


    // 2. USING .GET()
    // "Don't touch the treasure. Just give me the raw address of where it sits."
    // Use this only when an old function asks for an old-school raw pointer (*).
    //User* rawAddress = userPtr.get();


    RelationSchema SemanticValidator::validateRelationReference(
        const RelationReferenceNode* node
    )
    {
        if (!node)
        {
            throwSemanticError(
                SemanticErrorCategory::Schema,
                "Missing relation reference."
            );
        }

        auto it = schemasByRelationName.find(node->name);

        if (it == schemasByRelationName.end())
        {
            const std::string message =
                "Unknown relation '" + node->name + "'.";

            addError(SemanticErrorCategory::Name, message);
            throwSemanticError(SemanticErrorCategory::Name, message);
        }

        // for unordered map
        //.first always gives you the Key..
        //second always gives you the Value.
        return it->second;
    }

    RelationSchema SemanticValidator::validateBinaryExpression(
        const BinaryExpressionNode* node
    )
    {
        if (!node)
        {
            throwSemanticError(
                SemanticErrorCategory::Schema,
                "Missing binary expression."
            );
        }

        RelationSchema leftSchema = validateExpression(node->left.get());
        RelationSchema rightSchema = validateExpression(node->right.get());

        switch (node->op)
        {
        case BinaryOperator::Union:
            validateUnionCompatible(leftSchema, rightSchema, "union");
            return leftSchema;

        case BinaryOperator::Minus:
            validateUnionCompatible(leftSchema, rightSchema, "minus");
            return leftSchema;

        case BinaryOperator::Intersect:
            validateUnionCompatible(leftSchema, rightSchema, "intersect");
            return leftSchema;

        case BinaryOperator::Times:
            return combineSchemas(leftSchema, rightSchema, "times");
        }

        throwSemanticError(
            SemanticErrorCategory::Schema,
            "Unknown binary relational operator."
        );
    }

    RelationSchema SemanticValidator::validateJoin(
        const JoinNode* node
    )
    {
        if (!node)
        {
            throwSemanticError(
                SemanticErrorCategory::Schema,
                "Missing join expression."
            );
        }

        RelationSchema leftSchema = validateExpression(node->left.get());
        RelationSchema rightSchema = validateExpression(node->right.get());

        RelationSchema combinedSchema = combineSchemas(
            leftSchema,
            rightSchema,
            "join"
        );

        validateCondition(node->condition.get(), combinedSchema);

        return combinedSchema;
    }

    //In Relational Algebra (and in our compiler): Select means filtering rows based on a condition.
    //It is the equivalent of the SQL WHERE clause.

    RelationSchema SemanticValidator::validateSelect(
        const SelectNode* node
    )
    {
        //cheks if  null then error
        if (!node)
        {
            throwSemanticError(
                SemanticErrorCategory::Schema,
                "Missing select expression."
            );
        }

        RelationSchema inputSchema = validateExpression(node->expression.get());

        validateCondition(node->condition.get(), inputSchema);

        return inputSchema;
    }

    // In SQL: SELECT id, name is used to choose which columns you want to see. In relational algebra,
    // this column-choosing operation is actually called Project (handled by a ProjectNode).
    RelationSchema SemanticValidator::validateProject(
        const ProjectNode* node
    )
    {
        if (!node)
        {
            throwSemanticError(
                SemanticErrorCategory::Schema,
                "Missing project expression."
            );
        }

        RelationSchema inputSchema = validateExpression(node->expression.get());
        RelationSchema outputSchema;
        outputSchema.relationName = inputSchema.relationName;

        std::unordered_set<std::string> projectedNames;

        for (const auto& attributeReference : node->attributes)
        {
            // calling this method for each attribute eon by one makes sure that we refer to right ambiguous names
            const SemanticAttribute& attribute =
                resolveAttribute(attributeReference.get(), inputSchema);

            std::string outputName = qualifiedAttributeName(attribute);
            // since inserting in set will return false if the element is already present
            // second gives us the information if the element was already present as it is boolean
            if (!projectedNames.insert(outputName).second)
            {
                const std::string message =
                    "Duplicate projection attribute '" +
                    outputName +
                    "'.";

                addError(SemanticErrorCategory::Schema, message);
                throwSemanticError(SemanticErrorCategory::Schema, message);
            }

            outputSchema.attributes.push_back(attribute);
        }

        validateNoQualifiedCollisions(outputSchema, "project");

        return outputSchema;
    }

    RelationSchema SemanticValidator::validateRename(
        const RenameNode* node
    )
    {
        if (!node)
        {
            throwSemanticError(
                SemanticErrorCategory::Schema,
                "Missing rename expression."
            );
        }

        RelationSchema inputSchema = validateExpression(node->expression.get());

        RelationSchema outputSchema;
        // assign the new name to ouput schema and copies all values from the actual table we found
        outputSchema.relationName = node->relationName;

        for (const SemanticAttribute& attribute : inputSchema.attributes)
        {
            outputSchema.attributes.push_back(
                SemanticAttribute{
                    node->relationName,
                    attribute.name,
                    attribute.type
                }
            );
        }

        validateNoQualifiedCollisions(outputSchema, "rename");

        return outputSchema;
    }

    void SemanticValidator::validateCondition(
        const ConditionNode* condition,
        const RelationSchema& scope
    )
    {
        if (!condition)
        {
            throwSemanticError(
                SemanticErrorCategory::Schema,
                "Missing condition."
            );
        }

        switch (condition->kind)
        {
        case NodeKind::And:
        case NodeKind::Or:
        {
            //static_cast is a compile-time casting operator in C++ used to safely
            //convert one data type into another when the compiler already knows how to make the translation between them
            //if try two completely unrelated things (like a random class to an unrelated integer pointer),
            //the compiler will throw an error and stop you
            const auto* logical =
                static_cast<const LogicalConditionNode*>(condition);

            validateCondition(logical->left.get(), scope);
            validateCondition(logical->right.get(), scope);
            return;
        }

        case NodeKind::Not:
        {
            const auto* notNode =
                static_cast<const NotConditionNode*>(condition);

            validateCondition(notNode->operand.get(), scope);
            return;
        }

        case NodeKind::Comparison:
        {
            const auto* comparison =
                static_cast<const ComparisonNode*>(condition);

            SemanticValueType leftType =
                validateOperand(comparison->left.get(), scope);

            SemanticValueType rightType =
                validateOperand(comparison->right.get(), scope);

            if (leftType != rightType)
            {
                const std::string message =
                    "Incompatible comparison operand types: " +
                    typeToString(leftType) +
                    " and " +
                    typeToString(rightType) +
                    ".";

                addError(SemanticErrorCategory::Type, message);
                throwSemanticError(SemanticErrorCategory::Type, message);
            }

            return;
        }

        default:
            throwSemanticError(
                SemanticErrorCategory::Schema,
                "Invalid condition node."
            );
        }
    }

    SemanticValueType SemanticValidator::validateOperand(
        const OperandNode* operand,
        const RelationSchema& scope
    )
    {
        if (!operand)
        {
            throwSemanticError(
                SemanticErrorCategory::Schema,
                "Missing comparison operand."
            );
        }

        switch (operand->kind)
        {
        case NodeKind::LiteralOperand:
            return validateLiteralOperand(
                static_cast<const LiteralOperandNode*>(operand)
            );

        case NodeKind::AttributeOperand:
            return validateAttributeOperand(
                static_cast<const AttributeOperandNode*>(operand),
                scope
            );

        default:
            throwSemanticError(
                SemanticErrorCategory::Schema,
                "Invalid comparison operand node."
            );
        }
    }

    SemanticValueType SemanticValidator::validateLiteralOperand(
        const LiteralOperandNode* literal
    )
    {
        if (!literal)
        {
            throwSemanticError(
                SemanticErrorCategory::Schema,
                "Missing literal operand."
            );
        }

        if (literal->valueKind == ValueKind::Integer)
        {
            return SemanticValueType::Number;
        }

        return SemanticValueType::String;
    }

    SemanticValueType SemanticValidator::validateAttributeOperand(
        const AttributeOperandNode* operand,
        const RelationSchema& scope
    )
    {
        if (!operand || !operand->reference)
        {
            throwSemanticError(
                SemanticErrorCategory::Schema,
                "Missing attribute operand reference."
            );
        }

        const SemanticAttribute& attribute =
            resolveAttribute(operand->reference.get(), scope);

        return attribute.type;
    }


    /**
 * WHAT: Resolves a single column/attribute reference to its actual data schema.
 *
 * HOW:
 *  1. Loops through all available columns in the query's current scope.
 *  2. If the user specified a table name (e.g., users.id), it matches both the
 *     table name AND column name.
 *  3. If no table name was provided (e.g., just 'id'), it matches any column
 *     with that name, adding all possibilities to a temporary 'matches' list.
 *
 * WHY:
 *  We use a list ('matches') to catch ambiguity. If a column name exists in
 *  multiple tables and the user didn't specify which table they wanted, the list
 *  will catch more than 1 item, allowing us to safely throw an "Ambiguous" error.
 *  If exactly 1 unique match is found, it is successfully returned.
 *
 * NOTE: For queries with multiple columns (like SELECT id, name), this function
 *       is called separately, one-by-one, for each individual column.
 */

    const SemanticAttribute& SemanticValidator::resolveAttribute(
        const AttributeReferenceNode* reference,
        const RelationSchema& scope
    )
    {
        if (!reference)
        {
            throwSemanticError(
                SemanticErrorCategory::Schema,
                "Missing attribute reference."
            );
        }

        // a vector that pushes if match is found
        std::vector<const SemanticAttribute*> matches;

        for (const SemanticAttribute& attribute : scope.attributes)
        {
            if (attribute.name != reference->name)
            {
                continue;
            }

            if (reference->relationQualifier.has_value())
            {
                //uses AST.h AttributeRefernceNode relationqaulifier valuw hich telss which relation teh attribute belongs to
                if (attribute.relationName == reference->relationQualifier.value())
                {
                    matches.push_back(&attribute);
                }
            }
            else
            {
                matches.push_back(&attribute);
            }
        }

        if (reference->relationQualifier.has_value() && matches.empty())
        {
            const std::string qualifier = reference->relationQualifier.value();

            const std::string message =
                "Unknown qualified attribute '" +
                qualifier +
                "." +
                reference->name +
                "'.";

            addError(SemanticErrorCategory::Name, message);
            throwSemanticError(SemanticErrorCategory::Name, message);
        }

        if (matches.empty())
        {
            const std::string message =
                "Unknown attribute '" +
                reference->name +
                "'.";

            addError(SemanticErrorCategory::Name, message);
            throwSemanticError(SemanticErrorCategory::Name, message);
        }

        if (matches.size() > 1)
        {
            const std::string message =
                "Ambiguous unqualified attribute '" +
                reference->name +
                "'. Use a relation qualifier.";

            addError(SemanticErrorCategory::Ambiguity, message);
            throwSemanticError(SemanticErrorCategory::Ambiguity, message);
        }

        return *matches.front();
    }

    RelationSchema SemanticValidator::combineSchemas(
        const RelationSchema& left,
        const RelationSchema& right,
        const std::string& operationName
    )
    {
        //.reserve() is a built-in optimization method for std::vector in C++. It tells the vector ahead
        //of time exactly how much memory it needs to allocate.Hey combined.attributes vector, I am about to add all the columns
        //from the left table and the right table into you. Go ahead and request enough memory space from the system
        //to fit them all right now."

        RelationSchema combined;
        combined.relationName = left.relationName + "_" + operationName + "_" + right.relationName;

        combined.attributes.reserve(
            left.attributes.size() + right.attributes.size()
        );

        //modern efficient alternative
        // Modern alternative: Faster to read, same great performance under the hood
        // combined.attributes.insert(combined.attributes.end(), left.attributes.begin(), left.attributes.end());
        // combined.attributes.insert(combined.attributes.end(), right.attributes.begin(), right.attributes.end());

        for (const SemanticAttribute& attribute : left.attributes)
        {
            combined.attributes.push_back(attribute);
        }

        for (const SemanticAttribute& attribute : right.attributes)
        {
            combined.attributes.push_back(attribute);
        }

        validateNoQualifiedCollisions(combined, operationName);

        return combined;
    }

    /*
  * WHY THIS CHECK IS NEEDED:
  *
  * Each relation is validated separately to ensure that its own attribute names
  * are unique. However, a query can combine two otherwise valid relations using
  * TIMES or JOIN. After combining them, attributes are identified using their
  * relation-qualified names.
  *
  * Example:
  *     rename[E](Employees) times rename[E](Departments)
  *
  * Both input relations are individually valid, but the combined schema would
  * contain duplicate qualified attributes such as:
  *
  *     E.ID
  *     E.ID
  *
  * This is therefore a schema conflict created by the query, not by either
  * original relation definition.
  *
  * HOW WE HANDLE IT:
  *
  * combineSchemas() first copies the attributes from both input schemas into
  * one combined schema. It then calls validateNoQualifiedCollisions(), which
  * builds qualified names in the form:
  *
  *     relationName + "." + attributeName
  *
  * An unordered_set is used to track the qualified names already seen. If the
  * same qualified name appears more than once, the query is rejected with a
  * semantic schema error.
  *
  * This check is needed for binary operations such as TIMES and JOIN because
  * those operations create a new combined schema. It is not needed again for
  * relation definitions, and it is redundant for RENAME and PROJECT.
  */
    void SemanticValidator::validateNoQualifiedCollisions(
        const RelationSchema& schema,
        const std::string& operationName
    )
    {
        if (!schemaHasDuplicateQualifiedAttributes(schema))
        {
            return;
        }

        throw std::runtime_error(
            "[Semantic Schema Error] Conflicting qualified output attributes after '" +
            operationName +
            "'."
        );
    }

    bool SemanticValidator::schemaHasDuplicateQualifiedAttributes(
        const RelationSchema& schema
    )
    {
        std::unordered_set<std::string> seen;

        for (const SemanticAttribute& attribute : schema.attributes)
        {
            if (!seen.insert(qualifiedAttributeName(attribute)).second)
            {
                return true;
            }
        }

        return false;
    }


    void SemanticValidator::validateUnionCompatible(
        const RelationSchema& left,
        const RelationSchema& right,
        const std::string& operationName
    )
    {
        if (left.attributes.size() != right.attributes.size())
        {
            throw std::runtime_error(
                "[Semantic Schema Error] Relation schemas are not union-compatible for '" +
                operationName +
                "': attribute counts differ."
            );
        }

        for (size_t i = 0; i < left.attributes.size(); ++i)
        {
            const SemanticAttribute& leftAttribute = left.attributes[i];
            const SemanticAttribute& rightAttribute = right.attributes[i];

            if (leftAttribute.name != rightAttribute.name)
            {
                throw std::runtime_error(
                    "[Semantic Schema Error] Relation schemas are not union-compatible for '" +
                    operationName +
                    "': attribute name mismatch at position " +
                    std::to_string(i + 1) +
                    " ('" +
                    leftAttribute.name +
                    "' vs '" +
                    rightAttribute.name +
                    "')."
                );
            }

            if (leftAttribute.type != rightAttribute.type)
            {
                throw std::runtime_error(
                    "[Semantic Type Error] Relation schemas are not union-compatible for '" +
                    operationName +
                    "': attribute '" +
                    leftAttribute.name +
                    "' has type " +
                    typeToString(leftAttribute.type) +
                    " on the left but " +
                    typeToString(rightAttribute.type) +
                    " on the right."
                );
            }
        }
    }

    std::string SemanticValidator::qualifiedAttributeName(
        const SemanticAttribute& attribute
    )
    {
        return attribute.relationName + "." + attribute.name;
    }

    bool SemanticValidator::schemaHasRelationQualifier(
        const RelationSchema& schema,
        const std::string& relationName
    )
    {
        for (const SemanticAttribute& attribute : schema.attributes)
        {
            if (attribute.relationName == relationName)
            {
                return true;
            }
        }

        return false;
    }


} // query