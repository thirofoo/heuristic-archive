use std::time::Instant;

use itertools::Itertools;
use rand::{seq::SliceRandom, Rng, SeedableRng};

use crate::{
    common::contains,
    io::{Input, Operation, Output, IO},
};

use super::Solver;

pub struct GreedySolver<'a> {
    input: &'a Input,
    io: &'a IO,
}

impl GreedySolver<'_> {
    pub fn new<'a>(input: &'a Input, io: &'a IO) -> GreedySolver<'a> {
        GreedySolver { input, io }
    }
}

const BOARD_SIZE: i64 = 1000000;

#[derive(Clone)]
struct State {
    score: i64,
    triangle: ((i64, i64), (i64, i64), (i64, i64)),
}

impl State {
    fn new() -> Self {
        State {
            score: 0,
            triangle: ((0, 0), (0, 0), (0, 0)),
        }
    }

    // Higher is better
    fn evaluate(&mut self, input: &Input) -> i64 {
        let mut score = 0;
        let (x, y, z) = self.triangle;
        for x_pos in input.x_pos.iter() {
            if contains(*x_pos, x, y, z) {
                score += 1;
            }
        }
        for y_pos in input.y_pos.iter() {
            if contains(*y_pos, x, y, z) {
                score -= 10;
            }
        }
        for z_pos in input.z_pos.iter() {
            if contains(*z_pos, x, y, z) {
                score -= 10;
            }
        }
        score *= BOARD_SIZE;
        // 週の長さ
        let mut dist = 0.0;
        dist += (((x.0 - y.0).pow(2) + (x.1 - y.1).pow(2)) as f64).sqrt();
        dist += (((y.0 - z.0).pow(2) + (y.1 - z.1).pow(2)) as f64).sqrt();
        dist += (((z.0 - x.0).pow(2) + (z.1 - x.1).pow(2)) as f64).sqrt();
        score /= dist as i64;
        self.score = score;
        score
    }
}

enum Neighbor {
    EditTriangle,
}

impl Neighbor {
    fn choose(rng: &mut impl Rng) -> Self {
        let choice = rng.gen_range(0..1);
        match choice {
            _ => Neighbor::EditTriangle,
        }
    }

    fn apply(&self, state: &mut State, rng: &mut impl Rng) {
        match self {
            Neighbor::EditTriangle => {
                // Edit a triangle
                // 一点を近くのBOARD_SIZE / 5だけ動かす
                let dx = rng.gen_range(-BOARD_SIZE / 5..=BOARD_SIZE / 5);
                let dy = rng.gen_range(-BOARD_SIZE / 5..=BOARD_SIZE / 5);
                let choose = rng.gen_range(0..3);
                if choose == 0
                    && state.triangle.0 .0 + dx >= 0
                    && state.triangle.0 .0 + dx < BOARD_SIZE
                    && state.triangle.0 .1 + dy >= 0
                    && state.triangle.0 .1 + dy < BOARD_SIZE
                {
                    state.triangle.0 .0 += dx;
                    state.triangle.0 .1 += dy;
                }
                if choose == 1
                    && state.triangle.1 .0 + dx >= 0
                    && state.triangle.1 .0 + dx < BOARD_SIZE
                    && state.triangle.1 .1 + dy >= 0
                    && state.triangle.1 .1 + dy < BOARD_SIZE
                {
                    state.triangle.1 .0 += dx;
                    state.triangle.1 .1 += dy;
                }
                if choose == 2
                    && state.triangle.2 .0 + dx >= 0
                    && state.triangle.2 .0 + dx < BOARD_SIZE
                    && state.triangle.2 .1 + dy >= 0
                    && state.triangle.2 .1 + dy < BOARD_SIZE
                {
                    state.triangle.2 .0 += dx;
                    state.triangle.2 .1 += dy;
                }
            }
        }
    }
}

fn visualize_triangles(triangles: &[((i64, i64), (i64, i64), (i64, i64))], input: &Input) {
    let python_script = r#"
import matplotlib.pyplot as plt
import numpy as np

def visualize_triangles(triangles, x_pos, y_pos, z_pos):
    plt.figure()
    plt.xlim(0, 1000000)
    plt.ylim(0, 1000000)
    plt.gca().invert_yaxis()
    for triangle in triangles:
        x = [triangle['x'][0], triangle['x'][1], triangle['x'][2], triangle['x'][0]]
        y = [triangle['y'][0], triangle['y'][1], triangle['y'][2], triangle['y'][0]]
        plt.plot(x, y, marker='o', markersize=1)
        plt.fill(x, y, alpha=0.3)
    for x, y in x_pos:
        plt.plot(x, y, 'ro', markersize=1)
    for x, y in y_pos:
        plt.plot(x, y, 'go', markersize=1)
    for x, y in z_pos:
        plt.plot(x, y, 'bo', markersize=1)
    plt.savefig('triangles.png')
if __name__ == "__main__":
    triangles = {}
    x_pos = {}
    y_pos = {}
    z_pos = {}
    visualize_triangles(triangles, x_pos, y_pos, z_pos)
"#;

    let mut python_script = python_script.to_string();
    python_script = python_script.replace(
        "triangles = {}",
        &format!(
            "triangles = [{}]",
            triangles
                .iter()
                .map(|((ax, ay), (bx, by), (cx, cy))| {
                    format!(
                        "{{'x': [{}, {}, {}], 'y': [{}, {}, {}]}}",
                        ax, bx, cx, ay, by, cy
                    )
                })
                .collect::<Vec<_>>()
                .join(", ")
        ),
    );
    python_script = python_script.replace(
        "x_pos = {}",
        &format!(
            "x_pos = [{}]",
            input
                .x_pos
                .iter()
                .map(|(x, y)| format!("({}, {})", x, y))
                .collect::<Vec<_>>()
                .join(", ")
        ),
    );
    python_script = python_script.replace(
        "y_pos = {}",
        &format!(
            "y_pos = [{}]",
            input
                .y_pos
                .iter()
                .map(|(x, y)| format!("({}, {})", x, y))
                .collect::<Vec<_>>()
                .join(", ")
        ),
    );
    python_script = python_script.replace(
        "z_pos = {}",
        &format!(
            "z_pos = [{}]",
            input
                .z_pos
                .iter()
                .map(|(x, y)| format!("({}, {})", x, y))
                .collect::<Vec<_>>()
                .join(", ")
        ),
    );
    let out = std::process::Command::new("python3")
        .arg("-c")
        .arg(python_script)
        .output()
        .expect("failed to execute process");

    if !out.status.success() {
        eprintln!("Error: {}", String::from_utf8_lossy(&out.stderr));
    } else {
        eprintln!("Visualization saved to triangles.png");
    }
}

impl Solver for GreedySolver<'_> {
    fn solve(&mut self) -> Output {
        // xから3つ選ぶ組み合わせの中で、中に含まれるzが0のものを選ぶ
        let mut rng = rand::rngs::StdRng::from_entropy();

        let mut current_input = self.input.clone();
        let mut triangles = vec![];
        let mut operations = vec![];
        let (mut p1, mut q1) = ((-1, -1), (-1, -1));
        let p2 = (0, 0);
        let q2 = (0, 0);
        for _ in 0..30 {
            if current_input.x_pos.len() == 0 {
                break;
            }

            let mut current_state = State::new();
            current_state.triangle = ((0, 0), (BOARD_SIZE, 0), (BOARD_SIZE / 2, BOARD_SIZE));
            current_state.evaluate(self.input);
            eprintln!("initial_score: {}", current_state.score);
            let mut best_state = current_state.clone();
            let start_temp = 1e2;
            let end_temp = 1e-2;
            let mut temp = start_temp;
            let start = Instant::now();
            let tl = 50;
            let mut iter = 0;
            while start.elapsed().as_millis() < tl {
                iter += 1;
                let neighbor = Neighbor::choose(&mut rng);
                let mut new_state = current_state.clone();
                neighbor.apply(&mut new_state, &mut rng);
                let new_score = new_state.evaluate(&current_input);
                let delta = new_score - current_state.score;
                if delta > 0 || rng.gen::<f64>() < (-(delta as f64) / temp).exp() {
                    current_state.clone_from(&new_state);
                }
                if new_score > best_state.score {
                    best_state.clone_from(&new_state);
                }
                temp = start_temp
                    * (end_temp / start_temp).powf(start.elapsed().as_millis() as f64 / tl as f64);
            }
            eprintln!("iter: {}", iter);
            eprintln!("score: {}", best_state.score);
            eprintln!("triangle: {:?}", best_state.triangle);
            // current_inputから、best_state.triangleの中に含まれるx,y,zを削除する
            current_input.x_pos.retain(|&x| {
                !contains(
                    x,
                    best_state.triangle.0,
                    best_state.triangle.1,
                    best_state.triangle.2,
                )
            });
            current_input.y_pos.retain(|&y| {
                !contains(
                    y,
                    best_state.triangle.0,
                    best_state.triangle.1,
                    best_state.triangle.2,
                )
            });
            current_input.z_pos.retain(|&z| {
                !contains(
                    z,
                    best_state.triangle.0,
                    best_state.triangle.1,
                    best_state.triangle.2,
                )
            });
            if p1 == (-1, -1) && q1 == (-1, -1) {
                operations.push(Operation {
                    p1: best_state.triangle.0,
                    q1: best_state.triangle.1,
                    p2,
                    q2,
                });
            } else {
                operations.push(Operation {
                    p1: q1,
                    q1: best_state.triangle.0,
                    p2,
                    q2,
                });
                operations.push(Operation {
                    p1: best_state.triangle.0,
                    q1: best_state.triangle.1,
                    p2,
                    q2,
                });
            }
            operations.push(Operation {
                p1: best_state.triangle.0,
                q1: best_state.triangle.2,
                p2,
                q2,
            });
            p1 = best_state.triangle.0;
            q1 = best_state.triangle.2;

            triangles.push(best_state.triangle);
        }

        eprintln!("triangles: {:?}", triangles);

        // visualize_triangles(&triangles, self.input);

        Output { operations }
    }
}
