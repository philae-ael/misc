use std::collections::VecDeque;

#[derive(Default, Debug)]
pub struct AdjacencyGraph(Vec<Vec<Vertex>>);

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(transparent)]
pub struct Vertex(usize);

impl AdjacencyGraph {
    pub fn add_vertex(&mut self) -> Vertex {
        self.0.push(vec![]);
        Vertex(self.0.len() - 1)
    }

    pub fn add_edge(&mut self, a: Vertex, b: Vertex) {
        if !self.0[a.0].contains(&b) {
            self.0[a.0].push(b)
        }
    }

    pub fn len(&self) -> usize {
        self.0.len()
    }

    pub fn reverse(&self) -> Self {
        let mut new = vec![vec![]; self.len()];
        for (from, tos) in self.0.iter().enumerate() {
            for to in tos {
                new[to.0].push(Vertex(from));
            }
        }

        Self(new)
    }

    // Kahn's algorithm
    // Requires an acyclic graph
    pub fn toposort(&self) -> std::vec::Vec<Vertex> {
        let mut output = vec![];
        let mut l = VecDeque::new();

        let mut indegress = vec![0; self.len()];
        for vertices in self.0.iter().flatten() {
            indegress[vertices.0] += 1;
        }

        l.extend(
            indegress
                .iter()
                .cloned()
                .enumerate()
                .filter_map(|(idx, degree)| (degree == 0).then_some(idx)),
        );

        while let Some(v) = l.pop_front() {
            output.push(Vertex(v));

            for n in &self.0[v] {
                indegress[n.0] -= 1;
                if indegress[n.0] == 0 {
                    l.push_back(n.0);
                }
            }
        }

        assert_eq!(output.len(), self.len(), "a cycle has been detected");
        output
    }
}

impl From<crate::rand_graph::DAGGenerator> for AdjacencyGraph {
    fn from(g: crate::rand_graph::DAGGenerator) -> Self {
        let mut g2 = AdjacencyGraph::default();
        let v = (0..g.vertex_count())
            .map(|_| g2.add_vertex())
            .collect::<Vec<_>>();

        for edge in g.iter_edges() {
            g2.add_edge(v[edge.0], v[edge.1]);
        }
        g2
    }
}

#[cfg(test)]
mod test {
    use rand::{Rng, SeedableRng};

    use crate::toposort::Vertex;

    use super::AdjacencyGraph;

    #[test]
    fn g() {
        let mut g = AdjacencyGraph::default();
        let a = g.add_vertex();
        let b = g.add_vertex();
        let c = g.add_vertex();
        let d = g.add_vertex();
        let e = g.add_vertex();
        let f = g.add_vertex();

        g.add_edge(a, b);
        g.add_edge(b, c);
        g.add_edge(c, d);
        g.add_edge(d, e);
        g.add_edge(e, f);

        g.toposort();
    }

    #[test]
    fn g2() {
        let mut rng = rand::rngs::SmallRng::seed_from_u64(3565);
        for i in 0..500 {
            println!("{}", i);
            let a = rng.random_range(1..1000);
            let g: AdjacencyGraph = crate::rand_graph::generate_random_simply_connected_dag(
                &mut rng,
                a,
                None,
                crate::rand_graph::Ratio {
                    numerator: 2,
                    denominator: 3,
                },
            )
            .into();

            let t = g.toposort();

            let g2 = g.reverse();

            let mut visited = bitvec::bitbox![0; g.len()];
            for t in t {
                assert!(g2.0[t.0].iter().all(|Vertex(v)| visited[*v]));
                visited.set(t.0, true);
            }
        }
    }
}
