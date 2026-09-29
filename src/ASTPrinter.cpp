#include "ASTPrinter.h"
#include <vector>

namespace query
{
    //Binds the class's internal member variable out_ ( reference variable  std::ostream& out_;)
    //to the stream parameter out passed into the constructor. make it easy for reading
    ASTPrinter::ASTPrinter(std::ostream& out)
        : out_(out)
    {
    }

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

    void ASTPrinter::printTable(const Table table) {

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

    void ASTPrinter::printAll(const RelationFileNode* relFile, const QueryNode* query)
    {
        if (relFile)
        {
            out_ << "================ RELATIONS DATA ================\n";
            printRelationFile(*relFile);
            out_ << "\n";
        }

        if (query)
        {
            out_ << "=================== QUERY AST ==================\n";
            printQuery(*query);
            out_ << "\n";
        }
    }

    void ASTPrinter::printQuery(const QueryNode& query)
    {
        out_ << "QueryNode\n";
        if (query.expression)
        {
            printNode(query.expression.get(), "", true);
        }
        else
        {
            out_ << "|____ [Empty Expression]\n";
        }
    }

    void ASTPrinter::printRelationFile(const RelationFileNode& file)
    {
        out_ << "RelationFileNode (" << file.definitions.size() << " definitions)\n";
        for (size_t i = 0; i < file.definitions.size(); ++i)
        {
            bool isLast = (i == file.definitions.size() - 1);
            if (file.definitions[i])
            {
                printRelationDefinition(*file.definitions[i], "", isLast);
            }
        }
    }

    void ASTPrinter::printRelationDefinition(const RelationDefinitionNode& def, const std::string& prefix, bool isLast)
    {
        out_ << prefix << (isLast ? "|____ " : "|---- ") << "RelationDefinition: " << def.name << " (";
        for (size_t i = 0; i < def.attributes.size(); ++i)
        {
            out_ << def.attributes[i] << (i + 1 < def.attributes.size() ? ", " : "");
        }
        out_ << ")\n";

        std::string childPrefix = prefix + (isLast ? "      " : "|     ");

        for (size_t i = 0; i < def.rows.size(); ++i)
        {
            bool rowIsLast = (i == def.rows.size() - 1);
            if (def.rows[i])
            {
                printRelationRow(*def.rows[i], childPrefix, rowIsLast);
            }
        }
    }

    void ASTPrinter::printRelationRow(const RelationRowNode& row, const std::string& prefix, bool isLast)
    {
        out_ << prefix << (isLast ? "|____ " : "|---- ") << "Row: [";
        for (size_t i = 0; i < row.values.size(); ++i)
        {
            out_ << row.values[i].text << (i + 1 < row.values.size() ? ", " : "");
        }
        out_ << "]\n";
    }

    void ASTPrinter::printNode(const ASTNode* node, const std::string& prefix, bool isLast)
    {
        if (!node) return;

        // Print branch connector
        out_ << prefix << (isLast ? "|____ " : "|---- ");

        // Compute prefix for children (6 characters wide to align with "|____ ")
        std::string childPrefix = prefix + (isLast ? "      " : "|     ");

        switch (node->kind)
        {
        case NodeKind::RelationReference:
        {
            auto ref = static_cast<const RelationReferenceNode*>(node);
            out_ << "RelationReference: " << ref->name << "\n";
            break;
        }

        case NodeKind::AttributeReference:
        {
            auto attr = static_cast<const AttributeReferenceNode*>(node);
            out_ << "AttributeReference: ";
            if (attr->relationQualifier.has_value())
            {
                out_ << attr->relationQualifier.value() << ".";
            }
            out_ << attr->name << "\n";
            break;
        }

        case NodeKind::LiteralOperand:
        {
            auto lit = static_cast<const LiteralOperandNode*>(node);
            out_ << "LiteralOperand (" << valueKindToString(lit->valueKind) << "): " << lit->text << "\n";
            break;
        }

        case NodeKind::AttributeOperand:
        {
            auto op = static_cast<const AttributeOperandNode*>(node);
            out_ << "AttributeOperand\n";
            if (op->reference)
            {
                printNode(op->reference.get(), childPrefix, true);
            }
            break;
        }

        case NodeKind::Union:
        case NodeKind::Minus:
        case NodeKind::Intersect:
        case NodeKind::Times:
        {
            auto bin = static_cast<const BinaryExpressionNode*>(node);
            out_ << "BinaryExpression (" << binaryOpToString(bin->op) << ")\n";
            printNode(bin->left.get(), childPrefix, false);
            printNode(bin->right.get(), childPrefix, true);
            break;
        }

        case NodeKind::Join:
        {
            auto join = static_cast<const JoinNode*>(node);
            out_ << "JoinNode\n";
            printNode(join->left.get(), childPrefix, false);
            printNode(join->condition.get(), childPrefix, false);
            printNode(join->right.get(), childPrefix, true);
            break;
        }

        case NodeKind::Select:
        {
            auto sel = static_cast<const SelectNode*>(node);
            out_ << "SelectNode\n";
            printNode(sel->condition.get(), childPrefix, false);
            printNode(sel->expression.get(), childPrefix, true);
            break;
        }

        case NodeKind::Project:
        {
            auto proj = static_cast<const ProjectNode*>(node);
            out_ << "ProjectNode\n";

            size_t totalChildren = proj->attributes.size() + (proj->expression ? 1 : 0);
            size_t processed = 0;

            for (const auto& attr : proj->attributes)
            {
                processed++;
                bool itemIsLast = (processed == totalChildren);
                printNode(attr.get(), childPrefix, itemIsLast);
            }

            if (proj->expression)
            {
                processed++;
                bool itemIsLast = (processed == totalChildren);
                printNode(proj->expression.get(), childPrefix, itemIsLast);
            }
            break;
        }

        case NodeKind::Rename:
        {
            auto ren = static_cast<const RenameNode*>(node);
            out_ << "RenameNode: New relationName = " << ren->relationName << "\n";
            printNode(ren->expression.get(), childPrefix, true);
            break;
        }

        case NodeKind::And:
        case NodeKind::Or:
        {
            auto log = static_cast<const LogicalConditionNode*>(node);
            out_ << "LogicalCondition (" << logicalOpToString(log->op) << ")\n";
            printNode(log->left.get(), childPrefix, false);
            printNode(log->right.get(), childPrefix, true);
            break;
        }

        case NodeKind::Not:
        {
            auto notNode = static_cast<const NotConditionNode*>(node);
            out_ << "NotConditionNode\n";
            printNode(notNode->operand.get(), childPrefix, true);
            break;
        }

        case NodeKind::Comparison:
        {
            auto comp = static_cast<const ComparisonNode*>(node);
            out_ << "ComparisonNode (" << comparisonOpToString(comp->op) << ")\n";
            printNode(comp->left.get(), childPrefix, false);
            printNode(comp->right.get(), childPrefix, true);
            break;
        }

        default:
            out_ << "Unknown Node\n";
            break;
        }
    }

    std::string ASTPrinter::binaryOpToString(BinaryOperator op)
    {
        switch (op)
        {
        case BinaryOperator::Union:     return "UNION";
        case BinaryOperator::Minus:     return "MINUS";
        case BinaryOperator::Intersect: return "INTERSECT";
        case BinaryOperator::Times:     return "TIMES";
        }
        return "UNKNOWN";
    }

    std::string ASTPrinter::logicalOpToString(LogicalOperator op)
    {
        switch (op)
        {
        case LogicalOperator::And: return "AND";
        case LogicalOperator::Or:  return "OR";
        }
        return "UNKNOWN";
    }

    std::string ASTPrinter::comparisonOpToString(ComparisonOperator op)
    {
        switch (op)
        {
        case ComparisonOperator::Equal:        return "=";
        case ComparisonOperator::NotEqual:     return "!=";
        case ComparisonOperator::Less:         return "<";
        case ComparisonOperator::LessEqual:    return "<=";
        case ComparisonOperator::Greater:      return ">";
        case ComparisonOperator::GreaterEqual: return ">=";
        }
        return "UNKNOWN";
    }

    std::string ASTPrinter::valueKindToString(ValueKind kind)
    {
        switch (kind)
        {
        case ValueKind::Integer:      return "Integer";
        case ValueKind::QuotedString: return "QuotedString";
        case ValueKind::BareString:   return "BareString";
        }
        return "UNKNOWN";
    }
} // namespace query