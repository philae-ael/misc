use std::{
    cell::Cell,
    hint::black_box,
    panic::{AssertUnwindSafe, catch_unwind, resume_unwind},
    pin::{Pin, pin},
    task::{RawWaker, RawWakerVTable, Waker},
};

fn main() {
    let input = include_str!("input.json");

    let now = std::time::Instant::now();
    for _ in 0..10 {
        black_box(parse_json_serde(input));
    }
    let elapsed_serde = now.elapsed();
    println!("serde_json took: {:.2?}", elapsed_serde);

    let now = std::time::Instant::now();
    for _ in 0..10 {
        black_box(parse_json(input));
    }
    let elapsed_custom = now.elapsed();
    println!("custom parser took: {:.2?}", elapsed_custom);
}
fn parse_json_serde(input: &str) -> serde_json::Value {
    serde_json::from_str(input).unwrap()
}
fn parse_json(input: &str) -> Json<'_> {
    let my_waker = ParseContext {
        src: input,
        pos: Cell::new(0),
        row: Cell::new(1),
        col: Cell::new(1),
    };

    let mut token_stream = Tokenizer { current: None };

    let mut parser = pin!(parse(&mut token_stream));
    let waker = {
        const VTABLE: RawWakerVTable =
            RawWakerVTable::new(|data| RawWaker::new(data, &VTABLE), |_| {}, |_| {}, |_| {});
        RawWaker::new(&raw const my_waker as *const (), &VTABLE)
    };
    let waker = unsafe { Waker::from_raw(waker) };

    let mut ctx = std::task::Context::from_waker(&waker);

    let r = catch_unwind(AssertUnwindSafe(|| {
        loop {
            match std::future::Future::poll(parser.as_mut(), &mut ctx) {
                std::task::Poll::Ready(r) => break r,
                std::task::Poll::Pending => continue,
            }
        }
    }));
    match r {
        Err(err) => {
            let row = my_waker.row.get();
            let col = my_waker.col.get();
            eprintln!("Error at row {}, col {}", row, col);
            resume_unwind(err);
        }
        Ok(r) => r,
    }
}

// ============================================================================
// Parse context
// ============================================================================

struct ParseContext<'a> {
    src: &'a str,
    pos: Cell<usize>,
    row: Cell<usize>,
    col: Cell<usize>,
}

impl<'a> ParseContext<'a> {
    fn get_ctx(cx: &std::task::Context<'_>) -> &'a Self {
        let w = cx.waker().data();
        unsafe { &*(w as *const Self) }
    }

    fn peek(&self) -> Option<char> {
        self.src[self.pos.get()..].chars().next()
    }

    fn advance(&self, c: char) {
        self.pos.set(self.pos.get() + c.len_utf8());
        if c == '\n' {
            self.row.set(self.row.get() + 1);
            self.col.set(1);
        } else {
            self.col.set(self.col.get() + 1);
        }
    }

    fn slice(&self, start: usize, end: usize) -> &'a str {
        &self.src[start..end]
    }
}

// ============================================================================
// Character-level primitives
// ============================================================================

async fn peek() -> Option<char> {
    struct Peek;
    impl std::future::Future for Peek {
        type Output = Option<char>;
        fn poll(
            self: Pin<&mut Self>,
            cx: &mut std::task::Context<'_>,
        ) -> std::task::Poll<Self::Output> {
            std::task::Poll::Ready(ParseContext::get_ctx(cx).peek())
        }
    }
    Peek.await
}

async fn read() -> Option<char> {
    struct Read;
    impl std::future::Future for Read {
        type Output = Option<char>;
        fn poll(
            self: Pin<&mut Self>,
            cx: &mut std::task::Context<'_>,
        ) -> std::task::Poll<Self::Output> {
            let ctx = ParseContext::get_ctx(cx);
            let c = ctx.peek();
            if let Some(c) = c {
                ctx.advance(c);
            }
            std::task::Poll::Ready(c)
        }
    }
    Read.await
}

async fn char(expected: char) {
    let c = read().await;
    if c != Some(expected) {
        panic!("Expected '{}', got '{:?}'", expected, c);
    }
}
async fn string(expected: &str) {
    for expected_char in expected.chars() {
        char(expected_char).await;
    }
}

async fn skip_whitespace() {
    while let Some(c) = peek().await {
        if c.is_whitespace() {
            read().await;
        } else {
            break;
        }
    }
}

// Zero-copy slice capture
async fn capture_slice<F>(action: F) -> &'static str
where
    F: std::future::Future<Output = ()>,
{
    struct CaptureSlice<F> {
        action: F,
        started: bool,
        start: usize,
    }

    impl<F> std::future::Future for CaptureSlice<F>
    where
        F: std::future::Future<Output = ()>,
    {
        type Output = &'static str;
        fn poll(
            self: Pin<&mut Self>,
            cx: &mut std::task::Context<'_>,
        ) -> std::task::Poll<Self::Output> {
            let ctx = ParseContext::get_ctx(cx);
            let this = unsafe { self.get_unchecked_mut() };

            if !this.started {
                this.start = ctx.pos.get();
                this.started = true;
            }

            let action = unsafe { Pin::new_unchecked(&mut this.action) };
            match action.poll(cx) {
                std::task::Poll::Ready(()) => {
                    let slice = ctx.slice(this.start, ctx.pos.get());
                    std::task::Poll::Ready(slice)
                }
                std::task::Poll::Pending => std::task::Poll::Pending,
            }
        }
    }

    CaptureSlice {
        action,
        started: false,
        start: 0,
    }
    .await
}

// ============================================================================
// Tokenizer
// ============================================================================

#[derive(Clone, Debug, PartialEq)]
enum Token<'a> {
    BraceOpen,
    BraceClose,
    BracketOpen,
    BracketClose,
    Colon,
    Comma,
    String(&'a str),
    Number(f64),
    Bool(bool),
    Null,
    Eof,
}

async fn next_token() -> Token<'static> {
    skip_whitespace().await;

    let c = match peek().await {
        Some(c) => c,
        None => return Token::Eof,
    };

    match c {
        '{' => {
            read().await;
            Token::BraceOpen
        }
        '}' => {
            read().await;
            Token::BraceClose
        }
        '[' => {
            read().await;
            Token::BracketOpen
        }
        ']' => {
            read().await;
            Token::BracketClose
        }
        ':' => {
            read().await;
            Token::Colon
        }
        ',' => {
            read().await;
            Token::Comma
        }
        '"' => {
            read().await; // consume opening quote
            let s = capture_slice(async {
                let mut escaped = false;
                loop {
                    let c = read().await.expect("Unterminated string");
                    match c {
                        '"' if !escaped => break,
                        '\\' if !escaped => escaped = true,
                        _ => escaped = false,
                    }
                }
            })
            .await;
            // s includes the closing quote, so slice it off
            let s = &s[..s.len() - 1];
            Token::String(s)
        }
        '0'..='9' | '-' => {
            let s = capture_slice(async {
                read().await; // consume first char
                while let Some(c) = peek().await {
                    if c.is_ascii_digit()
                        || c == '.'
                        || c == 'e'
                        || c == 'E'
                        || c == '+'
                        || c == '-'
                    {
                        read().await;
                    } else {
                        break;
                    }
                }
            })
            .await;
            let n: f64 = s.parse().expect("Invalid number");
            Token::Number(n)
        }
        't' => {
            string("true").await;
            Token::Bool(true)
        }
        'f' => {
            string("false").await;
            Token::Bool(false)
        }
        'n' => {
            string("null").await;
            Token::Null
        }
        _ => {
            panic!("Unexpected character: {}", c);
        }
    }
}

struct Tokenizer<'a> {
    current: Option<Token<'a>>,
}

impl<'a> Tokenizer<'a> {
    async fn peek(&mut self) -> Token<'a> {
        if let Some(ref t) = self.current {
            t.clone()
        } else {
            let tok = next_token().await;
            self.current = Some(tok.clone());
            tok
        }
    }

    fn consume(&mut self) {
        self.current.take();
    }

    async fn expect(&mut self, expected: &[Token<'a>]) {
        let tok = self.peek().await;
        if !expected.contains(&tok) {
            panic!("Expected {:?}, got {:?}", expected, tok);
        }
        self.consume();
    }

    async fn expect_string(&mut self) -> &'a str {
        let tok = self.peek().await;
        if let Token::String(s) = tok {
            self.consume();
            s
        } else {
            panic!("Expected string, got {:?}", tok);
        }
    }
}

// ============================================================================
// Parser
// ============================================================================

#[derive(Debug)]
#[allow(dead_code)]
enum Json<'a> {
    Object(Vec<(&'a str, Json<'a>)>),
    Array(Vec<Json<'a>>),
    String(&'a str),
    Number(f64),
    Bool(bool),
    Null,
}

async fn parse<'a>(tokens: &mut Tokenizer<'a>) -> Json<'a> {
    match tokens.peek().await {
        Token::BraceOpen => {
            tokens.consume();
            let mut members = Vec::new();

            if tokens.peek().await != Token::BraceClose {
                loop {
                    let key = tokens.expect_string().await;
                    tokens.expect(&[Token::Colon]).await;
                    let value = Box::pin(parse(tokens)).await;
                    members.push((key, value));

                    match tokens.peek().await {
                        Token::Comma => {
                            tokens.consume();
                            continue;
                        }
                        Token::BraceClose => break,
                        t => panic!("Expected ',' or '}}', got {:?}", t),
                    }
                }
            }

            tokens.expect(&[Token::BraceClose]).await;
            Json::Object(members)
        }
        Token::BracketOpen => {
            tokens.consume();
            let mut elements = Vec::new();

            if tokens.peek().await != Token::BracketClose {
                loop {
                    elements.push(Box::pin(parse(tokens)).await);

                    match tokens.peek().await {
                        Token::Comma => {
                            tokens.consume();
                            continue;
                        }
                        Token::BracketClose => break,
                        t => panic!("Expected ',' or ']', got {:?}", t),
                    }
                }
            }

            tokens.expect(&[Token::BracketClose]).await;
            Json::Array(elements)
        }
        Token::String(s) => {
            tokens.consume();
            Json::String(s)
        }
        Token::Number(n) => {
            tokens.consume();
            Json::Number(n)
        }
        Token::Bool(b) => {
            tokens.consume();
            Json::Bool(b)
        }
        Token::Null => {
            tokens.consume();
            Json::Null
        }
        t => panic!("Unexpected token: {:?}", t),
    }
}
