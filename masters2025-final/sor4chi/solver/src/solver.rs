use crate::io::Output;

pub mod greedy;

pub trait Solver {
    fn solve(&mut self) -> Output;
}
