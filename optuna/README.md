# Optuna tuning

## Setup

```bash
python -m pip install -r optuna/requirements.txt
```

## Run

```bash
python optuna/optuna_tune.py --trials 50
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
