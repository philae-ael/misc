#![feature(const_for)]
const MAX_ITERATION: usize = 64;

fn precompute_cordic() -> [(f32, f32); MAX_ITERATION] {
    let mut output = [(0.0, 0.0); MAX_ITERATION];
    for i in 0..MAX_ITERATION {
        let p = 2.0f32.powi(-(i as i32));
        output[i] = (1.0f32 / (1.0f32 + p * p).sqrt(), f32::atan(p));
    }
    output
}

lazy_static::lazy_static! {
static ref CORDIC_TABLE : [(f32, f32);MAX_ITERATION] = precompute_cordic();
}

pub fn cordic(x: f32) -> (f32, f32, f32) {
    let (mut v_x, mut v_y) = (1., 0.);
    let mut beta = x;
    let mut p = 1.0f32;
    for i in 0..MAX_ITERATION {
        let sigma = beta.signum();
        let (k, dbeta_abs) = CORDIC_TABLE[i];

        #[rustfmt::skip]
        {
            (v_x, v_y) = (
                k*(v_x         + -v_y*sigma*p),
                k*(v_x*sigma*p +  v_y)
            );
        };

        beta = beta - sigma * dbeta_abs;
        p = p / 2.0;
    }
    (v_x, v_y, v_y / v_x)
}

fn main() {
}

#[cfg(test)]
mod tests {
    use crate::*;

    #[test]
    fn test_cordic() {
        const N: usize = 64;
        const EPS: f32 = 0.000001;
        for i in 0..N {
            let x = i as f32 / N as f32;
            let (c, s, t) = cordic(x);
            assert!(
                (c - x.cos()).abs() < EPS,
                "Invalid cos, got {c}, expected {} (x = {x})",
                x.cos()
            );
            assert!(
                (s - x.sin()).abs() < EPS,
                "Invalid sin, got {c}, expected {} (x = {x})",
                x.sin()
            );
            assert!(
                (t - x.tan()).abs() < EPS,
                "Invalid tan, got {c}, expected {} (x = {x})",
                x.tan()
            );
        }
    }
}
