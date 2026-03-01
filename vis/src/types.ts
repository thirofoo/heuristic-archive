export interface RobotPos {
  i: number;
  j: number;
  d: number; // 0=U, 1=R, 2=D, 3=L
  s: number; // internal state
}

export interface SimulationState {
  step: number;
  robots: RobotPos[];
}

export interface RouteInfo {
  head: [number, number, number][]; // [i, j, d]
  tail: [number, number, number][];
}

export interface SimulationResult {
  score: number;
  error?: string;
  states: SimulationState[];
  n: number;
  ak: number;
  am: number;
  aw: number;
  wall_v: number[][];
  wall_h: number[][];
  wall_v_orig: number[][];
  wall_h_orig: number[][];
  wall_v_new: number[][];
  wall_h_new: number[][];
  patrolled: boolean[][];
  num_robots: number;
  total_states: number;
  num_new_walls: number;
  cost: number;
  routes: RouteInfo[];
}

export interface ProblemInput {
  raw: string;
  n: number;
  ak: number;
  am: number;
  aw: number;
  wall_v: number[][];
  wall_h: number[][];
}
