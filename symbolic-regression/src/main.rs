use std::collections::HashMap;

macro_rules! hash_map {
    ($($key:ident: $value:expr),* $(,)?) => {{
        std::collections::HashMap::from([
            $((stringify!($key), $value)),*
        ])
    }}
}

#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
struct ExpressionHandle {
    id: usize,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
struct Variable {
    id: usize,
}

#[derive(Debug, Clone, Copy)]
enum Binop {
    Add,
    Sub,
    Mul,
    Div,
}

#[derive(Debug, Clone, Copy)]
enum ExpressionNode {
    Constant(f64),
    Variable(Variable),
    Binop(Binop, ExpressionHandle, ExpressionHandle),
}

#[derive(Debug, Clone, Copy)]
enum MaybeOrNext<T> {
    Value(T),
    Next(ExpressionHandle),
}

#[derive(Debug)]
struct ExpressionTree {
    nodes: Vec<MaybeOrNext<ExpressionNode>>,
    next_free: ExpressionHandle,
    root: ExpressionHandle,
}

impl ExpressionTree {
    fn new() -> Self {
        ExpressionTree {
            nodes: Vec::new(),
            next_free: ExpressionHandle { id: 0 },
            root: ExpressionHandle { id: 0 },
        }
    }

    fn add_node(&mut self, node: ExpressionNode) -> ExpressionHandle {
        let handle = self.next_free;
        let value = MaybeOrNext::Value(node);

        if handle.id == self.nodes.len() {
            self.nodes.push(value);
            self.next_free = ExpressionHandle {
                id: self.nodes.len(),
            };
        } else {
            match self.nodes[handle.id] {
                MaybeOrNext::Value(_) => {
                    panic!("invariant violated: next_free points to a used slot")
                }
                MaybeOrNext::Next(next) => {
                    self.next_free = next;
                    self.nodes[handle.id] = value;
                }
            }
        }
        handle
    }
    fn dump_node(&self, variable_map: &VariableMap, handle: ExpressionHandle) -> String {
        match self.nodes[handle.id] {
            MaybeOrNext::Value(ref node) => match node {
                ExpressionNode::Constant(c) => c.to_string(),
                ExpressionNode::Variable(v) => variable_map.map[v.id].clone(),
                ExpressionNode::Binop(op, lhs, rhs) => {
                    format!(
                        "({} {} {})",
                        self.dump_node(variable_map, *lhs),
                        match op {
                            Binop::Add => "+",
                            Binop::Sub => "-",
                            Binop::Mul => "*",
                            Binop::Div => "/",
                        },
                        self.dump_node(variable_map, *rhs)
                    )
                }
            },
            MaybeOrNext::Next(_) => "Free Slot".to_string(),
        }
    }
    fn dump(&self, variable_map: &VariableMap) -> String {
        self.dump_node(variable_map, self.root)
    }

    fn evaluate_node(&self, handle: ExpressionHandle, vars: &[f64]) -> f64 {
        match self.nodes[handle.id] {
            MaybeOrNext::Value(ref node) => match node {
                ExpressionNode::Constant(c) => *c,
                ExpressionNode::Variable(v) => vars[v.id],
                ExpressionNode::Binop(op, lhs, rhs) => {
                    let lhs = self.evaluate_node(*lhs, vars);
                    let rhs = self.evaluate_node(*rhs, vars);
                    match op {
                        Binop::Add => lhs + rhs,
                        Binop::Sub => lhs - rhs,
                        Binop::Mul => lhs * rhs,
                        Binop::Div => lhs / rhs,
                    }
                }
            },
            MaybeOrNext::Next(_) => panic!("Free Slot encountered during evaluation"),
        }
    }
    fn evaluate(&self, vars: &[f64]) -> f64 {
        self.evaluate_node(self.root, vars)
    }
    pub fn gc(&mut self) {
        let mut used = vec![0; self.nodes.len()];
        for node in &mut self.nodes {
            if let MaybeOrNext::Value(node) = node {
                match node {
                    ExpressionNode::Constant(_) | ExpressionNode::Variable(_) => {}
                    ExpressionNode::Binop(_, lhs, rhs) => {
                        used[lhs.id] += 1;
                        used[rhs.id] += 1;
                    }
                }
            }
        }
        used[self.root.id] += 1;

        let mut found_any_unused = true;
        while found_any_unused {
            found_any_unused = false;
            for i in 0..self.nodes.len() {
                let u = used[i];
                if u != 0 {
                    continue;
                }

                if let MaybeOrNext::Value(node) = self.nodes[i] {
                    found_any_unused = true;
                    match node {
                        ExpressionNode::Constant(_) | ExpressionNode::Variable(_) => {}
                        ExpressionNode::Binop(_, lhs, rhs) => {
                            used[lhs.id] -= 1;
                            used[rhs.id] -= 1;
                        }
                    }
                    self.nodes[i] = MaybeOrNext::Next(self.next_free);
                    self.next_free = ExpressionHandle { id: i };
                }
            }
        }
    }

    pub fn is_a_tree(&self) -> bool {
        let mut parent_count = vec![0; self.nodes.len()];
        parent_count[self.root.id] = 1;
        for node in &self.nodes {
            if let MaybeOrNext::Value(node) = node {
                match node {
                    ExpressionNode::Constant(_) | ExpressionNode::Variable(_) => {}
                    ExpressionNode::Binop(_, lhs, rhs) => {
                        if parent_count[lhs.id] > 0 || parent_count[rhs.id] > 0 {
                            return false;
                        }
                        parent_count[lhs.id] += 1;
                        parent_count[rhs.id] += 1;
                    }
                }
            }
        }

        true
    }
    pub fn is_a_dag(&self) -> bool {
        let mut visited = vec![false; self.nodes.len()];
        fn visit(tree: &ExpressionTree, visited: &mut [bool], handle: ExpressionHandle) -> bool {
            if visited[handle.id] {
                return false;
            }
            visited[handle.id] = true;
            if let MaybeOrNext::Value(node) = tree.nodes[handle.id] {
                match node {
                    ExpressionNode::Constant(_) | ExpressionNode::Variable(_) => true,
                    ExpressionNode::Binop(_, lhs, rhs) => {
                        visit(tree, visited, lhs) && visit(tree, visited, rhs)
                    }
                }
            } else {
                true
            }
        }
        visit(self, &mut visited, self.root)
    }

    pub fn to_dot_(&self, handle: ExpressionHandle, f: &mut dyn std::io::Write) {
        if let MaybeOrNext::Value(node) = self.nodes[handle.id] {
            match node {
                ExpressionNode::Constant(c) => {
                    writeln!(f, "  {} [label=\"{}\"];", handle.id, c).unwrap();
                }
                ExpressionNode::Variable(v) => {
                    writeln!(f, "  {} [label=\"v{}\"];", handle.id, v.id).unwrap();
                }
                ExpressionNode::Binop(op, lhs, rhs) => {
                    let op_str = match op {
                        Binop::Add => "+",
                        Binop::Sub => "-",
                        Binop::Mul => "*",
                        Binop::Div => "/",
                    };
                    writeln!(f, "  {} [label=\"{}\"];", handle.id, op_str).unwrap();
                    writeln!(f, "  {} -> {};", handle.id, lhs.id).unwrap();
                    writeln!(f, "  {} -> {};", handle.id, rhs.id).unwrap();
                    self.to_dot_(lhs, f);
                    self.to_dot_(rhs, f);
                }
            }
        }
    }
    pub fn to_dot(&self, f: &mut dyn std::io::Write) {
        writeln!(f, "digraph G {{").unwrap();
        self.to_dot_(self.root, f);
        writeln!(f, "}}").unwrap();
    }
}

pub struct CursorPosition {
    node: ExpressionHandle,
}

pub struct Modification {
    pos: CursorPosition,

    // By convertion CursorInsert replace the children of the node at CursorPosition
    // with the root of the provided ExpressionTree
    // And replace the variable named HOLE in the provided ExpressionTree with the node at
    // CursorPosition
    expr: ExpressionTree,
}

impl ExpressionTree {
    pub fn apply(&mut self, hole_var_id: Variable, modification: Modification) {
        let Modification { pos, mut expr } = modification;
        let node_to_replace = pos.node;

        let mut handle_mapping: HashMap<ExpressionHandle, ExpressionHandle> = HashMap::new();

        struct TreeMerger<'a> {
            src: &'a mut ExpressionTree,
            handle_mapping: &'a mut HashMap<ExpressionHandle, ExpressionHandle>,
            hole_var_id: Variable,
            node_to_replace: ExpressionHandle,
            target: &'a mut ExpressionTree,
        }
        impl<'a> TreeMerger<'a> {
            fn merge_tree_inner(&mut self, node: ExpressionHandle) -> ExpressionHandle {
                if let Some(&new_handle) = self.handle_mapping.get(&node) {
                    return new_handle;
                }
                let new_handle = match self.src.nodes[node.id] {
                    MaybeOrNext::Value(n) => match n {
                        ExpressionNode::Constant(c) => {
                            self.target.add_node(ExpressionNode::Constant(c))
                        }
                        ExpressionNode::Variable(v) => {
                            if v == self.hole_var_id {
                                self.node_to_replace
                            } else {
                                self.target.add_node(ExpressionNode::Variable(v))
                            }
                        }
                        ExpressionNode::Binop(op, lhs, rhs) => {
                            let new_lhs = self.merge_tree_inner(lhs);
                            let new_rhs = self.merge_tree_inner(rhs);
                            self.target
                                .add_node(ExpressionNode::Binop(op, new_lhs, new_rhs))
                        }
                    },
                    MaybeOrNext::Next(_) => panic!("Free Slot encountered during modification"),
                };
                self.handle_mapping.insert(node, new_handle);
                new_handle
            }

            fn merge_tree(&mut self, root: ExpressionHandle) {
                let mut to_replace = vec![];

                for (i, n) in self.target.nodes.iter().enumerate() {
                    if let MaybeOrNext::Value(ExpressionNode::Binop(_, lhs, rhs)) = n {
                        if *lhs == self.node_to_replace || *rhs == self.node_to_replace {
                            to_replace.push(ExpressionHandle { id: i })
                        }
                    }
                }

                let new_subtree_root = self.merge_tree_inner(root);
                for node in to_replace {
                    let MaybeOrNext::Value(ExpressionNode::Binop(_, lhs, rhs)) =
                        &mut self.target.nodes[node.id]
                    else {
                        panic!("target node has been unexpectedly modified ");
                    };

                    if *lhs == self.node_to_replace {
                        *lhs = new_subtree_root;
                    } else if *rhs == self.node_to_replace {
                        *rhs = new_subtree_root;
                    }
                }

                if self.target.root == self.node_to_replace {
                    self.target.root = new_subtree_root;
                }
            }
        }

        let root = expr.root;
        TreeMerger {
            src: &mut expr,
            handle_mapping: &mut handle_mapping,
            hole_var_id,
            node_to_replace,
            target: self,
        }
        .merge_tree(root);

        self.gc();
    }
}

struct VariableMap {
    map: Vec<String>,
}
impl VariableMap {
    fn new() -> Self {
        VariableMap { map: Vec::new() }
    }
    fn get_or_insert(&mut self, name: &str) -> Variable {
        for (i, v) in self.map.iter().enumerate() {
            if v == name {
                return Variable { id: i };
            }
        }
        let var = Variable { id: self.map.len() };
        self.map.push(name.to_string());
        var
    }

    fn var_map_to_var_vals(&self, var_mapping: &HashMap<&str, f64>) -> Vec<f64> {
        self.map
            .iter()
            .map(|name| *var_mapping.get(name.as_str()).unwrap_or(&f64::NAN))
            .collect()
    }
}

fn main() {
    let mut var_map = VariableMap::new();
    let var_x = var_map.get_or_insert("x");
    let var_y = var_map.get_or_insert("y");
    let hole_var = var_map.get_or_insert("_HOLE");

    let mut expr_tree = ExpressionTree::new();

    let x_handle = expr_tree.add_node(ExpressionNode::Variable(var_x));
    let y_handle = expr_tree.add_node(ExpressionNode::Variable(var_y));
    let _3_handle = expr_tree.add_node(ExpressionNode::Constant(3.0));
    let sum_handle = expr_tree.add_node(ExpressionNode::Binop(Binop::Add, x_handle, _3_handle));

    expr_tree.root = sum_handle;
    println!("{:?}", expr_tree.nodes);
    expr_tree.gc();
    println!("{:?}", expr_tree.nodes);

    let mut insert_tree = ExpressionTree::new();

    let hole_handle = insert_tree.add_node(ExpressionNode::Variable(hole_var));
    let x2_handle = insert_tree.add_node(ExpressionNode::Variable(var_x));
    let new_sum_handle =
        insert_tree.add_node(ExpressionNode::Binop(Binop::Mul, x2_handle, x2_handle));
    insert_tree.root = new_sum_handle;

    println!("{}", expr_tree.dump(&var_map));
    println!("{}", insert_tree.dump(&var_map));
    println!("Applying modification...");
    println!("Is a tree before: {}", expr_tree.is_a_tree());
    expr_tree.apply(
        hole_var,
        Modification {
            pos: CursorPosition { node: x_handle },
            expr: insert_tree,
        },
    );
    println!("Is a tree after: {}", expr_tree.is_a_tree());
    println!("{}", expr_tree.dump(&var_map));

    let vars = hash_map! {
        x: 3.0,
        y: 4.0,
    };
    println!("Evaluating with {:#?}", vars);
    println!(
        "{}",
        expr_tree.evaluate(&var_map.var_map_to_var_vals(&vars))
    );

    println!("DOT format:");
    expr_tree.to_dot(&mut std::io::stdout());
}
