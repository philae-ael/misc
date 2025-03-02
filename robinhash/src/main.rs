struct Bucket<K, V> {
    key: K,
    val: V,
    r: u8,
    empty: bool,
}

struct RHashMap<K, V, H = std::hash::BuildHasherDefault<std::hash::DefaultHasher>> {
    data: Box<[Bucket<K, V>]>,
    len: usize,
    hash_builder: H,
}

impl<K, V, H> Default for RHashMap<K, V, H>
where
    H: Default,
{
    fn default() -> Self {
        Self {
            data: Default::default(),
            len: 0,
            hash_builder: Default::default(),
        }
    }
}

impl<K, V, H> RHashMap<K, V, H> {
    pub fn new() -> Self
    where
        H: Default,
    {
        Default::default()
    }

    pub fn len(&mut self) -> usize {
        self.len
    }
    pub fn capacity(&mut self) -> usize {
        self.data.len()
    }

}

impl<K, V, H> RHashMap<K, V, H> where
        K: std::hash::Hash + std::cmp::Eq,
        H: std::hash::BuildHasher
{

    pub fn insert(&mut self, k: K, v: V) -> Option<V>
    {
        if self.len() == self.capacity() {
            self.grow();
        }

        let h = self.hash_builder.hash_one(&k);
        let bucket_idx = (h as usize) % self.len();

        let len = self.len();
        let mut bucket = Bucket {
            empty: false,
            key: k,
            val: v,
            r: 0,
        };
        for i in 0..len {
            let offset = (bucket_idx + i) % len;
            let b = &mut self.data[offset];
            if b.empty {
                return None;
            }

            if b.key == bucket.key {
                std::mem::swap(&mut b.val, &mut bucket.val);
                return Some(bucket.val);
            }

            if bucket.r > b.r {
                std::mem::swap(b, &mut bucket);
            }
            bucket.r += 1;
        }

        unreachable!();
    }

    fn grow(&mut self) {
        todo!()
    }

    pub fn get_raw(&self, k: &K) -> Option<*mut V>
    {
        let h = self.hash_builder.hash_one(k);
        let bucket_idx = (h as usize) % self.len();

        let len = self.len();
        for i in 0..len {
            let offset = (bucket_idx + i) % len;
            let b = &self.data[offset];
            if b.empty {
                //continue;
                return None; // ???
            }

            if &b.key == k {
                return Some(&b.val as *const _ as *mut _);
            }

            if bucket.r > b.r {
                std::mem::swap(b, &mut bucket);
            }
            bucket.r += 1;
        }
        return None;
    }

    pub fn get(&self), k: &K {
    }
}

fn main() {}

#[cfg(test)]
mod test {
    use crate::RHashMap;

    #[test]
    fn robin() {
        let mut rob = RHashMap::<u8, &'static str>::new();

        assert_eq!(rob.insert(0, "aaaa"), None);
        assert_eq!(rob.get(&0).copied(), Some("aaaa"));
        assert_eq!(rob.insert(0, "bbbb"), Some("bbbb"));
    }
}
