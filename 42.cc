#include <stack>
#include <string>
#include <functional>
#include <exception>
#include <cassert>
#include <unordered_map>

struct OperatorAlreadyDefined : public std::exception {
    char const *what() const throw() override { return "OperatorAlreadyDefined"; }
};

struct SyntaxError : public std::exception {
    char const *what() const throw() override { return "SyntaxError"; }
};

struct UnknownOperator : public std::exception {
    char const *what() const throw() override { return "UnknownOperator"; }
};

/* Function (void) -> (int) */
typedef std::function<int()> Lazy;

class LazyCalculator {
    /* Unordered map of known operators. Starts with +, -, *, / */
    std::unordered_map <char, std::function<int(Lazy, Lazy)> > operators {
        {'+', [](Lazy a, Lazy b) { return a() + b(); }},
        {'-', [](Lazy a, Lazy b) { return a() - b(); }},
        {'*', [](Lazy a, Lazy b) { return a() * b(); }},
        {'/', [](Lazy a, Lazy b) { return a() / b(); }}
    };
    /* Unordered map of three preset literals: 0, 2, 4 */
    std::unordered_map <char, Lazy> literals {
        {'0', []{ return 0; }},
        {'2', []{ return 2; }},
        {'4', []{ return 4; }}
    };

    public:
        Lazy parse(const std::string& s) const {
            /* stack of past literals/results of operations */
            std::stack <Lazy> lazyStack;

            for (char c : s) {
                auto literal = literals.find(c);
                if (literal != literals.end()) {
                    lazyStack.push(literal->second);
                }
                else {
                    auto op = operators.find(c);
                    if (op == operators.end()) {
                        throw UnknownOperator();
                    }
                    else if (lazyStack.size() < 2) {
                        throw SyntaxError();
                    }

                    Lazy first = lazyStack.top();
                    lazyStack.pop();
                    Lazy second = lazyStack.top();
                    lazyStack.pop();

                    lazyStack.push([first, second, op]() { return op->second(second, first); });
                }
            }

            if (lazyStack.size() != 1) {
                throw SyntaxError();
            }

            return lazyStack.top();
        }

        int calculate(const std::string& s) const {
            return parse(s)();
        }

        void define(char c, std::function<int(Lazy, Lazy)> fn) {
            if (operators.find(c) != operators.end() || literals.find(c) != literals.end()) {
                throw OperatorAlreadyDefined();
            }

            operators.insert(make_pair(c, fn));
        }
};

std::function<void(void)> operator*(int n, std::function<void(void)> fn) {
    return [=]() {
        for (int i = 0; i < n; i++)
            fn();
    };
}

int manytimes(Lazy n, Lazy fn) {
    (n() * fn)();  // Did you notice the type cast?
    return 0;
}

int main() {
    LazyCalculator calculator;

    // The only literals...
    assert(calculator.calculate("0") == 0);
    assert(calculator.calculate("2") == 2);
    assert(calculator.calculate("4") == 4);

    // Built-in operators.
    assert(calculator.calculate("42+") == 6);
    assert(calculator.calculate("24-") == -2);
    assert(calculator.calculate("42*") == 8);
    assert(calculator.calculate("42/") == 2);

    assert(calculator.calculate("42-2-") == 0);
    assert(calculator.calculate("242--") == 0);
    assert(calculator.calculate("22+2-2*2/0-") == 2);

    // The fun.
    calculator.define('!', [](Lazy a, Lazy b) { return a()*10 + b(); });
    assert(calculator.calculate("42!") == 42);

    std::string buffer;
    calculator.define(',', [](Lazy a, Lazy b) { a(); return b(); });
    calculator.define('P', [&buffer](Lazy, Lazy) { buffer += "pomidor"; return 0; });
    assert(calculator.calculate("42P42P42P42P42P42P42P42P42P42P42P42P42P42P42P4"
                                "2P,,,,42P42P42P42P42P,,,42P,42P,42P42P,,,,42P,"
                                ",,42P,42P,42P,,42P,,,42P,42P42P42P42P42P42P42P"
                                "42P,,,42P,42P,42P,,,,,,,,,,,,") == 0);
    assert(buffer.length() == 42 * std::string("pomidor").length());

    std::string buffer2 = std::move(buffer);
    buffer.clear();
    calculator.define('$', manytimes);
    assert(calculator.calculate("42!42P$") == 0);
    // Notice, how std::move worked.
    assert(buffer.length() == 42 * std::string("pomidor").length());

    calculator.define('?', [](Lazy a, Lazy b) { return a() ? b() : 0; });
    assert(calculator.calculate("042P?") == 0);
    assert(buffer == buffer2);

    assert(calculator.calculate("042!42P$?") == 0);
    assert(buffer == buffer2);

    calculator.define('1', [](Lazy, Lazy) { return 1; });
    assert(calculator.calculate("021") == 1);

    for (auto bad: {"", "42", "4+", "424+"}) {
        try {
            calculator.calculate(bad);
            assert(false);
        }
        catch (SyntaxError) {
        }
    }

    try {
        calculator.define('!', [](Lazy a, Lazy b) { return a()*10 + b(); });
        assert(false);
    }
    catch (OperatorAlreadyDefined) {
    }

    try {
        calculator.define('0', [](Lazy, Lazy) { return 0; });
        assert(false);
    }
    catch (OperatorAlreadyDefined) {
    }

    try {
        calculator.calculate("02&");
        assert(false);
    }
    catch (UnknownOperator) {
    }

    return 0;
}