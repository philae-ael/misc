use rand::rng;
use toposort::AdjacencyGraph;

pub mod rand_graph;
mod toposort;

fn main() {
    let g = rand_graph::generate_random_simply_connected_dag(
        &mut rng(),
        5,
        Some(10),
        rand_graph::Ratio {
            numerator: 3,
            denominator: 2,
        },
    );
    g.to_dot(&mut std::io::stdout()).unwrap();

    eprint!("{:?}", AdjacencyGraph::from(g).toposort());
}
