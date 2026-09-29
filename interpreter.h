#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <unordered_map>
#include <memory>
#include <iostream>
#include <sstream>
#include <chrono>
#include "ast.h"
#include "value.h"
#include "database.h"

using namespace std;

// Scoped Environment for Variable Management
class Environment : public enable_shared_from_this<Environment>
{
public:
    shared_ptr<Environment> enclosing;
    unordered_map<string, Value> values;

    Environment() : enclosing(nullptr) {}
    Environment(shared_ptr<Environment> parent) : enclosing(parent) {}

    void define(const string& name, const Value& value)
    {
        values[name] = value;
    }

    bool assign(const string& name, const Value& value)
    {
        if (values.find(name) != values.end())
        {
            values[name] = value;
            return true;
        }

        if (enclosing != nullptr)
        {
            return enclosing->assign(name, value);
        }

        return false;
    }

    Value get(const string& name)
    {
        auto it = values.find(name);
        if (it != values.end())
        {
            return it->second;
        }

        if (enclosing != nullptr)
        {
            return enclosing->get(name);
        }

        cerr << "Runtime Error: Undefined variable '" << name << "'." << endl;
        return Value::nil();
    }
};

struct BreakSignal : public exception {};
struct ContinueSignal : public exception {};
struct ReturnSignal : public exception
{
    Value value;
    ReturnSignal(const Value& v) : value(v) {}
};

class Interpreter
{
public:
    // Backwards-compatible variable map
    map<string, int> variables;

    shared_ptr<Environment> globals;
    shared_ptr<Environment> environment;

    // User-defined function registry
    unordered_map<string, shared_ptr<FunctionDeclaration>> functions;

    Interpreter()
    {
        globals = make_shared<Environment>();
        environment = globals;
    }

    // Modern expression evaluator returning Value
    Value eval(shared_ptr<Expression> expression)
    {
        if (expression == nullptr)
        {
            return Value::nil();
        }

        // 1. LiteralExpression
        LiteralExpression* literal = dynamic_cast<LiteralExpression*>(expression.get());
        if (literal != nullptr)
        {
            return literal->value;
        }

        // 2. NumberExpression (backwards compatibility)
        NumberExpression* number = dynamic_cast<NumberExpression*>(expression.get());
        if (number != nullptr)
        {
            return Value((long long)number->value);
        }

        // 3. VariableExpression
        VariableExpression* variable = dynamic_cast<VariableExpression*>(expression.get());
        if (variable != nullptr)
        {
            return environment->get(variable->name);
        }

        // 4. UnaryExpression
        UnaryExpression* unary = dynamic_cast<UnaryExpression*>(expression.get());
        if (unary != nullptr)
        {
            Value right = eval(unary->right);
            if (unary->operation == TokenType::MINUS)
            {
                if (right.type == ValueType::FLOAT) return Value(-right.floatVal);
                return Value(-right.intVal);
            }
            if (unary->operation == TokenType::BANG)
            {
                return Value(!right.isTruthy());
            }
        }

        // 5. BinaryExpression
        BinaryExpression* binary = dynamic_cast<BinaryExpression*>(expression.get());
        if (binary != nullptr)
        {
            // Short-circuit logical operators
            if (binary->operation == TokenType::AND)
            {
                Value left = eval(binary->left);
                if (!left.isTruthy()) return Value(false);
                return Value(eval(binary->right).isTruthy());
            }

            if (binary->operation == TokenType::OR)
            {
                Value left = eval(binary->left);
                if (left.isTruthy()) return Value(true);
                return Value(eval(binary->right).isTruthy());
            }

            Value left = eval(binary->left);
            Value right = eval(binary->right);

            switch (binary->operation)
            {
                case TokenType::PLUS:    return left + right;
                case TokenType::MINUS:   return left - right;
                case TokenType::STAR:    return left * right;
                case TokenType::SLASH:   return left / right;
                case TokenType::PERCENT: return left % right;
                default: break;
            }
        }

        // 6. ComparisonExpression
        ComparisonExpression* comparison = dynamic_cast<ComparisonExpression*>(expression.get());
        if (comparison != nullptr)
        {
            Value left = eval(comparison->left);
            Value right = eval(comparison->right);

            switch (comparison->operation)
            {
                case TokenType::GREATER:       return Value(left > right);
                case TokenType::LESS:          return Value(left < right);
                case TokenType::GREATER_EQUAL: return Value(left >= right);
                case TokenType::LESS_EQUAL:    return Value(left <= right);
                case TokenType::EQUAL_EQUAL:   return Value(left == right);
                case TokenType::NOT_EQUAL:     return Value(left != right);
                default: break;
            }
        }

        // 7. AssignExpression
        AssignExpression* assign = dynamic_cast<AssignExpression*>(expression.get());
        if (assign != nullptr)
        {
            Value value = eval(assign->value);
            if (!environment->assign(assign->name, value))
            {
                environment->define(assign->name, value);
            }
            variables[assign->name] = (int)value.asNumber();
            return value;
        }

        // 8. UpdateExpression (x++, x--, ++x, --x)
        UpdateExpression* update = dynamic_cast<UpdateExpression*>(expression.get());
        if (update != nullptr)
        {
            Value current = environment->get(update->name);
            Value updated;
            if (update->operation == TokenType::PLUS_PLUS)
            {
                if (current.type == ValueType::FLOAT)
                    updated = Value(current.floatVal + 1.0);
                else
                    updated = Value(current.intVal + 1);
            }
            else // MINUS_MINUS
            {
                if (current.type == ValueType::FLOAT)
                    updated = Value(current.floatVal - 1.0);
                else
                    updated = Value(current.intVal - 1);
            }

            if (!environment->assign(update->name, updated))
            {
                environment->define(update->name, updated);
            }
            variables[update->name] = (int)updated.asNumber();

            return update->isPrefix ? updated : current;
        }

        // 9. CompoundAssignExpression (x += expr, x -= expr, etc.)
        CompoundAssignExpression* compAssign = dynamic_cast<CompoundAssignExpression*>(expression.get());
        if (compAssign != nullptr)
        {
            Value current = environment->get(compAssign->name);
            Value rhs = eval(compAssign->value);
            Value result;

            switch (compAssign->operation)
            {
                case TokenType::PLUS_EQUAL:    result = current + rhs; break;
                case TokenType::MINUS_EQUAL:   result = current - rhs; break;
                case TokenType::STAR_EQUAL:    result = current * rhs; break;
                case TokenType::SLASH_EQUAL:   result = current / rhs; break;
                case TokenType::PERCENT_EQUAL: result = current % rhs; break;
                default: result = rhs; break;
            }

            if (!environment->assign(compAssign->name, result))
            {
                environment->define(compAssign->name, result);
            }
            variables[compAssign->name] = (int)result.asNumber();
            return result;
        }

        // 10. CallExpression (Function / MySQL calls)
        CallExpression* call = dynamic_cast<CallExpression*>(expression.get());
        if (call != nullptr)
        {
            return executeCall(call->callee, call->arguments);
        }

        // 11. ArrayLiteralExpression
        ArrayLiteralExpression* arrLit = dynamic_cast<ArrayLiteralExpression*>(expression.get());
        if (arrLit != nullptr)
        {
            auto vec = make_shared<vector<Value>>();
            for (auto& elem : arrLit->elements)
            {
                vec->push_back(eval(elem));
            }
            return Value::makeArray(vec);
        }

        // 12. ArrayIndexExpression: arr[i]
        ArrayIndexExpression* arrIdx = dynamic_cast<ArrayIndexExpression*>(expression.get());
        if (arrIdx != nullptr)
        {
            Value arr = eval(arrIdx->array);
            Value idx = eval(arrIdx->index);
            if (arr.type != ValueType::ARRAY || !arr.arrayVal)
            {
                cerr << "Runtime Error: Attempt to index a non-array value." << endl;
                return Value::nil();
            }
            long long i = idx.intVal;
            if (i < 0 || i >= (long long)arr.arrayVal->size())
            {
                cerr << "Runtime Error: Array index " << i << " out of bounds (size " << arr.arrayVal->size() << ")." << endl;
                return Value::nil();
            }
            return (*arr.arrayVal)[i];
        }

        // 13. ArrayIndexAssignExpression: arr[i] = val
        ArrayIndexAssignExpression* arrAssign = dynamic_cast<ArrayIndexAssignExpression*>(expression.get());
        if (arrAssign != nullptr)
        {
            Value arr = eval(arrAssign->array);
            Value idx = eval(arrAssign->index);
            Value val = eval(arrAssign->value);
            if (arr.type != ValueType::ARRAY || !arr.arrayVal)
            {
                cerr << "Runtime Error: Attempt to index-assign into a non-array value." << endl;
                return Value::nil();
            }
            long long i = idx.intVal;
            if (i < 0 || i >= (long long)arr.arrayVal->size())
            {
                cerr << "Runtime Error: Array index " << i << " out of bounds (size " << arr.arrayVal->size() << ")." << endl;
                return Value::nil();
            }
            (*arr.arrayVal)[i] = val;
            return val;
        }

        return Value::nil();
    }

    // Backwards-compatible evaluate returning int
    int evaluate(shared_ptr<Expression> expression)
    {
        Value v = eval(expression);
        return (int)v.asNumber();
    }

    // Execute built-in and MySQL function calls
    Value executeCall(const string& callee, const vector<shared_ptr<Expression>>& argExprs)
    {
        vector<Value> args;
        for (auto& a : argExprs)
        {
            args.push_back(eval(a));
        }

        // ── User-defined functions ──────────────────────────────────────────
        auto fnIt = functions.find(callee);
        if (fnIt != functions.end())
        {
            auto& fn = fnIt->second;
            if (args.size() != fn->params.size())
            {
                cerr << "Runtime Error: Function '" << callee << "' expects "
                     << fn->params.size() << " argument(s), got " << args.size() << "." << endl;
                return Value::nil();
            }

            // Create a new scope for the function, closing over globals
            shared_ptr<Environment> fnEnv = make_shared<Environment>(globals);
            for (size_t i = 0; i < fn->params.size(); i++)
            {
                fnEnv->define(fn->params[i], args[i]);
            }

            try
            {
                BlockStatement* blk = dynamic_cast<BlockStatement*>(fn->body.get());
                if (blk != nullptr)
                {
                    executeBlock(blk->statements, fnEnv);
                }
                else
                {
                    // Single-statement function body (shouldn't normally happen, but safe fallback)
                    shared_ptr<Environment> prevEnv = this->environment;
                    this->environment = fnEnv;
                    try { execute(fn->body); } catch (...) { this->environment = prevEnv; throw; }
                    this->environment = prevEnv;
                }
            }
            catch (const ReturnSignal& ret)
            {
                return ret.value;
            }
            return Value::nil();
        }

        // ── Array built-ins ────────────────────────────────────────────────

        // push(arr, val)  – appends to array in-place, returns new length
        if (callee == "push")
        {
            if (args.size() < 2)
            {
                cerr << "Runtime Error: push(array, value) requires 2 arguments." << endl;
                return Value::nil();
            }
            if (args[0].type != ValueType::ARRAY || !args[0].arrayVal)
            {
                cerr << "Runtime Error: push() first argument must be an array." << endl;
                return Value::nil();
            }
            args[0].arrayVal->push_back(args[1]);
            return Value((long long)args[0].arrayVal->size());
        }

        // pop(arr)  – removes and returns last element
        if (callee == "pop")
        {
            if (args.empty() || args[0].type != ValueType::ARRAY || !args[0].arrayVal)
            {
                cerr << "Runtime Error: pop(array) requires an array argument." << endl;
                return Value::nil();
            }
            if (args[0].arrayVal->empty())
            {
                cerr << "Runtime Error: pop() on empty array." << endl;
                return Value::nil();
            }
            Value last = args[0].arrayVal->back();
            args[0].arrayVal->pop_back();
            return last;
        }

        // len(str | arr)  – length of string or array
        if (callee == "len")
        {
            if (args.empty()) return Value(0LL);
            if (args[0].type == ValueType::ARRAY && args[0].arrayVal)
                return Value((long long)args[0].arrayVal->size());
            return Value((long long)args[0].toString().length());
        }

        // ── Type conversion built-ins ──────────────────────────────────────

        // str(val)  – convert any value to string
        if (callee == "str")
        {
            if (args.empty()) return Value(string(""));
            return Value(args[0].toString());
        }

        // int(val)  – convert value to integer
        if (callee == "int")
        {
            if (args.empty()) return Value(0LL);
            if (args[0].type == ValueType::STRING)
            {
                try { return Value((long long)stoll(args[0].stringVal)); }
                catch (...) { return Value(0LL); }
            }
            return Value((long long)args[0].asNumber());
        }

        // float(val)  – convert value to float
        if (callee == "float")
        {
            if (args.empty()) return Value(0.0);
            if (args[0].type == ValueType::STRING)
            {
                try { return Value(stod(args[0].stringVal)); }
                catch (...) { return Value(0.0); }
            }
            return Value(args[0].asNumber());
        }

        // bool(val)  – convert value to bool
        if (callee == "bool")
        {
            if (args.empty()) return Value(false);
            return Value(args[0].isTruthy());
        }

        // type(val)  – returns a string representing the type name
        if (callee == "type")
        {
            if (args.empty()) return Value(string("nil"));
            switch (args[0].type)
            {
                case ValueType::NIL:    return Value(string("nil"));
                case ValueType::BOOL:   return Value(string("bool"));
                case ValueType::INT:    return Value(string("int"));
                case ValueType::FLOAT:  return Value(string("float"));
                case ValueType::STRING: return Value(string("string"));
                case ValueType::ARRAY:  return Value(string("array"));
            }
            return Value(string("nil"));
        }

        // ── String built-ins ──────────────────────────────────────────────

        // substr(str, start [, length])  – substring
        if (callee == "substr")
        {
            if (args.empty()) return Value(string(""));
            string s = args[0].toString();
            long long start = args.size() > 1 ? args[1].intVal : 0;
            if (start < 0) start = max(0LL, (long long)s.size() + start);
            if (start >= (long long)s.size()) return Value(string(""));
            if (args.size() > 2)
            {
                long long len = args[2].intVal;
                if (len <= 0) return Value(string(""));
                return Value(s.substr((size_t)start, (size_t)len));
            }
            return Value(s.substr((size_t)start));
        }

        // split(str, delimiter)  – split string into array
        if (callee == "split")
        {
            if (args.empty()) return Value::makeArray(make_shared<vector<Value>>());
            string s = args[0].toString();
            string delim = args.size() > 1 ? args[1].toString() : " ";
            auto vec = make_shared<vector<Value>>();
            if (delim.empty())
            {
                for (char c : s) vec->push_back(Value(string(1, c)));
            }
            else
            {
                size_t pos = 0, found;
                while ((found = s.find(delim, pos)) != string::npos)
                {
                    vec->push_back(Value(s.substr(pos, found - pos)));
                    pos = found + delim.size();
                }
                vec->push_back(Value(s.substr(pos)));
            }
            return Value::makeArray(vec);
        }

        // join(arr, delimiter)  – join array elements into a string
        if (callee == "join")
        {
            if (args.empty() || args[0].type != ValueType::ARRAY || !args[0].arrayVal)
                return Value(string(""));
            string delim = args.size() > 1 ? args[1].toString() : "";
            string result = "";
            auto& arr = *args[0].arrayVal;
            for (size_t i = 0; i < arr.size(); i++)
            {
                if (i > 0) result += delim;
                result += arr[i].toString();
            }
            return Value(result);
        }

        // contains(str, substr)  – check if string contains substring
        if (callee == "contains")
        {
            if (args.size() < 2) return Value(false);
            return Value(args[0].toString().find(args[1].toString()) != string::npos);
        }

        // starts_with(str, prefix)
        if (callee == "starts_with")
        {
            if (args.size() < 2) return Value(false);
            string s = args[0].toString(), prefix = args[1].toString();
            return Value(s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix);
        }

        // ends_with(str, suffix)
        if (callee == "ends_with")
        {
            if (args.size() < 2) return Value(false);
            string s = args[0].toString(), suffix = args[1].toString();
            return Value(s.size() >= suffix.size() && s.substr(s.size() - suffix.size()) == suffix);
        }

        // to_upper(str)  – convert string to upper case
        if (callee == "to_upper")
        {
            if (args.empty()) return Value(string(""));
            string s = args[0].toString();
            transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return toupper(c); });
            return Value(s);
        }

        // to_lower(str)  – convert string to lower case
        if (callee == "to_lower")
        {
            if (args.empty()) return Value(string(""));
            string s = args[0].toString();
            transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return tolower(c); });
            return Value(s);
        }

        // trim(str)  – strip leading/trailing whitespace
        if (callee == "trim")
        {
            if (args.empty()) return Value(string(""));
            string s = args[0].toString();
            size_t start = s.find_first_not_of(" \t\r\n");
            if (start == string::npos) return Value(string(""));
            size_t end = s.find_last_not_of(" \t\r\n");
            return Value(s.substr(start, end - start + 1));
        }

        // replace(str, from, to)  – replace all occurrences of `from` with `to`
        if (callee == "replace")
        {
            if (args.size() < 3) return args.empty() ? Value(string("")) : Value(args[0].toString());
            string s = args[0].toString();
            string from = args[1].toString();
            string to   = args[2].toString();
            if (!from.empty())
            {
                size_t pos = 0;
                while ((pos = s.find(from, pos)) != string::npos)
                {
                    s.replace(pos, from.size(), to);
                    pos += to.size();
                }
            }
            return Value(s);
        }

        // index_of(str, substr)  – first index of substr in str, or -1
        if (callee == "index_of")
        {
            if (args.size() < 2) return Value(-1LL);
            size_t pos = args[0].toString().find(args[1].toString());
            return Value(pos == string::npos ? -1LL : (long long)pos);
        }

        // char_at(str, index)  – character at index as string
        if (callee == "char_at")
        {
            if (args.size() < 2) return Value(string(""));
            string s = args[0].toString();
            long long idx = args[1].intVal;
            if (idx < 0 || idx >= (long long)s.size()) return Value(string(""));
            return Value(string(1, s[(size_t)idx]));
        }

        // ── Math built-ins ────────────────────────────────────────────────

        // abs(x)  – absolute value
        if (callee == "abs")
        {
            if (args.empty()) return Value(0LL);
            if (args[0].type == ValueType::FLOAT) return Value(fabs(args[0].floatVal));
            return Value(llabs(args[0].intVal));
        }

        // sqrt(x)
        if (callee == "sqrt")
        {
            if (args.empty()) return Value(0.0);
            double x = args[0].asNumber();
            if (x < 0.0) { cerr << "Runtime Error: sqrt() of negative number." << endl; return Value(0.0); }
            return Value(std::sqrt(x));
        }

        // pow(base, exp)
        if (callee == "pow")
        {
            if (args.size() < 2) return Value(0.0);
            return Value(std::pow(args[0].asNumber(), args[1].asNumber()));
        }

        // floor(x)
        if (callee == "floor")
        {
            if (args.empty()) return Value(0LL);
            return Value((long long)std::floor(args[0].asNumber()));
        }

        // ceil(x)
        if (callee == "ceil")
        {
            if (args.empty()) return Value(0LL);
            return Value((long long)std::ceil(args[0].asNumber()));
        }

        // round(x)
        if (callee == "round")
        {
            if (args.empty()) return Value(0LL);
            return Value((long long)std::round(args[0].asNumber()));
        }

        // min(a, b)
        if (callee == "min")
        {
            if (args.size() < 2) return args.empty() ? Value::nil() : args[0];
            return (args[0] < args[1]) ? args[0] : args[1];
        }

        // max(a, b)
        if (callee == "max")
        {
            if (args.size() < 2) return args.empty() ? Value::nil() : args[0];
            return (args[0] > args[1]) ? args[0] : args[1];
        }

        // ── Range / array creation ─────────────────────────────────────────

        // range(end) or range(start, end) or range(start, end, step)
        if (callee == "range")
        {
            long long start = 0, end = 0, step = 1;
            if (args.size() == 1) { end = args[0].intVal; }
            else if (args.size() >= 2) { start = args[0].intVal; end = args[1].intVal; }
            if (args.size() >= 3) { step = args[2].intVal; if (step == 0) step = 1; }
            auto vec = make_shared<vector<Value>>();
            if (step > 0) { for (long long i = start; i < end; i += step) vec->push_back(Value(i)); }
            else          { for (long long i = start; i > end; i += step) vec->push_back(Value(i)); }
            return Value::makeArray(vec);
        }

        // ── I/O built-ins ─────────────────────────────────────────────────

        // input([prompt])  – read a line from stdin
        if (callee == "input")
        {
            if (!args.empty()) cout << args[0].toString();
            string line;
            if (!getline(cin, line)) return Value(string(""));
            return Value(line);
        }

        // print_no_newline(val)  – print without trailing newline
        if (callee == "print_no_newline")
        {
            if (!args.empty()) cout << args[0].toString();
            return Value::nil();
        }

        // ── Database built-ins ────────────────────────────────────────────

        // mysql_connect("host", "user", "pass", "dbname", port = 3306)
        if (callee == "mysql_connect")
        {
            string host = args.size() > 0 ? args[0].toString() : "localhost";
            string user = args.size() > 1 ? args[1].toString() : "root";
            string pass = args.size() > 2 ? args[2].toString() : "";
            string db   = args.size() > 3 ? args[3].toString() : "";
            int port    = args.size() > 4 ? (int)args[4].asNumber() : 3306;
            int handle = DatabaseManager::instance().connect(host, user, pass, db, port);
            return Value(handle);
        }

        // mock_db_connect("test_db")  – offline in-memory database
        if (callee == "mock_db_connect")
        {
            string db = args.size() > 0 ? args[0].toString() : "mock_db";
            int handle = DatabaseManager::instance().createMockConnection(db);
            return Value(handle);
        }

        // mysql_execute(handle, sql)  – DDL/DML statements
        if (callee == "mysql_execute")
        {
            if (args.size() < 2)
            {
                cerr << "Runtime Error: mysql_execute requires (handle, sql_query)." << endl;
                return Value(-1LL);
            }
            int handle = (int)args[0].asNumber();
            string sql = args[1].toString();
            return DatabaseManager::instance().execute(handle, sql);
        }

        // mysql_query(handle, sql)  – SELECT queries, returns formatted table string
        if (callee == "mysql_query" || callee == "mysql_fetch")
        {
            if (args.size() < 2)
            {
                cerr << "Runtime Error: mysql_query requires (handle, sql_query)." << endl;
                return Value::nil();
            }
            int handle = (int)args[0].asNumber();
            string sql = args[1].toString();
            return DatabaseManager::instance().query(handle, sql);
        }

        // mysql_close(handle)  – close a database connection
        if (callee == "mysql_close")
        {
            if (args.size() < 1)
            {
                cerr << "Runtime Error: mysql_close requires (handle)." << endl;
                return Value::nil();
            }
            int handle = (int)args[0].asNumber();
            DatabaseManager::instance().close(handle);
            return Value(true);
        }

        // ── Misc built-ins ────────────────────────────────────────────────

        // clock()  – milliseconds since epoch
        if (callee == "clock")
        {
            auto now = chrono::system_clock::now();
            auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()).count();
            return Value((long long)ms);
        }

        // assert(condition [, message])  – runtime assertion
        if (callee == "assert")
        {
            bool ok = !args.empty() && args[0].isTruthy();
            if (!ok)
            {
                string msg = args.size() > 1 ? args[1].toString() : "Assertion failed";
                cerr << "Runtime Error: " << msg << endl;
                throw runtime_error(msg);
            }
            return Value(true);
        }

        // is_nil(val)  – returns true if value is nil
        if (callee == "is_nil")
        {
            return Value(args.empty() || args[0].type == ValueType::NIL);
        }

        cerr << "Runtime Error: Call to unknown function '" << callee << "'." << endl;
        return Value::nil();
    }

    // Execute Statement
    void execute(shared_ptr<Statement> statement)
    {
        if (statement == nullptr) return;

        // 1. LetStatement
        LetStatement* let = dynamic_cast<LetStatement*>(statement.get());
        if (let != nullptr)
        {
            Value val = eval(let->value);
            environment->define(let->name, val);
            variables[let->name] = (int)val.asNumber();
            return;
        }

        // 2. AssignStatement
        AssignStatement* assign = dynamic_cast<AssignStatement*>(statement.get());
        if (assign != nullptr)
        {
            Value val = eval(assign->value);
            if (!environment->assign(assign->name, val))
            {
                environment->define(assign->name, val);
            }
            variables[assign->name] = (int)val.asNumber();
            return;
        }

        // 3. PrintStatement
        PrintStatement* printStmt = dynamic_cast<PrintStatement*>(statement.get());
        if (printStmt != nullptr)
        {
            Value val = eval(printStmt->expression);
            cout << val.toString() << endl;
            return;
        }

        // 4. BlockStatement { stmt1; stmt2; }
        BlockStatement* block = dynamic_cast<BlockStatement*>(statement.get());
        if (block != nullptr)
        {
            executeBlock(block->statements, make_shared<Environment>(environment));
            return;
        }

        // 5. IfStatement
        IfStatement* ifStmt = dynamic_cast<IfStatement*>(statement.get());
        if (ifStmt != nullptr)
        {
            Value cond = eval(ifStmt->condition);
            if (cond.isTruthy())
            {
                if (ifStmt->thenBranch != nullptr)
                {
                    execute(ifStmt->thenBranch);
                }
                else if (!ifStmt->body.empty())
                {
                    for (auto& s : ifStmt->body) execute(s);
                }
            }
            else if (ifStmt->elseBranch != nullptr)
            {
                execute(ifStmt->elseBranch);
            }
            return;
        }

        // 6. WhileStatement
        WhileStatement* whileStmt = dynamic_cast<WhileStatement*>(statement.get());
        if (whileStmt != nullptr)
        {
            while (eval(whileStmt->condition).isTruthy())
            {
                try
                {
                    execute(whileStmt->body);
                }
                catch (const BreakSignal&)
                {
                    break;
                }
                catch (const ContinueSignal&)
                {
                    continue;
                }
            }
            return;
        }

        // 7. ForStatement
        ForStatement* forStmt = dynamic_cast<ForStatement*>(statement.get());
        if (forStmt != nullptr)
        {
            shared_ptr<Environment> loopEnv = make_shared<Environment>(this->environment);
            shared_ptr<Environment> previous = this->environment;
            this->environment = loopEnv;

            try
            {
                if (forStmt->initializer != nullptr)
                {
                    execute(forStmt->initializer);
                }

                while (forStmt->condition == nullptr || eval(forStmt->condition).isTruthy())
                {
                    try
                    {
                        execute(forStmt->body);
                    }
                    catch (const BreakSignal&)
                    {
                        break;
                    }
                    catch (const ContinueSignal&)
                    {
                        // Proceed to increment
                    }

                    if (forStmt->increment != nullptr)
                    {
                        eval(forStmt->increment);
                    }
                }
            }
            catch (...)
            {
                this->environment = previous;
                throw;
            }

            this->environment = previous;
            return;
        }

        // 8. BreakStatement
        BreakStatement* breakStmt = dynamic_cast<BreakStatement*>(statement.get());
        if (breakStmt != nullptr)
        {
            throw BreakSignal();
        }

        // 9. ContinueStatement
        ContinueStatement* continueStmt = dynamic_cast<ContinueStatement*>(statement.get());
        if (continueStmt != nullptr)
        {
            throw ContinueSignal();
        }

        // 10. ExpressionStatement
        ExpressionStatement* exprStmt = dynamic_cast<ExpressionStatement*>(statement.get());
        if (exprStmt != nullptr)
        {
            eval(exprStmt->expression);
            return;
        }

        // 11. ReturnStatement
        ReturnStatement* returnStmt = dynamic_cast<ReturnStatement*>(statement.get());
        if (returnStmt != nullptr)
        {
            Value retVal = returnStmt->value ? eval(returnStmt->value) : Value::nil();
            throw ReturnSignal(retVal);
        }

        // 12. FunctionDeclaration
        FunctionDeclaration* fnDecl = dynamic_cast<FunctionDeclaration*>(statement.get());
        if (fnDecl != nullptr)
        {
            functions[fnDecl->name] = make_shared<FunctionDeclaration>(fnDecl->name, fnDecl->params, fnDecl->body);
            return;
        }
    }

    // Execute Block with Scoped Environment
    void executeBlock(const vector<shared_ptr<Statement>>& statements, shared_ptr<Environment> newEnv)
    {
        shared_ptr<Environment> previous = this->environment;
        this->environment = newEnv;

        try
        {
            for (const auto& stmt : statements)
            {
                execute(stmt);
            }
        }
        catch (...)
        {
            this->environment = previous;
            throw;
        }

        this->environment = previous;
    }

    // Execute entire program
    void interpret(shared_ptr<Program> program)
    {
        if (program == nullptr) return;

        for (const auto& stmt : program->statements)
        {
            execute(stmt);
        }
    }
};

#endif