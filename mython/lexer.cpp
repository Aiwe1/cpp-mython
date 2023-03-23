#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>
#include <string>

using namespace std;

namespace parse {

    bool operator==(const Token& lhs, const Token& rhs) {
        using namespace token_type;

        if (lhs.index() != rhs.index()) {
            return false;
        }
        if (lhs.Is<Char>()) {
            return lhs.As<Char>().value == rhs.As<Char>().value;
        }
        if (lhs.Is<Number>()) {
            return lhs.As<Number>().value == rhs.As<Number>().value;
        }
        if (lhs.Is<String>()) {
            return lhs.As<String>().value == rhs.As<String>().value;
        }
        if (lhs.Is<Id>()) {
            return lhs.As<Id>().value == rhs.As<Id>().value;
        }
        return true;
    }

    bool operator!=(const Token& lhs, const Token& rhs) {
        return !(lhs == rhs);
    }

    std::ostream& operator<<(std::ostream& os, const Token& rhs) {
        using namespace token_type;

#define VALUED_OUTPUT(type) \
    if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

        VALUED_OUTPUT(Number);
        VALUED_OUTPUT(Id);
        VALUED_OUTPUT(String);
        VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
    if (rhs.Is<type>()) return os << #type;

        UNVALUED_OUTPUT(Class);
        UNVALUED_OUTPUT(Return);
        UNVALUED_OUTPUT(If);
        UNVALUED_OUTPUT(Else);
        UNVALUED_OUTPUT(Def);
        UNVALUED_OUTPUT(Newline);
        UNVALUED_OUTPUT(Print);
        UNVALUED_OUTPUT(Indent);
        UNVALUED_OUTPUT(Dedent);
        UNVALUED_OUTPUT(And);
        UNVALUED_OUTPUT(Or);
        UNVALUED_OUTPUT(Not);
        UNVALUED_OUTPUT(Eq);
        UNVALUED_OUTPUT(NotEq);
        UNVALUED_OUTPUT(LessOrEq);
        UNVALUED_OUTPUT(GreaterOrEq);
        UNVALUED_OUTPUT(None);
        UNVALUED_OUTPUT(True);
        UNVALUED_OUTPUT(False);
        UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

        return os << "Unknown token :("sv;
    }

    Lexer::Lexer(std::istream& in) {
        using namespace token_type;
        int indent = 0;
        
        while (in) {
            string line;
            getline(in, line);
            if (line.size() == 0) {
                continue;
            }
            size_t i = 0;
            
            int indent_new = 0;
            while (i < line.size() && line[i] == ' ') {
                ++i;
                ++indent_new;
            }
            if (i == line.size() || line[i] == '#') {
                continue;
            }
            indent_new /= 2;
            while (indent > indent_new) {
                lexem_.emplace_back(Dedent());
                --indent;
            }
            while (indent < indent_new) {
                lexem_.emplace_back(Indent());
                ++indent;
            }         

            for (; i < line.size(); ++i) {
                if (line[i] == '#') {
                    break;
                }
                if (line[i] == ' ' || line[i] == '\t') {
                    continue;
                }
                if (line[i] >= '0' && line[i] <= '9') {
                    string s;
                    while (line[i] >= '0' && line[i] <= '9' && i < line.size()) {
                        s.push_back(line[i]);
                        ++i;
                    }
                    --i;
                    Number val;
                    val.value = stoi(s);
                    lexem_.push_back(val);
                    continue;
                }
                if (line[i] == '.' || line[i] == ',' || line[i] == '(' || line[i] == ')' || line[i] == '+' || line[i] == '-'
                        || line[i] == '=' || line[i] == '>' || line[i] == '<' || line[i] == '*' || line[i] == '/'
                        || line[i] == ':' || line[i] == '!') {
                    if (line[i] == '=' && i + 1 < line.size() && line[i + 1] == '=') {
                        lexem_.emplace_back(Eq());
                        ++i;
                        continue;
                    }
                    if (line[i] == '>' && i + 1 < line.size() && line[i + 1] == '=') {
                        lexem_.emplace_back(GreaterOrEq());
                        ++i;
                        continue;
                    }
                    if (line[i] == '<' && i + 1 < line.size() && line[i + 1] == '=') {
                        lexem_.emplace_back(LessOrEq());
                        ++i;
                        continue;
                    }
                    if (line[i] == '!' && i + 1 < line.size() && line[i + 1] == '=') {
                        ++i;
                        lexem_.emplace_back(NotEq());
                        continue;
                    }
                    Char val;
                    val.value = line[i];
                    lexem_.push_back(val);
                    continue;
                }

                if (line[i] == '\'') {
                    String val;
                    ++i;
                    while (line[i] != '\'' || line[i - 1] == '\\') {
                        if (line[i] == '\\') {
                            if (line[i + 1] == 't') {
                                val.value.push_back('\t');
                                i += 2;
                                continue;
                            }
                            if (line[i + 1] == 'n') {
                                val.value.push_back('\n');
                                i += 2;
                                continue;
                            }
                            if (line[i + 1] == '\"') {
                                val.value.push_back('\"');
                                i += 2;
                                continue;;
                            }
                            if (line[i + 1] == '\'') {
                                val.value.push_back('\'');
                                i += 2;
                                continue;
                            }
                        }
                        val.value.push_back(line[i]);
                        ++i;
                    }
                    lexem_.push_back(val);
                    continue;
                }
                if (line[i] == '\"') {
                    String val;
                    ++i;
                    while (line[i] != '\"' || line[i - 1] == '\\') {
                        if (line[i] == '\\') {
                            if (line[i + 1] == 't') {
                                val.value.push_back('\t');
                                i += 2;
                                continue;
                            }
                            if (line[i + 1] == 'n') {
                                val.value.push_back('\n');
                                i += 2;
                                continue;
                            }
                            if (line[i + 1] == '\"') {
                                val.value.push_back('\"');
                                i += 2;
                                continue;;
                            }
                            if (line[i + 1] == '\'') {
                                val.value.push_back('\'');
                                i += 2;
                                continue;
                            }
                        }
                        val.value.push_back(line[i]);
                        ++i;
                    }
                    lexem_.push_back(val);
                    continue;
                }

                if ((line[i] >= 'A' && line[i] <= 'Z') || (line[i] >= 'a' && line[i] <= 'z') || line[i] == '_') {
                    string s;
                    while (i < line.size() && ((line[i] >= 'A' && line[i] <= 'Z') || (line[i] >= 'a' && line[i] <= 'z') 
                        || line[i] == '_' || (line[i] >= '0' && line[i] <= '9'))) {
                        s.push_back(line[i]);
                        ++i;
                    }
                    --i;
                    if (s == "class") {
                        lexem_.emplace_back(Class());
                        continue;
                    }
                    if (s == "return") {
                        lexem_.emplace_back(Return());
                        continue;
                    }
                    if (s == "if") {
                        lexem_.emplace_back(If());
                        continue;
                    }
                    if (s == "else") {
                        lexem_.emplace_back(Else());
                        continue;
                    }
                    if (s == "def") {
                        lexem_.emplace_back(Def());
                        continue;
                    }
                    if (s == "print") {
                        lexem_.emplace_back(Print());
                        continue;
                    }
                    if (s == "and") {
                        lexem_.emplace_back(And());
                        continue;
                    }
                    if (s == "or") {
                        lexem_.emplace_back(Or());
                        continue;
                    }
                    if (s == "not") {
                        lexem_.emplace_back(Not());
                        continue;
                    }
                    if (s == "None") {
                        lexem_.emplace_back(None());
                        continue;
                    }
                    if (s == "True") {
                        lexem_.emplace_back(True());
                        continue;
                    }
                    if (s == "False") {
                        lexem_.emplace_back(False());
                        continue;
                    }
                    Id id_;
                    id_.value = s;
                    lexem_.push_back(id_);
                    continue;
                }

                Char val1;
                val1.value = line[i];
                lexem_.push_back(val1);
            }
            lexem_.emplace_back(Newline());
        }
        while (indent > 0) {
            lexem_.emplace_back(Dedent());
            --indent;
        }
        lexem_.emplace_back(Eof());
    }

    const Token& Lexer::CurrentToken() const {
        // Заглушка. Реализуйте метод самостоятельно
        if (cur_lex_ + 1 >= lexem_.size()) {
            return lexem_.back();
            //throw std::logic_error("Not implemented"s);
        }
        return lexem_[cur_lex_];
    }

    Token Lexer::NextToken() {
        if (cur_lex_ + 1 >= lexem_.size()) {
            return lexem_.back();
            //throw std::logic_error("Not implemented"s);
        }
        ++cur_lex_;
        return lexem_[cur_lex_];
    }

}  // namespace parse
