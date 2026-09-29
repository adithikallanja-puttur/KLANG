# KLANG — Custom Interpreter with MySQL Integration

KLANG is a C++-based programming language interpreter featuring dynamic types, control flow structures, string manipulation, and native **MySQL / MariaDB database integration**.

---

## 🌟 Key Features

1. **Rich Type System (`value.h`)**:
   - `INT` (64-bit integer)
   - `FLOAT` (double precision)
   - `BOOL` (`true` / `false`)
   - `STRING` (with escape sequences `\n`, `\t`, `\"`)
   - `NIL`
   - Dynamic type conversions and string concatenation (`"Count: " + count`).

2. **Control Flow**:
   - `print <expression>;`
   - `if (<condition>) { ... } else { ... }`
   - `while (<condition>) { ... }`
   - `for (<init>; <cond>; <step>) { ... }` with `break;` and `continue;`
   - Block scope `{ ... }` with lexical scoping.

3. **MySQL Database Integration (`database.h`)**:
   - `mysql_connect(host, user, password, db, port)`: Connects to a live MySQL/MariaDB server.
   - `mysql_execute(handle, sql)`: Runs DDL / DML (`CREATE TABLE`, `INSERT`, `UPDATE`, `DELETE`).
   - `mysql_query(handle, sql)`: Runs queries and renders ASCII formatted tables.
   - `mysql_close(handle)`: Safely closes the database connection.
   - `mock_db_connect(db)`: Built-in sandbox for offline testing without requiring a live server.

4. **Execution Modes (`main.cpp`)**:
   - **Demo / Test Suite**: `.\klang.exe`
   - **Script Execution**: `.\klang.exe <script.klang>`
   - **Interactive Shell (REPL)**: `.\klang.exe --repl`

---

## 🚀 Quick Start

### Compiling KLANG
Compile with C++17 and the MariaDB/MySQL client library:
```powershell
g++ -std=c++17 main.cpp -o klang.exe -lmariadb
```

### Starting the Portable MySQL Server
Your workspace includes one-click scripts to control the portable MySQL server:
- **Start MySQL**: Run `.\start_mysql.bat` (or double-click it)
- **Stop MySQL**: Run `.\stop_mysql.bat` (or double-click it)

Default credentials:
- **Host**: `127.0.0.1`
- **Port**: `3306`
- **User**: `root`
- **Password**: *(empty)*
- **Database**: `test`

### Running the Live MySQL Script
```powershell
.\klang.exe mysql_test.klang
```

### Interactive REPL
```powershell
.\klang.exe --repl
```
```text
klang> let a = 10;
klang> let b = 20;
klang> print "Sum: " + (a + b);
Sum: 30
klang> exit
```

---

## 📜 Syntax Guide

### Variables & Math
```javascript
let x = 10;
let y = 20;
let z = (x * 2) + (y / 2);
print "Result: " + z;
```

### Conditionals (If / Else)
```javascript
let score = 85;

if (score >= 90) {
    print "Grade: A";
} else {
    if (score >= 80) {
        print "Grade: B";
    } else {
        print "Grade: C or below";
    }
}
```

### While Loops
```javascript
let counter = 1;
while (counter <= 5) {
    print "Iteration #" + counter;
    counter = counter + 1;
}
```

### For Loops with Break & Continue
Supports standard increments (`i++`, `i--`, `i += 2`, `i = i + 1`), scoped variable declarations (`let i = ...`), and loop control (`break;`, `continue;`):
```javascript
let sum = 0;
for (let i = 1; i <= 10; i++) {
    if (i == 4) { continue; } // skip 4
    if (i > 6) { break; }    // terminate loop
    sum += i;
    print "Item " + i + ", running sum: " + sum;
}
print "Final Sum: " + sum;
```

### MySQL Integration
```javascript
// Connect to MySQL
let db = mysql_connect("127.0.0.1", "root", "", "test_db", 3306);

if (db > 0) {
    // Create Table
    mysql_execute(db, "CREATE TABLE IF NOT EXISTS users (id INT AUTO_INCREMENT PRIMARY KEY, name VARCHAR(50), balance INT);");

    // Insert Data
    mysql_execute(db, "INSERT INTO users (name, balance) VALUES ('Alice', 500);");
    mysql_execute(db, "INSERT INTO users (name, balance) VALUES ('Bob', 350);");

    // Query Data (renders formatted table)
    print mysql_query(db, "SELECT * FROM users;");

    // Close Connection
    mysql_close(db);
}
```

---

## 📂 Project Architecture

| File | Purpose |
|---|---|
| [token.h](file:///c:/Users/hp/Desktop/KLANG/token.h) | Token definitions, keywords, operators, delimiters |
| [lexer.h](file:///c:/Users/hp/Desktop/KLANG/lexer.h) | Tokenizer with string literals, comments, multi-line support |
| [value.h](file:///c:/Users/hp/Desktop/KLANG/value.h) | Dynamic runtime Value system (`int`, `float`, `string`, `bool`, `nil`) |
| [ast.h](file:///c:/Users/hp/Desktop/KLANG/ast.h) | AST node hierarchy for statements and expressions |
| [parser.h](file:///c:/Users/hp/Desktop/KLANG/parser.h) | Recursive descent parser with operator precedence |
| [database.h](file:///c:/Users/hp/Desktop/KLANG/database.h) | MySQL C API client integration & offline testing engine |
| [interpreter.h](file:///c:/Users/hp/Desktop/KLANG/interpreter.h) | AST evaluator, scoped environments, statement dispatcher |
| [main.cpp](file:///c:/Users/hp/Desktop/KLANG/main.cpp) | CLI runner, file loader, REPL, and demonstration tests |
| [example.klang](file:///c:/Users/hp/Desktop/KLANG/example.klang) | Sample script demonstrating language features |
| [mysql_test.klang](file:///c:/Users/hp/Desktop/KLANG/mysql_test.klang) | Template script for connecting to a live MySQL server |
