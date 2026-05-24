#pragma once

/**
 * @file JsonSceneParser.h
 * @brief Minimal, dependency-free JSON parser for evacuation scene files.
 *
 * Parses the building / nodes / edges JSON layout described in PROJ-02 into a
 * @ref SceneData object. No external JSON library and no STL containers are
 * used; the parser walks the raw text with a hand-written recursive scanner
 * and fills intrusive linked lists.
 *
 * Supported subset (sufficient for the project's data files):
 *   - objects, arrays, strings, numbers, booleans, null
 *   - nested objects/arrays
 * Comments and exotic escapes are not supported.
 */

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "SceneData.h"

/**
 * @brief Loads building evacuation scenes from JSON text or files.
 */
class JsonSceneParser {
 public:
    /**
     * @brief Parses a scene from a JSON file on disk.
     *
     * @param path Path to the input file (e.g. "data/input_default.json").
     * @param out  Scene object to populate; cleared semantics assume it is new.
     * @throws std::runtime_error if the file cannot be opened or is malformed.
     */
    static void loadFromFile(const std::string& path, SceneData& out) {
        std::ifstream in(path);
        if (!in.is_open()) {
            throw std::runtime_error("Cannot open input file: " + path);
        }
        std::stringstream buffer;
        buffer << in.rdbuf();
        const std::string text = buffer.str();
        loadFromString(text, out);
    }

    /**
     * @brief Parses a scene directly from a JSON string.
     *
     * @param text Raw JSON document.
     * @param out  Scene object to populate.
     * @throws std::runtime_error on malformed input.
     */
    static void loadFromString(const std::string& text, SceneData& out) {
        Cursor c{text, 0};
        skipWs(c);
        expect(c, '{');
        parseRootObject(c, out);
    }

 private:
    /// Lightweight scanning state: the text and a read position.
    struct Cursor {
        const std::string& s;
        std::size_t i;
    };

    // ----- low level helpers -------------------------------------------

    static void skipWs(Cursor& c) {
        while (c.i < c.s.size()) {
            char ch = c.s[c.i];
            if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
                ++c.i;
            } else {
                break;
            }
        }
    }

    static char peek(Cursor& c) {
        if (c.i >= c.s.size()) {
            throw std::runtime_error("Unexpected end of JSON input");
        }
        return c.s[c.i];
    }

    static void expect(Cursor& c, char ch) {
        skipWs(c);
        if (c.i >= c.s.size() || c.s[c.i] != ch) {
            throw std::runtime_error(std::string("JSON parse error: expected '") +
                                     ch + "'");
        }
        ++c.i;
    }

    /// Reads a double-quoted JSON string (basic escape handling).
    static std::string parseString(Cursor& c) {
        skipWs(c);
        expect(c, '"');
        std::string result;
        while (c.i < c.s.size()) {
            char ch = c.s[c.i++];
            if (ch == '"') return result;
            if (ch == '\\') {
                if (c.i >= c.s.size()) break;
                char esc = c.s[c.i++];
                switch (esc) {
                    case 'n': result += '\n'; break;
                    case 't': result += '\t'; break;
                    case 'r': result += '\r'; break;
                    case '"': result += '"';  break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/';  break;
                    default: result += esc;   break;
                }
            } else {
                result += ch;
            }
        }
        throw std::runtime_error("JSON parse error: unterminated string");
    }

    /// Reads a numeric token and returns it as a double.
    static double parseNumber(Cursor& c) {
        skipWs(c);
        std::size_t start = c.i;
        while (c.i < c.s.size()) {
            char ch = c.s[c.i];
            if ((ch >= '0' && ch <= '9') || ch == '-' || ch == '+' ||
                ch == '.' || ch == 'e' || ch == 'E') {
                ++c.i;
            } else {
                break;
            }
        }
        if (c.i == start) {
            throw std::runtime_error("JSON parse error: expected number");
        }
        return std::stod(c.s.substr(start, c.i - start));
    }

    /// Consumes a literal token (true / false / null).
    static bool parseLiteral(Cursor& c, const char* lit) {
        skipWs(c);
        std::size_t n = 0;
        while (lit[n] != '\0') ++n;
        if (c.s.compare(c.i, n, lit) == 0) {
            c.i += n;
            return true;
        }
        return false;
    }

    /// Skips an arbitrary JSON value (used for keys we do not care about).
    static void skipValue(Cursor& c) {
        skipWs(c);
        char ch = peek(c);
        if (ch == '"') {
            parseString(c);
        } else if (ch == '{') {
            ++c.i;
            skipWs(c);
            if (peek(c) == '}') { ++c.i; return; }
            while (true) {
                parseString(c);
                expect(c, ':');
                skipValue(c);
                skipWs(c);
                if (peek(c) == ',') { ++c.i; continue; }
                expect(c, '}');
                return;
            }
        } else if (ch == '[') {
            ++c.i;
            skipWs(c);
            if (peek(c) == ']') { ++c.i; return; }
            while (true) {
                skipValue(c);
                skipWs(c);
                if (peek(c) == ',') { ++c.i; continue; }
                expect(c, ']');
                return;
            }
        } else if (parseLiteral(c, "true") || parseLiteral(c, "false") ||
                   parseLiteral(c, "null")) {
            // literal consumed
        } else {
            parseNumber(c);
        }
    }

    // ----- domain parsing ----------------------------------------------

    static void parseRootObject(Cursor& c, SceneData& out) {
        skipWs(c);
        if (peek(c) == '}') { ++c.i; return; }
        while (true) {
            std::string key = parseString(c);
            expect(c, ':');
            if (key == "building") {
                parseBuilding(c, out);
            } else if (key == "nodes") {
                parseNodes(c, out);
            } else if (key == "edges") {
                parseEdges(c, out);
            } else if (key == "fire_spread_rate") {
                out.fire_spread_rate = parseNumber(c);
            } else {
                skipValue(c);
            }
            skipWs(c);
            if (peek(c) == ',') { ++c.i; continue; }
            expect(c, '}');
            return;
        }
    }

    static void parseBuilding(Cursor& c, SceneData& out) {
        expect(c, '{');
        skipWs(c);
        if (peek(c) == '}') { ++c.i; return; }
        while (true) {
            std::string key = parseString(c);
            expect(c, ':');
            if (key == "floors") {
                out.floors = static_cast<int>(parseNumber(c));
            } else if (key == "fire_origin") {
                out.fire_origin = parseString(c);
            } else {
                skipValue(c);
            }
            skipWs(c);
            if (peek(c) == ',') { ++c.i; continue; }
            expect(c, '}');
            return;
        }
    }

    static void parseNodes(Cursor& c, SceneData& out) {
        expect(c, '[');
        skipWs(c);
        if (peek(c) == ']') { ++c.i; return; }
        while (true) {
            SceneNode* node = new SceneNode();
            parseOneNode(c, *node);
            out.appendNode(node);
            skipWs(c);
            if (peek(c) == ',') { ++c.i; continue; }
            expect(c, ']');
            return;
        }
    }

    static void parseOneNode(Cursor& c, SceneNode& node) {
        expect(c, '{');
        skipWs(c);
        if (peek(c) == '}') { ++c.i; return; }
        while (true) {
            std::string key = parseString(c);
            expect(c, ':');
            if (key == "id") {
                node.id = parseString(c);
            } else if (key == "type") {
                node.type = parseString(c);
            } else if (key == "floor") {
                node.floor = static_cast<int>(parseNumber(c));
            } else if (key == "occupants") {
                node.occupants = static_cast<int>(parseNumber(c));
            } else if (key == "flammability") {
                node.flammability = parseNumber(c);
            } else if (key == "x") {
                node.x = static_cast<int>(parseNumber(c));
            } else if (key == "y") {
                node.y = static_cast<int>(parseNumber(c));
            } else {
                skipValue(c);
            }
            skipWs(c);
            if (peek(c) == ',') { ++c.i; continue; }
            expect(c, '}');
            return;
        }
    }

    static void parseEdges(Cursor& c, SceneData& out) {
        expect(c, '[');
        skipWs(c);
        if (peek(c) == ']') { ++c.i; return; }
        while (true) {
            SceneEdge* edge = new SceneEdge();
            parseOneEdge(c, *edge);
            out.appendEdge(edge);
            skipWs(c);
            if (peek(c) == ',') { ++c.i; continue; }
            expect(c, ']');
            return;
        }
    }

    static void parseOneEdge(Cursor& c, SceneEdge& edge) {
        expect(c, '{');
        skipWs(c);
        if (peek(c) == '}') { ++c.i; return; }
        while (true) {
            std::string key = parseString(c);
            expect(c, ':');
            if (key == "from") {
                edge.from = parseString(c);
            } else if (key == "to") {
                edge.to = parseString(c);
            } else if (key == "capacity") {
                edge.capacity = static_cast<int>(parseNumber(c));
            } else if (key == "traversal_time") {
                edge.traversal_time = static_cast<int>(parseNumber(c));
            } else if (key == "flammability") {
                edge.flammability = parseNumber(c);
            } else {
                skipValue(c);
            }
            skipWs(c);
            if (peek(c) == ',') { ++c.i; continue; }
            expect(c, '}');
            return;
        }
    }
};
