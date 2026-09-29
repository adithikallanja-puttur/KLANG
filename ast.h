#ifndef AST_H
#define AST_H

#include <memory>
#include <vector>
#include <iostream>
#include <string>
#include "token.h"
#include "value.h"

using namespace std;

// Base Expression
class Expression
{
public:
    virtual ~Expression() = default;
    virtual void print() = 0;
};

// Base Statement
class Statement
{
public:
    virtual ~Statement() = default;
};

// Literal Value Expression (numbers, strings, booleans, nil)
class LiteralExpression : public Expression
{
public:
    Value value;

    LiteralExpression(const Value& val) : value(val) {}

    void print() override
    {
        if (value.type == ValueType::STRING)
        {
            cout << "\"" << value.stringVal << "\"";
        }
        else
        {
            cout << value.toString();
        }
    }
};

// Preserved NumberExpression for backwards compatibility
class NumberExpression : public Expression
{
public:
    int value;

    NumberExpression(int value)
    {
        this->value = value;
    }

    void print() override
    {
        cout << value;
    }
};

// Variable identifier expression
class VariableExpression : public Expression
{
public:
    string name;

    VariableExpression(string name)
    {
        this->name = name;
    }

    void print() override
    {
        cout << name;
    }
};

// Unary expression (-x, !x)
class UnaryExpression : public Expression
{
public:
    TokenType operation;
    shared_ptr<Expression> right;

    UnaryExpression(TokenType op, shared_ptr<Expression> right)
        : operation(op), right(right) {}

    void print() override
    {
        cout << "(";
        if (operation == TokenType::MINUS) cout << "-";
        if (operation == TokenType::BANG) cout << "!";
        right->print();
        cout << ")";
    }
};

// Binary arithmetic / logical expression (+, -, *, /, %, &&, ||)
class BinaryExpression : public Expression
{
public:
    shared_ptr<Expression> left;
    TokenType operation;
    shared_ptr<Expression> right;

    BinaryExpression(
        shared_ptr<Expression> left,
        TokenType operation,
        shared_ptr<Expression> right)
    {
        this->left = left;
        this->operation = operation;
        this->right = right;
    }

    void print() override
    {
        cout << "(";
        left->print();

        switch (operation)
        {
            case TokenType::PLUS:    cout << " + "; break;
            case TokenType::MINUS:   cout << " - "; break;
            case TokenType::STAR:    cout << " * "; break;
            case TokenType::SLASH:   cout << " / "; break;
            case TokenType::PERCENT: cout << " % "; break;
            case TokenType::AND:     cout << " && "; break;
            case TokenType::OR:      cout << " || "; break;
            default: cout << " ? "; break;
        }

        right->print();
        cout << ")";
    }
};

// Comparison Expression (>, <, >=, <=, ==, !=)
class ComparisonExpression : public Expression
{
public:
    shared_ptr<Expression> left;
    TokenType operation;
    shared_ptr<Expression> right;

    ComparisonExpression(
        shared_ptr<Expression> left,
        TokenType operation,
        shared_ptr<Expression> right)
    {
        this->left = left;
        this->operation = operation;
        this->right = right;
    }

    void print() override
    {
        left->print();

        if(operation == TokenType::GREATER)
            cout << " > ";
        else if(operation == TokenType::LESS)
            cout << " < ";
        else if(operation == TokenType::GREATER_EQUAL)
            cout << " >= ";
        else if(operation == TokenType::LESS_EQUAL)
            cout << " <= ";
        else if(operation == TokenType::EQUAL_EQUAL)
            cout << " == ";
        else if(operation == TokenType::NOT_EQUAL)
            cout << " != ";

        right->print();
    }
};

// Function or method call expression: name(arg1, arg2, ...)
class CallExpression : public Expression
{
public:
    string callee;
    vector<shared_ptr<Expression>> arguments;

    CallExpression(string callee, vector<shared_ptr<Expression>> arguments)
        : callee(callee), arguments(arguments) {}

    void print() override
    {
        cout << callee << "(";
        for (size_t i = 0; i < arguments.size(); i++)
        {
            arguments[i]->print();
            if (i + 1 < arguments.size()) cout << ", ";
        }
        cout << ")";
    }
};

// Array literal: [expr1, expr2, ...]
class ArrayLiteralExpression : public Expression
{
public:
    vector<shared_ptr<Expression>> elements;

    ArrayLiteralExpression(vector<shared_ptr<Expression>> elements)
        : elements(elements) {}

    void print() override
    {
        cout << "[";
        for (size_t i = 0; i < elements.size(); i++)
        {
            elements[i]->print();
            if (i + 1 < elements.size()) cout << ", ";
        }
        cout << "]";
    }
};

// Array index expression: arr[expr]
class ArrayIndexExpression : public Expression
{
public:
    shared_ptr<Expression> array;
    shared_ptr<Expression> index;

    ArrayIndexExpression(shared_ptr<Expression> array, shared_ptr<Expression> index)
        : array(array), index(index) {}

    void print() override
    {
        array->print();
        cout << "[";
        index->print();
        cout << "]";
    }
};

// Array index assignment: arr[expr] = expr
class ArrayIndexAssignExpression : public Expression
{
public:
    shared_ptr<Expression> array;
    shared_ptr<Expression> index;
    shared_ptr<Expression> value;

    ArrayIndexAssignExpression(shared_ptr<Expression> array, shared_ptr<Expression> index, shared_ptr<Expression> value)
        : array(array), index(index), value(value) {}

    void print() override
    {
        array->print();
        cout << "[";
        index->print();
        cout << "] = ";
        value->print();
    }
};

// Variable assignment expression: x = 10
class AssignExpression : public Expression
{
public:
    string name;
    shared_ptr<Expression> value;

    AssignExpression(string name, shared_ptr<Expression> value)
        : name(name), value(value) {}

    void print() override
    {
        cout << name << " = ";
        value->print();
    }
};

// Postfix/Prefix increment & decrement (x++, x--, ++x, --x)
class UpdateExpression : public Expression
{
public:
    string name;
    TokenType operation; // PLUS_PLUS or MINUS_MINUS
    bool isPrefix;

    UpdateExpression(string name, TokenType operation, bool isPrefix)
        : name(name), operation(operation), isPrefix(isPrefix) {}

    void print() override
    {
        if (isPrefix)
        {
            cout << (operation == TokenType::PLUS_PLUS ? "++" : "--") << name;
        }
        else
        {
            cout << name << (operation == TokenType::PLUS_PLUS ? "++" : "--");
        }
    }
};

// Compound assignment expression: x += expr, x -= expr, etc.
class CompoundAssignExpression : public Expression
{
public:
    string name;
    TokenType operation; // PLUS_EQUAL, MINUS_EQUAL, STAR_EQUAL, SLASH_EQUAL, PERCENT_EQUAL
    shared_ptr<Expression> value;

    CompoundAssignExpression(string name, TokenType operation, shared_ptr<Expression> value)
        : name(name), operation(operation), value(value) {}

    void print() override
    {
        cout << name << " ";
        switch (operation)
        {
            case TokenType::PLUS_EQUAL:    cout << "+="; break;
            case TokenType::MINUS_EQUAL:   cout << "-="; break;
            case TokenType::STAR_EQUAL:    cout << "*="; break;
            case TokenType::SLASH_EQUAL:   cout << "/="; break;
            case TokenType::PERCENT_EQUAL: cout << "%="; break;
            default: cout << "="; break;
        }
        cout << " ";
        value->print();
    }
};

// STATEMENTS

// let x = expr;
class LetStatement : public Statement
{
public:
    string name;
    shared_ptr<Expression> value;

    LetStatement(string name, shared_ptr<Expression> value)
    {
        this->name = name;
        this->value = value;
    }
};

// Assignment statement: x = expr;
class AssignStatement : public Statement
{
public:
    string name;
    shared_ptr<Expression> value;

    AssignStatement(string name, shared_ptr<Expression> value)
        : name(name), value(value) {}
};

// print expr;
class PrintStatement : public Statement
{
public:
    shared_ptr<Expression> expression;

    PrintStatement(shared_ptr<Expression> expr)
        : expression(std::move(expr)) {}
};

// Block statement: { stmt1; stmt2; }
class BlockStatement : public Statement
{
public:
    vector<shared_ptr<Statement>> statements;

    BlockStatement(vector<shared_ptr<Statement>> stmts)
        : statements(std::move(stmts)) {}
};

// if (condition) stmt [else stmt]
class IfStatement : public Statement
{
public:
    shared_ptr<Expression> condition;
    shared_ptr<Statement> thenBranch;
    shared_ptr<Statement> elseBranch;
    vector<shared_ptr<Statement>> body; // Backwards compatibility

    IfStatement(
        shared_ptr<Expression> cond,
        shared_ptr<Statement> thenBr,
        shared_ptr<Statement> elseBr = nullptr)
        : condition(std::move(cond)), thenBranch(std::move(thenBr)), elseBranch(std::move(elseBr)) {}

    // Backwards-compatible constructor
    IfStatement(
        shared_ptr<Expression> cond,
        vector<shared_ptr<Statement>> bodyStmts)
        : condition(std::move(cond)), body(bodyStmts)
    {
        thenBranch = make_shared<BlockStatement>(body);
        elseBranch = nullptr;
    }
};

// while (condition) stmt
class WhileStatement : public Statement
{
public:
    shared_ptr<Expression> condition;
    shared_ptr<Statement> body;

    WhileStatement(shared_ptr<Expression> cond, shared_ptr<Statement> bodyStmt)
        : condition(std::move(cond)), body(std::move(bodyStmt)) {}
};

// for (initializer; condition; increment) stmt
class ForStatement : public Statement
{
public:
    shared_ptr<Statement> initializer;
    shared_ptr<Expression> condition;
    shared_ptr<Expression> increment;
    shared_ptr<Statement> body;

    ForStatement(
        shared_ptr<Statement> init,
        shared_ptr<Expression> cond,
        shared_ptr<Expression> incr,
        shared_ptr<Statement> bodyStmt)
        : initializer(std::move(init)), condition(std::move(cond)),
          increment(std::move(incr)), body(std::move(bodyStmt)) {}
};

// break;
class BreakStatement : public Statement
{
public:
    BreakStatement() = default;
};

// continue;
class ContinueStatement : public Statement
{
public:
    ContinueStatement() = default;
};

// return expr;
class ReturnStatement : public Statement
{
public:
    shared_ptr<Expression> value; // nullable (bare return)

    ReturnStatement(shared_ptr<Expression> val = nullptr)
        : value(std::move(val)) {}
};

// fn name(param1, param2, ...) { body }
class FunctionDeclaration : public Statement
{
public:
    string name;
    vector<string> params;
    shared_ptr<Statement> body; // a BlockStatement

    FunctionDeclaration(string fnName, vector<string> fnParams, shared_ptr<Statement> fnBody)
        : name(std::move(fnName)), params(std::move(fnParams)), body(std::move(fnBody)) {}
};

// Expression statement: expr;
class ExpressionStatement : public Statement
{
public:
    shared_ptr<Expression> expression;

    ExpressionStatement(shared_ptr<Expression> expr)
        : expression(std::move(expr)) {}
};

// Entire Program
class Program
{
public:
    vector<shared_ptr<Statement>> statements;
};

#endif