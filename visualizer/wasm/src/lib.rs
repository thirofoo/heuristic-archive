pub mod original;
use original::{
    compute_score, compute_score_details, parse_input, parse_output, Action, Output, SetMinMax,
    DIJ, DIR,
};
use std::collections::{HashMap, HashSet};
use svg::node::element::{Circle, Group, Line, Rectangle, Text};
use wasm_bindgen::prelude::*;
use web_sys::console;

#[wasm_bindgen]
pub fn gen(seed: i32, problemId: String) -> String {
    let input = original::gen(seed as u64, &problemId);
    format!("{}", input)
}

#[wasm_bindgen(getter_with_clone)]
pub struct Ret {
    pub score: i64,
    pub err: String,
    pub svg: String,
}

const SVG_WIDTH: usize = 800;
const SVG_HEIGHT: usize = 800;
const COLOR_HOTTEST_HSLA: &str = "hsl(349, 100%, 56%, 0.8)"; // #ff1e46 * 0.8
const COLOR_COOLEST_HSLA: &str = "hsl(210, 100%, 56%, 0.8)"; // #1e90ff * 0.8

#[derive(Debug, Clone, Copy)]
struct HslaColor {
    h: f64,
    s: f64,
    l: f64,
    a: f64,
}

fn decode_to_hsla(s: &str) -> HslaColor {
    let s2 = s
        .trim_start_matches("hsl(")
        .trim_end_matches(')')
        .split(',')
        .collect::<Vec<_>>();
    let h = s2[0].parse::<f64>().unwrap();
    let s = s2[1].trim().trim_end_matches('%').parse::<f64>().unwrap();
    let l = s2[2].trim().trim_end_matches('%').parse::<f64>().unwrap();
    let a = s2[3].trim().parse::<f64>().unwrap();
    HslaColor { h, s, l, a }
}

fn encode_to_hsla(c: HslaColor) -> String {
    format!("hsla({}, {}%, {}%, {})", c.h, c.s, c.l, c.a)
}

fn get_colors(cnt: usize) -> Vec<HslaColor> {
    let mut colors = vec![];
    let hottest = decode_to_hsla(COLOR_HOTTEST_HSLA);
    let coolest = decode_to_hsla(COLOR_COOLEST_HSLA);
    let mut h = coolest.h;
    let mut s = coolest.s;
    let mut l = coolest.l;
    let mut a = coolest.a;
    let dh = (coolest.h - hottest.h + 360.0) / (cnt as f64);
    let ds = (hottest.s - coolest.s) / (cnt as f64);
    let dl = (hottest.l - coolest.l) / (cnt as f64);
    let da = (hottest.a - coolest.a) / (cnt as f64);
    for _ in 0..cnt {
        colors.push(HslaColor { h, s, l, a });
        h = (h - dh) % 360.0;
        s += ds;
        l += dl;
        a += da;
    }
    colors
}

pub struct MapState {
    current: (usize, usize),
    rocks: HashSet<(usize, usize)>,
    juwels: HashMap<(usize, usize), char>,
}

impl MapState {
    fn new() -> Self {
        Self {
            current: (0, 0),
            rocks: HashSet::new(),
            juwels: HashMap::new(),
        }
    }

    fn update(&mut self, act: Action, input: &original::Input) {
        match act {
            Action::Move(d) => {
                let (di, dj) = DIJ[d];
                self.current.0 += di;
                self.current.1 += dj;
            }
            Action::Carry(d) => {
                let (di, dj) = DIJ[d];
                // if (cs[self.current.0][self.current.1] < 'a'
                //     || cs[self.current.0][self.current.1] > 'z')
                //     && cs[self.current.0][self.current.1] != '@'
                // {
                //     return (0, format!("No item to carry (turn {t})"), ());
                // }
                let jewel = self.juwels.get(&(self.current.0, self.current.1));
                let rock = self.rocks.contains(&(self.current.0, self.current.1));
                // 宝石も岩の現在地にない場合は何も運べない
                if jewel.is_none() && !rock {
                    return;
                }
                // 外側への運搬はできない
                if self.current.0 + di >= input.N || self.current.1 + dj >= input.N {
                    return;
                }
                // 移動さきに岩か宝石がある場合は何も運べない
                if self
                    .rocks
                    .contains(&(self.current.0 + di, self.current.1 + dj))
                    || self
                        .juwels
                        .contains_key(&(self.current.0 + di, self.current.1 + dj))
                {
                    return;
                }
                // 移動させる
                let res = self.juwels.remove(&(self.current.0, self.current.1));
                if let Some(j) = res {
                    self.juwels
                        .insert((self.current.0 + di, self.current.1 + dj), j);
                }
                let res = self.rocks.remove(&(self.current.0, self.current.1));
                if res {
                    self.rocks
                        .insert((self.current.0 + di, self.current.1 + dj));
                }
                // 運んだ先に穴があれば消す
                if matches!(
                    input.cs[self.current.0 + di][self.current.1 + dj],
                    'A'..='Z'
                ) {
                    self.rocks
                        .remove(&(self.current.0 + di, self.current.1 + dj));
                    self.juwels
                        .remove(&(self.current.0 + di, self.current.1 + dj));
                }

                self.current.0 += di;
                self.current.1 += dj;
            }
            // Action::Roll(d) => {
            //     let (di, dj) = DIJ[d];
            //     if (cs[self.current.0][self.current.1] < 'a'
            //         || cs[self.current.0][self.current.1] > 'z')
            //         && cs[self.current.0][self.current.1] != '@'
            //     {
            //         return (0, format!("No item to roll (turn {t})"), ());
            //     }
            //     let c = cs[self.current.0][self.current.1];
            //     cs[self.current.0][self.current.1] = '.';
            //     let mut crt = pos;
            //     loop {
            //         let next = (crt.0 + di, crt.1 + dj);
            //         if next.0 >= input.N
            //             || next.1 >= input.N
            //             || matches!(cs[next.0][next.1], '@' | 'a'..='z')
            //         {
            //             cs[crt.0][crt.1] = c;
            //             break;
            //         } else if matches!(cs[next.0][next.1], 'A'..='Z') {
            //             if cs[next.0][next.1].to_ascii_lowercase() == c {
            //                 A += 1;
            //             }
            //             break;
            //         } else {
            //             crt = next;
            //         }
            //     }
            // }
            _ => {}
        }
    }
}

#[wasm_bindgen]
pub fn vis(_input: String, _output: String, turn: usize) -> Ret {
    let input = parse_input(&_input);
    let res = parse_output(&input, &_output);
    if res.is_err() {
        return Ret {
            score: 0,
            err: res.err().unwrap(),
            svg: "".to_string(),
        };
    }
    let output = res.unwrap();
    let truncated = Output {
        out: output.out[..turn].to_vec(),
    };
    let res = compute_score(&input, &truncated);

    let cell_width = (SVG_WIDTH / input.N).min(50);
    let cell_height = (SVG_HEIGHT / input.N).min(50);
    let mut group = Group::new();
    for i in 0..input.N {
        for j in 0..input.N {
            let x = j * cell_width;
            let y = i * cell_height;
            let rect = Rectangle::new()
                .set("x", x)
                .set("y", y)
                .set("width", cell_width)
                .set("height", cell_height)
                .set("fill", "#fff")
                .set("stroke", "#4444")
                .set("stroke-width", 1);
            group = group.add(rect);
        }
    }
    let mut map_state = MapState::new();
    let mut kinds = HashSet::new();
    for i in 0..input.N {
        for j in 0..input.N {
            if input.cs[i][j] == '@' {
                map_state.rocks.insert((i, j));
            }
            if let 'a'..='z' = input.cs[i][j] {
                map_state.juwels.insert((i, j), input.cs[i][j]);
                kinds.insert(input.cs[i][j]);
            }
            if input.cs[i][j] == 'A' {
                map_state.current = (i, j);
            }
        }
    }
    let colors = get_colors(kinds.len());
    let mut color_map = HashMap::new();
    for (i, k) in kinds.iter().enumerate() {
        color_map.insert(*k, colors[i]);
    }
    for act in &truncated.out {
        map_state.update(*act, &input);
    }
    for i in 0..input.N {
        for j in 0..input.N {
            // 穴の位置を描画。
            if let 'A'..='Z' = input.cs[i][j] {
                let x = j * cell_width;
                let y = i * cell_height;
                // 灰色の丸とその中に文字を描画。
                let circle = Circle::new()
                    .set("cx", x + cell_width / 2)
                    .set("cy", y + cell_height / 2)
                    .set("r", cell_width / 2 - 2)
                    .set("fill", "#ccc")
                    .set(
                        "stroke",
                        encode_to_hsla(
                            *color_map.get(&input.cs[i][j].to_ascii_lowercase()).unwrap(),
                        ),
                    )
                    .set("stroke-width", 2);
                let text = Text::new(input.cs[i][j].to_string())
                    .set("x", x + cell_width / 2)
                    .set("y", y + cell_height / 2)
                    .set("text-anchor", "middle")
                    .set("dominant-baseline", "central")
                    .set("font-size", cell_width / 3);
                group = group.add(circle).add(text);
            }
            // 岩の位置を描画。
            if map_state.rocks.contains(&(i, j)) {
                let x = j * cell_width;
                let y = i * cell_height;
                let rect = Rectangle::new()
                    .set("x", x + 5)
                    .set("y", y + 5)
                    .set("width", cell_width - 10)
                    .set("height", cell_height - 10)
                    .set("fill", "#444")
                    .set("stroke", "#444")
                    .set("stroke-width", 1)
                    .set("rx", 5) // Set the x-axis radius for rounded corners
                    .set("ry", 5); // Set the y-axis radius for rounded corners
                group = group.add(rect);
            }
            // 宝石の位置を描画。
            if let Some(juwel) = map_state.juwels.get(&(i, j)) {
                let x = j * cell_width;
                let y = i * cell_height;
                let circle = Circle::new()
                    .set("cx", x + cell_width / 2)
                    .set("cy", y + cell_height / 2)
                    .set("r", cell_width / 4)
                    .set(
                        "fill",
                        encode_to_hsla(*color_map.get(&juwel.to_ascii_lowercase()).unwrap()),
                    );
                group = group.add(circle);
            }
            // 自身の位置を描画。
            if map_state.current == (i, j) {
                let x = j * cell_width;
                let y = i * cell_height;
                let circle = Circle::new()
                    .set("cx", x + cell_width / 2)
                    .set("cy", y + cell_height / 2)
                    .set("r", cell_width / 3)
                    .set("fill", "transparent")
                    .set("stroke", "red")
                    .set("stroke-width", 2);
                group = group.add(circle);
            }
        }
    }

    let svg = svg::Document::new()
        .set("width", SVG_WIDTH)
        .set("height", SVG_HEIGHT)
        .add(group);

    Ret {
        score: res.0,
        err: "".to_string(),
        svg: svg.to_string(),
    }
}

#[wasm_bindgen]
pub fn get_max_turn(_input: String, _output: String) -> usize {
    let input = parse_input(&_input);
    let output = parse_output(&input, &_output).unwrap();
    output.out.len()
}
