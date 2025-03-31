use bitflags::bitflags;
use rand::distr::Distribution;

bitflags! {
#[derive(Debug)]
    struct EdgeProperty : u8{
        const PRESENT   = 0x01;
    }
}

#[derive(Debug)]
pub struct DAGGenerator {
    // Stored in Column Major
    // outgoing edges properties of vertex i are edges[i]
    // ingoing edges properties of vertex j are edges[_][j]
    edges: Vec<Vec<EdgeProperty>>,

    edge_count: usize,
}

impl DAGGenerator {
    pub fn new_empty(n: usize) -> Self {
        Self {
            // We are buiding a of diagonal matrix with EdgeProperty::CONNECTED on the diagonal
            edges: (0..n)
                .map(|_i| (0..n).map(|_j| EdgeProperty::empty()).collect::<Vec<_>>())
                .collect(),
            edge_count: 0,
        }
    }

    pub fn vertex_count(&self) -> usize {
        self.edges.len()
    }

    pub fn has_edge(&self, (i, j): (usize, usize)) -> bool {
        self.edges[i][j].contains(EdgeProperty::PRESENT)
    }

    pub fn remove_edge(&mut self, (i, j): (usize, usize)) {
        if self.has_edge((i, j)) {
            self.edges[i][j].remove(EdgeProperty::PRESENT);
            self.edge_count -= 1;
        }
    }

    pub fn connected(&self, i: usize, j: usize) -> bool {
        if i == j {
            return true;
        }

        let mut x = vec![i];
        let mut visited = vec![];

        while let Some(v) = x.pop() {
            visited.push(v);

            for y in self.edges[v]
                .iter()
                .enumerate()
                .filter_map(|(a, p)| p.contains(EdgeProperty::PRESENT).then_some(a))
            {
                if y == j {
                    return true;
                }

                if !visited.contains(&y) {
                    x.push(y);
                }
            }
        }

        false
    }

    pub fn try_insert_edge(&mut self, (i, j): (usize, usize)) -> Option<()> {
        if self.connected(j, i) {
            return None;
        }

        self.edge_count += 1;
        self.edges[i][j].insert(EdgeProperty::PRESENT);
        Some(())
    }
    pub fn iter_edges(&self) -> impl Iterator<Item = (usize, usize)> {
        self.edges.iter().enumerate().flat_map(|(i, p)| {
            p.iter().enumerate().filter_map(move |(j, property)| {
                property.contains(EdgeProperty::PRESENT).then_some((i, j))
            })
        })
    }

    pub fn to_dot(&self, w: &mut impl std::io::Write) -> std::io::Result<()> {
        writeln!(w, "digraph {{")?;
        //writeln!(w, "\trankdir=\"LR\"")?;
        for (i, j) in self.iter_edges() {
            writeln!(w, "\t{i} -> {j}")?;
        }
        write!(w, "}}")
    }

    pub fn edge_count(&self) -> usize {
        self.edge_count
    }
}

// Melançon, G. and Philippe, F. (2004)
// ‘Generating connected acyclic digraphs uniformly at random’,
// Information Processing Letters, 90(4), pp. 209–213.
// Available at: https://doi.org/10.1016/j.ipl.2003.06.002

pub fn generate_random_dag(
    rng: &mut impl rand::Rng,
    n: usize,
    iter_count: Option<usize>,
) -> DAGGenerator {
    let mut graph = DAGGenerator::new_empty(n);

    let index_selector =
        rand::distr::Uniform::<usize>::new(0, n).expect("can't get a distribution?");

    // Note: This does not give a *simply connected graph*
    for _ in 0..iter_count.unwrap_or(5 * n) {
        let i = index_selector.sample(rng);
        let j = index_selector.sample(rng);
        let edge = (i, j);

        if graph.has_edge(edge) {
            graph.remove_edge(edge);
        } else {
            graph.try_insert_edge(edge);
        }
    }

    graph
}

#[derive(Debug, Clone, Copy)]
pub struct Ratio {
    pub numerator: usize,
    pub denominator: usize,
}

impl Ratio {}

impl std::ops::Mul<usize> for Ratio {
    type Output = Ratio;

    fn mul(self, rhs: usize) -> Self::Output {
        Self {
            numerator: rhs * self.denominator,
            denominator: self.denominator,
        }
    }
}

impl std::cmp::PartialEq<usize> for Ratio {
    fn eq(&self, other: &usize) -> bool {
        self.numerator == other * self.denominator
    }
}
impl std::cmp::PartialOrd<usize> for Ratio {
    fn partial_cmp(&self, other: &usize) -> Option<std::cmp::Ordering> {
        Some(self.numerator.cmp(&(other * self.denominator)))
    }
}

pub fn generate_random_simply_connected_dag(
    rng: &mut impl rand::Rng,
    n: usize,
    iter_count: Option<usize>,
    ratio: Ratio,
) -> DAGGenerator {
    let mut graph = DAGGenerator::new_empty(n);
    for i in 1..n {
        graph.try_insert_edge((i - 1, i));
    }

    let index_selector =
        rand::distr::Uniform::<usize>::new(0, n).expect("can't get a distribution?");

    // Note: This does not give a *simply connected graph*
    for _ in 0..iter_count.unwrap_or(2 * n) {
        let i = index_selector.sample(rng);
        let j = index_selector.sample(rng);
        let edge = (i, j);

        if graph.has_edge(edge) {
            graph.remove_edge(edge);
        } else if ratio * graph.vertex_count() > graph.edge_count() + 1 {
            graph.try_insert_edge(edge);
        } else {
            // prevent too dense things
        }
    }

    graph
}

#[cfg(test)]
mod test {
    use super::generate_random_dag;
    use rand::SeedableRng;

    #[test]
    fn test_generate_random_graph() {
        let mut rng = rand::prelude::SmallRng::seed_from_u64(54542475);
        generate_random_dag(&mut rng, 5, None);
    }
}
