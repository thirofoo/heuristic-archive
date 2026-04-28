#![allow(non_snake_case)]

use rand::prelude::*;
use serde::Serialize;
use wasm_bindgen::prelude::*;

#[wasm_bindgen(start)]
pub fn init_panic_hook() {
    console_error_panic_hook::set_once();
}

// === Helpers ===

macro_rules! mat {
    ($($e:expr),*) => { Vec::from(vec![$($e),*]) };
    ($($e:expr,)*) => { Vec::from(vec![$($e),*]) };
    ($e:expr; $d:expr) => { Vec::from(vec![$e; $d]) };
    ($e:expr; $d:expr $(; $ds:expr)+) => { Vec::from(vec![mat![$e $(; $ds)*]; $d]) };
}

const DIJ: [(usize, usize); 4] = [(!0, 0), (0, 1), (1, 0), (0, !0)];
const DIR_CHARS: [char; 4] = ['U', 'R', 'D', 'L'];

fn has_wall(wall_v: &[Vec<u8>], wall_h: &[Vec<u8>], i: usize, j: usize, d: usize) -> bool {
    let N = wall_v.len();
    let i2 = i.wrapping_add(DIJ[d].0);
    let j2 = j.wrapping_add(DIJ[d].1);
    if i2 >= N || j2 >= N {
        return true;
    }
    if i == i2 {
        wall_v[i][j.min(j2)] == 1
    } else {
        wall_h[i.min(i2)][j] == 1
    }
}

// === Data structures ===

#[derive(Clone, Debug)]
struct Input {
    N: usize,
    AK: i64,
    AM: i64,
    AW: i64,
    wall_v: Vec<Vec<u8>>,
    wall_h: Vec<Vec<u8>>,
}

#[derive(Clone, Debug)]
struct Robot {
    m: usize,
    i: usize,
    j: usize,
    d: usize,
    a0: Vec<char>,
    b0: Vec<usize>,
    a1: Vec<char>,
    b1: Vec<usize>,
}

struct Output {
    robots: Vec<Robot>,
    wall_v: Vec<Vec<u8>>,
    wall_h: Vec<Vec<u8>>,
}

// === Parsing ===

fn parse_input_str(f: &str) -> Input {
    let mut iter = f.split_whitespace();
    let N: usize = iter.next().unwrap().parse().unwrap();
    let AK: i64 = iter.next().unwrap().parse().unwrap();
    let AM: i64 = iter.next().unwrap().parse().unwrap();
    let AW: i64 = iter.next().unwrap().parse().unwrap();

    let mut wall_v = Vec::new();
    for _ in 0..N {
        let line = iter.next().unwrap();
        wall_v.push(line.bytes().map(|b| b - b'0').collect());
    }

    let mut wall_h = Vec::new();
    for _ in 0..N - 1 {
        let line = iter.next().unwrap();
        wall_h.push(line.bytes().map(|b| b - b'0').collect());
    }

    Input {
        N,
        AK,
        AM,
        AW,
        wall_v,
        wall_h,
    }
}

fn format_input(input: &Input) -> String {
    let mut s = format!("{} {} {} {}\n", input.N, input.AK, input.AM, input.AW);
    for row in &input.wall_v {
        for &v in row {
            s.push(if v == 1 { '1' } else { '0' });
        }
        s.push('\n');
    }
    for row in &input.wall_h {
        for &v in row {
            s.push(if v == 1 { '1' } else { '0' });
        }
        s.push('\n');
    }
    s
}

fn parse_output_str(input: &Input, f: &str) -> Result<Output, String> {
    let mut iter = f.split_whitespace();

    let K: usize = iter
        .next()
        .ok_or("Unexpected EOF")?
        .parse()
        .map_err(|_| "Failed to parse K".to_string())?;
    if K < 1 || K > input.N * input.N {
        return Err(format!("K out of range: {}", K));
    }

    let mut robots = Vec::new();
    for _ in 0..K {
        let m: usize = iter
            .next()
            .ok_or("Unexpected EOF")?
            .parse()
            .map_err(|_| "Failed to parse m".to_string())?;
        if m < 1 || m > 4 * input.N * input.N {
            return Err(format!("m out of range: {}", m));
        }
        let i: usize = iter
            .next()
            .ok_or("Unexpected EOF")?
            .parse()
            .map_err(|_| "Failed to parse i".to_string())?;
        let j: usize = iter
            .next()
            .ok_or("Unexpected EOF")?
            .parse()
            .map_err(|_| "Failed to parse j".to_string())?;
        if i >= input.N || j >= input.N {
            return Err(format!("Position out of range: ({}, {})", i, j));
        }
        let d_str: String = iter
            .next()
            .ok_or("Unexpected EOF")?
            .to_string();
        let d_char = d_str.chars().next().unwrap();
        let d = DIR_CHARS
            .iter()
            .position(|&c| c == d_char)
            .ok_or_else(|| format!("Invalid direction: {}", d_char))?;

        let mut a0 = Vec::new();
        let mut b0 = Vec::new();
        let mut a1 = Vec::new();
        let mut b1 = Vec::new();

        for _ in 0..m {
            let a: char = iter
                .next()
                .ok_or("Unexpected EOF")?
                .chars()
                .next()
                .unwrap();
            if !['R', 'L', 'F'].contains(&a) {
                return Err(format!("Invalid action: {}", a));
            }
            a0.push(a);
            let b: usize = iter
                .next()
                .ok_or("Unexpected EOF")?
                .parse()
                .map_err(|_| "Failed to parse b0".to_string())?;
            if b >= m {
                return Err(format!("b0 out of range: {}", b));
            }
            b0.push(b);

            let a: char = iter
                .next()
                .ok_or("Unexpected EOF")?
                .chars()
                .next()
                .unwrap();
            if !['R', 'L'].contains(&a) {
                return Err(format!("Invalid wall action: {}", a));
            }
            a1.push(a);
            let b: usize = iter
                .next()
                .ok_or("Unexpected EOF")?
                .parse()
                .map_err(|_| "Failed to parse b1".to_string())?;
            if b >= m {
                return Err(format!("b1 out of range: {}", b));
            }
            b1.push(b);
        }

        robots.push(Robot {
            m,
            i,
            j,
            d,
            a0,
            b0,
            a1,
            b1,
        });
    }

    let mut wall_v = Vec::new();
    for _ in 0..input.N {
        let line = iter.next().ok_or("Unexpected EOF (wall_v)")?;
        let line = line.trim();
        if line.len() != input.N - 1 {
            return Err(format!(
                "Invalid wall_v line length: {} (expected {})",
                line.len(),
                input.N - 1
            ));
        }
        wall_v.push(line.bytes().map(|b| b - b'0').collect());
    }

    let mut wall_h = Vec::new();
    for _ in 0..input.N - 1 {
        let line = iter.next().ok_or("Unexpected EOF (wall_h)")?;
        let line = line.trim();
        if line.len() != input.N {
            return Err(format!(
                "Invalid wall_h line length: {} (expected {})",
                line.len(),
                input.N
            ));
        }
        wall_h.push(line.bytes().map(|b| b - b'0').collect());
    }

    Ok(Output {
        robots,
        wall_v,
        wall_h,
    })
}

// === Generation ===

fn gen_input(seed: u64, problem: &str) -> Input {
    let mut rng = rand_chacha::ChaCha20Rng::seed_from_u64(seed);
    let N = 20usize;
    let (AK, AM, AW): (i64, i64, i64) = match problem {
        "A" => (0, 1, 1000),
        "B" => (1000, rng.gen_range(1..=10), rng.gen_range(1..=10)),
        "C" => (1000, 1, 1000),
        _ => panic!("invalid problem id: {}", problem),
    };
    let X = rng.gen_range(1..=6i32);
    let Y = rng.gen_range(1..=6i32);
    loop {
        let mut wall_v = mat![0u8; N; N - 1];
        let mut wall_h = mat![0u8; N - 1; N];
        for _ in 0..X {
            let dir = rng.gen_range(0..2i32);
            let L = rng.gen_range(5..=15i32) as usize;
            let i = rng.gen_range(0..N as i32) as usize;
            let j = rng.gen_range(0..N as i32 - 1) as usize;
            if dir == 0 {
                for i in (i + 1).saturating_sub(L)..=i {
                    wall_v[i][j] = 1;
                }
            } else {
                for i in i..=(i + L - 1).min(N - 1) {
                    wall_v[i][j] = 1;
                }
            }
        }
        for _ in 0..Y {
            let dir = rng.gen_range(0..2i32);
            let L = rng.gen_range(5..=15i32) as usize;
            let i = rng.gen_range(0..N as i32 - 1) as usize;
            let j = rng.gen_range(0..N as i32) as usize;
            if dir == 0 {
                for j in (j + 1).saturating_sub(L)..=j {
                    wall_h[i][j] = 1;
                }
            } else {
                for j in j..=(j + L - 1).min(N - 1) {
                    wall_h[i][j] = 1;
                }
            }
        }
        // Connectivity check
        let mut visited = mat![false; N; N];
        let mut stack = vec![(0usize, 0usize)];
        visited[0][0] = true;
        let mut num = 0;
        while let Some((i, j)) = stack.pop() {
            num += 1;
            for d in 0..4 {
                if !has_wall(&wall_v, &wall_h, i, j, d) {
                    let i2 = i.wrapping_add(DIJ[d].0);
                    let j2 = j.wrapping_add(DIJ[d].1);
                    if i2 < N && j2 < N && !visited[i2][j2] {
                        visited[i2][j2] = true;
                        stack.push((i2, j2));
                    }
                }
            }
        }
        if num == N * N {
            return Input {
                N,
                AK,
                AM,
                AW,
                wall_v,
                wall_h,
            };
        }
    }
}

// === Serialization structures ===

#[derive(Serialize)]
struct WasmInput {
    raw: String,
    n: usize,
    ak: i64,
    am: i64,
    aw: i64,
    wall_v: Vec<Vec<u8>>,
    wall_h: Vec<Vec<u8>>,
}

#[derive(Serialize, Clone)]
struct RobotPos {
    i: usize,
    j: usize,
    d: usize,
    s: usize,
}

#[derive(Serialize)]
struct StepState {
    step: usize,
    robots: Vec<RobotPos>,
}

#[derive(Serialize)]
struct RouteInfo {
    head: Vec<[usize; 3]>,
    tail: Vec<[usize; 3]>,
}

#[derive(Serialize)]
struct SimResult {
    score: i64,
    error: Option<String>,
    states: Vec<StepState>,
    n: usize,
    ak: i64,
    am: i64,
    aw: i64,
    wall_v: Vec<Vec<u8>>,
    wall_h: Vec<Vec<u8>>,
    wall_v_orig: Vec<Vec<u8>>,
    wall_h_orig: Vec<Vec<u8>>,
    wall_v_new: Vec<Vec<u8>>,
    wall_h_new: Vec<Vec<u8>>,
    patrolled: Vec<Vec<bool>>,
    num_robots: usize,
    total_states: usize,
    num_new_walls: usize,
    cost: i64,
    routes: Vec<RouteInfo>,
}

// === Route computation ===

struct RouteData {
    head: Vec<(usize, usize, usize, usize)>, // (i, j, d, s)
    tail: Vec<(usize, usize, usize, usize)>,
}

fn compute_routes(robots: &[Robot], wall_v: &[Vec<u8>], wall_h: &[Vec<u8>], n: usize) -> Vec<RouteData> {
    let mut routes = Vec::new();

    for robot in robots {
        let mut visited = mat![!0usize; n; n; 4; robot.m];
        let mut route: Vec<(usize, usize, usize, usize)> = Vec::new();
        let mut i = robot.i;
        let mut j = robot.j;
        let mut d = robot.d;
        let mut s = 0usize;

        loop {
            route.push((i, j, d, s));
            let t = route.len() - 1;

            if visited[i][j][d][s] != !0 {
                let c = visited[i][j][d][s];
                let head = route[..=c].to_vec();
                let tail = route[c..].to_vec();
                routes.push(RouteData { head, tail });
                break;
            }
            visited[i][j][d][s] = t;

            let wall = has_wall(wall_v, wall_h, i, j, d);
            let (a, b) = if wall {
                (robot.a1[s], robot.b1[s])
            } else {
                (robot.a0[s], robot.b0[s])
            };

            match a {
                'R' => d = (d + 1) % 4,
                'L' => d = (d + 3) % 4,
                'F' => {
                    i = i.wrapping_add(DIJ[d].0);
                    j = j.wrapping_add(DIJ[d].1);
                }
                _ => unreachable!(),
            }
            s = b;
        }
    }

    routes
}

fn position_at(route: &RouteData, t: usize) -> (usize, usize, usize, usize) {
    if t < route.head.len() {
        route.head[t]
    } else if route.tail.len() > 1 {
        let cycle_len = route.tail.len() - 1;
        let idx = (t - route.head.len() + 1) % cycle_len;
        route.tail[idx]
    } else {
        route.tail[0]
    }
}

// === WASM Exports ===

#[wasm_bindgen]
pub fn parse_input(input_text: &str) -> JsValue {
    let input = parse_input_str(input_text);
    let wasm_input = WasmInput {
        raw: input_text.to_string(),
        n: input.N,
        ak: input.AK,
        am: input.AM,
        aw: input.AW,
        wall_v: input.wall_v,
        wall_h: input.wall_h,
    };
    serde_wasm_bindgen::to_value(&wasm_input).unwrap()
}

#[wasm_bindgen]
pub fn generate(seed: u64, problem_type: &str) -> JsValue {
    let input = gen_input(seed, problem_type);
    let raw = format_input(&input);
    let wasm_input = WasmInput {
        raw,
        n: input.N,
        ak: input.AK,
        am: input.AM,
        aw: input.AW,
        wall_v: input.wall_v,
        wall_h: input.wall_h,
    };
    serde_wasm_bindgen::to_value(&wasm_input).unwrap()
}

#[wasm_bindgen]
pub fn simulate(input_text: &str, output_text: &str) -> JsValue {
    let input = parse_input_str(input_text);
    let out = match parse_output_str(&input, output_text) {
        Ok(o) => o,
        Err(e) => {
            let result = SimResult {
                score: 0,
                error: Some(e),
                states: vec![],
                n: input.N,
                ak: input.AK,
                am: input.AM,
                aw: input.AW,
                wall_v: input.wall_v.clone(),
                wall_h: input.wall_h.clone(),
                wall_v_orig: input.wall_v,
                wall_h_orig: input.wall_h,
                wall_v_new: vec![],
                wall_h_new: vec![],
                patrolled: mat![false; 0; 0],
                num_robots: 0,
                total_states: 0,
                num_new_walls: 0,
                cost: 0,
                routes: vec![],
            };
            return serde_wasm_bindgen::to_value(&result).unwrap();
        }
    };

    // Combine walls (original OR new)
    let mut combined_wall_v = input.wall_v.clone();
    let mut combined_wall_h = input.wall_h.clone();
    for i in 0..input.N {
        for j in 0..input.N - 1 {
            if out.wall_v[i][j] == 1 {
                combined_wall_v[i][j] = 1;
            }
        }
    }
    for i in 0..input.N - 1 {
        for j in 0..input.N {
            if out.wall_h[i][j] == 1 {
                combined_wall_h[i][j] = 1;
            }
        }
    }

    // Compute routes
    let routes = compute_routes(&out.robots, &combined_wall_v, &combined_wall_h, input.N);

    // Determine patrolled cells (only from tail/periodic part)
    let mut patrolled = mat![false; input.N; input.N];
    for route in &routes {
        for &(i, j, _, _) in &route.tail {
            patrolled[i][j] = true;
        }
    }

    // Check if all cells are patrolled
    let all_patrolled = patrolled
        .iter()
        .all(|row| row.iter().all(|&v| v));
    let patrolled_count: usize = patrolled
        .iter()
        .flat_map(|r| r.iter())
        .filter(|&&v| v)
        .count();

    // Compute costs
    let K = out.robots.len();
    let M: usize = out.robots.iter().map(|r| r.m).sum();
    let W: usize = out
        .wall_v
        .iter()
        .flat_map(|r| r.iter())
        .filter(|&&c| c == 1)
        .count()
        + out
            .wall_h
            .iter()
            .flat_map(|r| r.iter())
            .filter(|&&c| c == 1)
            .count();
    let V = input.AK * (K as i64 - 1) + input.AM * M as i64 + input.AW * W as i64;

    // Compute score
    let (score, error) = if !all_patrolled {
        (
            0i64,
            Some(format!(
                "Not all cells are patrolled: {}/{}",
                patrolled_count,
                input.N * input.N
            )),
        )
    } else {
        let base =
            input.AK * (input.N * input.N - 1) as i64 + input.AM * (input.N * input.N) as i64;
        let mut score = 1i64;
        if V < base {
            score += (1e6 * (base as f64 / V as f64).log2()).round() as i64;
        }
        (score, None)
    };

    // Generate step states
    let max_step = routes
        .iter()
        .map(|r| r.head.len() + r.tail.len())
        .max()
        .unwrap_or(0)
        .min(5000);

    let mut states = Vec::with_capacity(max_step);
    for step in 0..max_step {
        let mut robot_positions = Vec::with_capacity(routes.len());
        for route in &routes {
            let (i, j, d, s) = position_at(route, step);
            robot_positions.push(RobotPos { i, j, d, s });
        }
        states.push(StepState {
            step,
            robots: robot_positions,
        });
    }

    let route_infos: Vec<RouteInfo> = routes
        .iter()
        .map(|r| RouteInfo {
            head: r.head.iter().map(|&(i, j, d, _)| [i, j, d]).collect(),
            tail: r.tail.iter().map(|&(i, j, d, _)| [i, j, d]).collect(),
        })
        .collect();

    let result = SimResult {
        score,
        error,
        states,
        n: input.N,
        ak: input.AK,
        am: input.AM,
        aw: input.AW,
        wall_v: combined_wall_v,
        wall_h: combined_wall_h,
        wall_v_orig: input.wall_v,
        wall_h_orig: input.wall_h,
        wall_v_new: out.wall_v,
        wall_h_new: out.wall_h,
        patrolled,
        num_robots: K,
        total_states: M,
        num_new_walls: W,
        cost: V,
        routes: route_infos,
    };

    serde_wasm_bindgen::to_value(&result).unwrap()
}
