#include "json.hpp"

#include <cctype>
#include <cmath>
#include <cstdio>

namespace askofa {

const Json* Json::find(const std::string& key) const {
    if (type != Type::Object) return nullptr;
    for (const auto& kv : objectValue) {
        if (kv.first == key) return &kv.second;
    }
    return nullptr;
}

namespace {

struct Parser {
    const std::string& text;
    size_t pos = 0;
    std::string error;

    void skipWs() {
        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) ++pos;
    }
    bool fail(const std::string& msg) { error = msg + " at offset " + std::to_string(pos); return false; }

    bool parseValue(Json& out) {
        skipWs();
        if (pos >= text.size()) return fail("unexpected end of input");
        char c = text[pos];
        if (c == '{') return parseObject(out);
        if (c == '[') return parseArray(out);
        if (c == '"') { std::string s; if (!parseString(s)) return false; out = Json::makeString(s); return true; }
        if (c == 't' || c == 'f') return parseBool(out);
        if (c == 'n') {
            if (text.compare(pos, 4, "null") == 0) { pos += 4; out = Json(); return true; }
            return fail("invalid literal");
        }
        return parseNumber(out);
    }
    bool parseBool(Json& out) {
        if (text.compare(pos, 4, "true") == 0) { pos += 4; out = Json::makeBool(true); return true; }
        if (text.compare(pos, 5, "false") == 0) { pos += 5; out = Json::makeBool(false); return true; }
        return fail("invalid literal");
    }
    bool parseNumber(Json& out) {
        size_t start = pos;
        if (pos < text.size() && (text[pos] == '-' || text[pos] == '+')) ++pos;
        bool any = false;
        while (pos < text.size() && (std::isdigit(static_cast<unsigned char>(text[pos])) || text[pos] == '.' ||
                                     text[pos] == 'e' || text[pos] == 'E' || text[pos] == '-' || text[pos] == '+')) {
            ++pos; any = true;
        }
        if (!any) return fail("invalid number");
        try {
            out = Json::makeNumber(std::stod(text.substr(start, pos - start)));
        } catch (...) {
            return fail("invalid number");
        }
        return true;
    }
    bool parseString(std::string& out) {
        if (text[pos] != '"') return fail("expected string");
        ++pos;
        out.clear();
        while (pos < text.size()) {
            char c = text[pos++];
            if (c == '"') return true;
            if (c == '\\' && pos < text.size()) {
                char e = text[pos++];
                switch (e) {
                    case 'n': out += '\n'; break;
                    case 't': out += '\t'; break;
                    case 'r': out += '\r'; break;
                    case '"': out += '"'; break;
                    case '\\': out += '\\'; break;
                    case '/': out += '/'; break;
                    case 'u': {
                        if (pos + 4 > text.size()) return fail("bad \\u escape");
                        unsigned code = 0;
                        for (int i = 0; i < 4; ++i) {
                            char h = text[pos++];
                            code <<= 4;
                            if (h >= '0' && h <= '9') code += static_cast<unsigned>(h - '0');
                            else if (h >= 'a' && h <= 'f') code += static_cast<unsigned>(h - 'a' + 10);
                            else if (h >= 'A' && h <= 'F') code += static_cast<unsigned>(h - 'A' + 10);
                            else return fail("bad \\u escape");
                        }
                        // Encode as UTF-8 (BMP only; scene files are plain text).
                        if (code < 0x80) out += static_cast<char>(code);
                        else if (code < 0x800) {
                            out += static_cast<char>(0xC0 | (code >> 6));
                            out += static_cast<char>(0x80 | (code & 0x3F));
                        } else {
                            out += static_cast<char>(0xE0 | (code >> 12));
                            out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                            out += static_cast<char>(0x80 | (code & 0x3F));
                        }
                        break;
                    }
                    default: return fail("bad escape");
                }
            } else {
                out += c;
            }
        }
        return fail("unterminated string");
    }
    bool parseArray(Json& out) {
        ++pos; // [
        out = Json::makeArray();
        skipWs();
        if (pos < text.size() && text[pos] == ']') { ++pos; return true; }
        while (true) {
            Json v;
            if (!parseValue(v)) return false;
            out.arrayValue.push_back(std::move(v));
            skipWs();
            if (pos >= text.size()) return fail("unterminated array");
            if (text[pos] == ',') { ++pos; continue; }
            if (text[pos] == ']') { ++pos; return true; }
            return fail("expected ',' or ']' in array");
        }
    }
    bool parseObject(Json& out) {
        ++pos; // {
        out = Json::makeObject();
        skipWs();
        if (pos < text.size() && text[pos] == '}') { ++pos; return true; }
        while (true) {
            skipWs();
            std::string key;
            if (!parseString(key)) return false;
            skipWs();
            if (pos >= text.size() || text[pos] != ':') return fail("expected ':' in object");
            ++pos;
            Json v;
            if (!parseValue(v)) return false;
            out.objectValue.emplace_back(std::move(key), std::move(v));
            skipWs();
            if (pos >= text.size()) return fail("unterminated object");
            if (text[pos] == ',') { ++pos; continue; }
            if (text[pos] == '}') { ++pos; return true; }
            return fail("expected ',' or '}' in object");
        }
    }
};

void writeEscaped(const std::string& s, std::string& out) {
    out += '"';
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\t': out += "\\t"; break;
            case '\r': out += "\\r"; break;
            default: out += c; break;
        }
    }
    out += '"';
}

void writeValue(const Json& v, std::string& out, int indent) {
    std::string pad(static_cast<size_t>(indent) * 2, ' ');
    std::string padIn(static_cast<size_t>(indent + 1) * 2, ' ');
    switch (v.type) {
        case Json::Type::Null: out += "null"; break;
        case Json::Type::Bool: out += v.boolValue ? "true" : "false"; break;
        case Json::Type::Number: {
            char buf[40];
            if (v.numberValue == static_cast<long long>(v.numberValue) && std::fabs(v.numberValue) < 9e15) {
                std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(v.numberValue));
            } else {
                std::snprintf(buf, sizeof(buf), "%.6g", v.numberValue);
            }
            out += buf;
            break;
        }
        case Json::Type::String: writeEscaped(v.stringValue, out); break;
        case Json::Type::Array: {
            if (v.arrayValue.empty()) { out += "[]"; break; }
            out += "[\n";
            for (size_t i = 0; i < v.arrayValue.size(); ++i) {
                out += padIn;
                writeValue(v.arrayValue[i], out, indent + 1);
                if (i + 1 < v.arrayValue.size()) out += ",";
                out += "\n";
            }
            out += pad + "]";
            break;
        }
        case Json::Type::Object: {
            if (v.objectValue.empty()) { out += "{}"; break; }
            out += "{\n";
            for (size_t i = 0; i < v.objectValue.size(); ++i) {
                out += padIn;
                writeEscaped(v.objectValue[i].first, out);
                out += ": ";
                writeValue(v.objectValue[i].second, out, indent + 1);
                if (i + 1 < v.objectValue.size()) out += ",";
                out += "\n";
            }
            out += pad + "}";
            break;
        }
    }
}

} // namespace

bool parseJson(const std::string& text, Json& out, std::string& error) {
    Parser p{text, 0, ""};
    if (!p.parseValue(out)) { error = p.error; return false; }
    p.skipWs();
    if (p.pos != text.size()) { error = "trailing characters at offset " + std::to_string(p.pos); return false; }
    return true;
}

std::string stringifyJson(const Json& value) {
    std::string out;
    writeValue(value, out, 0);
    out += "\n";
    return out;
}

} // namespace askofa
