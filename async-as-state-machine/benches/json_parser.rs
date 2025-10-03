use criterion::{Criterion, black_box, criterion_group, criterion_main};

fn benchmark_parser(c: &mut Criterion) {
    c.bench_function("parse json", |b| {
        b.iter(|| {
            // Your parsing code here
            // Wrap inputs in black_box() to prevent optimization
            black_box(parse_json_string(black_box(JSON_INPUT)));
        });
    });
}

criterion_group!(benches, benchmark_parser);
criterion_main!(benches);
