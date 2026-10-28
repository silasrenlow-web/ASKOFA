// ASKOFA Engine - a small JSON reader/writer, written from scratch.
// Supports exactly what scene files need: objects, arrays, strings,
// numbers, booleans and null. No external JSON library.
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace askofa {

struct Json {
    enum class Type { Null, Bool, Number, String, Array, Object };
    Type type = Type::Null;
    bool boolValue = false;
    double numberValue = 0;
    std::string stringValue;
    std::vector<Json> arrayValue;
    std::vector<std::pair<std::string, Json>> objectValue;

    static Json makeBool(bool b) { Json j; j.type = Type::Bool; j.boolValue = b; return j; }
    static Json makeNumber(double n) { Json j; j.type = Type::Number; j.numberValue = n; return j; }
    static Json makeString(std::string s) { Json j; j.type = Type::String; j.stringValue = std::move(s); return j; }
    static Json makeArray() { Json j; j.type = Type::Array; return j; }
    static Json makeObject() { Json j; j.type = Type::Object; return j; }

    bool isNull() const { return type == Type::Null; }
    const Json* find(const std::string& key) const;
    const Json& at(const std::string& key, const Json& fallback) const {
        const Json* v = find(key);
        return v ? *v : fallback;
    }
    double numberOr(double fallback) const { return type == Type::Number ? numberValue : fallback; }
    std::string stringOr(const std::string& fallback) const { return type == Type::String ? stringValue : fallback; }
    bool boolOr(bool fallback) const { return type == Type::Bool ? boolValue : fallback; }
};

// Parses text; on error returns false and fills `error` with a message.
bool parseJson(const std::string& text, Json& out, std::string& error);

// Pretty-prints with 2-space indent (scene files stay human-readable).
std::string stringifyJson(const Json& value);

} // namespace askofa
