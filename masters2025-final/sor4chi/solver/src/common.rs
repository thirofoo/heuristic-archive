pub fn orient(a: (i64, i64), b: (i64, i64), p: (i64, i64)) -> i64 {
    let (dx1, dy1) = (b.0 - a.0, b.1 - a.1);
    let (dx2, dy2) = (p.0 - a.0, p.1 - a.1);
    dx1 * dy2 - dy1 * dx2
}

pub fn contains(p: (i64, i64), a: (i64, i64), b: (i64, i64), c: (i64, i64)) -> bool {
    if orient(a, b, c) == 0 {
        if orient(a, b, p) != 0 || orient(a, c, p) != 0 {
            return false;
        }
        let min_x = a.0.min(b.0).min(c.0);
        let max_x = a.0.max(b.0).max(c.0);
        let min_y = a.1.min(b.1).min(c.1);
        let max_y = a.1.max(b.1).max(c.1);
        return (min_x <= p.0 && p.0 <= max_x) && (min_y <= p.1 && p.1 <= max_y);
    }
    let c1 = orient(a, b, p);
    let c2 = orient(b, c, p);
    let c3 = orient(c, a, p);
    (c1 >= 0 && c2 >= 0 && c3 >= 0) || (c1 <= 0 && c2 <= 0 && c3 <= 0)
}
