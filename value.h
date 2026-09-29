#ifndef VALUE_H
#define VALUE_H

#include <iostream>
#include <sstream>
#include <string>

using namespace std;

enum class ValueType { NIL, INT, FLOAT, BOOL, STRING, ARRAY };

struct Value {
  ValueType type = ValueType::NIL;
  long long intVal = 0;
  double floatVal = 0.0;
  bool boolVal = false;
  string stringVal = "";
  shared_ptr<vector<Value>> arrayVal; // for ARRAY type
  Value()
      : type(ValueType::NIL), intVal(0), floatVal(0.0), boolVal(false),
        stringVal("") {}
  Value(long long v)
      : type(ValueType::INT), intVal(v), floatVal((double)v), boolVal(v != 0),
        stringVal("") {}
  Value(int v)
      : type(ValueType::INT), intVal(v), floatVal((double)v), boolVal(v != 0),
        stringVal("") {}
  Value(double v)
      : type(ValueType::FLOAT), intVal((long long)v), floatVal(v),
        boolVal(v != 0.0), stringVal("") {}
  Value(bool v)
      : type(ValueType::BOOL), intVal(v ? 1 : 0), floatVal(v ? 1.0 : 0.0),
        boolVal(v), stringVal("") {}
  Value(const string &v)
      : type(ValueType::STRING), intVal(0), floatVal(0.0), boolVal(!v.empty()),
        stringVal(v) {}
  Value(const char *v)
      : type(ValueType::STRING), intVal(0), floatVal(0.0), boolVal(v && *v),
        stringVal(v ? v : "") {}

  static Value nil() { return Value(); }

  // Construct an array Value
  static Value makeArray(shared_ptr<vector<Value>> arr) {
    Value v;
    v.type = ValueType::ARRAY;
    v.arrayVal = arr;
    v.boolVal = true;
    return v;
  }

  bool isTruthy() const {
    switch (type) {
    case ValueType::NIL:
      return false;
    case ValueType::BOOL:
      return boolVal;
    case ValueType::INT:
      return intVal != 0;
    case ValueType::FLOAT:
      return floatVal != 0.0;
    case ValueType::STRING:
      return !stringVal.empty();
    case ValueType::ARRAY:
      return arrayVal && !arrayVal->empty();
    }
    return false;
  }

  string toString() const {
    switch (type) {
    case ValueType::NIL:
      return "nil";
    case ValueType::BOOL:
      return boolVal ? "true" : "false";
    case ValueType::INT:
      return to_string(intVal);
    case ValueType::FLOAT: {
      ostringstream ss;
      ss << floatVal;
      return ss.str();
    }
    case ValueType::STRING:
      return stringVal;
    case ValueType::ARRAY: {
      string s = "[";
      if (arrayVal) {
        for (size_t i = 0; i < arrayVal->size(); i++) {
          if (i > 0) s += ", ";
          s += (*arrayVal)[i].toString();
        }
      }
      s += "]";
      return s;
    }
    }
    return "nil";
  }

  double asNumber() const {
    if (type == ValueType::INT)
      return (double)intVal;
    if (type == ValueType::FLOAT)
      return floatVal;
    if (type == ValueType::BOOL)
      return boolVal ? 1.0 : 0.0;
    if (type == ValueType::STRING) {
      try {
        return stod(stringVal);
      } catch (...) {
        return 0.0;
      }
    }
    return 0.0;
  }
};

// Operator overloads for Value
inline Value operator+(const Value &a, const Value &b) {
  if (a.type == ValueType::STRING || b.type == ValueType::STRING) {
    return Value(a.toString() + b.toString());
  }
  if (a.type == ValueType::FLOAT || b.type == ValueType::FLOAT) {
    return Value(a.asNumber() + b.asNumber());
  }
  return Value(a.intVal + b.intVal);
}

inline Value operator-(const Value &a, const Value &b) {
  if (a.type == ValueType::FLOAT || b.type == ValueType::FLOAT) {
    return Value(a.asNumber() - b.asNumber());
  }
  return Value(a.intVal - b.intVal);
}

inline Value operator*(const Value &a, const Value &b) {
  if (a.type == ValueType::FLOAT || b.type == ValueType::FLOAT) {
    return Value(a.asNumber() * b.asNumber());
  }
  return Value(a.intVal * b.intVal);
}

inline Value operator/(const Value &a, const Value &b) {
  double bNum = b.asNumber();
  if (bNum == 0.0) {
    cerr << "Runtime Error: Division by zero." << endl;
    return Value(0);
  }
  if (a.type == ValueType::FLOAT || b.type == ValueType::FLOAT) {
    return Value(a.asNumber() / bNum);
  }
  return Value(a.intVal / b.intVal);
}

inline Value operator%(const Value &a, const Value &b) {
  if (b.intVal == 0) {
    cerr << "Runtime Error: Modulo by zero." << endl;
    return Value(0);
  }
  return Value(a.intVal % b.intVal);
}

inline bool operator==(const Value &a, const Value &b) {
  if (a.type != b.type) {
    if ((a.type == ValueType::INT || a.type == ValueType::FLOAT) &&
        (b.type == ValueType::INT || b.type == ValueType::FLOAT)) {
      return a.asNumber() == b.asNumber();
    }
    return false;
  }
  switch (a.type) {
  case ValueType::NIL:
    return true;
  case ValueType::BOOL:
    return a.boolVal == b.boolVal;
  case ValueType::INT:
    return a.intVal == b.intVal;
  case ValueType::FLOAT:
    return a.floatVal == b.floatVal;
  case ValueType::STRING:
    return a.stringVal == b.stringVal;
  case ValueType::ARRAY:
    return a.arrayVal == b.arrayVal; // pointer equality (same array object)
  }
  return false;
}

inline bool operator!=(const Value &a, const Value &b) { return !(a == b); }

inline bool operator<(const Value &a, const Value &b) {
  if (a.type == ValueType::STRING && b.type == ValueType::STRING) {
    return a.stringVal < b.stringVal;
  }
  return a.asNumber() < b.asNumber();
}

inline bool operator<=(const Value &a, const Value &b) {
  return (a < b) || (a == b);
}

inline bool operator>(const Value &a, const Value &b) { return !(a <= b); }

inline bool operator>=(const Value &a, const Value &b) { return !(a < b); }

#endif
