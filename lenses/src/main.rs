use std::marker::PhantomData;

pub trait Lens<'a, A: 'a, B: 'a>: Copy {
    fn view(&self, a: &'a A) -> &'a B;
    fn set(&self, a: &'a mut A) -> &'a mut B;

    fn compose<C: 'a>(self, l2: impl Lens<'a, B, C>) -> impl Lens<'a, A, C> {
        ComposeLens(self, l2, PhantomData, PhantomData)
    }
}

pub struct ComposeLens<'a, A, B, C, L1: Lens<'a, A, B>, L2: Lens<'a, B, C>>(
    L1,
    L2,
    PhantomData<fn(&'a mut A) -> &'a mut B>,
    PhantomData<fn(&'a mut B) -> &'a mut C>,
);

impl<'a, A, B, C, L1: Lens<'a, A, B>, L2: Lens<'a, B, C>> Copy
    for ComposeLens<'a, A, B, C, L1, L2>
{
}
impl<'a, A, B, C, L1: Lens<'a, A, B>, L2: Lens<'a, B, C>> Clone
    for ComposeLens<'a, A, B, C, L1, L2>
{
    fn clone(&self) -> Self {
        *self
    }
}

impl<'a, A, B, C, L1: Lens<'a, A, B>, L2: Lens<'a, B, C>> Lens<'a, A, C>
    for ComposeLens<'a, A, B, C, L1, L2>
{
    fn view(&self, a: &'a A) -> &'a C {
        self.1.view(self.0.view(a))
    }

    fn set(&self, a: &'a mut A) -> &'a mut C {
        self.1.set(self.0.set(a))
    }
}

pub struct LensOffset<A, B, const OFFSET: usize>(PhantomData<fn(&mut A) -> &mut B>);

impl<A, B, const OFFSET: usize> Copy for LensOffset<A, B, OFFSET> {}
impl<A, B, const OFFSET: usize> Clone for LensOffset<A, B, OFFSET> {
    fn clone(&self) -> Self {
        *self
    }
}

impl<'a, A: 'a, B: 'a, const OFFSET: usize> LensOffset<A, B, OFFSET> {
    /// # Safety
    /// TODO
    pub unsafe fn new_unchecked(_: *const B) -> impl Lens<'a, A, B> {
        Self(PhantomData)
    }
}

impl<'a, A: 'a, B: 'a, const OFFSET: usize> Lens<'a, A, B> for LensOffset<A, B, OFFSET> {
    fn view(&self, a: &'a A) -> &'a B {
        unsafe { &*(core::ptr::from_ref(a).byte_add(OFFSET) as *const B) }
    }

    fn set(&self, a: &'a mut A) -> &'a mut B {
        unsafe { &mut *(core::ptr::from_mut(a).byte_add(OFFSET) as *mut B) }
    }
}

// This uses LensOffset which store the offset in the type
// -> There will be a trait impl for each type x field with a lens?
macro_rules! lens {
    ($t:ty, $field:ident) => {{
        let l = unsafe {
            const OFFSET: usize = core::mem::offset_of!($t, $field);
            let uninit = <core::mem::MaybeUninit<$t>>::uninit();
            let ptr_field = &raw const (*uninit.as_ptr()).$field;
            LensOffset::<$t, _, OFFSET>::new_unchecked(ptr_field)
        };
        l
    }};
}

#[derive(Copy, Clone)]
pub struct A {
    a1: usize,
    a2: usize,
}

#[derive(Copy, Clone)]
pub struct B {
    b1: A,
    b2: A,
}

// Note:
// those two compile to the same asm
//	movdqu xmm0, xmmword ptr [rdi]
//	movdqu xmm1, xmmword ptr [rdi + 16]
//	paddq xmm1, xmm0
//	pshufd xmm0, xmm1, 0xee
//	paddq xmm0, xmm1
//	movq rax, xmm0
//	ret
#[inline(never)]
fn run(b: &B) -> usize {
    let a1 = lens!(A, a1);
    let a2 = lens!(A, a2);
    let b1 = lens!(B, b1);
    let b2 = lens!(B, b2);

    let b1a1 = b1.compose(a1);
    let b1a2 = b1.compose(a2);
    let b2a1 = b2.compose(a1);
    let b2a2 = b2.compose(a2);
    b1a1.view(b) + b1a2.view(b) + b2a1.view(b) + b2a2.view(b)
}
#[inline(never)]
fn run2(b: &B) -> usize {
    b.b1.a1 + b.b1.a2 + b.b2.a1 + b.b2.a2
}

fn main() {
    let b = B {
        b1: A { a1: 1, a2: 2 },
        b2: A { a1: 3, a2: 4 },
    };

    assert_eq!(run(&b), run2(&b))
}
