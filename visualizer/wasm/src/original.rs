#![allow(non_snake_case, unused_macros)]

use itertools::Itertools;
use proconio::input;
use rand::prelude::*;
use std::ops::RangeBounds;

pub trait SetMinMax {
    fn setmin(&mut self, v: Self) -> bool;
    fn setmax(&mut self, v: Self) -> bool;
}
impl<T> SetMinMax for T
where
    T: PartialOrd,
{
    fn setmin(&mut self, v: T) -> bool {
        *self > v && {
            *self = v;
            true
        }
    }
    fn setmax(&mut self, v: T) -> bool {
        *self < v && {
            *self = v;
            true
        }
    }
}

#[macro_export]
macro_rules! mat {
	($($e:expr),*) => { Vec::from(vec![$($e),*]) };
	($($e:expr,)*) => { Vec::from(vec![$($e),*]) };
	($e:expr; $d:expr) => { Vec::from(vec![$e; $d]) };
	($e:expr; $d:expr $(; $ds:expr)+) => { Vec::from(vec![mat![$e $(; $ds)*]; $d]) };
}

#[derive(Clone, Debug)]
pub struct Input {
    pub X: usize,
    pub Y: usize,
    pub Z: usize,
    pub ps: Vec<(i64, i64)>,
}

impl std::fmt::Display for Input {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        writeln!(f, "{} {} {}", self.X, self.Y, self.Z)?;
        for (x, y) in &self.ps {
            writeln!(f, "{} {}", x, y)?;
        }
        Ok(())
    }
}

pub fn parse_input(f: &str) -> Input {
    let f = proconio::source::once::OnceSource::from(f);
    input! {
        from f,
        X: usize, Y: usize, Z: usize,
        ps: [(i64, i64); X + Y + Z],
    }
    Input { X, Y, Z, ps }
}

pub fn read<T: Copy + PartialOrd + std::fmt::Display + std::str::FromStr, R: RangeBounds<T>>(
    token: Option<&str>,
    range: R,
) -> Result<T, String> {
    if let Some(v) = token {
        if let Ok(v) = v.parse::<T>() {
            if !range.contains(&v) {
                Err(format!("Out of range: {}", v))
            } else {
                Ok(v)
            }
        } else {
            Err(format!("Parse error: {}", v))
        }
    } else {
        Err("Unexpected EOF".to_owned())
    }
}

pub struct Output {
    pub out: Vec<((i64, i64), (i64, i64), (i64, i64), (i64, i64))>,
}

pub fn parse_output(_input: &Input, f: &str) -> Result<Output, String> {
    let mut out = vec![];
    let mut ss = f.split_whitespace().peekable();
    while ss.peek().is_some() {
        let x0 = read(ss.next(), 0..=1_000_000)?;
        let y0 = read(ss.next(), 0..=1_000_000)?;
        let x1 = read(ss.next(), 0..=1_000_000)?;
        let y1 = read(ss.next(), 0..=1_000_000)?;
        let x2 = read(ss.next(), 0..=1_000_000)?;
        let y2 = read(ss.next(), 0..=1_000_000)?;
        let x3 = read(ss.next(), 0..=1_000_000)?;
        let y3 = read(ss.next(), 0..=1_000_000)?;
        out.push(((x0, y0), (x1, y1), (x2, y2), (x3, y3)));
        if out.len() > 10000 {
            return Err("Too many output".to_owned());
        }
    }
    Ok(Output { out })
}

fn dist2(p: (i64, i64), q: (i64, i64)) -> i64 {
    let dx = p.0 - q.0;
    let dy = p.1 - q.1;
    dx * dx + dy * dy
}

fn dist(p: (i64, i64), q: (i64, i64)) -> f64 {
    (dist2(p, q) as f64).sqrt()
}

fn orient(a: (i64, i64), b: (i64, i64), p: (i64, i64)) -> i64 {
    let (dx1, dy1) = (b.0 - a.0, b.1 - a.1);
    let (dx2, dy2) = (p.0 - a.0, p.1 - a.1);
    dx1 * dy2 - dy1 * dx2
}

fn contains(p: (i64, i64), a: (i64, i64), b: (i64, i64), c: (i64, i64)) -> bool {
    if orient(a, b, c) == 0 {
        if orient(a, b, p) != 0 || orient(a, c, p) != 0 {
            return false;
        }
        let min_x = a.0.min(b.0).min(c.0);
        let max_x = a.0.max(b.0).max(c.0);
        let min_y = a.1.min(b.1).min(c.1);
        let max_y = a.1.max(b.1).max(c.1);
        return (min_x <= p.0 && p.0 <= max_x) && (min_y <= p.1 && p.1 <= max_y);
    }
    let c1 = orient(a, b, p);
    let c2 = orient(b, c, p);
    let c3 = orient(c, a, p);
    (c1 >= 0 && c2 >= 0 && c3 >= 0) || (c1 <= 0 && c2 <= 0 && c3 <= 0)
}

pub fn gen(seed: u64, problem: &str) -> Input {
    let mut rng = rand_chacha::ChaCha20Rng::seed_from_u64(seed);
    let (X, Y, Z) = match problem {
        "A" => (100, 0, rng.gen_range(10i32..=100) as usize),
        "B" => (100, 100, 0),
        "C" => (100, 100, rng.gen_range(1i32..=100) as usize),
        x => panic!("Unknown problem: {}", x),
    };
    let mut ps = vec![];
    for iter in 0..3 {
        loop {
            let n = rng.gen_range(5i32..=10i32) as usize;
            let ws = (0..n).map(|_| rng.gen::<f64>()).collect_vec();
            let cs = (0..n)
                .map(|_| {
                    (
                        rng.gen_range(200000..=800000),
                        rng.gen_range(200000..=800000),
                    )
                })
                .collect_vec();
            let sigma = (0..n)
                .map(|_| {
                    let sigma_x = rng.gen_range(30000..=90000) as f64;
                    let sigma_y = rng.gen_range(30000..=90000) as f64;
                    (sigma_x, sigma_y)
                })
                .collect_vec();
            let theta = (0..n)
                .map(|_| rng.gen_range(0.0..=std::f64::consts::PI))
                .collect_vec();
            let is = (0..n).collect_vec();
            let target = ps.len() + [X, Y, Z][iter];
            while ps.len() < target {
                let i = *is.choose_weighted(&mut rng, |&i| ws[i]).unwrap();
                let (c, (sigma_x, sigma_y), theta) = (cs[i], sigma[i], theta[i]);
                let bx: f64 = rng.sample(rand_distr::StandardNormal);
                let by: f64 = rng.sample(rand_distr::StandardNormal);
                let x = (c.0 as f64 + theta.cos() * sigma_x * bx - theta.sin() * sigma_y * by)
                    .round() as i64;
                let y = (c.1 as f64 + theta.sin() * sigma_x * bx + theta.cos() * sigma_y * by)
                    .round() as i64;
                if 1 <= x
                    && x < 1000000
                    && 1 <= y
                    && y < 1000000
                    && ps.iter().all(|&q| dist2((x, y), q) >= 1000000)
                {
                    ps.push((x, y));
                }
            }
            if iter > 0 {
                break;
            } else {
                if ps.iter().any(|&(x, y)| x <= 400000 && y <= 400000)
                    && ps.iter().any(|&(x, y)| x <= 400000 && y >= 600000)
                    && ps.iter().any(|&(x, y)| x >= 600000 && y <= 400000)
                    && ps.iter().any(|&(x, y)| x >= 600000 && y >= 600000)
                {
                    break;
                } else {
                    ps.clear();
                }
            }
        }
    }
    Input { X, Y, Z, ps }
}

pub fn compute_score(input: &Input, out: &Output) -> (i64, String) {
    let (mut score, err, _) = compute_score_details(input, &out.out);
    if err.len() > 0 {
        score = 0;
    }
    (score, err)
}

pub fn compute_score_details(
    input: &Input,
    out: &[((i64, i64), (i64, i64), (i64, i64), (i64, i64))],
) -> (i64, String, (f64, Vec<bool>)) {
    let mut done = vec![false; input.X + input.Y + input.Z];
    let mut sum = input.Z;
    if out.len() == 0 {
        return (0, "Empty output".to_owned(), (0.0, done));
    }
    let (mut p1, mut q1, mut p2, mut q2) = out[0];
    let mut T = 0.0;
    for t in 1..out.len() {
        let (p1n, q1n, p2n, q2n) = out[t];
        T += (dist(p1, p1n) + dist(q1, q1n)).max(dist(p2, p2n) + dist(q2, q2n));
        for i in 0..input.ps.len() {
            if !done[i]
                && (contains(input.ps[i], p1, q1, p1n) || contains(input.ps[i], p1n, q1, q1n))
            {
                done[i] = true;
                if i < input.X {
                    sum += 1;
                } else if i >= input.X + input.Y {
                    sum -= 1;
                }
            }
            if !done[i]
                && (contains(input.ps[i], p2, q2, p2n) || contains(input.ps[i], p2n, q2, q2n))
            {
                done[i] = true;
                if i >= input.X && i < input.X + input.Y {
                    sum += 1;
                } else if i >= input.X + input.Y {
                    sum -= 1;
                }
            }
        }
        p1 = p1n;
        q1 = q1n;
        p2 = p2n;
        q2 = q2n;
    }
    assert!(sum <= input.X + input.Y + input.Z);
    let score = if sum == input.X + input.Y + input.Z && T <= 1e8 {
        (1e6 * (1.0 + (1e8 / T).log2())).round() as i64
    } else {
        (1e6 * sum as f64 / (input.X + input.Y + input.Z) as f64).round() as i64
    };
    (score, String::new(), (T, done))
}
