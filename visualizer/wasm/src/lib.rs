pub mod original;
use original::{
    compute_score, compute_score_details, parse_input, parse_output, Output, SetMinMax,
};
use std::collections::{HashMap, HashSet};
use svg::node::element::{Circle, Group, Line, Rectangle, Text, Title};
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

const BOARD_SIZE: usize = 1000000;

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

    let mut group = Group::new();

    // X, Y, Zをそれぞれ表示
    let colors = get_colors(3);
    for i in 0..input.X {
        let x = (input.ps[i].0 as f64) * SVG_WIDTH as f64 / BOARD_SIZE as f64;
        let y = (input.ps[i].1 as f64) * SVG_HEIGHT as f64 / BOARD_SIZE as f64;
        let color = encode_to_hsla(colors[0]);
        let circle = Circle::new()
            .set("cx", x)
            .set("cy", y)
            .set("r", 5)
            .set("fill", color);
        group = group.add(circle);
    }
    for i in 0..input.Y {
        let x = (input.ps[input.X + i].0 as f64) * SVG_WIDTH as f64 / BOARD_SIZE as f64;
        let y = (input.ps[input.X + i].1 as f64) * SVG_HEIGHT as f64 / BOARD_SIZE as f64;
        let color = encode_to_hsla(colors[1]);
        let circle = Circle::new()
            .set("cx", x)
            .set("cy", y)
            .set("r", 5)
            .set("fill", color);
        group = group.add(circle);
    }
    for i in 0..input.Z {
        let x = (input.ps[input.X + input.Y + i].0 as f64) * SVG_WIDTH as f64 / BOARD_SIZE as f64;
        let y = (input.ps[input.X + input.Y + i].1 as f64) * SVG_HEIGHT as f64 / BOARD_SIZE as f64;
        let color = encode_to_hsla(colors[2]);
        let circle = Circle::new()
            .set("cx", x)
            .set("cy", y)
            .set("r", 5)
            .set("fill", color);
        group = group.add(circle);
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
