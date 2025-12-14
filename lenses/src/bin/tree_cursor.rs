// no reason why this is in lenses

pub struct Node<NodeData> {
    data: NodeData,
    children: std::collections::HashMap<String, slotmap::DefaultKey>,
}

pub struct Tree<NodeData> {
    nodes: slotmap::SlotMap<slotmap::DefaultKey, Node<NodeData>>,
    root: slotmap::DefaultKey,
}
impl<NodeData> Tree<NodeData> {
    pub fn new(root: NodeData) -> Self {
        let mut nodes = slotmap::SlotMap::new();
        let root_key = nodes.insert(Node {
            data: root,
            children: std::collections::HashMap::new(),
        });
        Self {
            nodes,
            root: root_key,
        }
    }
    pub fn cursor_mut(&mut self) -> CursorMut<'_, NodeData, NatZero> {
        CursorMut {
            current: self.root,
            tree: self,
            hiearchy: vec![],
            _marker: std::marker::PhantomData,
        }
    }
}

pub struct NatZero;
pub struct NatUnknown;
pub struct NatSucc<T>(std::marker::PhantomData<T>);

pub struct CursorMut<'a, Node, Depth = NatUnknown> {
    tree: &'a mut Tree<Node>,
    current: slotmap::DefaultKey,
    hiearchy: Vec<slotmap::DefaultKey>,
    _marker: std::marker::PhantomData<Depth>,
}

impl<'a, NodeData, Depth> CursorMut<'a, NodeData, Depth> {
    pub fn data(&self) -> Option<&NodeData> {
        self.tree.nodes.get(self.current).map(|node| &node.data)
    }
    pub fn data_mut(&mut self) -> Option<&mut NodeData> {
        self.tree
            .nodes
            .get_mut(self.current)
            .map(|node| &mut node.data)
    }
    fn get_node(&self) -> Option<&Node<NodeData>> {
        self.tree.nodes.get(self.current)
    }
    pub fn iter_children_mut(&mut self) -> ChildIteratorMut {
        let child_keys = if let Some(node) = self.get_node() {
            node.children
                .iter()
                .map(|(k, &v)| (k.clone(), v))
                .collect::<Vec<_>>()
        } else {
            Vec::new()
        };
        ChildIteratorMut {
            child_keys: child_keys.into_iter(),
        }
    }
    pub fn add_child(
        mut self,
        label: impl Into<String>,
        data: NodeData,
    ) -> CursorMut<'a, NodeData, NatSucc<Depth>> {
        let child_node = Node {
            data,
            children: std::collections::HashMap::new(),
        };
        let child_key = self.tree.nodes.insert(child_node);
        if let Some(current_node) = self.tree.nodes.get_mut(self.current) {
            current_node.children.insert(label.into(), child_key);
        }
        self.hiearchy.push(self.current);
        CursorMut {
            current: child_key,
            tree: self.tree,
            hiearchy: self.hiearchy,
            _marker: std::marker::PhantomData,
        }
    }

    pub fn child_by_label(
        mut self,
        label: &str,
    ) -> Option<CursorMut<'a, NodeData, NatSucc<Depth>>> {
        if let Some(node) = self.get_node()
            && let Some(&child_key) = node.children.get(label)
        {
            self.hiearchy.push(self.current);
            return Some(CursorMut {
                current: child_key,
                tree: self.tree,
                hiearchy: self.hiearchy,
                _marker: std::marker::PhantomData,
            });
        }
        None
    }
    pub fn try_parent(mut self) -> Option<CursorMut<'a, NodeData>> {
        if let Some(parent_key) = self.hiearchy.pop() {
            return Some(CursorMut {
                current: parent_key,
                tree: self.tree,
                hiearchy: self.hiearchy,
                _marker: std::marker::PhantomData,
            });
        }
        None
    }
}

impl<'a, NodeData, ParentDepth> CursorMut<'a, NodeData, NatSucc<ParentDepth>> {
    pub fn parent(mut self) -> CursorMut<'a, NodeData, ParentDepth> {
        // The unwrap is safe here because of the type-level depth tracking
        let parent_key = self.hiearchy.pop().unwrap();
        CursorMut {
            current: parent_key,
            tree: self.tree,
            hiearchy: self.hiearchy,
            _marker: std::marker::PhantomData,
        }
    }
}

pub struct ChildIteratorMut {
    child_keys: std::vec::IntoIter<(String, slotmap::DefaultKey)>,
}
impl ChildIteratorMut {
    pub fn next<'tree, NodeData>(
        &mut self,
        tree: &'tree mut Tree<NodeData>,
    ) -> Option<(String, CursorMut<'tree, NodeData>)> {
        if let Some((l, r)) = self.child_keys.next() {
            Some((
                l,
                CursorMut {
                    current: r,
                    tree,
                    // Note that this prevent to go back up the tree from this cursor
                    hiearchy: vec![],
                    _marker: std::marker::PhantomData,
                },
            ))
        } else {
            None
        }
    }
}

// The real focus: a bidirectional lens trait

pub trait Lens<NodeData> {
    type Projected;

    fn get(&self) -> Self::Projected;
    fn pushback(&mut self, value: Self::Projected);
}

// Rather than clone, i would prefer references, or views
impl<T: Clone, Depth> Lens<T> for CursorMut<'_, T, Depth> {
    type Projected = T;

    fn get(&self) -> Self::Projected {
        // Note the unwrap: idk how to handle missing data here
        self.data().cloned().unwrap()
    }

    fn pushback(&mut self, value: Self::Projected) {
        if let Some(data) = self.data_mut() {
            *data = value;
        }
    }
}

pub fn main() {
    let mut tree = Tree::new("root");

    // I don't really like this syntax as it's verrry error-prone + readability is not great when
    // not aligned properly
    // an easier syntax would be a bit more imperative, but would avoid these issues
    #[rustfmt::skip]
    tree.cursor_mut()
        .add_child("child1", "child1_data")
            .add_child("grandchild1", "grandchild1_data").parent()
            .add_child("grandchild2", "grandchild2_data") .parent()
            .parent()
        .add_child("child2", "child2_data")
            .add_child("grandchild3", "grandchild3_data").parent()
            .parent();

    // Traverse and print the tree
    fn print_tree_inner<NodeData: std::fmt::Display, D>(
        cursor: &mut CursorMut<'_, NodeData, D>,
        depth: usize,
    ) {
        if let Some(data) = cursor.data() {
            println!("{}{}", "  ".repeat(2 * depth), data);

            let mut child_iterator = cursor.iter_children_mut();
            while let Some((label, mut child_cursor)) = child_iterator.next(cursor.tree) {
                println!("{}- {}", "  ".repeat(2 * depth + 1), label);
                print_tree_inner(&mut child_cursor, depth + 1);
            }
        }
    }
    fn print_tree<NodeData: std::fmt::Display, D>(cursor: &mut CursorMut<'_, NodeData, D>) {
        print_tree_inner(cursor, 0);
    }

    println!("Initial tree:");
    print_tree(&mut tree.cursor_mut());

    println!();

    let cursor = tree.cursor_mut().child_by_label("child1").unwrap();
    let mut lens = cursor;
    let data = lens.get();
    println!("Data at child1: {}", data);
    println!();

    lens.pushback("updated_child1_data");
    println!("Pushed back updated data to child1.");
    println!();
    println!("After update:");
    print_tree_inner(&mut tree.cursor_mut(), 0);
}
