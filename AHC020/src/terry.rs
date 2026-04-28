use itertools::{izip, Itertools};
use lib::{acl::dsu::Dsu, is_covered, parse_input, ChangeMinMax, Input, Output};
use rand::Rng;
use rand_pcg::Pcg64Mcg;
use std::{cmp::Reverse, collections::BinaryHeap};

const MAX_POWER: i32 = 5000;

/// 色々改善した焼きなまし
fn main() {
    let input = parse_input();
    let mut env = Environment::new(&input, false, 0);
    let state = generate_initial_solution(&env);
    let mut state = annealing(&env, state, 1.5, 3e6, 3e5, 1000.0, 200.0);
    env.consider_edges = true;

    // 一旦連結にする
    for i in 0..input.station_count {
        if state.powers[i] == 0 {
            state.change_power(&env, i, 1);
        }
    }

    let state = annealing(&env, state, 0.45, 3e5, 1e4, 200.0, 10.0);
    let output = Output::new(state.powers.clone(), state.get_edges(&env).unwrap(), &input);

    println!("{}", output);
    eprintln!("score: {}", output.calc_score(&input));
}

fn generate_initial_solution(env: &Environment) -> State {
    let input = &env.input;

    let mut dsu = Dsu::new(input.station_count);
    let mut edges = vec![];

    for &i in &env.edge_candidates {
        let (u, v, _) = input.edges[i];

        if !dsu.same(u, v) {
            dsu.merge(u, v);
            edges.push((u, v));
        }
    }

    let powers = vec![power_binary_search(&input); input.station_count];
    State::new(powers, env)
}

fn power_binary_search(input: &Input) -> i32 {
    let mut ok = MAX_POWER;
    let mut ng = -1;

    while (ok - ng).abs() > 1 {
        let mid = (ok + ng) / 2;
        let mut broadcasted = vec![false; input.resident_count];

        for i in 0..input.resident_count {
            broadcasted[i] = input
                .stations
                .iter()
                .any(|b| is_covered(&input.residents[i], b, mid));
        }

        if broadcasted.iter().all(|&b| b) {
            ok = mid;
        } else {
            ng = mid;
        }
    }

    ok
}

#[derive(Debug, Clone)]
struct Environment<'a> {
    input: &'a Input,
    points: Vec<Vec<(i32, usize)>>,
    consider_edges: bool,
    edge_candidates: Vec<usize>,
    approx_edge_cost: Vec<i64>,
    output_interval: usize,
}

impl<'a> Environment<'a> {
    fn new(input: &'a Input, consider_edges: bool, output_interval: usize) -> Self {
        Self {
            input,
            points: Self::generate_points_vec(input),
            consider_edges,
            edge_candidates: Self::generate_edge_candidates(input),
            approx_edge_cost: Self::generate_approx_edge_costs(input),
            output_interval,
        }
    }

    fn generate_points_vec(input: &Input) -> Vec<Vec<(i32, usize)>> {
        let mut result = vec![];

        for j in 0..input.station_count {
            let mut points = vec![];

            for i in 0..input.resident_count {
                let dist_sq = input.stations[j].calc_sq_dist(&input.residents[i]);
                points.push((dist_sq, i));
            }

            points.sort_unstable();
            result.push(points);
        }

        result
    }

    fn generate_edge_candidates(input: &Input) -> Vec<usize> {
        let mut all_edges = (0..input.edge_count)
            .map(|i| (input.edges[i].2, i))
            .collect_vec();

        all_edges.sort_unstable();
        all_edges.iter().map(|&(_, i)| i).collect_vec()
    }

    fn generate_approx_edge_costs(input: &Input) -> Vec<i64> {
        let mut graph = vec![vec![]; input.station_count];

        for &(u, v, w) in &input.edges {
            graph[u].push((v, w));
            graph[v].push((u, w));
        }

        let mut dists = vec![std::i64::MAX / 2; input.station_count];
        let mut approx_costs = vec![0; input.station_count];
        let mut queue = BinaryHeap::new();
        dists[0] = 0;
        queue.push(Reverse((0, 0)));

        while let Some(Reverse((cost, v))) = queue.pop() {
            if cost > dists[v] {
                continue;
            }

            for &(next, w) in &graph[v] {
                let next_cost = cost + w as i64;

                if dists[next].change_min(next_cost) {
                    queue.push(Reverse((next_cost, next)));

                    // 使った辺のコストを近似コストとする
                    approx_costs[next] = w as i64;
                }
            }
        }

        approx_costs
    }
}

#[derive(Debug, Clone)]
struct State {
    powers: Vec<i32>,
    covered_count: Vec<usize>,
    covering_count: Vec<usize>,
    not_covered_count: usize,
    power_cost_sum: i64,
    approx_edge_cost_sum: i64,
    edge_cost: i64,
}

impl State {
    fn new(powers: Vec<i32>, env: &Environment) -> Self {
        let mut covered_count = vec![0; env.input.resident_count];

        for (i, h) in env.input.residents.iter().enumerate() {
            covered_count[i] = env
                .input
                .stations
                .iter()
                .zip(powers.iter())
                .filter(|(b, &p)| is_covered(h, b, p))
                .count();
        }

        let mut covering_count = vec![];

        for j in 0..env.input.station_count {
            let mut i = 0;
            let p2 = powers[j] * powers[j];

            while i < env.points[j].len() && p2 >= env.points[j][i].0 {
                i += 1;
            }

            covering_count.push(i);
        }

        let not_covered_count = covered_count.iter().filter(|&&i| i == 0).count();
        let power_cost_sum = powers.iter().map(|&p| (p * p) as i64).sum();
        let approx_edge_cost_sum = izip!(powers.iter(), env.approx_edge_cost.iter())
            .map(|(&p, &c)| if p > 0 { c } else { 0 })
            .sum();

        let mut state = Self {
            powers,
            covered_count,
            covering_count,
            not_covered_count,
            power_cost_sum,
            approx_edge_cost_sum,
            edge_cost: 0,
        };
        state.recalc_edge_cost(env);

        state
    }

    fn change_power(&mut self, env: &Environment, index: usize, new_power: i32) {
        let count = &mut self.covering_count[index];
        let points = &env.points[index];
        let old_power = self.powers[index];
        let p2 = new_power * new_power;
        self.power_cost_sum += p2 as i64 - (old_power * old_power) as i64;

        // 減らす
        while *count > 0 && points[*count - 1].0 > p2 {
            Self::decrease_covered_count(
                &mut self.covered_count,
                &mut self.not_covered_count,
                points[*count - 1].1,
            );
            *count -= 1;
        }

        // 増やす
        while *count < env.input.resident_count && points[*count].0 <= p2 {
            Self::increase_covered_count(
                &mut self.covered_count,
                &mut self.not_covered_count,
                points[*count].1,
            );

            *count += 1;
        }

        if old_power == 0 && new_power > 0 {
            self.approx_edge_cost_sum += env.approx_edge_cost[index];
            self.edge_cost = -1;
        } else if old_power > 0 && new_power == 0 {
            self.approx_edge_cost_sum -= env.approx_edge_cost[index];
            self.edge_cost = -1;
        }

        self.powers[index] = new_power;
    }

    fn decrease_covered_count(
        covered_count: &mut [usize],
        not_covered_count: &mut usize,
        v: usize,
    ) {
        covered_count[v] -= 1;

        if covered_count[v] == 0 {
            *not_covered_count += 1;
        }
    }

    fn increase_covered_count(
        covered_count: &mut [usize],
        not_covered_count: &mut usize,
        v: usize,
    ) {
        if covered_count[v] == 0 {
            *not_covered_count -= 1;
        }

        covered_count[v] += 1;
    }

    fn calc_score(&mut self, env: &Environment) -> i64 {
        if self.not_covered_count > 0 {
            return std::i64::MAX / 2;
        }

        let mut score = self.power_cost_sum;

        if env.consider_edges {
            if self.edge_cost == -1 {
                self.recalc_edge_cost(env);
            }

            score += self.edge_cost;
        } else {
            score += self.approx_edge_cost_sum;
        }

        score
    }

    fn recalc_edge_cost(&mut self, env: &Environment) {
        self.edge_cost = self.calc_edge_cost(env);
    }

    fn calc_edge_cost(&self, env: &Environment) -> i64 {
        if let Ok(edges) = self.get_edges(env) {
            let mut score = 0;

            for i in edges {
                let (_, _, w) = env.input.edges[i];
                score += w as i64;
            }

            score
        } else {
            std::i64::MAX / 2
        }
    }

    fn get_edges(&self, env: &Environment) -> Result<Vec<usize>, Vec<usize>> {
        let mut dsu = Dsu::new(env.input.station_count);
        let mut edges = vec![];

        for &i in &env.edge_candidates {
            let (u, v, _) = env.input.edges[i];

            if (u != 0 && self.powers[u] == 0) || self.powers[v] == 0 {
                continue;
            }

            if !dsu.same(u, v) {
                dsu.merge(u, v);
                edges.push(i);
            }
        }

        let connected = (0..env.input.station_count).all(|i| self.powers[i] == 0 || dsu.same(0, i));

        if connected {
            Ok(edges)
        } else {
            Err(edges)
        }
    }
}

trait Action {
    fn apply(&self, env: &Environment, state: &mut State);
    fn rollback(&self, env: &Environment, state: &mut State);
}

#[derive(Debug, Clone, Copy)]
struct ChangePower {
    index: usize,
    old_power: i32,
    new_power: i32,
    old_edge_cost: i64,
}

impl ChangePower {
    fn new(index: usize, old_power: i32, new_power: i32, old_edge_cost: i64) -> Self {
        Self {
            index,
            old_power,
            new_power,
            old_edge_cost,
        }
    }
}

impl Action for ChangePower {
    fn apply(&self, env: &Environment, state: &mut State) {
        state.change_power(env, self.index, self.new_power);
    }

    fn rollback(&self, env: &Environment, state: &mut State) {
        state.change_power(env, self.index, self.old_power);
        state.edge_cost = self.old_edge_cost;
    }
}

fn generate_action(
    env: &Environment,
    state: &State,
    rng: &mut Pcg64Mcg,
    r0: f64,
    r1: f64,
    t: f64,
) -> Box<dyn Action> {
    
    let range = (r0 * (1.0 - t) + r1 * t).round() as i32;

    loop {
        let index = rng.gen_range(0, env.input.station_count);
        let power_diff = rng.gen_range(-range, range + 1);
        let new_power = (state.powers[index] + power_diff).max(0).min(MAX_POWER);

        if state.powers[index] == new_power {
            continue;
        }

        return Box::new(ChangePower::new(
            index,
            state.powers[index],
            new_power,
            state.edge_cost,
        ));
    }
}

fn annealing(
    env: &Environment,
    initial_solution: State,
    duration: f64,
    temp0: f64,
    temp1: f64,
    r0: f64,
    r1: f64,
) -> State {
    let mut solution = initial_solution;
    let mut best_solution = solution.clone();
    let mut current_score = solution.calc_score(env);
    let mut best_score = current_score;
    let init_score = current_score;

    let mut all_iter = 0;
    let mut valid_iter = 0;
    let mut accepted_count = 0;
    let mut update_count = 0;
    let mut rng = rand_pcg::Pcg64Mcg::new(42);

    let duration_inv = 1.0 / duration;
    let since = std::time::Instant::now();

    let mut inv_temp = 1.0 / temp0;
    let mut time = 0.0;

    loop {
        if env.output_interval > 0 && all_iter % env.output_interval == 0 {
            let edges = match solution.get_edges(&env) {
                Ok(edges) => edges,
                Err(edges) => edges,
            };
            let output = Output::new(solution.powers.clone(), edges, &env.input);
            println!("{}", output);
        }

        all_iter += 1;
        if (all_iter & ((1 << 10) - 1)) == 0 {
            time = (std::time::Instant::now() - since).as_secs_f64() * duration_inv;
            let temp = f64::powf(temp0, 1.0 - time) * f64::powf(temp1, time);
            inv_temp = 1.0 / temp;

            if time >= 1.0 {
                break;
            }
        }

        // 変形
        let action = generate_action(env, &solution, &mut rng, r0, r1, time);
        action.apply(env, &mut solution);

        // スコア計算
        let new_score = solution.calc_score(env);
        let score_diff = new_score - current_score;

        if score_diff <= 0 || rng.gen_bool(f64::exp(-score_diff as f64 * inv_temp)) {
            // 解の更新
            current_score = new_score;
            accepted_count += 1;

            if best_score.change_min(current_score) {
                best_solution = solution.clone();
                update_count += 1;
            }
        } else {
            action.rollback(env, &mut solution);
        }

        valid_iter += 1;
    }

    eprintln!("===== annealing =====");
    eprintln!("init score : {}", init_score);
    eprintln!("score      : {}", best_score);
    eprintln!("all iter   : {}", all_iter);
    eprintln!("valid iter : {}", valid_iter);
    eprintln!("accepted   : {}", accepted_count);
    eprintln!("updated    : {}", update_count);
    eprintln!("");

    best_solution
}

mod lib {
    use acl::dsu::Dsu;
    use itertools::Itertools;
    use proconio::{input, marker::Usize1};
    use std::fmt::Display;

    pub trait ChangeMinMax {
        fn change_min(&mut self, v: Self) -> bool;
        fn change_max(&mut self, v: Self) -> bool;
    }

    impl<T: PartialOrd> ChangeMinMax for T {
        fn change_min(&mut self, v: T) -> bool {
            *self > v && {
                *self = v;
                true
            }
        }

        fn change_max(&mut self, v: T) -> bool {
            *self < v && {
                *self = v;
                true
            }
        }
    }

    #[derive(Clone, Debug)]
    pub struct Input {
        pub station_count: usize,
        pub edge_count: usize,
        pub resident_count: usize,
        pub stations: Vec<Point>,
        pub edges: Vec<(usize, usize, i32)>,
        pub residents: Vec<Point>,
    }

    #[derive(Clone, Debug)]
    pub struct Output<'a> {
        pub powers: Vec<i32>,
        pub edge_count: usize,
        pub edges: Vec<usize>,
        input: &'a Input,
    }

    impl<'a> Output<'a> {
        pub fn new(powers: Vec<i32>, edges: Vec<usize>, input: &'a Input) -> Self {
            Self {
                powers,
                edge_count: edges.len(),
                edges,
                input,
            }
        }

        pub fn calc_score(&self, input: &Input) -> i64 {
            let is_connected = self.get_connection_status(input);
            let broadcasted = self.get_broadcasted_count(input, &is_connected);
            if broadcasted < input.resident_count {
                return broadcasted as i64;
            }

            self.calc_cost(input)
        }

        fn get_connection_status(&self, input: &Input) -> Vec<bool> {
            let mut dsu = Dsu::new(input.station_count);

            for &i in &self.edges {
                let (u, v, _) = input.edges[i];
                dsu.merge(u, v);
            }

            (0..input.station_count).map(|i| dsu.same(0, i)).collect()
        }

        fn get_broadcasted_count(&self, input: &Input, is_connected: &[bool]) -> usize {
            let mut broadcasted = vec![false; input.resident_count];

            for i in 0..input.station_count {
                if !is_connected[i] {
                    continue;
                }

                for j in 0..input.resident_count {
                    let dist_sq = input.stations[i].calc_sq_dist(&input.residents[j]);
                    let power = self.powers[i];
                    broadcasted[j] |= dist_sq <= power * power;
                }
            }

            broadcasted.iter().filter(|&&b| b).count()
        }

        fn calc_cost(&self, input: &Input) -> i64 {
            let mut broadcast_cost = 0;

            for &p in &self.powers {
                let p = p as i64;
                broadcast_cost += p * p;
            }

            let mut edge_cost = 0;

            for &i in &self.edges {
                let (_, _, w) = input.edges[i];
                edge_cost += w as i64;
            }

            eprintln!("broadcast_cost: {:>10}", broadcast_cost);
            eprintln!("edge_cost     : {:>10}", edge_cost);

            broadcast_cost + edge_cost
        }
    }

    impl<'a> Display for Output<'a> {
        fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
            writeln!(f, "{}", self.powers.iter().map(|p| p.to_string()).join(" "))?;

            let mut used = vec![0; self.input.edge_count];

            for &e in &self.edges {
                used[e] = 1;
            }

            write!(f, "{}", used.iter().map(|u| u.to_string()).join(" "))?;

            Ok(())
        }
    }

    #[derive(Clone, Debug, Copy, PartialEq, Eq, Hash)]
    pub struct Point {
        pub x: i32,
        pub y: i32,
    }

    impl Point {
        pub fn new(x: i32, y: i32) -> Self {
            Self { x, y }
        }

        pub fn calc_sq_dist(&self, other: &Point) -> i32 {
            let dx = self.x - other.x;
            let dy = self.y - other.y;
            dx * dx + dy * dy
        }
    }

    pub fn parse_input() -> Input {
        input! {
            n: usize,
            m: usize,
            k: usize,
            stations: [(i32, i32); n],
            edges: [(Usize1, Usize1, i32); m],
            residents: [(i32, i32); k],
        }

        Input {
            station_count: n,
            edge_count: m,
            resident_count: k,
            stations: stations.iter().map(|&(x, y)| Point::new(x, y)).collect(),
            edges,
            residents: residents.iter().map(|&(x, y)| Point::new(x, y)).collect(),
        }
    }

    pub fn is_covered(p1: &Point, p2: &Point, power: i32) -> bool {
        p1.calc_sq_dist(p2) <= power * power
    }

    pub mod acl {
        pub mod dsu {
            pub struct Dsu {
                n: usize,
                parent_or_size: Vec<i32>,
            }

            impl Dsu {
                pub fn new(size: usize) -> Self {
                    Self {
                        n: size,
                        parent_or_size: vec![-1; size],
                    }
                }

                pub fn merge(&mut self, a: usize, b: usize) -> usize {
                    assert!(a < self.n);
                    assert!(b < self.n);
                    let (mut x, mut y) = (self.leader(a), self.leader(b));
                    if x == y {
                        return x;
                    }
                    if -self.parent_or_size[x] < -self.parent_or_size[y] {
                        std::mem::swap(&mut x, &mut y);
                    }
                    self.parent_or_size[x] += self.parent_or_size[y];
                    self.parent_or_size[y] = x as i32;
                    x
                }

                pub fn same(&mut self, a: usize, b: usize) -> bool {
                    assert!(a < self.n);
                    assert!(b < self.n);
                    self.leader(a) == self.leader(b)
                }

                pub fn leader(&mut self, a: usize) -> usize {
                    assert!(a < self.n);
                    if self.parent_or_size[a] < 0 {
                        return a;
                    }
                    self.parent_or_size[a] = self.leader(self.parent_or_size[a] as usize) as i32;
                    self.parent_or_size[a] as usize
                }
            }
        }
    }
}
