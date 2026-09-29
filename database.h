#ifndef DATABASE_H
#define DATABASE_H

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#if __has_include(<mariadb/mysql.h>)
#include <mariadb/mysql.h>
#elif __has_include(<mysql/mysql.h>)
#include <mysql/mysql.h>
#elif __has_include(<mysql.h>)
#include <mysql.h>
#endif
#include "value.h"

using namespace std;

class DatabaseManager {
private:
  unordered_map<int, MYSQL *> connections;
  int nextConnectionId = 1;

  // Built-in in-memory fallback store for offline testing
  struct MockTable {
    vector<string> columns;
    vector<vector<string>> rows;
  };
  unordered_map<int, unordered_map<string, MockTable>> mockDatabases;

  DatabaseManager() {}

public:
  static DatabaseManager &instance() {
    static DatabaseManager inst;
    return inst;
  }

  ~DatabaseManager() {
    for (auto &pair : connections) {
      if (pair.second != nullptr) {
        mysql_close(pair.second);
      }
    }
    connections.clear();
  }

  // ----------------------------------------------------
  // LIVE MYSQL INTEGRATION
  // ----------------------------------------------------

  int connect(const string &host, const string &user, const string &password,
              const string &db, int port = 3306) {
    MYSQL *conn = mysql_init(nullptr);
    if (conn == nullptr) {
      cerr << "[MySQL Error] mysql_init() failed (insufficient memory)."
           << endl;
      return 0;
    }

    // Set connect timeout to 4 seconds to prevent long hangs on unavailable
    // hosts
    unsigned int timeout = 4;
    mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

    const char *h = host.empty() ? "localhost" : host.c_str();
    const char *u = user.empty() ? nullptr : user.c_str();
    const char *p = password.empty() ? nullptr : password.c_str();
    const char *d = db.empty() ? nullptr : db.c_str();

    if (mysql_real_connect(conn, h, u, p, d, port, nullptr, 0) == nullptr) {
      cerr << "[MySQL Error] Connection failed to " << host << ":" << port
           << " - " << mysql_error(conn) << endl;
      mysql_close(conn);
      return 0;
    }

    int id = nextConnectionId++;
    connections[id] = conn;
    cout << "[MySQL] Connected successfully to " << host << ":" << port;
    if (!db.empty())
      cout << " (Database: " << db << ")";
    cout << " [Connection ID: " << id << "]" << endl;

    return id;
  }

  Value execute(int handle, const string &sql) {
    auto it = connections.find(handle);
    if (it == connections.end() || it->second == nullptr) {
      // Check mock databases
      if (mockDatabases.find(handle) != mockDatabases.end()) {
        return mockExecute(handle, sql);
      }

      cerr << "[MySQL Error] Invalid connection handle: " << handle << endl;
      return Value(-1);
    }

    MYSQL *conn = it->second;
    if (mysql_query(conn, sql.c_str()) != 0) {
      cerr << "[MySQL Error] Query failed: " << mysql_error(conn) << endl;
      cerr << "  SQL: " << sql << endl;
      return Value(-1);
    }

    long long affected = (long long)mysql_affected_rows(conn);
    return Value(affected);
  }

  Value query(int handle, const string &sql) {
    auto it = connections.find(handle);
    if (it == connections.end() || it->second == nullptr) {
      // Check mock databases
      if (mockDatabases.find(handle) != mockDatabases.end()) {
        return mockQuery(handle, sql);
      }

      cerr << "[MySQL Error] Invalid connection handle: " << handle << endl;
      return Value::nil();
    }

    MYSQL *conn = it->second;
    if (mysql_query(conn, sql.c_str()) != 0) {
      cerr << "[MySQL Error] Query failed: " << mysql_error(conn) << endl;
      return Value::nil();
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (result == nullptr) {
      if (mysql_field_count(conn) == 0) {
        // Query was not a SELECT statement (e.g. UPDATE, INSERT)
        return Value((long long)mysql_affected_rows(conn));
      }
      cerr << "[MySQL Error] store_result failed: " << mysql_error(conn)
           << endl;
      return Value::nil();
    }

    int numFields = mysql_num_fields(result);
    MYSQL_FIELD *fields = mysql_fetch_fields(result);

    ostringstream out;
    vector<int> colWidths(numFields, 0);

    for (int i = 0; i < numFields; i++) {
      int len = fields[i].name ? (int)string(fields[i].name).length() : 4;
      colWidths[i] = max(colWidths[i], max(len, 10));
    }

    // Collect all rows
    vector<vector<string>> rows;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
      vector<string> rowData;
      for (int i = 0; i < numFields; i++) {
        string val = row[i] ? row[i] : "NULL";
        colWidths[i] = max(colWidths[i], (int)val.length());
        rowData.push_back(val);
      }
      rows.push_back(rowData);
    }

    // Format ASCII table
    auto printSeparator = [&]() {
      out << "+";
      for (int w : colWidths) {
        out << string(w + 2, '-') << "+";
      }
      out << "\n";
    };

    printSeparator();
    out << "|";
    for (int i = 0; i < numFields; i++) {
      out << " " << left << setw(colWidths[i]) << fields[i].name << " |";
    }
    out << "\n";
    printSeparator();

    for (const auto &r : rows) {
      out << "|";
      for (int i = 0; i < numFields; i++) {
        out << " " << left << setw(colWidths[i]) << r[i] << " |";
      }
      out << "\n";
    }
    printSeparator();

    out << "(" << rows.size() << " row" << (rows.size() == 1 ? "" : "s")
        << " in set)\n";

    mysql_free_result(result);
    return Value(out.str());
  }

  void close(int handle) {
    auto it = connections.find(handle);
    if (it != connections.end()) {
      if (it->second != nullptr) {
        mysql_close(it->second);
      }
      connections.erase(it);
      cout << "[MySQL] Closed connection handle: " << handle << endl;
    }

    mockDatabases.erase(handle);
  }

  // ----------------------------------------------------
  // IN-MEMORY MOCK / OFFLINE DATABASE ENGINE
  // (Used when testing without live MySQL server)
  // ----------------------------------------------------

  int createMockConnection(const string &dbName) {
    int id = nextConnectionId++;
    mockDatabases[id] = unordered_map<string, MockTable>();
    cout << "[MockDB] Initialized offline database sandbox: '" << dbName
         << "' [Handle: " << id << "]" << endl;
    return id;
  }

  Value mockExecute(int handle, const string &sql) {
    auto &tables = mockDatabases[handle];
    string upper = sql;
    for (char &c : upper)
      c = toupper((unsigned char)c);

    if (upper.find("CREATE TABLE") != string::npos) {
      size_t start = upper.find("TABLE");
      if (start != string::npos) {
        string rest = sql.substr(start + 5);
        size_t ifNot = upper.find("IF NOT EXISTS", start);
        if (ifNot != string::npos)
          rest = sql.substr(ifNot + 13);
        while (!rest.empty() && (rest.front() == ' ' || rest.front() == '\t'))
          rest.erase(0, 1);
        size_t parenOpen = rest.find('(');
        string tableName = rest.substr(0, parenOpen);
        while (!tableName.empty() && (tableName.back() == ' ' || tableName.back() == '\t' || tableName.back() == '`'))
          tableName.pop_back();
        while (!tableName.empty() && (tableName.front() == ' ' || tableName.front() == '\t' || tableName.front() == '`'))
          tableName.erase(0, 1);

        MockTable table;
        if (parenOpen != string::npos) {
          size_t parenClose = rest.rfind(')');
          if (parenClose != string::npos && parenClose > parenOpen) {
            string colDefs = rest.substr(parenOpen + 1, parenClose - parenOpen - 1);
            string curCol = "";
            int parenDepth = 0;
            for (char ch : colDefs) {
              if (ch == '(') parenDepth++;
              else if (ch == ')') parenDepth--;
              else if (ch == ',' && parenDepth == 0) {
                stringstream ss(curCol);
                string colName;
                if (ss >> colName) {
                  while (!colName.empty() && (colName.front() == '`' || colName.front() == ' ')) colName.erase(0, 1);
                  while (!colName.empty() && (colName.back() == '`' || colName.back() == ' ')) colName.pop_back();
                  table.columns.push_back(colName);
                }
                curCol = "";
                continue;
              }
              curCol += ch;
            }
            if (!curCol.empty()) {
              stringstream ss(curCol);
              string colName;
              if (ss >> colName) {
                while (!colName.empty() && (colName.front() == '`' || colName.front() == ' ')) colName.erase(0, 1);
                while (!colName.empty() && (colName.back() == '`' || colName.back() == ' ')) colName.pop_back();
                table.columns.push_back(colName);
              }
            }
          }
        }
        if (table.columns.empty()) {
          table.columns = {"id", "name", "value"};
        }
        tables[tableName] = table;
        return Value(0);
      }
    } else if (upper.find("INSERT INTO") != string::npos) {
      size_t start = upper.find("INTO");
      string rest = sql.substr(start + 4);
      while (!rest.empty() && (rest.front() == ' ' || rest.front() == '\t'))
        rest.erase(0, 1);
      size_t paren = rest.find('(');
      size_t space = rest.find(' ');
      size_t splitPos = min(paren == string::npos ? 999999ULL : paren,
                            space == string::npos ? 999999ULL : space);
      string tableName = rest.substr(0, splitPos);
      while (!tableName.empty() && (tableName.back() == ' ' || tableName.back() == '\t' || tableName.back() == '`'))
        tableName.pop_back();
      while (!tableName.empty() && (tableName.front() == ' ' || tableName.front() == '\t' || tableName.front() == '`'))
        tableName.erase(0, 1);

      size_t valPos = upper.find("VALUES");
      vector<string> rowValues;
      if (valPos != string::npos) {
        size_t pOpen = sql.find('(', valPos);
        size_t pClose = sql.rfind(')');
        if (pOpen != string::npos && pClose != string::npos && pClose > pOpen) {
          string inside = sql.substr(pOpen + 1, pClose - pOpen - 1);
          string curVal = "";
          bool inQuote = false;
          char quoteChar = '\0';
          for (char ch : inside) {
            if ((ch == '\'' || ch == '"') && !inQuote) {
              inQuote = true;
              quoteChar = ch;
            } else if (inQuote && ch == quoteChar) {
              inQuote = false;
            } else if (ch == ',' && !inQuote) {
              while (!curVal.empty() && (curVal.front() == ' ' || curVal.front() == '\t')) curVal.erase(0, 1);
              while (!curVal.empty() && (curVal.back() == ' ' || curVal.back() == '\t')) curVal.pop_back();
              rowValues.push_back(curVal);
              curVal = "";
              continue;
            } else {
              curVal += ch;
            }
          }
          while (!curVal.empty() && (curVal.front() == ' ' || curVal.front() == '\t')) curVal.erase(0, 1);
          while (!curVal.empty() && (curVal.back() == ' ' || curVal.back() == '\t')) curVal.pop_back();
          rowValues.push_back(curVal);
        }
      }

      if (tables.find(tableName) == tables.end()) {
        tables[tableName] = MockTable{{"id", "name", "score"}, {}};
      }
      if (rowValues.empty()) {
        int newId = (int)tables[tableName].rows.size() + 1;
        rowValues = {to_string(newId), "Record " + to_string(newId), "100"};
      }
      tables[tableName].rows.push_back(rowValues);
      return Value(1);
    }

    return Value(1);
  }

  Value mockQuery(int handle, const string &sql) {
    auto &tables = mockDatabases[handle];
    string upper = sql;
    for (char &c : upper)
      c = toupper((unsigned char)c);

    string tableName = "";
    size_t fromPos = upper.find("FROM");
    if (fromPos != string::npos) {
      string rest = sql.substr(fromPos + 4);
      while (!rest.empty() && (rest.front() == ' ' || rest.front() == '\t'))
        rest.erase(0, 1);
      size_t space = rest.find_first_of(" ;\r\n");
      tableName = (space == string::npos) ? rest : rest.substr(0, space);
      while (!tableName.empty() && (tableName.back() == ' ' || tableName.back() == ';' || tableName.back() == '`'))
        tableName.pop_back();
      while (!tableName.empty() && (tableName.front() == ' ' || tableName.front() == '`'))
        tableName.erase(0, 1);
    }

    MockTable *targetTable = nullptr;
    if (!tableName.empty() && tables.find(tableName) != tables.end()) {
      targetTable = &tables[tableName];
    } else if (!tables.empty()) {
      targetTable = &tables.begin()->second;
    }

    if (targetTable == nullptr) {
      return Value("Empty set\n");
    }

    const auto &columns = targetTable->columns;
    const auto &rows = targetTable->rows;
    int numCols = (int)columns.size();

    vector<int> colWidths(numCols, 0);
    for (int i = 0; i < numCols; i++) {
      colWidths[i] = max((int)columns[i].length(), 4);
    }
    for (const auto &r : rows) {
      for (int i = 0; i < numCols && i < (int)r.size(); i++) {
        colWidths[i] = max(colWidths[i], (int)r[i].length());
      }
    }

    ostringstream out;
    auto printSeparator = [&]() {
      out << "+";
      for (int w : colWidths) {
        out << string(w + 2, '-') << "+";
      }
      out << "\n";
    };

    printSeparator();
    out << "|";
    for (int i = 0; i < numCols; i++) {
      out << " " << left << setw(colWidths[i]) << columns[i] << " |";
    }
    out << "\n";
    printSeparator();

    for (const auto &r : rows) {
      out << "|";
      for (int i = 0; i < numCols; i++) {
        string val = i < (int)r.size() ? r[i] : "";
        out << " " << left << setw(colWidths[i]) << val << " |";
      }
      out << "\n";
    }
    printSeparator();

    out << "(" << rows.size() << " row" << (rows.size() == 1 ? "" : "s")
        << " in set)\n";

    return Value(out.str());
  }
};

#endif
