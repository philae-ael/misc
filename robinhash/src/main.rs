#[derive(Debug)]
struct BucketFull<K, V> {
    key: K,
    val: V,
    r: u8,
}
#[derive(Default, Debug)]
enum Bucket<K, V> {
    #[default]
    Empty,
    Full(BucketFull<K, V>),
}

pub struct RHashMap<K, V, H = std::hash::BuildHasherDefault<std::hash::DefaultHasher>> {
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

impl<K, V> RHashMap<K, V, std::hash::BuildHasherDefault<std::hash::DefaultHasher>> {
    pub fn new() -> Self {
        Default::default()
    }
}

impl<K, V, H> RHashMap<K, V, H> {
    pub fn with_capacity(cap: usize, hash_builder: H) -> Self {
        Self {
            data: Vec::with_capacity(cap).into(),
            hash_builder,
            len: 0,
        }
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }
    pub fn len(&self) -> usize {
        self.len
    }
    pub fn load_factor(&self) -> f32 {
        self.len() as f32 / self.capacity() as f32
    }
    pub fn capacity(&self) -> usize {
        self.data.len()
    }
}

impl<K: std::fmt::Debug, V: std::fmt::Debug, H> RHashMap<K, V, H>
where
    K: std::hash::Hash + std::cmp::Eq,
    H: std::hash::BuildHasher,
{
    /// # Safety
    /// There should be enough room in the hashmap:
    /// self.len() < self.capacity()
    pub unsafe fn insert_unchecked(&mut self, k: K, v: V) -> Option<V> {
        let h = self.hash_builder.hash_one(&k);
        let bucket_idx = (h as usize) % self.capacity();

        let mut bucket = BucketFull {
            key: k,
            val: v,
            r: 0,
        };

        let cap = self.capacity();
        for i in 0..cap {
            println!("{:?}", self.data[(bucket_idx + i) % cap]);
            match &mut self.data[(bucket_idx + i) % cap] {
                b @ Bucket::Empty => {
                    self.len += 1;
                    *b = Bucket::Full(bucket);
                    return None;
                }
                Bucket::Full(bf) => {
                    if bf.key == bucket.key {
                        std::mem::swap(&mut bf.val, &mut bucket.val);
                        return Some(bucket.val);
                    }

                    if bucket.r > bf.r {
                        std::mem::swap(bf, &mut bucket);
                    }
                    bucket.r += 1;
                }
            }
        }

        unreachable!();
    }

    pub fn insert(&mut self, k: K, v: V) -> Option<V> {
        const MAX_LOAD_FACTOR: f32 = 0.9;
        const _: () = assert!(MAX_LOAD_FACTOR <= 1.0);

        if self.load_factor() >= MAX_LOAD_FACTOR || self.capacity() == 0 {
            let new_cap = usize::max(
                f32::ceil(1.0 / MAX_LOAD_FACTOR) as usize,
                self.capacity() * 2,
            );
            let new_sliced_box = Vec::from_iter((0..new_cap).map(|_| Default::default())).into();

            let old = std::mem::replace(&mut self.data, new_sliced_box);
            let old_len = self.len;
            self.len = 0;

            for data in old {
                match data {
                    Bucket::Empty => (),
                    Bucket::Full(BucketFull { key, val, .. }) => unsafe {
                        self.insert_unchecked(key, val);
                    },
                }
            }

            assert_eq!(self.len, old_len);
            assert!(self.load_factor() < MAX_LOAD_FACTOR);
        }

        // # Safety
        // if
        unsafe { self.insert_unchecked(k, v) }
    }

    pub fn get_raw(&self, k: &K) -> Option<*const V> {
        if self.is_empty() {
            return None;
        }

        let cap = self.capacity();
        let h = (self.hash_builder.hash_one(k) as usize) % cap;
        for i in 0..cap {
            let offset = (h + i) % cap;
            let b = &self.data[offset];
            match b {
                Bucket::Empty => {
                    return None;
                }
                Bucket::Full(bf) => {
                    if &bf.key == k {
                        return Some(&bf.val);
                    }
                }
            }
        }

        debug_assert_eq!(self.len(), self.capacity());
        None
    }

    pub fn get(&self, k: &K) -> Option<&V> {
        self.get_raw(k).map(|ptr| unsafe { &*ptr })
    }

    pub fn get_mut(&self, k: &K) -> Option<&mut V> {
        self.get_raw(k).map(|ptr| unsafe { &mut *(ptr as *mut _) })
    }
}

fn main() {}

#[cfg(test)]
mod tests {
    use std::hash::{BuildHasher, BuildHasherDefault};

    use super::*;

    #[test]
    fn test_insert_and_get_single_item() {
        let mut map = RHashMap::new();
        map.insert(1, "one");
        assert_eq!(map.get(&1), Some(&"one"));
    }

    #[test]
    fn test_get_nonexistent_key() {
        let mut map = RHashMap::new();
        map.insert(1, "one");
        assert_eq!(map.get(&2), None);
    }

    #[test]
    fn test_insert_overwrite() {
        let mut map = RHashMap::new();
        map.insert(1, "one");
        let old_value = map.insert(1, "ONE");
        assert_eq!(old_value, Some("one"));
        assert_eq!(map.get(&1), Some(&"ONE"));
    }

    #[test]
    fn test_multiple_items() {
        let mut map = RHashMap::new();
        map.insert(1, "one");
        map.insert(2, "two");
        map.insert(3, "three");

        assert_eq!(map.get(&1), Some(&"one"));
        assert_eq!(map.get(&2), Some(&"two"));
        assert_eq!(map.get(&3), Some(&"three"));
    }

    #[test]
    fn test_collisions() {
        // This test assumes keys that would naturally collide in your hash function
        // You might need to adapt this based on your specific implementation

        #[derive(Default)]
        struct Hasher {
            hash: u64,
        }

        impl std::hash::Hasher for Hasher {
            fn finish(&self) -> u64 {
                self.hash
            }

            fn write(&mut self, bytes: &[u8]) {
                self.hash = bytes.iter().fold(self.hash, |a, b| (a + *b as u64) % 10);
            }
        }

        let bh = BuildHasherDefault::<Hasher>::new();
        assert_eq!(bh.hash_one(1), bh.hash_one(11));
        assert_eq!(bh.hash_one(21), bh.hash_one(11));

        // Let's assume we're using a simple modulo hash function with a small table size
        // and these keys would collide
        let mut map = RHashMap::with_capacity(0, bh);
        map.insert(1, "one");
        map.insert(11, "eleven");
        map.insert(21, "twenty-one");

        assert_eq!(map.get(&1), Some(&"one"));
        assert_eq!(map.get(&11), Some(&"eleven"));
        assert_eq!(map.get(&21), Some(&"twenty-one"));
    }

    #[test]
    fn test_with_string_keys() {
        let mut map = RHashMap::new();
        map.insert("hello".to_string(), 1);
        map.insert("world".to_string(), 2);

        assert_eq!(map.get(&"hello".to_string()), Some(&1));
        assert_eq!(map.get(&"world".to_string()), Some(&2));
        assert_eq!(map.get(&"rust".to_string()), None);
    }

    #[test]
    fn test_insert_return_value() {
        let mut map = RHashMap::new();

        // When inserting a new key, it should return None
        assert_eq!(map.insert(1, "one"), None);

        // When inserting an existing key, it should return the old value
        assert_eq!(map.insert(1, "ONE"), Some("one"));
    }

    #[test]
    fn test_empty_map() {
        let map: RHashMap<i32, &str> = RHashMap::new();
        assert_eq!(map.get(&1), None);
    }

    #[test]
    fn test_large_number_of_items() {
        let mut map = RHashMap::new();

        // Insert a large number of items to test resizing
        for i in 0..1000 {
            map.insert(i, i.to_string());
        }

        // Verify all items can be retrieved
        for i in 0..1000 {
            assert_eq!(map.get(&i), Some(&i.to_string()));
        }
    }

    #[test]
    fn test_complex_keys() {
        #[derive(Hash, Eq, PartialEq, Debug, Clone)]
        struct ComplexKey {
            id: i32,
            name: String,
        }

        let mut map = RHashMap::new();

        let key1 = ComplexKey {
            id: 1,
            name: "one".to_string(),
        };
        let key2 = ComplexKey {
            id: 2,
            name: "two".to_string(),
        };

        map.insert(key1.clone(), "value1");
        map.insert(key2.clone(), "value2");

        assert_eq!(map.get(&key1), Some(&"value1"));
        assert_eq!(map.get(&key2), Some(&"value2"));

        // A different key with the same id but different name
        let different_key = ComplexKey {
            id: 1,
            name: "different".to_string(),
        };
        assert_eq!(map.get(&different_key), None);
    }
}
