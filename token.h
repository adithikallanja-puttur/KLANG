#ifndef TOKEN_H
#define TOKEN_H

#include <string>

using namespace std;

enum class TokenType
{
    // Keywords
    LET,
    PRINT,
    IF,
    ELSE,
    WHILE,
    FOR,
    BREAK,
    CONTINUE,
    TRUE,
    FALSE,
    FN,
    RETURN,

    // Identifiers and literals
    IDENTIFIER,
    NUMBER,
    STRING,

    // Assignment & Arithmetic Operators
    EQUAL,          // =
    PLUS,           // +
    MINUS,          // -
    STAR,           // *
    SLASH,          // /
    PERCENT,        // %
    PLUS_PLUS,      // ++
    MINUS_MINUS,    // --
    PLUS_EQUAL,     // +=
    MINUS_EQUAL,    // -=
    STAR_EQUAL,     // *=
    SLASH_EQUAL,    // /=
    PERCENT_EQUAL,  // %=

    // Logical Operators
    BANG,           // !
    AND,            // &&
    OR,             // ||

    // Comparison Operators
    GREATER,        // >
    LESS,           // <
    GREATER_EQUAL,  // >=
    LESS_EQUAL,     // <=
    EQUAL_EQUAL,    // ==
    NOT_EQUAL,      // !=

    // Delimiters
    LEFT_PAREN,     // (
    RIGHT_PAREN,    // )
    LEFT_BRACE,     // {
    RIGHT_BRACE,    // }
    LEFT_BRACKET,   // [
    RIGHT_BRACKET,  // ]
    SEMICOLON,      // ;
    COMMA,          // ,
    QUESTION,       // ?

    EOF_TOKEN
};

struct Token
{
    TokenType type;
    string value;
    int line = 1;
};

inline string tokenTypeName(TokenType type)
{
    switch(type)
    {
        case TokenType::LET:           return "LET";
        case TokenType::PRINT:         return "PRINT";
        case TokenType::IF:            return "IF";
        case TokenType::ELSE:          return "ELSE";
        case TokenType::WHILE:         return "WHILE";
        case TokenType::FOR:           return "FOR";
        case TokenType::BREAK:         return "BREAK";
        case TokenType::CONTINUE:      return "CONTINUE";
        case TokenType::TRUE:          return "TRUE";
        case TokenType::FALSE:         return "FALSE";
        case TokenType::FN:            return "FN";
        case TokenType::RETURN:        return "RETURN";

        case TokenType::IDENTIFIER:    return "IDENTIFIER";
        case TokenType::NUMBER:        return "NUMBER";
        case TokenType::STRING:        return "STRING";

        case TokenType::EQUAL:         return "EQUAL";
        case TokenType::PLUS:          return "PLUS";
        case TokenType::MINUS:         return "MINUS";
        case TokenType::STAR:          return "STAR";
        case TokenType::SLASH:         return "SLASH";
        case TokenType::PERCENT:       return "PERCENT";
        case TokenType::PLUS_PLUS:     return "PLUS_PLUS";
        case TokenType::MINUS_MINUS:   return "MINUS_MINUS";
        case TokenType::PLUS_EQUAL:    return "PLUS_EQUAL";
        case TokenType::MINUS_EQUAL:   return "MINUS_EQUAL";
        case TokenType::STAR_EQUAL:    return "STAR_EQUAL";
        case TokenType::SLASH_EQUAL:   return "SLASH_EQUAL";
        case TokenType::PERCENT_EQUAL: return "PERCENT_EQUAL";

        case TokenType::BANG:          return "BANG";
        case TokenType::AND:           return "AND";
        case TokenType::OR:            return "OR";

        case TokenType::GREATER:       return "GREATER";
        case TokenType::LESS:          return "LESS";
        case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
        case TokenType::LESS_EQUAL:    return "LESS_EQUAL";
        case TokenType::EQUAL_EQUAL:   return "EQUAL_EQUAL";
        case TokenType::NOT_EQUAL:     return "NOT_EQUAL";

        case TokenType::LEFT_PAREN:    return "LEFT_PAREN";
        case TokenType::RIGHT_PAREN:   return "RIGHT_PAREN";
        case TokenType::LEFT_BRACE:    return "LEFT_BRACE";
        case TokenType::RIGHT_BRACE:   return "RIGHT_BRACE";
        case TokenType::LEFT_BRACKET:  return "LEFT_BRACKET";
        case TokenType::RIGHT_BRACKET: return "RIGHT_BRACKET";
        case TokenType::SEMICOLON:     return "SEMICOLON";
        case TokenType::COMMA:         return "COMMA";
        case TokenType::QUESTION:      return "QUESTION";

        case TokenType::EOF_TOKEN:     return "EOF";
    }

    return "UNKNOWN";
}

#endif