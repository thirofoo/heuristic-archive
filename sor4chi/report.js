import { execSync } from "child_process";
import fs from "fs";
import path from "path";

// argを取得
const args = process.argv.slice(2);
const probrem = args[0];
if (!/^[A-C]$/.test(probrem)) {
  console.log(`Invalid Problem: ${probrem}`);
  process.exit(1);
}
if (args.length > 1) {
  console.log("Invalid Arguments");
  process.exit(1);
}
if (args.length === 0) {
  console.log("No Arguments");
  process.exit(1);
}

console.log("Compiling...");
execSync("cd solver && cargo build -r");

const report = {};

const abs = (p) => path.resolve(__dirname, p);
const solverDir = abs("solver");
const toolsDir = abs("tools");
const reportsDir = abs("reports");

const c = {
  red: (s) => `\x1b[31m${s}\x1b[0m`,
  yellow: (s) => `\x1b[33m${s}\x1b[0m`,
  green: (s) => `\x1b[32m${s}\x1b[0m`,
};

const percentageLogger = (rate) => {
  return rate > 0
    ? c.green(`+${(rate * 100).toFixed(2)}%`)
    : rate < 0
    ? c.red(`${(rate * 100).toFixed(2)}%`)
    : `${(rate * 100).toFixed(2)}%`;
};

const SEED_START = 0;
const SEED_END = 99;
const PARALLEL_SIZE = 1;
console.log(`Testing seeds from ${SEED_START} to ${SEED_END}...`);

execSync(`rm -rf ${toolsDir}/out`);
execSync(`rm -rf ${toolsDir}/err`);
execSync(`mkdir ${toolsDir}/out`);
execSync(`mkdir ${toolsDir}/err`);
execSync(`mkdir -p ${reportsDir}`);
execSync(`mkdir -p ${reportsDir}/${probrem}`);
execSync(`cd ${toolsDir} && cargo build -r`);

const seeds = [];
for (let seed = SEED_START; seed <= SEED_END; seed++) {
  seed = seed.toString().padStart(4, "0");
  seeds.push(seed);
}
const chunks = [];
for (let i = 0; i < seeds.length; i += PARALLEL_SIZE) {
  chunks.push(seeds.slice(i, i + PARALLEL_SIZE));
}
for (let i = 0; i < chunks.length; i++) {
  // execute in multi process
  const chunk = chunks[i];
  const commands = chunk.map(
    (seed) =>
      `time ${solverDir}/target/release/solve < ${toolsDir}/in${probrem}/${seed}.txt > ${toolsDir}/out/${seed}.txt && ${toolsDir}/target/release/score ${toolsDir}/in${probrem}/${seed}.txt ${toolsDir}/out/${seed}.txt > ${toolsDir}/err/${seed}.txt`
  );
  execSync(commands.join(" & "));
  for (const seed of chunk) {
    const res = fs.readFileSync(`${toolsDir}/err/${seed}.txt`);
    const SCORE_RE = /Score = (\d+)/;
    const match = SCORE_RE.exec(res.toString());
    if (match) {
      console.log(`Seed ${seed}: ${match[1]}`);
      report[seed] = parseInt(match[1]);
    } else {
      console.log(`Seed ${seed}: Failed`);
    }
  }
}

const reports = fs.readdirSync(reportsDir).filter((f) => f.endsWith(".json"));
reports.sort();
reports.forEach((f) => {
  const otherReport = JSON.parse(
    fs.readFileSync(`${reportsDir}/${probrem}/${f}`).toString()
  );
  const diffRate = {};
  const res = [];
  for (const seed in report) {
    // 最大化
    const diff = report[seed] - otherReport[seed];
    diffRate[seed] = diff / otherReport[seed];
  }
  console.log(`Diff overview with ${f}:`);
  console.log(res.join(" "));
  const avgDiffRate =
    Object.values(diffRate).reduce((a, b) => a + b, 0) /
    Object.keys(diffRate).length;
  console.log(`Average diff rate: ${percentageLogger(avgDiffRate)}`);
});

const now = new Date();
const reportPath = `${now.getTime()}.json`;
fs.writeFileSync(
  `${reportsDir}/${probrem}/${reportPath}`,
  JSON.stringify(report, null, 2)
);
