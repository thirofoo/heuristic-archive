#![allow(non_snake_case, unused_macros)]

use itertools::Itertools;
use proconio::{input, marker::Chars};
use rand::prelude::*;
use std::ops::RangeBounds;
use svg::node::element::{Circle, Group, Line, Rectangle, Style, Title};

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
    pub N: usize,
    pub M: usize,
    pub V: usize,
    pub s: Vec<Vec<bool>>,
    pub t: Vec<Vec<bool>>,
}

impl std::fmt::Display for Input {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        writeln!(f, "{} {} {}", self.N, self.M, self.V)?;
        for i in 0..self.N {
            writeln!(
                f,
                "{}",
                self.s[i]
                    .iter()
                    .map(|&b| if b { '1' } else { '0' })
                    .collect::<String>()
            )?;
        }
        for i in 0..self.N {
            writeln!(
                f,
                "{}",
                self.t[i]
                    .iter()
                    .map(|&b| if b { '1' } else { '0' })
                    .collect::<String>()
            )?;
        }
        Ok(())
    }
}

pub fn parse_input(f: &str) -> Input {
    let f = proconio::source::once::OnceSource::from(f);
    input! {
        from f,
        N: usize, M: usize, V: usize,
        s: [Chars; N],
        t: [Chars; N],
    }
    let s = s
        .iter()
        .map(|v| v.iter().map(|&c| c == '1').collect())
        .collect();
    let t = t
        .iter()
        .map(|v| v.iter().map(|&c| c == '1').collect())
        .collect();
    Input { N, M, V, s, t }
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
    pub pL: Vec<(usize, usize)>,
    pub init: (i32, i32),
    pub S: Vec<Vec<char>>,
}

pub fn parse_output(input: &Input, f: &str) -> Result<Output, String> {
    let mut ss = f.split_whitespace();
    let V = read(ss.next(), 1..=input.V)?;
    let mut pL = vec![];
    for u in 1..=V - 1 {
        let p = read(ss.next(), 0..u)?;
        let L = read(ss.next(), 1..=input.N - 1)?;
        pL.push((p, L));
    }
    let init = (
        read(ss.next(), 0..=input.N as i32 - 1)?,
        read(ss.next(), 0..=input.N as i32 - 1)?,
    );
    let mut S = vec![];
    while let Some(s) = ss.next() {
        let s = s.chars().collect_vec();
        if s.len() != 2 * V {
            return Err("Invalid operation length".to_owned());
        }
        S.push(s);
    }
    if S.len() > 100000 {
        return Err("Too many output".to_owned());
    }
    Ok(Output { pL, init, S })
}

pub fn gen(seed: u64, fix_N: Option<usize>, fix_M: Option<usize>, fix_V: Option<usize>) -> Input {
    let mut rng = rand_chacha::ChaCha20Rng::seed_from_u64(seed ^ 3);
    let mut N = rng.gen_range(15i32..=30) as usize;
    if let Some(fix_N) = fix_N {
        N = fix_N;
    }
    let mut M = rng.gen_range(((N * N + 9) / 10) as i32..=(N * N / 2) as i32) as usize;
    if let Some(fix_M) = fix_M {
        M = fix_M.min(N * N / 2);
    }
    let mut V = rng.gen_range(5i32..=15) as usize;
    if let Some(fix_V) = fix_V {
        V = fix_V;
    }
    let mut st;
    loop {
        st = vec![mat![false; N; N]; 2];
        for s in &mut st {
            let mut w = mat![0.0; N; N];
            let c = rng.gen_range(1..=5);
            for _ in 0..c {
                let cx = rng.gen_range(-1.0..=N as f64);
                let cy = rng.gen_range(-1.0..=N as f64);
                let a = rng.gen::<f64>();
                let sigma = rng.gen_range(2.0..=5.0);
                for i in 0..N {
                    for j in 0..N {
                        let dx = i as f64 - cx;
                        let dy = j as f64 - cy;
                        w[i][j] += a * (-(dx * dx + dy * dy) / (2.0 * sigma * sigma)).exp();
                    }
                }
            }
            let mut ps = vec![];
            for i in 0..N {
                for j in 0..N {
                    ps.push((i, j));
                }
            }
            for _ in 0..M {
                let &(i, j) = ps.choose_weighted(&mut rng, |&(i, j)| w[i][j]).unwrap();
                s[i][j] = true;
                ps.retain(|&p| p != (i, j));
            }
        }
        let mut diff = 0;
        for i in 0..N {
            for j in 0..N {
                if st[0][i][j] != st[1][i][j] {
                    diff += 1;
                }
            }
        }
        if diff >= M {
            break;
        }
    }
    Input {
        N,
        M,
        V,
        s: st[0].clone(),
        t: st[1].clone(),
    }
}

pub fn compute_score(input: &Input, out: &Output) -> (i64, String) {
    let (mut score, err, _) = compute_score_details(input, &out, out.S.len());
    if err.len() > 0 {
        score = 0;
    }
    (score, err)
}

pub const DIJ: [(i32, i32); 4] = [(0, 1), (1, 0), (0, -1), (-1, 0)];

pub struct State {
    pub N: usize,
    pub V: usize,
    pub r: (i32, i32),
    pub pL: Vec<(usize, usize)>,
    pub is_leaf: Vec<bool>,
    pub dirs: Vec<usize>,
    pub has: Vec<bool>,
    pub board: Vec<Vec<bool>>,
}

impl State {
    pub fn new(input: &Input, r: (i32, i32), pL: &Vec<(usize, usize)>) -> Self {
        let V = pL.len() + 1;
        let mut is_leaf = vec![true; V];
        for &(p, _) in pL {
            is_leaf[p] = false;
        }
        State {
            N: input.N,
            V,
            r,
            pL: pL.clone(),
            is_leaf,
            dirs: vec![0; V],
            has: vec![false; V],
            board: input.s.clone(),
        }
    }
    pub fn get(&self, mut u: usize) -> (i32, i32) {
        let mut vs = vec![];
        while u > 0 {
            let (v, l) = self.pL[u - 1];
            vs.push((self.dirs[u], l));
            u = v;
        }
        let mut p = self.r;
        let mut dir = 0;
        for &(d, l) in vs.iter().rev() {
            dir = (dir + d) % 4;
            let (dx, dy) = DIJ[dir];
            p.0 += l as i32 * dx;
            p.1 += l as i32 * dy;
        }
        p
    }
    pub fn apply(&mut self, s: &[char]) -> Result<(), String> {
        match s[0] {
            'U' => {
                self.r.0 -= 1;
                if self.r.0 < 0 {
                    return Err(format!("The root coordinate is out of range."));
                }
            }
            'D' => {
                self.r.0 += 1;
                if self.r.0 == self.N as i32 {
                    return Err(format!("The root coordinate is out of range."));
                }
            }
            'L' => {
                self.r.1 -= 1;
                if self.r.1 < 0 {
                    return Err(format!("The root coordinate is out of range."));
                }
            }
            'R' => {
                self.r.1 += 1;
                if self.r.1 == self.N as i32 {
                    return Err(format!("The root coordinate is out of range."));
                }
            }
            '.' => {}
            _ => {
                return Err(format!("Invalid operation: {}", s[0]));
            }
        }
        for i in 1..self.V {
            match s[i] {
                'L' => {
                    self.dirs[i] = (self.dirs[i] + 3) % 4;
                }
                'R' => {
                    self.dirs[i] = (self.dirs[i] + 1) % 4;
                }
                '.' => {}
                _ => {
                    return Err(format!("Invalid operation: {}", s[i]));
                }
            }
        }
        for i in 0..self.V {
            match s[self.V + i] {
                'P' => {
                    if !self.is_leaf[i] {
                        return Err(format!("The vertex {} is not a leaf.", i));
                    } else {
                        let (x, y) = self.get(i);
                        if x < 0 || y < 0 || x >= self.N as i32 || y >= self.N as i32 {
                            return Err(format!("The leaf coordinate is out of range."));
                        }
                        if self.has[i] {
                            if self.board[x as usize][y as usize] {
                                return Err(format!(
                                    "You cannot put multiple takoyaki on the same square."
                                ));
                            }
                            self.has[i] = false;
                            self.board[x as usize][y as usize] = true;
                        } else {
                            if !self.board[x as usize][y as usize] {
                                return Err(format!("({}, {}) does not contain takoyaki.", x, y));
                            }
                            self.has[i] = true;
                            self.board[x as usize][y as usize] = false;
                        }
                    }
                }
                '.' => {}
                _ => {
                    return Err(format!("Invalid operation: {}", s[self.V + i]));
                }
            }
        }
        Ok(())
    }
}

pub fn compute_score_details(input: &Input, out: &Output, t: usize) -> (i64, String, State) {
    let mut state = State::new(input, out.init, &out.pL);
    for s in &out.S[..t] {
        if let Err(err) = state.apply(s) {
            return (0, err, state);
        }
    }
    let mut M2 = 0;
    for i in 0..input.N {
        for j in 0..input.N {
            if input.t[i][j] && state.board[i][j] {
                M2 += 1;
            }
        }
    }
    let score = if M2 == input.M {
        t as i64
    } else {
        100000 + 1000 * (input.M as i64 - M2 as i64)
    };
    (score, String::new(), state)
}

/// 0 <= val <= 1
pub fn color(mut val: f64) -> String {
    val.setmin(1.0);
    val.setmax(0.0);
    let (r, g, b) = if val < 0.5 {
        let x = val * 2.0;
        (
            30. * (1.0 - x) + 144. * x,
            144. * (1.0 - x) + 255. * x,
            255. * (1.0 - x) + 30. * x,
        )
    } else {
        let x = val * 2.0 - 1.0;
        (
            144. * (1.0 - x) + 255. * x,
            255. * (1.0 - x) + 30. * x,
            30. * (1.0 - x) + 70. * x,
        )
    };
    format!(
        "#{:02x}{:02x}{:02x}",
        r.round() as i32,
        g.round() as i32,
        b.round() as i32
    )
}

pub fn rect(x: f64, y: f64, w: f64, h: f64, fill: &str) -> Rectangle {
    Rectangle::new()
        .set("x", x)
        .set("y", y)
        .set("width", w)
        .set("height", h)
        .set("fill", fill)
}

pub fn group(title: String) -> Group {
    Group::new().add(Title::new(title))
}

#[wasm_bindgen::prelude::wasm_bindgen]
#[derive(Clone, Debug, Default)]
pub struct VisResult {
    pub score: i64,
    #[wasm_bindgen(getter_with_clone)]
    pub err: String,
    #[wasm_bindgen(getter_with_clone)]
    pub vis: String,
    #[wasm_bindgen(getter_with_clone)]
    pub last: String,
    #[wasm_bindgen(getter_with_clone)]
    pub next: String,
}

pub fn vis_default(input: &Input, out: &Output) -> VisResult {
    let mut ret = vis(input, out, out.S.len() as f64, false, 1);
    if ret.err.len() > 0 {
        ret.score = 0;
    }
    ret
}

pub fn vis(input: &Input, out: &Output, tf: f64, show_number: bool, margin: usize) -> VisResult {
    let t = tf as usize;
    let delta = tf - t as f64;
    let D = 600.0 / (input.N + margin * 2) as f64;
    let W = 600.0;
    let H = 600.0;
    let (score, err, state) = compute_score_details(input, &out, t);
    let mut doc = svg::Document::new()
        .set("id", "vis")
        .set("viewBox", (-5.0, -5.0, W + 10.0, H + 10.0))
        .set("width", W + 10.0)
        .set("height", H + 10.0)
        .set("style", "background-color:white");
    doc = doc.add(Style::new(format!(
        "text {{text-anchor: middle;dominant-baseline: central;}}"
    )));
    for i in 0..input.N {
        for j in 0..input.N {
            let mut g = group(format!("({},{})", i, j));
            if input.t[i][j] {
                g = g.add(
                    rect(
                        (j + margin) as f64 * D,
                        (i + margin) as f64 * D,
                        D,
                        D,
                        "pink",
                    )
                    .set("stroke", "black")
                    .set("stroke-width", 1),
                );
            } else {
                g = g.add(
                    rect(
                        (j + margin) as f64 * D,
                        (i + margin) as f64 * D,
                        D,
                        D,
                        "white",
                    )
                    .set("stroke", "black")
                    .set("stroke-width", 1),
                );
            }
            if state.board[i][j] {
                g = g.add(
                    Circle::new()
                        .set("cx", (j + margin) as f64 * D + D / 2.0)
                        .set("cy", (i + margin) as f64 * D + D / 2.0)
                        .set("r", D / 3.0)
                        .set("fill", "saddlebrown"),
                );
            }
            doc = doc.add(g);
        }
    }
    let mut ps;
    if delta > 0.0 && t < out.S.len() {
        let s = &out.S[t];
        let mut r = (state.r.0 as f64, state.r.1 as f64);
        let mut dirs = state.dirs.iter().map(|&d| d as f64).collect_vec();
        match s[0] {
            'U' => {
                r.0 -= delta;
            }
            'D' => {
                r.0 += delta;
            }
            'L' => {
                r.1 -= delta;
            }
            'R' => {
                r.1 += delta;
            }
            _ => {}
        }
        for i in 1..state.V {
            match s[i] {
                'L' => {
                    dirs[i] -= delta;
                }
                'R' => {
                    dirs[i] += delta;
                }
                _ => {}
            }
        }
        ps = vec![];
        for mut u in 0..state.V {
            let mut vs = vec![];
            while u > 0 {
                let (v, l) = state.pL[u - 1];
                vs.push((dirs[u], l));
                u = v;
            }
            let mut p = r;
            let mut dir = 0.0;
            for &(d, l) in vs.iter().rev() {
                dir += d;
                let (dx, dy) = (dir * std::f64::consts::PI / 2.0).sin_cos();
                p.0 += l as f64 * dx;
                p.1 += l as f64 * dy;
            }
            ps.push(p);
        }
    } else {
        ps = (0..state.V)
            .map(|u| state.get(u))
            .map(|p| (p.0 as f64, p.1 as f64))
            .collect_vec();
    }
    let margin = margin as f64;
    for u in 0..state.V {
        if state.has[u] {
            doc = doc.add(
                group(format!("vertex {}", u)).add(
                    Circle::new()
                        .set("cx", (ps[u].1 + margin) * D + D / 2.0)
                        .set("cy", (ps[u].0 + margin) * D + D / 2.0)
                        .set("r", D / 3.0)
                        .set("fill", "limegreen"),
                ),
            );
        }
    }
    for u in 1..state.V {
        let p = state.pL[u - 1].0;
        doc = doc.add(
            group(format!("edge ({}, {})", p, u)).add(
                Line::new()
                    .set("x1", (ps[p].1 + margin) * D + D / 2.0)
                    .set("y1", (ps[p].0 + margin) * D + D / 2.0)
                    .set("x2", (ps[u].1 + margin) * D + D / 2.0)
                    .set("y2", (ps[u].0 + margin) * D + D / 2.0)
                    .set("stroke", "gray")
                    .set("stroke-width", 2),
            ),
        );
    }
    for u in 0..state.V {
        let mut g = group(format!("vertex {}", u));
        if show_number {
            g = g.add(
                svg::node::element::Text::new(format!("{}", u))
                    .set("x", (ps[u].1 + margin) * D + D / 2.0)
                    .set("y", (ps[u].0 + margin) * D + D / 2.0)
                    .set("font-size", D * 2.0 / 3.0)
                    .set("fill", "black"),
            );
        } else {
            g = g.add(
                Circle::new()
                    .set("cx", (ps[u].1 + margin) * D + D / 2.0)
                    .set("cy", (ps[u].0 + margin) * D + D / 2.0)
                    .set("r", if u == 0 { 4 } else { 3 })
                    .set("fill", if u == 0 { "red" } else { "black" }),
            );
        }
        doc = doc.add(g);
    }
    VisResult {
        score,
        err,
        vis: doc.to_string(),
        last: if t == 0 {
            String::new()
        } else {
            out.S[t - 1].iter().collect()
        },
        next: if t < out.S.len() {
            out.S[t].iter().collect()
        } else {
            String::new()
        },
    }
}
