#![allow(dead_code)]

pub const fn assert_copy<T: Copy>() {}

macro_rules! field_ref {
    ($src:ty, $field:ident : $ty:ty) => {
        unsafe {
            assert_copy::<$ty>();
            FieldRef::<$src>::new(
                std::mem::offset_of!($src, $field),
                std::mem::size_of::<$ty>(),
            )
        }
    };
}

pub trait LensView<Src> {
    fn from_src(src: &Src) -> Self;
    fn to_src(&self, src: &mut Src);
}

pub struct FieldRef<Src> {
    offset: usize,
    size: usize,
    _marker: std::marker::PhantomData<fn(*mut Src) -> *mut ()>,
}

impl<Src> Clone for FieldRef<Src> {
    fn clone(&self) -> Self {
        *self
    }
}
impl<Src> Copy for FieldRef<Src> {}

impl<Src> FieldRef<Src> {
    /// # Safety
    /// given a valid pointer to `Src`, the offset must point to a valid field of type `T`
    pub const unsafe fn new(offset: usize, size: usize) -> Self {
        Self {
            offset,
            size,
            _marker: std::marker::PhantomData,
        }
    }

    pub fn ptr(&self, src: &Src) -> *const () {
        let base = src as *const Src as *const u8;
        unsafe { base.add(self.offset) as *const () }
    }
    pub fn copy_into<Dst>(&self, src: &Src, dst_field: FieldRef<Dst>, dst: &mut Dst) {
        let src_ptr = self.ptr(src) as *const u8;
        let dst_ptr = dst_field.ptr(dst) as *mut u8;
        unsafe {
            std::ptr::copy_nonoverlapping(src_ptr, dst_ptr, self.size);
        }
    }
}

/// # Safety
/// The implementor must ensure that the VIEW_DESC and VIEW_DESC_SRC correctly describe the mapping
/// between Src and Self.
/// Additionally, the base_memory function must return a valid uninitialized memory for Self.
/// Such that, after copying the data using the field refs, the resulting instance is valid.
pub unsafe trait View<Src>: Sized + 'static
where
    Src: 'static,
{
    const VIEW_DESC: &[FieldRef<Self>];
    const VIEW_DESC_SRC: &[FieldRef<Src>];
    fn base_memory() -> std::mem::MaybeUninit<Self>;
}

impl<V: 'static, Src: 'static> LensView<Src> for V
where
    V: View<Src>,
{
    fn from_src(src: &Src) -> Self {
        let mut uninit = V::base_memory();
        // Safety: The implementor of View must ensure that the base_memory is valid after copying
        // the fields.
        let view = unsafe { uninit.as_mut_ptr().as_mut().unwrap() };
        // Copy fields from src to view
        assert_eq!(V::VIEW_DESC_SRC.len(), V::VIEW_DESC.len(),);
        for (src_field_ref, view_field_ref) in
            std::iter::zip(V::VIEW_DESC_SRC.iter(), V::VIEW_DESC.iter())
        {
            src_field_ref.copy_into(src, *view_field_ref, view);
        }
        // Safety: The implementor of View must ensure that the base_memory is valid after copying
        unsafe { uninit.assume_init() }
    }

    fn to_src(&self, src: &mut Src) {
        // Copy fields from view to src
        assert_eq!(V::VIEW_DESC_SRC.len(), V::VIEW_DESC.len(),);
        for (src_field_ref, view_field_ref) in
            std::iter::zip(V::VIEW_DESC_SRC.iter(), V::VIEW_DESC.iter())
        {
            view_field_ref.copy_into(self, *src_field_ref, src);
        }
    }
}

macro_rules! view_struct {
    ($view_name:ident, $src_name:ident, { $($field_name:ident : $field_type:ty),* $(,)? }) => {
        struct $view_name {
            $($field_name: $field_type),*
        }

        unsafe impl View<$src_name> for $view_name {
            const VIEW_DESC: &[FieldRef<Self>] = &[
                $(field_ref!($view_name, $field_name : $field_type)),*
            ];
            const VIEW_DESC_SRC: &[FieldRef<$src_name>] = &[
                $(field_ref!($src_name, $field_name : $field_type)),*
            ];
            fn base_memory() -> std::mem::MaybeUninit<Self> {
                std::mem::MaybeUninit::uninit()
            }
        }
    };
}

pub struct A {
    pub value: i32,
    pub value2: String,
}
view_struct!(AView, A, {
    value: i32,
});

// Not implemented because String is not Copy
// The idea to make that works would be to add more lenses so that on this field we would have a
// lens that would convert between &str and String, or clone idk
// view_struct!(AView2, A, {
//     value2: String,
// });

// like this (not implemented yet)
// view_struct!(AView2, A, {
//     // we are seeing value2 through a lens that clones it in one
//     // direction and replace the src field in the other
//     value2: String | cloneLens,
// });

fn main() {
    let mut a = A {
        value: 42,
        value2: "Hello".to_string(),
    };
    let mut a_view = AView::from_src(&a);
    a_view.value += 1;
    a_view.to_src(&mut a);

    assert_eq!(a.value, 43);
}
