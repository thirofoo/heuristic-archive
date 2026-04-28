mod original_lib;

use crate::original_lib::{gen as original_gen, Sim, MAX_T};
use original_lib::{compute_score, parse_input, parse_output, Input, Output};
use svg::node::element::{Circle, Group, Line, Rectangle, Text, Title};

use wasm_bindgen::prelude::*;

#[wasm_bindgen]
pub fn gen(seed: i32, t: char) -> String {
    let i = original_gen(seed as u64, t);
    format!("{}", i)
}

#[wasm_bindgen(getter_with_clone)]
pub struct Ret {
    pub score: i64,
    pub err: String,
    pub svg: String,
}

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

const SVG_SIZE: usize = 800;
const DRONE_AREA_SIZE: usize = 200_000;
const CLEAR_RADIUS: usize = 2000;

fn convert_svg_coord(x: f64, y: f64) -> (f64, f64) {
    (
        x * SVG_SIZE as f64 / DRONE_AREA_SIZE as f64,
        -1.0 * y * SVG_SIZE as f64 / DRONE_AREA_SIZE as f64,
    )
}

fn generate_svg(input: &Input, output: &Output, turn: usize) -> String {
    let border = Rectangle::new()
        .set("x", 0)
        .set("y", 0)
        .set("width", SVG_SIZE)
        .set("height", SVG_SIZE)
        .set("fill", "none")
        .set("stroke", "black")
        .set("stroke-width", 1);
    let mut texts = Group::new();
    // 右上に +x, +yと書く
    texts = texts.add(
        Text::new()
            .set("x", SVG_SIZE - 20)
            .set("y", 20)
            .set("text-anchor", "end")
            .set("font-size", 20)
            .add(svg::node::Text::new("+x")),
    );
    texts = texts.add(
        Text::new()
            .set("x", SVG_SIZE - 20)
            .set("y", 40)
            .set("text-anchor", "end")
            .set("font-size", 20)
            .add(svg::node::Text::new("+y")),
    );
    // 左下に -x, -yと書く
    texts = texts.add(
        Text::new()
            .set("x", 20)
            .set("y", SVG_SIZE - 20)
            .set("text-anchor", "start")
            .set("font-size", 20)
            .add(svg::node::Text::new("-x")),
    );
    texts = texts.add(
        Text::new()
            .set("x", 20)
            .set("y", SVG_SIZE - 40)
            .set("text-anchor", "start")
            .set("font-size", 20)
            .add(svg::node::Text::new("-y")),
    );
    let mut walls = Group::new();
    for wall in input.walls.iter() {
        let (x1, y1) = convert_svg_coord(wall.0 as f64, wall.1 as f64);
        let (x2, y2) = convert_svg_coord(wall.2 as f64, wall.3 as f64);

        walls = walls.add(
            Line::new()
                .set("x1", x1)
                .set("y1", y1)
                .set("x2", x2)
                .set("y2", y2)
                .set("stroke", "black")
                .set("stroke-width", 1),
        );
    }
    walls = walls.set(
        "transform",
        format!("translate({}, {})", SVG_SIZE / 2, SVG_SIZE / 2),
    );
    let mut drones = Group::new();
    let mut trajectory = Group::new();
    // loop with sim
    let mut sim = Sim::new(input);
    let drone_before = Circle::new().set("r", 2).set("fill", "rgba(0, 0, 0, 0.3)");
    let drone_current = Circle::new().set("r", 2).set("fill", "blue");
    let mut before = convert_svg_coord(sim.p.0, sim.p.1);
    let colors = get_colors(output.out.len());
    for (i, (mut a, mut x, mut y)) in output.out.iter().enumerate() {
        let (ret, hit, d) = sim.query(input, a, x, y);
        let (x, y) = convert_svg_coord(sim.p.0, sim.p.1);
        drones = drones.add(drone_before.clone().set("cx", x).set("cy", y));
        trajectory = trajectory.add(
            Line::new()
                .set("x1", before.0)
                .set("y1", before.1)
                .set("x2", x)
                .set("y2", y)
                .set("stroke", encode_to_hsla(colors[i]))
                .set("stroke-width", 1),
        );
        before.0 = x;
        before.1 = y;
        if sim.visited.iter().all(|&b| b) {
            break;
        }
        if turn == i {
            break;
        }
    }
    let (x, y) = convert_svg_coord(sim.p.0, sim.p.1);
    drones = drones.add(drone_current.clone().set("cx", x).set("cy", y));
    drones = drones.set(
        "transform",
        format!("translate({}, {})", SVG_SIZE / 2, SVG_SIZE / 2),
    );
    trajectory = trajectory.set(
        "transform",
        format!("translate({}, {})", SVG_SIZE / 2, SVG_SIZE / 2),
    );
    let target_unreached = Circle::new().set("r", 3).set("fill", "red");
    let target_unreached_border = Circle::new()
        .set(
            "r",
            CLEAR_RADIUS as f64 * SVG_SIZE as f64 / DRONE_AREA_SIZE as f64,
        )
        .set("fill", "rgba(255, 0, 0, 0.1)")
        .set("stroke", "rgba(255, 0, 0, 0.3)")
        .set("stroke-width", 1);
    let target_reached = Circle::new().set("r", 3).set("fill", "green");
    let target_reached_border = Circle::new()
        .set(
            "r",
            CLEAR_RADIUS as f64 * SVG_SIZE as f64 / DRONE_AREA_SIZE as f64,
        )
        .set("fill", "rgba(0, 255, 0, 0.1)")
        .set("stroke", "rgba(0, 255, 0, 0.3)")
        .set("stroke-width", 1);
    let mut targets = Group::new();
    // for target in input.ps.iter() {
    for i in 0..input.ps.len() {
        let target = input.ps[i];
        let (x, y) = convert_svg_coord(target.0 as f64, target.1 as f64);
        let (circle, border_circle) = if sim.visited[i] {
            (target_reached.clone(), target_reached_border.clone())
        } else {
            (target_unreached.clone(), target_unreached_border.clone())
        };
        let title = Title::new().add(svg::node::Text::new(format!("{:?}", target)));
        let circle_group = Group::new()
            .add(title)
            .add(circle.set("cx", x).set("cy", y))
            .add(border_circle.set("cx", x).set("cy", y));
        targets = targets.add(circle_group);
    }
    targets = targets.set(
        "transform",
        format!("translate({}, {})", SVG_SIZE / 2, SVG_SIZE / 2),
    );
    let svg = svg::Document::new()
        .set("width", SVG_SIZE)
        .set("height", SVG_SIZE)
        .add(border)
        .add(walls)
        .add(drones)
        .add(targets)
        .add(texts)
        .add(trajectory);
    svg.to_string()
}

#[wasm_bindgen]
extern "C" {
    #[wasm_bindgen(js_namespace = console)]
    fn log(s: &str);
}

#[wasm_bindgen]
pub fn vis(_input: String, _output: String, turn: usize) -> Ret {
    let input = parse_input(&_input);
    let output = parse_output(&input, &_output);
    match output {
        Ok(output) => {
            let (score, err) = compute_score(&input, &output);
            let svg = generate_svg(&input, &output, turn);
            Ret { score, err, svg }
        }
        Err(err) => Ret {
            score: 0,
            err,
            svg: "".to_string(),
        },
    }
}

#[wasm_bindgen]
pub fn get_max_turn(_input: String, _output: String) -> usize {
    let input = parse_input(&_input);
    let out = parse_output(&input, &_output);
    match out {
        Ok(out) => out.out.len(),
        Err(_) => 0,
    }
}
