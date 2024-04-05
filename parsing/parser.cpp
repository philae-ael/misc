#include <cstdio>
#include <cstdlib>
#include <optional>
#include <readline/readline.h>

enum class TokenTag { EndOfStream, Integer, Binop, ParenOpen, ParenClose };

struct BinopInfo {
  const char *name;
  int precedence;
  int (*op)(int, int);
};

const BinopInfo binop_infos[] = {
    {"mul", 100, [](int a, int b) { return a * b; }},
    {"div", 100, [](int a, int b) { return a / b; }},
    {"add", 50, [](int a, int b) { return a + b; }},
    {"sub", 50, [](int a, int b) { return a - b; }},
};

struct Token {
  TokenTag tag;
  union {
    struct {
      int value;
    } integer;
    struct {
      BinopInfo infos;
    } binop;
  };
};

struct Tokenizer {
  const char *rest;
  Token next() {
    const char *c;

    // skip whitespaces
    while (true) {
      c = next_char();
      if (c == nullptr) {
        return {TokenTag::EndOfStream};
      }
      if (*c != ' ') {
        break;
      }
    }

    switch (*c) {
    case '(':
      return {TokenTag::ParenOpen};
    case ')':
      return {TokenTag::ParenClose};
    case '0' ... '9': {
      int v = *c - '0';
      while (true) {
        const char *p = peek_char();
        if (p == NULL || *p < '0' || *p > '9') {
          break;
        }
        c = next_char();
        v = v * 10 + *c - '0';
      }
      return {
          .tag = TokenTag::Integer,
          .integer = {v},
      };
    }
    case '*':
      return {.tag = TokenTag::Binop, .binop = {binop_infos[0]}};
    case '/':
      return {.tag = TokenTag::Binop, .binop = {binop_infos[1]}};
    case '+':
      return {.tag = TokenTag::Binop, .binop = {binop_infos[2]}};
    case '-':
      return {.tag = TokenTag::Binop, .binop = {binop_infos[3]}};
    default:
      fprintf(stderr, "don't know how to parse %c\n", *c);
      exit(-1);
    }
  }

private:
  const char *next_char() {
    if (*rest == '\0') {
      return nullptr;
    }
    return rest++;
  }
  const char *peek_char() {
    if (*rest == '\0') {
      return nullptr;
    }
    return rest;
  }
};

const size_t PARSER_STACK_SIZE = 50;
struct Parser {
  Tokenizer tokenizer;
  int stack[PARSER_STACK_SIZE] = {};
  size_t cur = 0;
  std::optional<Token> curToken;
  void parse(int precedence = -9999) {
    while (true) {
      Token tok = peek();
      switch (tok.tag) {
      case TokenTag::EndOfStream:
        return;
      case TokenTag::Integer:
        advance();
        push(tok.integer.value);
        break;
      case TokenTag::Binop: {
        if (tok.binop.infos.precedence <= precedence) {
          return;
        }
        advance();
        parse(tok.binop.infos.precedence);
        int b = pop();
        int a = pop();
        int res = tok.binop.infos.op(a, b);
        printf("%s(%d, %d) = %d\n", tok.binop.infos.name, a, b, res);
        push(res);
        break;
      }
      case TokenTag::ParenOpen:
        advance();
        parse();
        if (peek().tag != TokenTag::ParenClose) {
          fprintf(stderr, "missing clossing parenthesis\n");
          exit(-1);
        }
        break;
      case TokenTag::ParenClose:
        return;
      }
    }
  }

private:
  Token peek() {
    if (curToken) {
      return *curToken;
    }
    return *(curToken = tokenizer.next());
  }
  void advance() {
    curToken.reset();
    peek();
  }
  void push(int i) {
    if (cur == PARSER_STACK_SIZE) {
      fprintf(stderr, "parser stack full\n");
      exit(-1);
    }
    stack[cur++] = i;
  }
  int pop() {
    if (cur == 0) {
      fprintf(stderr, "parser stack empty\n");
      exit(-1);
    }
    return stack[--cur];
  }
};

int main(int argc, char *argv[]) {
  const char *line;
  while ((line = readline(">> ")) != NULL) {
    Parser parser{Tokenizer{line}};
    parser.parse();

    for (size_t i = 0; i < parser.cur; i++) {
      printf("%zu: %d\n", i, parser.stack[i]);
    }
  }
  return 0;
}
