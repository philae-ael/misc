#include <signal.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <readline/readline.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
[[noreturn]] [[gnu::format(printf, 1, 2)]] void fatal(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  raise(SIGTRAP);
  exit(1);
}

typedef enum TokenTag {
  TokenTag_EndOfStream,
  TokenTag_Integer,
  TokenTag_Binop,
  TokenTag_ParenOpen,
  TokenTag_ParenClose,
} TokenTag;

typedef struct BinopInfo {
  char op;
  const char *name;
  int binding_power_left;
  int binding_power_right;
} BinopInfo;

const BinopInfo binop_infos[] = {
    {'*', "mul", 3, 4}, {'/', "div", 3, 4}, {'+', "add", 1, 2}, {'-', "sub", 1, 2}, {'^', "pow", 5, 6},
};
thread_local char tmp_buf_str[256];
[[gnu::format(printf, 1, 2)]] const char *tmp_snprintf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  if (vsnprintf(tmp_buf_str, sizeof(tmp_buf_str), fmt, ap) < 0) {
    fatal("tmp_snprintf failed \n");
  }
  va_end(ap);
  return tmp_buf_str;
}

typedef struct TokenLoc {
  int line;
  int column;
} TokenLoc;
typedef struct Token {
  TokenTag tag;
  union {
    struct {
      int value;
    } integer;
    struct {
      BinopInfo infos;
    } binop;
  };
} Token;

typedef struct TokenError {
  const char *message;
  TokenLoc loc;
} TokenError;
typedef struct TokenResult {
  union {
    Token token;
    TokenError error;
  };
  bool is_error;
} TokenResult;

typedef struct Tokenizer {
  const char *rest;
  TokenResult tok;
  TokenLoc loc;
} Tokenizer;

TokenResult tokenizer_peek(Tokenizer *tokenizer) { return tokenizer->tok; }

char tokenizer_next_char(Tokenizer *tokenizer) {
  if (*tokenizer->rest == '\0') {
    return '\0';
  }
  char c = *tokenizer->rest++;
  if (c == '\n') {
    tokenizer->loc.line++;
    tokenizer->loc.column = 1;
  } else {
    tokenizer->loc.column++;
  }
  return c;
}

const char *tokenizer_peek_char(Tokenizer *tokenizer) {
  if (*tokenizer->rest == '\0') {
    return nullptr;
  }
  return tokenizer->rest;
}
void tokenizer_next(Tokenizer *t) {
  if (t->tok.is_error) {
    return;
  }
  char c;

  // skip whitespaces
  while (true) {
    c = tokenizer_next_char(t);
    if (c == '\0') {
      t->tok.token = (Token){TokenTag_EndOfStream};
      return;
    }
    if (c != ' ' && c != '\t' && c != '\n') {
      break;
    }
  }

  switch (c) {
  case '(':
    t->tok.token = (Token){TokenTag_ParenOpen};
    return;
  case ')':
    t->tok.token = (Token){TokenTag_ParenClose};
    return;
  case '0' ... '9': {
    int v = c - '0';
    while (true) {
      const char *p = tokenizer_peek_char(t);
      if (p == NULL || *p < '0' || *p > '9') {
        break;
      }
      c = tokenizer_next_char(t);
      v = v * 10 + c - '0';
    }
    t->tok.token = (Token){.tag = TokenTag_Integer, .integer = {v}};
    return;
  }
  default:
    for (unsigned long i = 0; i < ARRAY_SIZE(binop_infos); i++) {
      if (c == binop_infos[i].op) {
        t->tok.token = (Token){.tag = TokenTag_Binop, .binop = {binop_infos[i]}};
        return;
      }
    }

    t->tok = (TokenResult){.is_error = true,
                           .error = {
                               .message = tmp_snprintf("invalid character: '%c' ", c),
                               .loc = t->loc,
                           }};

    return;
  }
}

Tokenizer tokenizer_create(const char *input) {
  Tokenizer t = {input, {.is_error = false, .token = {TokenTag_EndOfStream}}, {1, 0}};
  tokenizer_next(&t);
  return t;
}

typedef struct memory_pool {
  void *ptr;
  size_t cur;
  size_t capacity;
} memory_pool;

void *memory_pool_alloc(memory_pool *pool, size_t size, size_t align) {
  size_t cur_aligned = (pool->cur + align - 1) & ~(align - 1);
  if (cur_aligned + size > pool->capacity) {
    fatal("out of memory\n");
  }
  void *p = (char *)pool->ptr + cur_aligned;
  pool->cur = cur_aligned + size;
  return p;
}
void memory_pool_reset(memory_pool *pool) { pool->cur = 0; }

memory_pool memory_pool_create(size_t capacity) {
  return (memory_pool){
      .ptr = malloc(capacity),
      .cur = 0,
      .capacity = capacity,
  };
}
void memory_pool_destroy(memory_pool *pool) { free(pool->ptr); }

typedef struct Parser {
  Tokenizer tokenizer;
  memory_pool *pool;
} Parser;

typedef struct ParserError {
  enum {
    ParserError_None,
    ParserError_UnexpectedToken,
    ParserError_UnexpectedEOF,
    ParserError_Other,
    ParserError_TokenError,
  } tag;
  union {
    struct {
      Token got;
      size_t expected_size;
      TokenTag *expected_tags;
    } unexpected_token;
    TokenError token_error;
  };
  TokenLoc loc;
} ParserError;

ParserError parser_expect(Parser *p, size_t size, TokenTag *tags) {
  TokenResult res = tokenizer_peek(&p->tokenizer);
  if (res.is_error) {
    return (ParserError){.tag = ParserError_TokenError, .token_error = res.error, .loc = res.error.loc};
  }

  for (size_t i = 0; i < size; i++) {
    if (res.token.tag == tags[i]) {
      goto found;
    }
  }

  TokenTag *tag_cpy = memory_pool_alloc(p->pool, sizeof(Token) * size, alignof(Token));
  memcpy(tag_cpy, tags, sizeof(Token) * size);
  return (ParserError){
      .tag = ParserError_UnexpectedToken,
      .unexpected_token =
          {
              .got = res.token,
              .expected_size = size,
              .expected_tags = tag_cpy,
          },
      .loc = p->tokenizer.loc,
  };
found:
  tokenizer_next(&p->tokenizer);
  return (ParserError){.tag = ParserError_None};
}

typedef struct ASTNode {
  enum {
    ASTNodeTag_Integer,
    ASTNodeTag_Binop,
  } tag;
  union {
    struct {
      int value;
    } integer;
    struct {
      BinopInfo infos;
      struct ASTNode *left;
      struct ASTNode *right;
    } binop;
  };
} ASTNode;

typedef struct ParserResult {
  union {
    ASTNode *node;
    ParserError error;
  };
  bool is_error;
} ParserResult;

ParserResult parser_parse_(Parser *p, int minimum_binding_power) {
  ASTNode *node = nullptr;

  TokenResult tokres = tokenizer_peek(&p->tokenizer);
  if (tokres.is_error) {
    return (ParserResult){.is_error = true,
                          .error = {
                              .tag = ParserError_TokenError,
                              .token_error = tokres.error,
                              .loc = tokres.error.loc,
                          }};
  }
  Token tok = tokres.token;
  switch (tok.tag) {
  case TokenTag_Integer: {
    tokenizer_next(&p->tokenizer);

    node = memory_pool_alloc(p->pool, sizeof(ASTNode), alignof(ASTNode));
    *node = (ASTNode){ASTNodeTag_Integer, .integer = {tok.integer.value}};
    break;
  }
  case TokenTag_ParenOpen: {
    tokenizer_next(&p->tokenizer);

    ParserResult res = parser_parse_(p, 0);
    if (res.is_error) {
      return res;
    }
    node = res.node;

    ParserError err = parser_expect(p, 1, (TokenTag[]){TokenTag_ParenClose});
    if (err.tag) {
      return (ParserResult){.is_error = true, .error = err};
    }
    break;
  }
  default:
    return (ParserResult){
        .is_error = true,
        .error = parser_expect(p, 2, (TokenTag[]){TokenTag_Integer, TokenTag_ParenOpen}),
    };
  }

  while (true) {
    TokenResult res = tokenizer_peek(&p->tokenizer);
    if (res.is_error) {
      return (ParserResult){.is_error = true,
                            .error = {
                                .tag = ParserError_TokenError,
                                .token_error = res.error,
                                .loc = res.error.loc,
                            }};
    }
    Token op = res.token;
    switch (op.tag) {
    case TokenTag_EndOfStream:
      goto outer;
    case TokenTag_ParenClose:
      goto outer;
    case TokenTag_Binop: {
      if (op.binop.infos.binding_power_left < minimum_binding_power) {
        goto outer;
      }
      tokenizer_next(&p->tokenizer);
      ParserResult res = parser_parse_(p, op.binop.infos.binding_power_right);
      if (res.is_error) {
        return res;
      }
      ASTNode *right = res.node;
      if (right == NULL) {
        return (ParserResult){
            .is_error = true,
            .error =
                {
                    .tag = ParserError_UnexpectedEOF,
                    .loc = p->tokenizer.loc,
                },
        };
      }
      ASTNode *new_node = memory_pool_alloc(p->pool, sizeof(ASTNode), alignof(ASTNode));
      *new_node = (ASTNode){
          .tag = ASTNodeTag_Binop,
          .binop = {op.binop.infos, node, right},
      };
      node = new_node;
      break;
    }
    default:
      return (ParserResult){
          .is_error = true,
          .error = parser_expect(p, 2, (TokenTag[]){TokenTag_Binop, TokenTag_ParenClose}),
      };
    }
  }
outer:

  return (ParserResult){.is_error = false, .node = node};
}
ParserResult parser_parse(Parser *p) { return parser_parse_(p, 0); }

Parser parser_create(const char *input, memory_pool *pool) {
  Parser p = {
      .tokenizer = tokenizer_create(input),
      .pool = pool,
  };
  return p;
}
struct writer {
  memory_pool *pool;
  char *buf;
  char *buf_end;
  size_t capacity;
};
void writer_reserve(struct writer *w, size_t n) {
  if (w->capacity < n) {
    memory_pool_alloc(w->pool, n - w->capacity, alignof(char));
    w->capacity = n;
  }
}
void writer_write(struct writer *w, const char *s, size_t n) {
  writer_reserve(w, n);
  memcpy(w->buf_end, s, n);
  w->buf_end += n;
  w->capacity -= n;
}

[[gnu::format(printf, 2, 3)]]
void writer_writef(struct writer *w, const char *fmt, ...) {
  va_list ap, ap2;
  va_start(ap, fmt);
  va_copy(ap2, ap);
  int n = vsnprintf(nullptr, 0, fmt, ap);
  va_end(ap);
  if (n < 0) {
    fatal("vsnprintf failed\n");
  }

  writer_reserve(w, n + 1);
  n = vsnprintf(w->buf_end, n + 1, fmt, ap2);
  va_end(ap2);

  w->buf_end += n;
  w->capacity -= n;
}

const char *writer_finish(struct writer *w) {
  writer_write(w, "\0", 1);
  return w->buf;
}
void dump_sexpr_(ASTNode *node, struct writer *w) {
  switch (node->tag) {
  case ASTNodeTag_Integer:
    writer_writef(w, "%d", node->integer.value);
    break;
  case ASTNodeTag_Binop:
    writer_writef(w, "(%s ", node->binop.infos.name);
    dump_sexpr_(node->binop.left, w);
    writer_write(w, " ", 1);
    dump_sexpr_(node->binop.right, w);
    writer_write(w, ")", 1);
    break;
  default:
    fatal("invalid AST node\n");
  }
}

const char *dump_sexpr(ASTNode *node, memory_pool *pool) {
  struct writer w = {pool, nullptr, nullptr};
  w.buf = memory_pool_alloc(pool, 0, alignof(char));
  w.buf_end = w.buf;

  dump_sexpr_(node, &w);
  return writer_finish(&w);
}

int eval(ASTNode *node) {
  switch (node->tag) {
  case ASTNodeTag_Integer:
    return node->integer.value;
  case ASTNodeTag_Binop: {
    int left = eval(node->binop.left);
    int right = eval(node->binop.right);
    switch (node->binop.infos.op) {
    case '+':
      return left + right;
    case '-':
      return left - right;
    case '*':
      return left * right;
    case '/':
      return left / right;
    case '^': {
      // fast pow
      int result = 1;
      while (right > 0) {
        if (right & 1) {
          result *= left;
        }
        left *= left;
        right >>= 1;
      }
      return result;
    }
    default:
      fatal("invalid operator: '%c'\n", node->binop.infos.op);
    }
  }
  default:
    fatal("invalid AST node\n");
  }
}

void print_token(Token tok) {
  switch (tok.tag) {
  case TokenTag_EndOfStream:
    printf("EndOfStream");
    break;
  case TokenTag_Integer:
    printf("Integer(%d)", tok.integer.value);
    break;
  case TokenTag_Binop:
    printf("Binop('%c')", tok.binop.infos.op);
    break;
  case TokenTag_ParenOpen:
    printf("ParenOpen");
    break;
  case TokenTag_ParenClose:
    printf("ParenClose");
    break;
  default:
    printf("`Unknown token`");
    break;
  }
}

void print_error(ParserError err) {
  printf("Error at line %d, column %d: ", err.loc.line, err.loc.column);
  switch (err.tag) {
  case ParserError_UnexpectedToken:
    printf("Unexpected token: ");
    print_token(err.unexpected_token.got);
    printf("; expected one of: ");
    for (size_t i = 0; i < err.unexpected_token.expected_size; i++) {
      print_token((Token){err.unexpected_token.expected_tags[i]});
      if (i + 1 < err.unexpected_token.expected_size)
        printf(", ");
    }
    printf("\n");
    break;
  case ParserError_UnexpectedEOF:
    printf("Unexpected end of file\n");
    break;
  case ParserError_TokenError:
    printf("Token error: %s\n", err.token_error.message);
    break;
  case ParserError_Other:
    printf("Other parser error\n");
    break;
  default:
    printf("Unknown parser error\n");
    break;
  }
}
void test_parser() {
  struct test_case {
    const char *input;
    const char *expected;
  } test_cases[] = {
      {"1 + 2 * 3", "(add 1 (mul 2 3))"},
      {"(1 + 2) * 3", "(mul (add 1 2) 3)"},
      {"1 + 2 + 3", "(add (add 1 2) 3)"},
  };
  memory_pool pool = memory_pool_create(1024);
  for (size_t i = 0; i < ARRAY_SIZE(test_cases); i++) {
    const char *input = test_cases[i].input;
    const char *expected = test_cases[i].expected;

    memory_pool_reset(&pool);
    Parser parser = parser_create(input, &pool);
    ParserResult res = parser_parse(&parser);
    if (res.is_error) {
      print_error(res.error);
      fatal("parser failed: %s\n", input);
    }
    ASTNode *node = res.node;
    if (node == NULL) {
      fatal("parser failed: %s\n", input);
    }

    const char *actual = dump_sexpr(node, &pool);
    if (strcmp(actual, expected) != 0) {
      fatal("test case %zu failed: expected: %s, actual: %s\n", i, expected, actual);
    }
    printf("test case %zu passed: %s => %s\n", i, input, actual);
  }
}

int main() {
  test_parser();
  const char *line;
  memory_pool pool = memory_pool_create(1024 * 1024);

  if (isatty(0)) {
    printf("Welcome to the calculator REPL!\n");
    printf("Type an expression and press Enter to evaluate it.\n");
    printf("Press Ctrl+D (EOF) or enter an empty line to exit.\n");
    while ((line = readline(">> ")) != NULL) {
      if (line[0] == '\0') {
        goto cleanup;
      }

      Parser parser = parser_create(line, &pool);
      ParserResult res = parser_parse(&parser);
      if (res.is_error) {
        print_error(res.error);
      } else if (res.node != NULL) {
        const char *sexpr = dump_sexpr(res.node, &pool);
        printf("%s", sexpr);
        int result = eval(res.node);
        printf(" = %d\n", result);
      }

    cleanup:
      memory_pool_reset(&pool);
      free((void *)line);
    }
  } else {
    const char *input = nullptr;
    size_t len = 0;

    char buf[1024];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf) - 1, stdin))) {
      buf[n] = '\0';
      size_t old_len = len;
      len += n;
      input = (const char *)realloc((void *)input, len + 1);
      if (input == NULL) {
        fatal("realloc failed\n");
      }
      memcpy((char *)input + old_len, buf, n + 1);
    }
    printf("Input: %s\n", input);
    Tokenizer tokenizer = tokenizer_create(input);
    while (true) {
      TokenResult res = tokenizer_peek(&tokenizer);
      if (res.is_error) {
        print_error((ParserError){.tag = ParserError_TokenError, .token_error = res.error, .loc = res.error.loc});
        break;
      }
      Token tok = res.token;

      print_token(tok);
      printf("\n");
      tokenizer_next(&tokenizer);
      if (tok.tag == TokenTag_EndOfStream) {
        break;
      }
    }

    Parser parser = parser_create(input, &pool);
    ParserResult res = parser_parse(&parser);
    if (res.is_error) {
      print_error(res.error);
    } else if (res.node != NULL) {
      const char *sexpr = dump_sexpr(res.node, &pool);
      printf("%s", sexpr);
      int result = eval(res.node);
      printf(" = %d\n", result);
    }
  }

  memory_pool_destroy(&pool);
  return 0;
}
