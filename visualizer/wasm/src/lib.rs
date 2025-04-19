pub mod original;
use original::{
    compute_score, compute_score_details, contains, dist, parse_input, parse_output, Output,
    SetMinMax,
};
use std::collections::{HashMap, HashSet};
use svg::node::element::{Circle, Group, Line, Path, Rectangle, Text, Title};
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
const COLOR_HOTTEST_HSLA: &str = "hsl(349, 100%, 40%, 0.8)"; // #ff1e46 * 0.8
const COLOR_COOLEST_HSLA: &str = "hsl(210, 100%, 40%, 0.8)"; // #1e90ff * 0.8

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

fn create_triangle_svg(path: Vec<(f64, f64)>, color: &str) -> Path {
    let mut path_str = String::new();
    path_str.push('M');
    path_str.push_str(&format!("{},{}", path[0].0, path[0].1));
    for (x, y) in &path[1..] {
        path_str.push_str(&format!(" L{},{}", x, y));
    }
    path_str.push_str(" Z");

    Path::new()
        .set("d", path_str)
        .set("fill", format!("{}30", color))
        .set("stroke", format!("{}cc", color))
        .set("stroke-width", 1)
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
    let (mut score, err, (time, done), (p1, q1, p2, q2)) =
        compute_score_details(&input, &truncated.out);
    if err.len() > 0 {
        score = 0;
    }

    let mut group = Group::new();

    // X, Y, Zをそれぞれ表示
    let colors = get_colors(3);
    for i in 0..input.X {
        let idx = i as usize;
        if done[idx] {
            continue;
        }
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
        let idx = (input.X + i) as usize;
        if done[idx] {
            continue;
        }
        let x = (input.ps[idx].0 as f64) * SVG_WIDTH as f64 / BOARD_SIZE as f64;
        let y = (input.ps[idx].1 as f64) * SVG_HEIGHT as f64 / BOARD_SIZE as f64;
        let color = encode_to_hsla(colors[1]);
        let circle = Circle::new()
            .set("cx", x)
            .set("cy", y)
            .set("r", 5)
            .set("fill", color);
        group = group.add(circle);
    }
    for i in 0..input.Z {
        let idx = (input.X + input.Y + i) as usize;
        if done[idx] {
            continue;
        }
        let x = (input.ps[idx].0 as f64) * SVG_WIDTH as f64 / BOARD_SIZE as f64;
        let y = (input.ps[idx].1 as f64) * SVG_HEIGHT as f64 / BOARD_SIZE as f64;
        let color = encode_to_hsla(colors[2]);
        let circle = Circle::new()
            .set("cx", x)
            .set("cy", y)
            .set("r", 5)
            .set("fill", color);
        group = group.add(circle);
    }

    if 0 < turn && turn < output.out.len() {
        let resized_ta_l1 = (
            (output.out[turn - 1].0 .0 as f64) * SVG_WIDTH as f64 / BOARD_SIZE as f64,
            (output.out[turn - 1].0 .1 as f64) * SVG_HEIGHT as f64 / BOARD_SIZE as f64,
        );
        let resized_ta_r1 = (
            (output.out[turn - 1].1 .0 as f64) * SVG_WIDTH as f64 / BOARD_SIZE as f64,
            (output.out[turn - 1].1 .1 as f64) * SVG_HEIGHT as f64 / BOARD_SIZE as f64,
        );
        let resized_ao_l1 = (
            (output.out[turn - 1].2 .0 as f64) * SVG_WIDTH as f64 / BOARD_SIZE as f64,
            (output.out[turn - 1].2 .1 as f64) * SVG_HEIGHT as f64 / BOARD_SIZE as f64,
        );
        let resized_ao_r1 = (
            (output.out[turn - 1].3 .0 as f64) * SVG_WIDTH as f64 / BOARD_SIZE as f64,
            (output.out[turn - 1].3 .1 as f64) * SVG_HEIGHT as f64 / BOARD_SIZE as f64,
        );
        let resized_ta_l2 = (
            (output.out[turn].0 .0 as f64) * SVG_WIDTH as f64 / BOARD_SIZE as f64,
            (output.out[turn].0 .1 as f64) * SVG_HEIGHT as f64 / BOARD_SIZE as f64,
        );
        let resized_ta_r2 = (
            (output.out[turn].1 .0 as f64) * SVG_WIDTH as f64 / BOARD_SIZE as f64,
            (output.out[turn].1 .1 as f64) * SVG_HEIGHT as f64 / BOARD_SIZE as f64,
        );
        let resized_ao_l2 = (
            (output.out[turn].2 .0 as f64) * SVG_WIDTH as f64 / BOARD_SIZE as f64,
            (output.out[turn].2 .1 as f64) * SVG_HEIGHT as f64 / BOARD_SIZE as f64,
        );
        let resized_ao_r2 = (
            (output.out[turn].3 .0 as f64) * SVG_WIDTH as f64 / BOARD_SIZE as f64,
            (output.out[turn].3 .1 as f64) * SVG_HEIGHT as f64 / BOARD_SIZE as f64,
        );

        // takahashi 三角形 p q p'
        group = group.add(create_triangle_svg(
            vec![resized_ta_l1, resized_ta_r1, resized_ta_l2],
            "#ff1e46",
        ));
        // takahashi 三角形 p' q q'
        group = group.add(create_triangle_svg(
            vec![resized_ta_l2, resized_ta_r1, resized_ta_r2],
            "#ff1e46",
        ));

        // aoki 三角形 p q p'
        group = group.add(create_triangle_svg(
            vec![resized_ao_l1, resized_ao_r1, resized_ao_l2],
            "#1e90ff",
        ));
        // aoki 三角形 p' q q'
        group = group.add(create_triangle_svg(
            vec![resized_ao_l2, resized_ao_r1, resized_ao_r2],
            "#1e90ff",
        ));
    }

    // 左下にtimeを表示
    let text = Text::new(format!("Time: {:.2}", time))
        .set("x", 0)
        .set("y", SVG_HEIGHT - 20)
        .set("font-size", 16)
        .set("fill", "black");
    group = group.add(text);

    let svg = svg::Document::new()
        .set("width", SVG_WIDTH)
        .set("height", SVG_HEIGHT)
        .add(group);

    Ret {
        score,
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
