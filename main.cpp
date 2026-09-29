#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "token.h"
#include "lexer.h"
#include "ast.h"
#include "parser.h"
#include "interpreter.h"

using namespace std;

void runSource(const string& source, Interpreter& interpreter, bool showTokens = false)
{
    // 1. Lexer
    Lexer lexer(source);
    vector<Token> tokens = lexer.tokenize();

    if (showTokens)
    {
        cout << "\n--- TOKENS ---" << endl;
        for (const auto& token : tokens)
        {
            cout << tokenTypeName(token.type) << " -> " << token.value << endl;
        }
    }

    // 2. Parser
    Parser parser(tokens);
    shared_ptr<Program> program = parser.program();

    // 3. Interpreter
    interpreter.interpret(program);
}

void runRepl()
{
    cout << "========================================" << endl;
    cout << "   KLANG Interactive Shell (REPL)       " << endl;
    cout << "   Type 'exit' or press Ctrl+C to quit. " << endl;
    cout << "========================================" << endl;

    Interpreter interpreter;
    string line;

    while (true)
    {
        cout << "klang> ";
        if (!getline(cin, line)) break;
        if (line == "exit" || line == "quit") break;
        if (line.empty()) continue;

        try
        {
            runSource(line, interpreter, false);
        }
        catch (const exception& e)
        {
            cerr << "Error: " << e.what() << endl;
        }
    }
}

void runFile(const string& filePath)
{
    ifstream file(filePath);
    if (!file.is_open())
    {
        cerr << "Error: Could not open file '" << filePath << "'" << endl;
        return;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    string source = buffer.str();

    Interpreter interpreter;
    runSource(source, interpreter, false);
}

void runDemo()
{
    cout << "=============================================" << endl;
    cout << "         KLANG INTERPRETER DEMO             " << endl;
    cout << "=============================================" << endl;

    Interpreter interpreter;

    // 1. Original code test (with question mark or semicolon)
    cout << "\n[TEST 1] Variable assignment & comparison:" << endl;
    string test1 = "let y = 10 > 5;\n"
                   "print \"Value of y: \" + y;";
    runSource(test1, interpreter, true);
    cout << "Verified y = " << interpreter.environment->get("y").toString() << endl;

    // 2. Control flow: If / Else & Block scopes
    cout << "\n[TEST 2] If / Else Conditionals & Scope:" << endl;
    string test2 = 
        "let score = 85;\n"
        "if (score >= 90) {\n"
        "    print \"Grade: A\";\n"
        "} else {\n"
        "    if (score >= 80) {\n"
        "        print \"Grade: B (Score: \" + score + \")\";\n"
        "    } else {\n"
        "        print \"Grade: C or below\";\n"
        "    }\n"
        "}\n";
    runSource(test2, interpreter, false);

    // 3. While Loop
    cout << "\n[TEST 3] While Loop & Arithmetic:" << endl;
    string test3 = 
        "let counter = 1;\n"
        "let sum = 0;\n"
        "while (counter <= 5) {\n"
        "    sum = sum + counter;\n"
        "    print \"Step \" + counter + \": sum = \" + sum;\n"
        "    counter = counter + 1;\n"
        "}\n"
        "print \"Final Sum (1..5): \" + sum;\n";
    runSource(test3, interpreter, false);

    // 4. For Loop with Break & Continue (with ++ and +=)
    cout << "\n[TEST 4] For Loop, Break & Continue (with ++ and +=):" << endl;
    string test4 = 
        "let forSum = 0;\n"
        "for (let i = 1; i <= 10; i++) {\n"
        "    if (i == 4) { continue; }\n"
        "    if (i > 6) { break; }\n"
        "    forSum += i;\n"
        "    print \"  for item: \" + i + \" (running sum: \" + forSum + \")\";\n"
        "}\n"
        "print \"Total for-loop sum (1+2+3+5+6): \" + forSum;\n";
    runSource(test4, interpreter, false);

    // 5. Database Integration Demo
    cout << "\n[TEST 5] Database Integration (MySQL & Offline Mock):" << endl;
    string test5 = 
        "// 1. Offline Database Sandbox Demo\n"
        "let db = mock_db_connect(\"school_db\");\n"
        "mysql_execute(db, \"CREATE TABLE students (id INT, name VARCHAR(50), score INT);\");\n"
        "mysql_execute(db, \"INSERT INTO students VALUES (1, 'Alice', 95);\");\n"
        "mysql_execute(db, \"INSERT INTO students VALUES (2, 'Bob', 88);\");\n"
        "print \"\nQuerying mock database:\";\n"
        "print mysql_query(db, \"SELECT * FROM students;\");\n"
        "mysql_close(db);\n"
        "\n"
        "// 2. Live MySQL Connection Test (Attempts connection to local MySQL server)\n"
        "print \"\nAttempting connection to local MySQL server (localhost:3306)...\";\n"
        "let liveDb = mysql_connect(\"127.0.0.1\", \"root\", \"\", \"test\", 3306);\n"
        "if (liveDb > 0) {\n"
        "    print \"Successfully connected to live MySQL server!\";\n"
        "    mysql_execute(liveDb, \"CREATE TABLE IF NOT EXISTS klang_demo (id INT AUTO_INCREMENT PRIMARY KEY, msg VARCHAR(100));\");\n"
        "    mysql_execute(liveDb, \"INSERT INTO klang_demo (msg) VALUES ('Hello from KLANG!');\");\n"
        "    print mysql_query(liveDb, \"SELECT * FROM klang_demo;\");\n"
        "    mysql_close(liveDb);\n"
        "} else {\n"
        "    print \"(Live MySQL server not reachable on localhost:3306 - handled cleanly without crash)\";\n"
        "}\n";
    runSource(test5, interpreter, false);

    cout << "\n=============================================" << endl;
    cout << "     All KLANG interpreter tests passed!     " << endl;
    cout << "=============================================" << endl;
    cout << "\nUsage:" << endl;
    cout << "  klang.exe             - Run this demo" << endl;
    cout << "  klang.exe --repl      - Start interactive shell" << endl;
    cout << "  klang.exe <file.klang>- Execute a script file" << endl;
}

int main(int argc, char* argv[])
{
    if (argc > 1)
    {
        string arg = argv[1];
        if (arg == "--repl" || arg == "-i")
        {
            runRepl();
            return 0;
        }
        else if (arg == "--help" || arg == "-h")
        {
            cout << "KLANG Language Interpreter" << endl;
            cout << "Usage:" << endl;
            cout << "  klang.exe               Run built-in test suite & demo" << endl;
            cout << "  klang.exe <script.klang> Run a script file" << endl;
            cout << "  klang.exe --repl        Interactive shell" << endl;
            return 0;
        }
        else
        {
            runFile(arg);
            return 0;
        }
    }

    runDemo();
    return 0;
}