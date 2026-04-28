#![allow(non_snake_case, unused_macros)]

use proconio::input;
use rand::prelude::*;
use std::io::prelude::*;
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

pub const MAX_T: usize = 5000;

#[derive(Clone, Debug)]
pub struct Input {
    pub eps: f64,
    pub delta: f64,
    pub s: (i64, i64),
    pub ps: Vec<(i64, i64)>,
    pub walls: Vec<(i64, i64, i64, i64)>,
    pub fs: Vec<(i64, i64)>,
    pub alphas: Vec<f64>,
}

impl std::fmt::Display for Input {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        writeln!(
            f,
            "{} {} {:.2} {:.2}",
            self.ps.len(),
            self.walls.len(),
            self.eps,
            self.delta
        )?;
        writeln!(f, "{} {}", self.s.0, self.s.1)?;
        for i in 0..self.ps.len() {
            writeln!(f, "{} {}", self.ps[i].0, self.ps[i].1)?;
        }
        for i in 0..self.walls.len() {
            writeln!(
                f,
                "{} {} {} {}",
                self.walls[i].0, self.walls[i].1, self.walls[i].2, self.walls[i].3
            )?;
        }
        for i in 0..MAX_T {
            writeln!(f, "{}", self.alphas[i])?;
        }
        for i in 0..MAX_T {
            writeln!(f, "{} {}", self.fs[i].0, self.fs[i].1)?;
        }
        Ok(())
    }
}

pub fn parse_input(f: &str) -> Input {
    let f = proconio::source::once::OnceSource::from(f);
    input! {
        from f,
        N: usize, M: usize, eps: f64, delta: f64,
        s: (i64, i64),
        ps: [(i64, i64); N],
        walls: [(i64, i64, i64, i64); M],
        alphas: [f64; MAX_T],
        fs: [(i64, i64); MAX_T],
    }
    Input {
        eps,
        delta,
        s,
        ps,
        walls,
        fs,
        alphas,
    }
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
    pub out: Vec<(char, i64, i64)>,
}

pub fn parse_output(_input: &Input, f: &str) -> Result<Output, String> {
    let mut out = vec![];
    for line in f.lines() {
        if line.starts_with('#') {
            continue;
        }
        let mut it = line.split_whitespace();
        let a = read(it.next(), 'A'..'Z')?;
        let x = read(it.next(), -100000..=100000)?;
        let y = read(it.next(), -100000..=100000)?;
        if a != 'A' && a != 'S' {
            return Err(format!("Invalid action: {}", a));
        } else if a == 'A' && x * x + y * y > 500 * 500 {
            return Err(format!("Out of range: ({}, {})", x, y));
        } else if a == 'S' && x * x + y * y > 10000000000 {
            return Err(format!("Out of range: ({}, {})", x, y));
        } else if a == 'S' && (x, y) == (0, 0) {
            return Err(format!("Out of range: ({}, {})", x, y));
        }
        out.push((a, x, y));
    }
    if out.len() > MAX_T {
        return Err(format!("Too many actions: {}", out.len()));
    }
    Ok(Output { out })
}

pub fn gen(seed: u64, problem: char) -> Input {
    let mut rng = rand_chacha::ChaCha20Rng::seed_from_u64(seed);
    let N = 10;
    let (M, eps, delta) = match problem {
        'A' => (
            0,
            rng.gen_range(1..=100) as f64,
            rng.gen_range(1..=20) as f64 * 0.01,
        ),
        'B' => (10, rng.gen_range(0..=1) as f64, 0.01),
        'C' => (
            rng.gen_range(1..=10i32) as usize,
            rng.gen_range(1..=100) as f64,
            rng.gen_range(1..=20) as f64 * 0.01,
        ),
        _ => {
            panic!("Unknown problem: {}", problem)
        }
    };
    let s = (rng.gen_range(-99999..=99999), rng.gen_range(-99999..=99999));
    let mut ps: Vec<(i64, i64)> = vec![];
    while ps.len() < N {
        let p = (
            rng.gen_range(-100000..=100000),
            rng.gen_range(-100000..=100000),
        );
        if ps
            .iter()
            .chain(&[s])
            .any(|&q| (p.0 - q.0) * (p.0 - q.0) + (p.1 - q.1) * (p.1 - q.1) < 5000 * 5000)
        {
            continue;
        }
        ps.push(p);
    }
    let mut walls: Vec<(i64, i64, i64, i64)> = vec![];
    while walls.len() < M {
        let x1 = rng.gen_range(-90000..=90000);
        let y1 = rng.gen_range(-90000..=90000);
        let x2 = x1 + rng.gen_range(-100000..=100000);
        let y2 = y1 + rng.gen_range(-100000..=100000);
        if (x2 < -100000 || 100000 < x2) && (y2 < -100000 || 100000 < y2) || (x2, y2) == (0, 0) {
            continue;
        }
        let x2 = x2.min(100000).max(-100000);
        let y2 = y2.min(100000).max(-100000);
        if walls.iter().all(|w| {
            !P::crs_ss(
                (P(x1 as f64, y1 as f64), P(x2 as f64, y2 as f64)),
                (P(w.0 as f64, w.1 as f64), P(w.2 as f64, w.3 as f64)),
            )
        }) {
            if !P::crs_sp(
                (P(x1 as f64, y1 as f64), P(x2 as f64, y2 as f64)),
                P(s.0 as f64, s.1 as f64),
            ) {
                walls.push((x1, y1, x2, y2));
            }
        }
    }
    let alphas = (0..MAX_T)
        .map(|_| loop {
            let t = 1.0 + rng.sample::<f64, _>(rand_distr::StandardNormal) * delta;
            if t > 0.0 {
                break t;
            }
        })
        .collect();
    let fs = (0..MAX_T)
        .map(|_| {
            (
                (rng.sample::<f64, _>(rand_distr::StandardNormal) * eps).round() as i64,
                (rng.sample::<f64, _>(rand_distr::StandardNormal) * eps).round() as i64,
            )
        })
        .collect();
    Input {
        eps,
        delta,
        s,
        ps,
        walls,
        alphas,
        fs,
    }
}

pub fn compute_score(input: &Input, out: &Output) -> (i64, String) {
    let (mut score, mut err, (_, _, visited)) = compute_score_details(input, &out.out);
    // if visited.iter().any(|&b| !b) {
    //     err = "Unexpected EOF".to_owned();
    // }
    if err.len() > 0 {
        score = 0;
    }
    (score, err)
}

pub struct Sim {
    pub visited: Vec<bool>,
    score: i64,
    crt_score: i64,
    pub p: P,
    pub v: P,
    t: usize,
}

impl Sim {
    pub fn new(input: &Input) -> Self {
        let visited = vec![false; input.ps.len()];
        let score = 0;
        let crt_score = 0;
        let p = P(input.s.0 as f64, input.s.1 as f64);
        let v = P(0.0, 0.0);
        Self {
            visited,
            score,
            crt_score,
            p,
            v,
            t: 0,
        }
    }
    pub fn query(&mut self, input: &Input, mv: char, x: i64, y: i64) -> (i32, Vec<usize>, i64) {
        let mut ret = -1;
        match mv {
            'A' => {
                self.v = self.v + P(x as f64, y as f64);
            }
            'S' => {
                let mut d = 1e9;
                for wall in input.walls.iter().chain(
                    [
                        (-100000, -100000, -100000, 100000),
                        (-100000, 100000, 100000, 100000),
                        (100000, 100000, 100000, -100000),
                        (100000, -100000, -100000, -100000),
                    ]
                    .iter(),
                ) {
                    let dir = P(x as f64, y as f64);
                    let w1 = P(wall.0 as f64, wall.1 as f64);
                    let w2 = P(wall.2 as f64, wall.3 as f64);
                    if let Some(p) = P::pi_ll((self.p, self.p + dir), (w1, w2)) {
                        if sig(dir.det(w1 - self.p)) * sig(dir.det(w2 - self.p)) <= 0
                            && (p - self.p).dot(dir) >= 0.0
                        {
                            d.setmin((p - self.p).abs2().sqrt());
                        }
                    }
                }
                d *= input.alphas[self.t];
                ret = d.round() as i64;
            }
            _ => {
                unreachable!()
            }
        }
        self.v = self.v + P(input.fs[self.t].0 as f64, input.fs[self.t].1 as f64);
        self.crt_score -= 2;
        self.t += 1;
        let q = self.p + self.v;
        if q.0 < -100000.0
            || 100000.0 < q.0
            || q.1 < -100000.0
            || 100000.0 < q.1
            || input.walls.iter().any(|&(x1, y1, x2, y2)| {
                P::crs_ss(
                    (P(x1 as f64, y1 as f64), P(x2 as f64, y2 as f64)),
                    (self.p, q),
                )
            })
        {
            self.crt_score -= 100;
            self.v = P(0.0, 0.0);
            return (1, vec![], ret);
        } else {
            let mut hit = vec![];
            for i in 0..input.ps.len() {
                if !self.visited[i]
                    && P::dist2_sp((self.p, q), P(input.ps[i].0 as f64, input.ps[i].1 as f64))
                        <= 1000000.0
                {
                    self.visited[i] = true;
                    self.crt_score += 1000;
                    hit.push(i);
                }
            }
            self.p = q;
            self.score.setmax(self.crt_score);
            (0, hit, ret)
        }
    }
}

pub fn compute_score_details(
    input: &Input,
    out: &[(char, i64, i64)],
) -> (i64, String, (P, P, Vec<bool>)) {
    let mut sim = Sim::new(input);
    for &(mv, x, y) in out {
        sim.query(input, mv, x, y);
    }
    (sim.score, String::new(), (sim.p, sim.v, sim.visited))
}

use itertools::Itertools;
use std::io::{BufRead, BufReader, Read};
use std::process::ChildStdout;

fn read_line(stdout: &mut BufReader<ChildStdout>, local: bool) -> Result<String, String> {
    loop {
        let mut out = String::new();
        match stdout.read_line(&mut out) {
            Ok(0) | Err(_) => {
                return Err(format!("Your program has terminated unexpectedly"));
            }
            _ => (),
        }
        if local {
            print!("{}", out);
        }
        let v = out.trim();
        if v.len() == 0 || v.starts_with("#") {
            continue;
        }
        return Ok(v.to_owned());
    }
}

pub fn exec(p: &mut std::process::Child, local: bool) -> Result<i64, String> {
    let mut input = String::new();
    std::io::stdin().read_to_string(&mut input).unwrap();
    let input = parse_input(&input);
    let mut stdin = std::io::BufWriter::new(p.stdin.take().unwrap());
    let mut stdout = std::io::BufReader::new(p.stdout.take().unwrap());
    let _ = writeln!(
        stdin,
        "{} {} {:.2} {:.2}",
        input.ps.len(),
        input.walls.len(),
        input.eps,
        input.delta
    );
    let _ = writeln!(stdin, "{} {}", input.s.0, input.s.1);
    for i in 0..input.ps.len() {
        let _ = writeln!(stdin, "{} {}", input.ps[i].0, input.ps[i].1);
    }
    for i in 0..input.walls.len() {
        let _ = writeln!(
            stdin,
            "{} {} {} {}",
            input.walls[i].0, input.walls[i].1, input.walls[i].2, input.walls[i].3
        );
    }
    let _ = stdin.flush();
    let mut sim = Sim::new(&input);
    for _ in 0..MAX_T {
        if local {
            println!("#p {:.0} {:.0}", sim.p.0, sim.p.1);
            println!("#v {:.0} {:.0}", sim.v.0, sim.v.1);
        }
        let line = read_line(&mut stdout, local)?;
        let mut it = line.split_whitespace();
        let a = read(it.next(), 'A'..'Z')?;
        let x = read(it.next(), -100000..=100000)?;
        let y = read(it.next(), -100000..=100000)?;
        if a != 'A' && a != 'S' {
            return Err(format!("Invalid action: {}", a));
        } else if a == 'A' && x * x + y * y > 500 * 500 {
            return Err(format!("Out of range: ({}, {})", x, y));
        } else if a == 'S' && x * x + y * y > 10000000000 {
            return Err(format!("Out of range: ({}, {})", x, y));
        } else if a == 'S' && (x, y) == (0, 0) {
            return Err(format!("Out of range: ({}, {})", x, y));
        }
        let (ret, hit, d) = sim.query(&input, a, x, y);
        if d >= 0 {
            let _ = writeln!(stdin, "{}", d);
        }
        let _ = writeln!(stdin, "{} {}", ret, hit.len());
        if hit.len() > 0 {
            let _ = writeln!(stdin, "{}", hit.iter().join(" "));
        }
        let _ = stdin.flush();
        if sim.visited.iter().all(|&b| b) {
            break;
        }
    }
    p.wait().unwrap();
    Ok(sim.score)
}

use std::cmp::Ordering;
use std::ops::*;
#[derive(Clone, Copy, Default, Debug, PartialEq, PartialOrd)]
pub struct P(pub f64, pub f64);

impl Add for P {
    type Output = P;
    fn add(self, a: P) -> P {
        P(self.0 + a.0, self.1 + a.1)
    }
}

impl Sub for P {
    type Output = P;
    fn sub(self, a: P) -> P {
        P(self.0 - a.0, self.1 - a.1)
    }
}

impl Mul<f64> for P {
    type Output = P;
    fn mul(self, a: f64) -> P {
        P(self.0 * a, self.1 * a)
    }
}

impl P {
    pub fn dot(self, a: P) -> f64 {
        (self.0 * a.0) + (self.1 * a.1)
    }
    pub fn det(self, a: P) -> f64 {
        (self.0 * a.1) - (self.1 * a.0)
    }
    pub fn abs2(self) -> f64 {
        self.dot(self)
    }
}

fn sig<T>(x: T) -> i32
where
    T: Default + PartialOrd,
{
    match x.partial_cmp(&T::default()) {
        Some(Ordering::Greater) => 1,
        Some(Ordering::Less) => -1,
        _ => 0,
    }
}

impl P {
    pub fn dist2_sp((p1, p2): (P, P), q: P) -> f64 {
        if (p2 - p1).dot(q - p1) <= 0.0 {
            (q - p1).abs2()
        } else if (p1 - p2).dot(q - p2) <= 0.0 {
            (q - p2).abs2()
        } else {
            P::dist2_lp((p1, p2), q)
        }
    }
    pub fn dist2_lp((p1, p2): (P, P), q: P) -> f64 {
        let det = (p2 - p1).det(q - p1);
        det * det / (p2 - p1).abs2()
    }
    pub fn crs_sp((p1, p2): (P, P), q: P) -> bool {
        P::crs_lp((p1, p2), q) && (q - p1).dot(q - p2) <= 0.0
    }
    pub fn crs_lp((p1, p2): (P, P), q: P) -> bool {
        (p2 - p1).det(q - p1) == 0.0
    }
    pub fn crs_ss((p1, p2): (P, P), (q1, q2): (P, P)) -> bool {
        let sort = |a, b| {
            if a < b {
                (a, b)
            } else {
                (b, a)
            }
        };
        let (lp0, up0) = sort(p1.0, p2.0);
        let (lq0, uq0) = sort(q1.0, q2.0);
        let (lp1, up1) = sort(p1.1, p2.1);
        let (lq1, uq1) = sort(q1.1, q2.1);
        if up0 < lq0 || uq0 < lp0 || up1 < lq1 || uq1 < lp1 {
            return false;
        }
        return sig((p2 - p1).det(q1 - p1)) * sig((p2 - p1).det(q2 - p1)) <= 0
            && sig((q2 - q1).det(p1 - q1)) * sig((q2 - q1).det(p2 - q1)) <= 0;
    }
    pub fn pi_ll((p1, p2): (P, P), (q1, q2): (P, P)) -> Option<P> {
        let d = (q2 - q1).det(p2 - p1);
        if d == 0.0 {
            return None;
        }
        let r = p1 * d + (p2 - p1) * (q2 - q1).det(q1 - p1);
        Some(P(r.0 / d, r.1 / d))
    }
}
