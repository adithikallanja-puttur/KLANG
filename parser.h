#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <iostream>
#include <memory>
#include <initializer_list>
#include "token.h"
#include "ast.h"

using namespace std;

class Parser
{
private:
    vector<Token> tokens;
    int current;

    const Token& peek() const
    {
        if (current >= (int)tokens.size())
        {
            static Token eofToken = {TokenType::EOF_TOKEN, "", 0};
            return eofToken;
        }
        return tokens[current];
    }

    const Token& previous() const
    {
        return tokens[current - 1];
    }

    bool isAtEnd() const
    {
        return peek().type == TokenType::EOF_TOKEN;
    }

    bool check(TokenType type) const
    {
        if (isAtEnd()) return false;
        return peek().type == type;
    }

    Token advance()
    {
        if (!isAtEnd()) current++;
        return previous();
    }

    bool match(initializer_list<TokenType> types)
    {
        for (TokenType type : types)
        {
            if (check(type))
            {
                advance();
                return true;
            }
        }
        return false;
    }

    Token consume(TokenType type, const string& message)
    {
        if (check(type)) return advance();
        cerr << "Syntax Error at line " << peek().line << ": " << message 
             << " (got " << tokenTypeName(peek().type) << " '" << peek().value << "')" << endl;
        return peek();
    }

    void optionalSemicolon()
    {
        if (check(TokenType::SEMICOLON))
        {
            advance();
        }
        else if (check(TokenType::QUESTION))
        {
            // Backwards compatibility with question mark terminator
            advance();
        }
    }

public:
    Parser(vector<Token> tokens)
    {
        this->tokens = tokens;
        current = 0;
    }

    // PRIMARY
    shared_ptr<Expression> primary()
    {
        if (match({TokenType::FALSE}))
        {
            return make_shared<LiteralExpression>(Value(false));
        }

        if (match({TokenType::TRUE}))
        {
            return make_shared<LiteralExpression>(Value(true));
        }

        if (match({TokenType::NUMBER}))
        {
            string numStr = previous().value;
            if (numStr.find('.') != string::npos)
            {
                return make_shared<LiteralExpression>(Value(stod(numStr)));
            }
            else
            {
                return make_shared<NumberExpression>(stoi(numStr));
            }
        }

        if (match({TokenType::STRING}))
        {
            return make_shared<LiteralExpression>(Value(previous().value));
        }

        // Array literal: [elem1, elem2, ...]
        if (match({TokenType::LEFT_BRACKET}))
        {
            vector<shared_ptr<Expression>> elements;
            if (!check(TokenType::RIGHT_BRACKET))
            {
                do
                {
                    elements.push_back(expression());
                } while (match({TokenType::COMMA}));
            }
            consume(TokenType::RIGHT_BRACKET, "Expected ']' after array elements.");
            return make_shared<ArrayLiteralExpression>(elements);
        }

        if (match({TokenType::IDENTIFIER}))
        {
            return make_shared<VariableExpression>(previous().value);
        }

        if (match({TokenType::LEFT_PAREN}))
        {
            shared_ptr<Expression> expr = expression();
            consume(TokenType::RIGHT_PAREN, "Expected ')' after expression.");
            return expr;
        }

        cerr << "Syntax Error at line " << peek().line << ": Expected expression, found '" 
             << peek().value << "' (" << tokenTypeName(peek().type) << ")" << endl;
        advance();
        return make_shared<LiteralExpression>(Value::nil());
    }

    // FUNCTION / METHOD CALL
    shared_ptr<Expression> finishCall(shared_ptr<Expression> callee)
    {
        vector<shared_ptr<Expression>> arguments;

        if (!check(TokenType::RIGHT_PAREN))
        {
            do
            {
                arguments.push_back(expression());
            } while (match({TokenType::COMMA}));
        }

        consume(TokenType::RIGHT_PAREN, "Expected ')' after function arguments.");

        VariableExpression* varExpr = dynamic_cast<VariableExpression*>(callee.get());
        string name = varExpr ? varExpr->name : "anonymous";

        return make_shared<CallExpression>(name, arguments);
    }

    shared_ptr<Expression> call()
    {
        shared_ptr<Expression> expr = primary();

        while (true)
        {
            if (match({TokenType::LEFT_PAREN}))
            {
                expr = finishCall(expr);
            }
            else if (match({TokenType::LEFT_BRACKET}))
            {
                // Array indexing: expr[index]
                shared_ptr<Expression> index = expression();
                consume(TokenType::RIGHT_BRACKET, "Expected ']' after array index.");
                expr = make_shared<ArrayIndexExpression>(expr, index);
            }
            else
            {
                break;
            }
        }

        return expr;
    }

    // POSTFIX (x++, x--)
    shared_ptr<Expression> postfix()
    {
        shared_ptr<Expression> expr = call();

        if (match({TokenType::PLUS_PLUS, TokenType::MINUS_MINUS}))
        {
            TokenType op = previous().type;
            VariableExpression* var = dynamic_cast<VariableExpression*>(expr.get());
            if (var != nullptr)
            {
                return make_shared<UpdateExpression>(var->name, op, false);
            }
            cerr << "Syntax Error at line " << previous().line << ": Invalid target for postfix increment/decrement." << endl;
        }

        return expr;
    }

    // UNARY (-x, !x, ++x, --x)
    shared_ptr<Expression> unary()
    {
        if (match({TokenType::BANG, TokenType::MINUS}))
        {
            TokenType op = previous().type;
            shared_ptr<Expression> right = unary();
            return make_shared<UnaryExpression>(op, right);
        }

        if (match({TokenType::PLUS_PLUS, TokenType::MINUS_MINUS}))
        {
            TokenType op = previous().type;
            Token nameToken = consume(TokenType::IDENTIFIER, "Expected variable name after prefix increment/decrement.");
            return make_shared<UpdateExpression>(nameToken.value, op, true);
        }

        return postfix();
    }

    // FACTOR (*, /, %)
    shared_ptr<Expression> factor()
    {
        shared_ptr<Expression> left = unary();

        while (match({TokenType::STAR, TokenType::SLASH, TokenType::PERCENT}))
        {
            TokenType op = previous().type;
            shared_ptr<Expression> right = unary();
            left = make_shared<BinaryExpression>(left, op, right);
        }

        return left;
    }

    // TERM (+, -)
    shared_ptr<Expression> term()
    {
        shared_ptr<Expression> left = factor();

        while (match({TokenType::PLUS, TokenType::MINUS}))
        {
            TokenType op = previous().type;
            shared_ptr<Expression> right = factor();
            left = make_shared<BinaryExpression>(left, op, right);
        }

        return left;
    }

    // COMPARISON (<, <=, >, >=)
    shared_ptr<Expression> comparison()
    {
        shared_ptr<Expression> left = term();

        while (match({TokenType::GREATER, TokenType::GREATER_EQUAL, TokenType::LESS, TokenType::LESS_EQUAL}))
        {
            TokenType op = previous().type;
            shared_ptr<Expression> right = term();
            left = make_shared<ComparisonExpression>(left, op, right);
        }

        return left;
    }

    // EQUALITY (==, !=)
    shared_ptr<Expression> equality()
    {
        shared_ptr<Expression> left = comparison();

        while (match({TokenType::EQUAL_EQUAL, TokenType::NOT_EQUAL}))
        {
            TokenType op = previous().type;
            shared_ptr<Expression> right = comparison();
            left = make_shared<ComparisonExpression>(left, op, right);
        }

        return left;
    }

    // LOGICAL AND (&&)
    shared_ptr<Expression> logicalAnd()
    {
        shared_ptr<Expression> left = equality();

        while (match({TokenType::AND}))
        {
            TokenType op = previous().type;
            shared_ptr<Expression> right = equality();
            left = make_shared<BinaryExpression>(left, op, right);
        }

        return left;
    }

    // LOGICAL OR (||)
    shared_ptr<Expression> logicalOr()
    {
        shared_ptr<Expression> left = logicalAnd();

        while (match({TokenType::OR}))
        {
            TokenType op = previous().type;
            shared_ptr<Expression> right = logicalAnd();
            left = make_shared<BinaryExpression>(left, op, right);
        }

        return left;
    }

    // ASSIGNMENT (x = expr, x += expr, arr[i] = expr, etc.)
    shared_ptr<Expression> assignment()
    {
        shared_ptr<Expression> expr = logicalOr();

        if (match({TokenType::EQUAL, TokenType::PLUS_EQUAL, TokenType::MINUS_EQUAL,
                   TokenType::STAR_EQUAL, TokenType::SLASH_EQUAL, TokenType::PERCENT_EQUAL}))
        {
            Token opToken = previous();
            shared_ptr<Expression> value = assignment();

            VariableExpression* var = dynamic_cast<VariableExpression*>(expr.get());
            if (var != nullptr)
            {
                if (opToken.type == TokenType::EQUAL)
                {
                    return make_shared<AssignExpression>(var->name, value);
                }
                else
                {
                    return make_shared<CompoundAssignExpression>(var->name, opToken.type, value);
                }
            }

            // Array index assignment: arr[i] = val
            ArrayIndexExpression* arrIdx = dynamic_cast<ArrayIndexExpression*>(expr.get());
            if (arrIdx != nullptr && opToken.type == TokenType::EQUAL)
            {
                return make_shared<ArrayIndexAssignExpression>(arrIdx->array, arrIdx->index, value);
            }

            cerr << "Syntax Error at line " << opToken.line << ": Invalid assignment target." << endl;
        }

        return expr;
    }

    shared_ptr<Expression> expression()
    {
        return assignment();
    }

    // STATEMENTS

    shared_ptr<Statement> letStatement()
    {
        return letStatementImpl(true);
    }

    // Version used inside for-init that does NOT consume the trailing ;
    shared_ptr<Statement> forLetStatement()
    {
        return letStatementImpl(false);
    }

    shared_ptr<Statement> letStatementImpl(bool consumeSemi)
    {
        Token nameToken = consume(TokenType::IDENTIFIER, "Expected variable name after 'let'.");

        consume(TokenType::EQUAL, "Expected '=' after variable name in 'let' declaration.");

        shared_ptr<Expression> value = expression();

        if (consumeSemi) optionalSemicolon();

        return make_shared<LetStatement>(nameToken.value, value);
    }

    shared_ptr<Statement> printStatement()
    {
        shared_ptr<Expression> value = expression();

        optionalSemicolon();

        return make_shared<PrintStatement>(value);
    }

    shared_ptr<Statement> blockStatement()
    {
        vector<shared_ptr<Statement>> statements;

        while (!check(TokenType::RIGHT_BRACE) && !isAtEnd())
        {
            shared_ptr<Statement> stmt = declaration();
            if (stmt != nullptr)
            {
                statements.push_back(stmt);
            }
        }

        consume(TokenType::RIGHT_BRACE, "Expected '}' after block.");
        return make_shared<BlockStatement>(statements);
    }

    shared_ptr<Statement> ifStatement()
    {
        bool hasParen = match({TokenType::LEFT_PAREN});
        shared_ptr<Expression> condition = expression();
        if (hasParen)
        {
            consume(TokenType::RIGHT_PAREN, "Expected ')' after if condition.");
        }

        shared_ptr<Statement> thenBranch = statement();
        shared_ptr<Statement> elseBranch = nullptr;

        if (match({TokenType::ELSE}))
        {
            elseBranch = statement();
        }

        return make_shared<IfStatement>(condition, thenBranch, elseBranch);
    }

    shared_ptr<Statement> whileStatement()
    {
        bool hasParen = match({TokenType::LEFT_PAREN});
        shared_ptr<Expression> condition = expression();
        if (hasParen)
        {
            consume(TokenType::RIGHT_PAREN, "Expected ')' after while condition.");
        }

        shared_ptr<Statement> body = statement();

        return make_shared<WhileStatement>(condition, body);
    }

    shared_ptr<Statement> forStatement()
    {
        bool hasParen = match({TokenType::LEFT_PAREN});

        shared_ptr<Statement> initializer = nullptr;
        if (match({TokenType::SEMICOLON}))
        {
            initializer = nullptr;
        }
        else if (match({TokenType::LET}))
        {
            initializer = forLetStatement();
        }
        else
        {
            initializer = expressionOrAssignStatement();
        }

        shared_ptr<Expression> condition = nullptr;
        if (!check(TokenType::SEMICOLON))
        {
            condition = expression();
        }
        consume(TokenType::SEMICOLON, "Expected ';' after for loop condition.");

        shared_ptr<Expression> increment = nullptr;
        if (hasParen)
        {
            if (!check(TokenType::RIGHT_PAREN))
            {
                increment = expression();
            }
            consume(TokenType::RIGHT_PAREN, "Expected ')' after for clauses.");
        }
        else
        {
            if (!check(TokenType::LEFT_BRACE) && !isAtEnd())
            {
                increment = expression();
            }
        }

        shared_ptr<Statement> body = statement();

        return make_shared<ForStatement>(initializer, condition, increment, body);
    }

    shared_ptr<Statement> expressionOrAssignStatement()
    {
        shared_ptr<Expression> expr = expression();

        optionalSemicolon();

        AssignExpression* assign = dynamic_cast<AssignExpression*>(expr.get());
        if (assign != nullptr)
        {
            return make_shared<AssignStatement>(assign->name, assign->value);
        }

        return make_shared<ExpressionStatement>(expr);
    }

    shared_ptr<Statement> statement()
    {
        if (match({TokenType::PRINT}))
        {
            return printStatement();
        }

        if (match({TokenType::IF}))
        {
            return ifStatement();
        }

        if (match({TokenType::WHILE}))
        {
            return whileStatement();
        }

        if (match({TokenType::FOR}))
        {
            return forStatement();
        }

        if (match({TokenType::BREAK}))
        {
            optionalSemicolon();
            return make_shared<BreakStatement>();
        }

        if (match({TokenType::CONTINUE}))
        {
            optionalSemicolon();
            return make_shared<ContinueStatement>();
        }

        if (match({TokenType::SEMICOLON}))
        {
            return make_shared<BlockStatement>(vector<shared_ptr<Statement>>{});
        }

        if (match({TokenType::LEFT_BRACE}))
        {
            return blockStatement();
        }

        // return statement
        if (match({TokenType::RETURN}))
        {
            shared_ptr<Expression> val = nullptr;
            if (!check(TokenType::SEMICOLON) && !check(TokenType::RIGHT_BRACE) && !isAtEnd())
            {
                val = expression();
            }
            optionalSemicolon();
            return make_shared<ReturnStatement>(val);
        }

        return expressionOrAssignStatement();
    }

    shared_ptr<Statement> declaration()
    {
        if (match({TokenType::LET}))
        {
            return letStatement();
        }

        // fn name(params) { body }
        if (match({TokenType::FN}))
        {
            Token nameToken = consume(TokenType::IDENTIFIER, "Expected function name after 'fn'.");
            consume(TokenType::LEFT_PAREN, "Expected '(' after function name.");

            vector<string> params;
            if (!check(TokenType::RIGHT_PAREN))
            {
                do
                {
                    Token param = consume(TokenType::IDENTIFIER, "Expected parameter name.");
                    params.push_back(param.value);
                } while (match({TokenType::COMMA}));
            }
            consume(TokenType::RIGHT_PAREN, "Expected ')' after parameters.");
            consume(TokenType::LEFT_BRACE, "Expected '{' before function body.");
            shared_ptr<Statement> body = blockStatement();

            return make_shared<FunctionDeclaration>(nameToken.value, params, body);
        }

        return statement();
    }

    shared_ptr<Program> program()
    {
        shared_ptr<Program> prog = make_shared<Program>();

        while (!isAtEnd())
        {
            shared_ptr<Statement> stmt = declaration();
            if (stmt != nullptr)
            {
                prog->statements.push_back(stmt);
            }
            else
            {
                advance();
            }
        }

        return prog;
    }
};

#endif