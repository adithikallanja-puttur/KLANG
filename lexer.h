#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <unordered_map>
#include "token.h"

using namespace std;

class Lexer
{
private:
    string source;
    int current;
    int line;

    char peek() const
    {
        if (current >= (int)source.length()) return '\0';
        return source[current];
    }

    char peekNext() const
    {
        if (current + 1 >= (int)source.length()) return '\0';
        return source[current + 1];
    }

    char scan()
    {
        return source[current++];
    }

    bool match(char expected)
    {
        if (current >= (int)source.length()) return false;
        if (source[current] != expected) return false;
        current++;
        return true;
    }

    Token makeToken(TokenType type, const string& value)
    {
        Token token;
        token.type = type;
        token.value = value;
        token.line = line;
        return token;
    }

    string readWord()
    {
        string text = "";
        while (current < (int)source.length())
        {
            char c = source[current];
            if ((c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') ||
                c == '_')
            {
                text += scan();
            }
            else
            {
                break;
            }
        }
        return text;
    }

    Token word(const string& text)
    {
        static const unordered_map<string, TokenType> keywords = {
            {"let",      TokenType::LET},
            {"print",    TokenType::PRINT},
            {"if",       TokenType::IF},
            {"else",     TokenType::ELSE},
            {"while",    TokenType::WHILE},
            {"for",      TokenType::FOR},
            {"break",    TokenType::BREAK},
            {"continue", TokenType::CONTINUE},
            {"true",     TokenType::TRUE},
            {"false",    TokenType::FALSE},
            {"fn",       TokenType::FN},
            {"return",   TokenType::RETURN}
        };

        auto it = keywords.find(text);
        if (it != keywords.end())
        {
            return makeToken(it->second, text);
        }
        return makeToken(TokenType::IDENTIFIER, text);
    }

    string readNumber()
    {
        string text = "";
        while (current < (int)source.length() && source[current] >= '0' && source[current] <= '9')
        {
            text += scan();
        }

        // Handle floating point decimals (e.g. 3.14)
        if (current < (int)source.length() && source[current] == '.' &&
            current + 1 < (int)source.length() && source[current + 1] >= '0' && source[current + 1] <= '9')
        {
            text += scan(); // consume '.'
            while (current < (int)source.length() && source[current] >= '0' && source[current] <= '9')
            {
                text += scan();
            }
        }

        return text;
    }

    Token readString()
    {
        string text = "";
        scan(); // consume opening quote

        while (current < (int)source.length() && source[current] != '"')
        {
            if (source[current] == '\n')
            {
                line++;
            }

            if (source[current] == '\\' && current + 1 < (int)source.length())
            {
                scan(); // consume '\'
                char escaped = scan();
                switch (escaped)
                {
                    case 'n': text += '\n'; break;
                    case 't': text += '\t'; break;
                    case 'r': text += '\r'; break;
                    case '\\': text += '\\'; break;
                    case '"': text += '"'; break;
                    default: text += escaped; break;
                }
            }
            else
            {
                text += scan();
            }
        }

        if (current < (int)source.length() && source[current] == '"')
        {
            scan(); // consume closing quote
        }

        return makeToken(TokenType::STRING, text);
    }

public:
    Lexer(string source)
    {
        this->source = source;
        current = 0;
        line = 1;
    }

    vector<Token> tokenize()
    {
        vector<Token> tokens;

        while (current < (int)source.length())
        {
            char c = source[current];

            // Ignore whitespace
            if (c == ' ' || c == '\t' || c == '\r')
            {
                current++;
                continue;
            }

            // Newlines
            if (c == '\n')
            {
                line++;
                current++;
                continue;
            }

            // Comments (// until end of line)
            if (c == '/' && peekNext() == '/')
            {
                current += 2;
                while (current < (int)source.length() && source[current] != '\n')
                {
                    current++;
                }
                continue;
            }

            // String literals
            if (c == '"')
            {
                tokens.push_back(readString());
                continue;
            }

            // Words / Identifiers / Keywords
            if ((c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                c == '_')
            {
                string text = readWord();
                tokens.push_back(word(text));
                continue;
            }

            // Numbers
            if (c >= '0' && c <= '9')
            {
                string text = readNumber();
                tokens.push_back(makeToken(TokenType::NUMBER, text));
                continue;
            }

            // Two-character symbols
            if (c == '+' && peekNext() == '+') { current += 2; tokens.push_back(makeToken(TokenType::PLUS_PLUS, "++")); continue; }
            if (c == '-' && peekNext() == '-') { current += 2; tokens.push_back(makeToken(TokenType::MINUS_MINUS, "--")); continue; }
            if (c == '+' && peekNext() == '=') { current += 2; tokens.push_back(makeToken(TokenType::PLUS_EQUAL, "+=")); continue; }
            if (c == '-' && peekNext() == '=') { current += 2; tokens.push_back(makeToken(TokenType::MINUS_EQUAL, "-=")); continue; }
            if (c == '*' && peekNext() == '=') { current += 2; tokens.push_back(makeToken(TokenType::STAR_EQUAL, "*=")); continue; }
            if (c == '/' && peekNext() == '=') { current += 2; tokens.push_back(makeToken(TokenType::SLASH_EQUAL, "/=")); continue; }
            if (c == '%' && peekNext() == '=') { current += 2; tokens.push_back(makeToken(TokenType::PERCENT_EQUAL, "%=")); continue; }
            if (c == '=' && peekNext() == '=') { current += 2; tokens.push_back(makeToken(TokenType::EQUAL_EQUAL, "==")); continue; }
            if (c == '!' && peekNext() == '=') { current += 2; tokens.push_back(makeToken(TokenType::NOT_EQUAL, "!=")); continue; }
            if (c == '<' && peekNext() == '=') { current += 2; tokens.push_back(makeToken(TokenType::LESS_EQUAL, "<=")); continue; }
            if (c == '>' && peekNext() == '=') { current += 2; tokens.push_back(makeToken(TokenType::GREATER_EQUAL, ">=")); continue; }
            if (c == '&' && peekNext() == '&') { current += 2; tokens.push_back(makeToken(TokenType::AND, "&&")); continue; }
            if (c == '|' && peekNext() == '|') { current += 2; tokens.push_back(makeToken(TokenType::OR, "||")); continue; }

            // Single-character symbols
            switch (c)
            {
                case '=': current++; tokens.push_back(makeToken(TokenType::EQUAL, "=")); break;
                case '+': current++; tokens.push_back(makeToken(TokenType::PLUS, "+")); break;
                case '-': current++; tokens.push_back(makeToken(TokenType::MINUS, "-")); break;
                case '*': current++; tokens.push_back(makeToken(TokenType::STAR, "*")); break;
                case '/': current++; tokens.push_back(makeToken(TokenType::SLASH, "/")); break;
                case '%': current++; tokens.push_back(makeToken(TokenType::PERCENT, "%")); break;
                case '!': current++; tokens.push_back(makeToken(TokenType::BANG, "!")); break;
                case '<': current++; tokens.push_back(makeToken(TokenType::LESS, "<")); break;
                case '>': current++; tokens.push_back(makeToken(TokenType::GREATER, ">")); break;
                case '(': current++; tokens.push_back(makeToken(TokenType::LEFT_PAREN, "(")); break;
                case ')': current++; tokens.push_back(makeToken(TokenType::RIGHT_PAREN, ")")); break;
                case '{': current++; tokens.push_back(makeToken(TokenType::LEFT_BRACE, "{")); break;
                case '}': current++; tokens.push_back(makeToken(TokenType::RIGHT_BRACE, "}")); break;
                case '[': current++; tokens.push_back(makeToken(TokenType::LEFT_BRACKET, "[")); break;
                case ']': current++; tokens.push_back(makeToken(TokenType::RIGHT_BRACKET, "]")); break;
                case ';': current++; tokens.push_back(makeToken(TokenType::SEMICOLON, ";")); break;
                case ',': current++; tokens.push_back(makeToken(TokenType::COMMA, ",")); break;
                case '?': current++; tokens.push_back(makeToken(TokenType::QUESTION, "?")); break;
                default:
                    // Unknown character, skip it
                    current++;
                    break;
            }
        }

        tokens.push_back(makeToken(TokenType::EOF_TOKEN, ""));
        return tokens;
    }
};

#endif