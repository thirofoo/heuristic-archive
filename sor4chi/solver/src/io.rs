use proconio::input;

#[derive(Clone)]
pub struct Input {
    pub x: usize,
    pub y: usize,
    pub z: usize,
    pub x_pos: Vec<(i64, i64)>, // 燃えるごみ(高橋くん担当)
    pub y_pos: Vec<(i64, i64)>, // 燃えないごみ(青木くん担当)
    pub z_pos: Vec<(i64, i64)>, // 資源ごみ
}

pub struct Operation {
    pub p1: (i64, i64),
    pub q1: (i64, i64),
    pub p2: (i64, i64),
    pub q2: (i64, i64),
}

pub struct Output {
    pub operations: Vec<Operation>,
}

#[derive(Default)]
pub struct IO {}

impl IO {
    pub fn read(&mut self) -> Input {
        input! {
            x: usize,
            y: usize,
            z: usize,
            x_pos: [(i64, i64); x],
            y_pos: [(i64, i64); y],
            z_pos: [(i64, i64); z],
        }

        Input {
            x,
            y,
            z,
            x_pos,
            y_pos,
            z_pos,
        }
    }

    pub fn write(&self, output: &Output) {
        let mut operations = vec![];
        for op in &output.operations {
            operations.push(format!(
                "{} {} {} {} {} {} {} {}",
                op.p1.0, op.p1.1, op.q1.0, op.q1.1, op.p2.0, op.p2.1, op.q2.0, op.q2.1
            ));
        }
        println!("{}", operations.join("\n"));
    }
}
