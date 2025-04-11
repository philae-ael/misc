/// See https://en.wikipedia.org/wiki/Disjoint-set_data_structure
/// This is O(n) in space
/// more or less O(1) for each operation, in average
#[derive(Default)]
pub struct DisjointSet {
    parents: Vec<usize>,
    ranks: Vec<u8>,
}

impl DisjointSet {
    fn parent_node(&self, node_idx: usize) -> Option<usize> {
        let parent_idx = self.parents[node_idx];
        if parent_idx == node_idx {
            return None;
        }
        Some(parent_idx)
    }

    pub fn insert_set(&mut self) -> usize {
        let node_idx = self.parents.len();
        self.parents.push(node_idx);
        self.ranks.push(0);
        node_idx
    }

    // The find operation takes self by mut ref
    // As it improve the tree structure during it's traversal
    pub fn find(&mut self, node_idx: usize) -> usize {
        let mut node = node_idx;
        while let Some(parent_idx) = self.parent_node(node) {
            self.parents[node] = self.parents[parent_idx];
            node = parent_idx
        }
        node
    }

    pub fn merge(&mut self, set_a_idx: usize, set_b_idx: usize) {
        let mut a_idx = self.find(set_a_idx);
        let mut b_idx = self.find(set_b_idx);

        if a_idx == b_idx {
            return;
        }

        if self.ranks[a_idx] < self.ranks[b_idx] {
            std::mem::swap(&mut a_idx, &mut b_idx);
        }

        self.parents[b_idx] = a_idx;
        if self.ranks[a_idx] == self.ranks[b_idx] {
            self.ranks[b_idx] += 1
        }
    }
}

#[cfg(test)]
mod test {
    #[test]
    fn test1() {
        let mut disjoint_set = super::DisjointSet::default();

        let a1 = disjoint_set.insert_set();
        let b2 = disjoint_set.insert_set();
        let c3 = disjoint_set.insert_set();

        disjoint_set.merge(a1, b2);

        assert_eq!(disjoint_set.find(a1), disjoint_set.find(b2));
        assert_ne!(disjoint_set.find(a1), disjoint_set.find(c3));
        assert_ne!(disjoint_set.find(b2), disjoint_set.find(c3));
    }
}
