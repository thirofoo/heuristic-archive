# Optuna tuning

## Setup

```bash
python -m pip install -r optuna/requirements.txt
```

## Run

```bash
python optuna/optuna_tune.py --trials 50
```

## Run (per-M tuning, recommended)

Tune only one `M` row in `params.hpp` using seeds with matching `M`.

```bash
python optuna/optuna_tune_by_m.py --target-m 2 --trials 50 --seed-count 24
python optuna/optuna_tune_by_m.py --target-m 3 --trials 50 --seed-count 24
```

Optional: fix `U` and optimize robust objective `mean - lambda * std`.

```bash
python optuna/optuna_tune_by_m.py --target-m 3 --target-u 2 --risk-lambda 0.30
```

If you want to pass a config explicitly:

```bash
python optuna/optuna_tune.py --trials 50 --pahcer-cmd "pahcer run -c pahcer_config.toml --freeze-best-scores"
```

## Notes
- The script overwrites params.hpp on each trial.
- The pahcer command can be customized with `--pahcer-cmd` (default: `pahcer run --freeze-best-scores`).
- Relative score is parsed from pahcer output ("Relative Score").
- If your pahcer version needs a config flag, try `-c pahcer_config.toml` (some versions do not accept `--config`).
