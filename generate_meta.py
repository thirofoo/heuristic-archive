#!/usr/bin/env python3
"""Generate meta.json for all contest directories in the monorepo."""

import argparse
from datetime import datetime
import html as html_lib
from html.parser import HTMLParser
import json
import os
import re
import sys
import time
import urllib.error
import urllib.parse
import urllib.request

ATCODER_USER = "through"

CONTEST_DATA = {
    "AHC002": {
        "title": "AtCoder Heuristic Contest 002",
        "date": "2021-04-25",
        "rank": None,
        "performance": None,
    },
    "AHC005": {
        "title": "AtCoder Heuristic Contest 005",
        "date": "2021-08-07",
        "rank": None,
        "performance": None,
    },
    "AHC007": {
        "title": "THIRD PROGRAMMING CONTEST 2021 (AtCoder Heuristic Contest 007)",
        "date": "2021-12-12",
        "rank": 613,
        "performance": 126,
    },
    "AHC008": {
        "title": "MC Digital Programming Contest 2022 (AtCoder Heuristic Contest 008)",
        "date": "2022-02-26",
        "rank": 159,
        "performance": 1680,
    },
    "AHC009": {
        "title": "Monoxer Programming Contest 2022 (AtCoder Heuristic Contest 009)",
        "date": "2022-03-26",
        "rank": 420,
        "performance": 1178,
    },
    "AHC010": {
        "title": "ALGO ARTIS Programming Contest 2022 (AtCoder Heuristic Contest 010)",
        "date": "2022-04-24",
        "rank": 264,
        "performance": 1396,
    },
    "AHC011": {
        "title": "AtCoder Heuristic Contest 011",
        "date": "2022-06-05",
        "rank": 231,
        "performance": 1554,
    },
    "AHC012": {
        "title": "AtCoder Heuristic Contest 012",
        "date": "2022-07-03",
        "rank": 257,
        "performance": 1471,
    },
    "AHC013": {
        "title": "RECRUIT Nihonbashi Half Marathon 2022 Summer (AHC013)",
        "date": "2022-08-16",
        "rank": 96,
        "performance": 1906,
    },
    "AHC014": {
        "title": "estie Programming Contest 2022 (AtCoder Heuristic Contest 014)",
        "date": "2022-10-01",
        "rank": 91,
        "performance": 1914,
    },
    "AHC015": {
        "title": "TOYOTA MOTOR CORPORATION Programming Contest 2022 (AtCoder Heuristic Contest 015)",
        "date": "2022-10-30",
        "rank": 417,
        "performance": 1196,
    },
    "AHC016": {
        "title": "HACK TO THE FUTURE 2023 qual (AtCoder Heuristic Contest 016)",
        "date": "2022-11-20",
        "rank": 230,
        "performance": 1652,
    },
    "AHC017": {
        "title": "THIRD PROGRAMMING CONTEST 2022 (AtCoder Heuristic Contest 017)",
        "date": "2023-02-05",
        "rank": 139,
        "performance": 1868,
    },
    "AHC018": {
        "title": "RECRUIT Nihonbashi Half Marathon 2023 Winter (AtCoder Heuristic Contest 018)",
        "date": "2023-02-26",
        "rank": 137,
        "performance": 1912,
    },
    "AHC019": {
        "title": "MC Digital Programming Contest 2023 (AtCoder Heuristic Contest 019)",
        "date": "2023-04-02",
        "rank": 37,
        "performance": 2315,
    },
    "AHC020": {
        "title": "ALGO ARTIS Programming Contest 2023 (AtCoder Heuristic Contest 020)",
        "date": "2023-06-11",
        "rank": 372,
        "performance": 1411,
    },
    "AHC021": {
        "title": "TOYOTA Programming Contest 2023 Summer (AtCoder Heuristic Contest 021)",
        "date": "2023-06-25",
        "rank": 444,
        "performance": 1386,
    },
    "AHC022": {
        "title": "RECRUIT Nihonbashi Half Marathon 2023 Summer (AtCoder Heuristic Contest 022)",
        "date": "2023-08-20",
        "rank": 103,
        "performance": 2013,
    },
    "AHC023": {
        "title": "Asprova Programming Contest 10 (AtCoder Heuristic Contest 023)",
        "date": "2023-09-10",
        "rank": 408,
        "performance": 1289,
        "task_id": "asprocon10_a",
    },
    "AHC024": {
        "title": "Marubeni Programming Contest 2023 (AtCoder Heuristic Contest 024)",
        "date": "2023-09-24",
        "rank": 220,
        "performance": 1568,
    },
    "AHC025": {
        "title": "AtCoder Heuristic Contest 025",
        "date": "2023-10-22",
        "rank": 115,
        "performance": 1968,
    },
    "AHC026": {
        "title": "Toyota Programming Contest 2023#6 (AtCoder Heuristic Contest 026)",
        "date": "2023-11-05",
        "rank": 41,
        "performance": 2329,
    },
    "AHC027": {
        "title": "HACK TO THE FUTURE 2024 (AtCoder Heuristic Contest 027)",
        "date": "2023-12-10",
        "rank": 331,
        "performance": 1467,
    },
    "AHC028": {
        "title": "ALGO ARTIS Programming Contest 2023 Winter (AtCoder Heuristic Contest 028)",
        "date": "2024-01-13",
        "rank": 124,
        "performance": 1989,
    },
    "AHC029": {
        "title": "RECRUIT Nihonbashi Half Marathon 2024 Winter (AtCoder Heuristic Contest 029)",
        "date": "2023-12-26",
        "rank": 64,
        "performance": 2140,
    },
    "AHC030": {
        "title": "THIRD Programming Contest 2023 (AtCoder Heuristic Contest 030)",
        "date": "2024-02-19",
        "rank": 86,
        "performance": 2072,
    },
    "AHC031": {
        "title": "MC Digital Programming Contest 2024 (AtCoder Heuristic Contest 031)",
        "date": "2024-04-01",
        "rank": 350,
        "performance": 1462,
    },
    "AHC032": {
        "title": "AtCoder Heuristic Contest 032",
        "date": "2024-04-07",
        "rank": 85,
        "performance": 2139,
    },
    "AHC033": {
        "title": "Toyota Programming Contest 2024#5 (AtCoder Heuristic Contest 033)",
        "date": "2024-05-27",
        "rank": 138,
        "performance": 1879,
    },
    "AHC034": {
        "title": "Toyota Programming Contest 2024#6 (AtCoder Heuristic Contest 034)",
        "date": "2024-06-16",
        "rank": 156,
        "performance": 1856,
    },
    "AHC035": {
        "title": "ALGO ARTIS Programming Contest 2024 Summer (AtCoder Heuristic Contest 035)",
        "date": "2024-07-21",
        "rank": 191,
        "performance": 1809,
    },
    "AHC036": {
        "title": "RECRUIT Nihonbashi Half Marathon 2024 Summer (AtCoder Heuristic Contest 036)",
        "date": "2024-09-02",
        "rank": 134,
        "performance": 1911,
    },
    "AHC037": {
        "title": "Asprova Programming Contest 11 (AtCoder Heuristic Contest 037)",
        "date": "2024-09-15",
        "rank": 600,
        "performance": 1064,
    },
    "AHC038": {
        "title": "Toyota Programming Contest 2024#10 (AtCoder Heuristic Contest 038)",
        "date": "2024-10-14",
        "rank": 182,
        "performance": 1736,
    },
    "AHC039": {
        "title": "THIRD Programming Contest 2024 (AtCoder Heuristic Contest 039)",
        "date": "2024-11-10",
        "rank": 43,
        "performance": 2251,
    },
    "AHC040": {
        "title": "HACK TO THE FUTURE 2025 (AtCoder Heuristic Contest 040)",
        "date": "2024-12-09",
        "rank": 137,
        "performance": 1903,
    },
    "AHC041": {
        "title": "ALGO ARTIS Programming Contest 2025 Winter (AtCoder Heuristic Contest 041)",
        "date": "2025-01-19",
        "rank": 113,
        "performance": 2046,
    },
    "AHC042": {
        "title": "AtCoder Heuristic Contest 042",
        "date": "2025-02-02",
        "rank": None,
        "performance": None,
    },
    "AHC043": {
        "title": "RECRUIT Nihonbashi Half Marathon 2025 Winter (AtCoder Heuristic Contest 043)",
        "date": "2025-02-24",
        "rank": 135,
        "performance": 1951,
    },
    "AHC045": {
        "title": "THIRD Programming Contest 2025 (AtCoder Heuristic Contest 045)",
        "date": "2025-04-07",
        "rank": 106,
        "performance": 2033,
    },
    "AHC047": {
        "title": "Toyota Programming Contest 2025#2 (AtCoder Heuristic Contest 047)",
        "date": "2025-05-18",
        "rank": 258,
        "performance": 1684,
    },
    "AHC048": {
        "title": "MC Digital Programming Contest 2025 (AtCoder Heuristic Contest 048)",
        "date": "2025-06-09",
        "rank": 48,
        "performance": 2346,
    },
    "AHC049": {
        "title": "Toyota Programming Contest 2025#3 (AtCoder Heuristic Contest 049)",
        "date": "2025-06-21",
        "rank": 415,
        "performance": 1376,
    },
    "AHC050": {
        "title": "AtCoder Heuristic Contest 050",
        "date": "2025-07-06",
        "rank": 46,
        "performance": 2374,
    },
    "AHC051": {
        "title": "THIRD Programming Contest 2025 Summer (AtCoder Heuristic Contest 051)",
        "date": "2025-08-11",
        "rank": 1103,
        "performance": None,
    },
    "AHC052": {
        "title": "AtCoder Heuristic Contest 052",
        "date": "2025-08-23",
        "rank": 776,
        "performance": None,
    },
    "AHC053": {
        "title": "12th Asprova Programming Contest (AtCoder Heuristic Contest 053)",
        "date": "2025-09-13",
        "rank": 126,
        "performance": 1986,
    },
    "AHC054": {
        "title": "ALGO ARTIS Programming Contest 2025 Summer (AtCoder Heuristic Contest 054)",
        "date": "2025-09-29",
        "rank": 171,
        "performance": 1868,
    },
    "AHC055": {
        "title": "RECRUIT Nihonbashi Half Marathon 2025 Autumn (AtCoder Heuristic Contest 055)",
        "date": "2025-10-19",
        "rank": 170,
        "performance": 1850,
    },
    "AHC056": {
        "title": "HACK TO THE FUTURE 2026 (AtCoder Heuristic Contest 056)",
        "date": "2025-11-17",
        "rank": 252,
        "performance": 1711,
    },
    "AHC058": {
        "title": "ALGO ARTIS Programming Contest 2025 December (AtCoder Heuristic Contest 058)",
        "date": "2025-12-14",
        "rank": 138,
        "performance": 1936,
    },
    "AHC059": {
        "title": "AtCoder Heuristic Contest 059",
        "date": "2026-01-10",
        "rank": 42,
        "performance": 2344,
    },
    "AHC061": {
        "title": "THIRD Programming Contest 2026 (AtCoder Heuristic Contest 061)",
        "date": "2026-02-23",
        "rank": 30,
        "performance": 2500,
    },
    "AHC062": {
        "title": "UNIQUE VISION Programming Contest 2026 Spring (AtCoder Heuristic Contest 062)",
        "date": "2026-03-14",
        "rank": 52,
        "performance": 2236,
    },
    "AHC063": {
        "title": "AtCoder Heuristic Contest 063",
        "date": "2026-04-13",
        "rank": 4,
        "performance": 3023,
    },
    "AHC064": {
        "title": "JR WEST and ALGO ARTIS Programming Contest (AtCoder Heuristic Contest 064)",
        "date": "2026-04-26",
        "rank": 799,
        "performance": None,
    },
    "masters2024-final": {
        "title": "第一回マスターズ選手権 -決勝-",
        "date": "2024-04-20",
        "rank": None,
        "performance": None,
        "contest_type": "masters",
        "task_id": "masters2024_final_a",
    },
    "masters2024-qual": {
        "title": "第一回マスターズ選手権 -予選-",
        "date": "2024-03-03",
        "rank": None,
        "performance": None,
        "contest_type": "masters",
        "contest_slug": "masters-qual",
        "task_id": "masters_qual_a",
    },
    "masters2025-final": {
        "title": "第二回マスターズ選手権 -決勝-",
        "date": "2025-04-19",
        "rank": None,
        "performance": None,
        "contest_type": "masters",
        "task_id": "masters2025_final_a",
    },
    "masters2025-qual": {
        "title": "The 2nd Masters Championship -qual-",
        "date": "2025-03-02",
        "rank": None,
        "performance": None,
        "contest_type": "masters",
        "task_id": "masters2025_qual_a",
    },
    "masters2026-qual": {
        "title": "The 3rd Masters Championship -qual-",
        "date": "2026-03-01",
        "rank": None,
        "performance": None,
        "contest_type": "masters",
        "task_id": "masters2026_qual_a",
    },
}


def build_problem_url(dir_name: str, data: dict) -> str:
    slug = data.get("contest_slug", dir_name.lower())
    if data.get("task_id"):
        return f"https://atcoder.jp/contests/{slug}/tasks/{data['task_id']}"
    if data.get("contest_type") == "masters":
        return f"https://atcoder.jp/contests/{slug}/tasks"
    return f"https://atcoder.jp/contests/{slug}/tasks/{slug}_a"


def build_editorial_url(dir_name: str, data: dict) -> str:
    slug = data.get("contest_slug", dir_name.lower())
    return f"https://atcoder.jp/contests/{slug}/editorial"


def build_contest_url(dir_name: str, data: dict) -> str:
    slug = data.get("contest_slug", dir_name.lower())
    return f"https://atcoder.jp/contests/{slug}"


class AtCoderStatementParser(HTMLParser):
    """Extract Japanese section paragraphs from an AtCoder task page."""

    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.in_ja = False
        self.ja_depth = 0
        self.current_tag = None
        self.current_heading = None
        self.current_parts = []
        self.sections = {}

    def handle_starttag(self, tag, attrs):
        attrs_dict = dict(attrs)
        classes = attrs_dict.get("class", "").split()
        if tag == "span" and "lang-ja" in classes and not self.in_ja:
            self.in_ja = True
            self.ja_depth = 1
            return

        if not self.in_ja:
            return

        self.ja_depth += 1
        if tag in {"h3", "p"}:
            self.current_tag = tag
            self.current_parts = []

    def handle_endtag(self, tag):
        if not self.in_ja:
            return

        if tag == self.current_tag:
            text = normalize_text("".join(self.current_parts))
            if text:
                if self.current_tag == "h3":
                    self.current_heading = text
                    self.sections.setdefault(text, [])
                elif self.current_tag == "p" and self.current_heading:
                    self.sections.setdefault(self.current_heading, []).append(text)
            self.current_tag = None
            self.current_parts = []

        self.ja_depth -= 1
        if self.ja_depth <= 0:
            self.in_ja = False

    def handle_data(self, data):
        if self.in_ja and self.current_tag:
            self.current_parts.append(data)


class AtCoderEditorialParser(HTMLParser):
    """Extract editorial links from an AtCoder editorial index page."""

    def __init__(self, base_url: str):
        super().__init__(convert_charrefs=True)
        self.base_url = base_url
        self.seen_section = False
        self.in_footer = False
        self.in_li = False
        self.in_anchor = False
        self.current_href = None
        self.current_prefix_parts = []
        self.current_anchor_parts = []
        self.current_anchors = []
        self.editorials = []
        self.seen_urls = set()

    def handle_starttag(self, tag, attrs):
        attrs_dict = dict(attrs)
        classes = attrs_dict.get("class", "").split()
        if tag == "footer" or "footer" in classes:
            self.in_footer = True
            return

        if self.in_footer:
            return

        if tag == "h3":
            self.seen_section = True
            return

        if self.seen_section and tag == "li":
            self.in_li = True
            self.current_prefix_parts = []
            self.current_anchors = []
            return

        if self.in_li and tag == "a":
            self.in_anchor = True
            self.current_href = attrs_dict.get("href")
            self.current_anchor_parts = []

    def handle_endtag(self, tag):
        if tag == "footer":
            self.in_footer = False
            return

        if self.in_footer:
            return

        if self.in_li and self.in_anchor and tag == "a":
            title = normalize_text("".join(self.current_anchor_parts))
            if title and self.current_href:
                url = urllib.parse.urljoin(self.base_url, self.current_href)
                self.current_anchors.append({"title": title, "url": url})
            self.in_anchor = False
            self.current_href = None
            self.current_anchor_parts = []
            return

        if self.in_li and tag == "li":
            rank = parse_editorial_rank("".join(self.current_prefix_parts))
            for anchor in self.current_anchors:
                if "/users/" in anchor["url"]:
                    continue
                if anchor["title"] == "Image":
                    continue
                if anchor["url"] in self.seen_urls:
                    continue
                self.editorials.append(
                    {
                        "title": anchor["title"],
                        "url": anchor["url"],
                        "rank": rank,
                    }
                )
                self.seen_urls.add(anchor["url"])
                break
            self.in_li = False
            self.current_prefix_parts = []
            self.current_anchors = []

    def handle_data(self, data):
        if self.in_li and self.in_anchor:
            self.current_anchor_parts.append(data)
        elif self.in_li and not self.current_anchors:
            self.current_prefix_parts.append(data)


def normalize_text(text: str) -> str:
    text = re.sub(r"\s+", " ", text)
    text = text.replace(" ,", ",").replace(" .", ".")
    text = text.replace("( ", "(").replace(" )", ")")
    return text.strip()


def parse_editorial_rank(text: str) -> int | None:
    text = normalize_text(text)
    match = re.search(r"\b(\d+)(?:st|nd|rd|th)\s+place\b", text)
    if match:
        return int(match.group(1))
    match = re.search(r"(\d+)\s*位", text)
    if match:
        return int(match.group(1))
    return None


def format_duration(minutes: int) -> str:
    days, remainder = divmod(minutes, 24 * 60)
    hours, mins = divmod(remainder, 60)
    parts = []
    if days:
        parts.append(f"{days}日")
    if hours:
        parts.append(f"{hours}時間")
    if mins:
        parts.append(f"{mins}分")
    if not parts:
        return "0分"
    return "".join(parts)


def term_tag(duration_minutes: int) -> str:
    if duration_minutes <= 24 * 60:
        return "短期"
    return "長期"


def with_term_tag(tags: list[str], duration_minutes: int | None) -> list[str]:
    filtered_tags = [
        tag for tag in tags if tag not in {"short-term", "long-term", "短期", "長期"}
    ]
    if duration_minutes is not None:
        filtered_tags.append(term_tag(duration_minutes))
    return filtered_tags


def order_meta(meta: dict) -> dict:
    key_order = [
        "id",
        "title",
        "date",
        "startDate",
        "endDate",
        "durationMinutes",
        "durationText",
        "rank",
        "score",
        "performance",
        "language",
        "submissionUrl",
        "tags",
        "visualizer",
        "thumbnail",
        "problemUrl",
        "description",
        "editorials",
    ]
    ordered = {key: meta[key] for key in key_order if key in meta}
    for key, value in meta.items():
        if key not in ordered:
            ordered[key] = value
    return ordered


def should_materialize_dir(dir_name: str, data: dict, existing_dirs: set[str]) -> bool:
    return dir_name in existing_dirs or data.get("rank") is not None


def ensure_placeholder_src(dir_path: str) -> None:
    src_path = os.path.join(dir_path, "src")
    gitkeep_path = os.path.join(src_path, ".gitkeep")
    if os.path.exists(src_path):
        return
    os.makedirs(src_path, exist_ok=True)
    with open(gitkeep_path, "w", encoding="utf-8"):
        pass


def detect_visualizer(dir_path: str) -> str | None:
    visualizer_path = os.path.join(dir_path, "visualizer.mp4")
    if os.path.exists(visualizer_path):
        return "visualizer.mp4"
    return None


def detect_thumbnail(dir_path: str) -> str | None:
    thumbnail_path = os.path.join(dir_path, "thumbnail.webp")
    if os.path.exists(thumbnail_path):
        return "thumbnail.webp"
    return None


def detect_language(dir_path: str) -> str | None:
    src_path = os.path.join(dir_path, "src")
    candidates = (
        ("last_submission.cpp", "C++"),
        ("main.cpp", "C++"),
        ("last_submission.rs", "Rust"),
        ("main.rs", "Rust"),
        ("last_submission.py", "Python"),
        ("main.py", "Python"),
    )
    for filename, language in candidates:
        if os.path.exists(os.path.join(src_path, filename)):
            return language
    return None


def normalize_language(language: str | None) -> str | None:
    if not language:
        return None
    if "C++" in language:
        return "C++"
    if "Rust" in language:
        return "Rust"
    if "Python" in language or "PyPy" in language:
        return "Python"
    return language.split(" (", 1)[0]


def build_submission_url(dir_name: str, data: dict) -> str | None:
    submission_id = data.get("submission_id")
    if submission_id is None:
        return None
    slug = data.get("contest_slug", dir_name.lower())
    return f"https://atcoder.jp/contests/{slug}/submissions/{submission_id}"


def fetch_user_submissions(user: str) -> list[dict]:
    submissions = []
    from_second = 0
    while True:
        url = (
            "https://kenkoooo.com/atcoder/atcoder-api/v3/user/submissions"
            f"?user={urllib.parse.quote(user)}&from_second={from_second}"
        )
        request = urllib.request.Request(
            url,
            headers={
                "User-Agent": "Mozilla/5.0",
                "Accept": "application/json",
                "Referer": "https://kenkoooo.com/atcoder/",
            },
        )
        with urllib.request.urlopen(request, timeout=60) as response:
            batch = json.load(response)
        if not batch:
            break

        submissions.extend(batch)
        next_from_second = max(submission["epoch_second"] for submission in batch) + 1
        if next_from_second <= from_second:
            break
        from_second = next_from_second

    return submissions


def build_latest_submission_map(submissions: list[dict]) -> dict[str, dict]:
    latest = {}
    for submission in submissions:
        contest_id = submission.get("contest_id")
        if not contest_id:
            continue
        current = latest.get(contest_id)
        submission_key = (submission.get("epoch_second", 0), submission.get("id", 0))
        current_key = (
            current.get("epoch_second", 0),
            current.get("id", 0),
        ) if current else None
        if current is None or submission_key > current_key:
            latest[contest_id] = submission
    return latest


def contest_slug(dir_name: str, data: dict) -> str:
    return data.get("contest_slug", dir_name.lower())


def fetch_schedule(contest_url: str) -> dict:
    request = urllib.request.Request(
        contest_url,
        headers={"User-Agent": "Heuristic-Monorepo meta generator"},
    )
    with urllib.request.urlopen(request, timeout=20) as response:
        html = response.read().decode("utf-8", errors="replace")

    text = html_lib.unescape(re.sub(r"<[^>]+>", " ", html))
    text = normalize_text(text)
    match = re.search(
        r"Contest Duration:\s*"
        r"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}[+-]\d{4})"
        r"\s*-\s*"
        r"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}[+-]\d{4})",
        text,
    )
    if not match:
        return {}

    start = datetime.strptime(match.group(1), "%Y-%m-%d %H:%M:%S%z")
    end = datetime.strptime(match.group(2), "%Y-%m-%d %H:%M:%S%z")
    duration_minutes = int((end - start).total_seconds() // 60)
    return {
        "startDate": start.isoformat(timespec="seconds"),
        "endDate": end.isoformat(timespec="seconds"),
        "durationMinutes": duration_minutes,
        "durationText": format_duration(duration_minutes),
    }


def fetch_description(problem_url: str) -> str:
    request = urllib.request.Request(
        problem_url,
        headers={"User-Agent": "Heuristic-Monorepo meta generator"},
    )
    with urllib.request.urlopen(request, timeout=20) as response:
        html = response.read().decode("utf-8", errors="replace")

    parser = AtCoderStatementParser()
    parser.feed(html)

    for heading in ("問題文", "ストーリー"):
        paragraphs = parser.sections.get(heading, [])
        if paragraphs:
            return paragraphs[0]
    return ""


def fetch_editorials(editorial_url: str) -> list[dict[str, str]]:
    request = urllib.request.Request(
        editorial_url,
        headers={"User-Agent": "Heuristic-Monorepo meta generator"},
    )
    with urllib.request.urlopen(request, timeout=20) as response:
        html = response.read().decode("utf-8", errors="replace")

    parser = AtCoderEditorialParser(editorial_url)
    parser.feed(html)
    return parser.editorials


def generate_meta(
    dir_name: str,
    data: dict,
    dir_path: str | None = None,
    fetch_descriptions: bool = False,
    fetch_schedule_data: bool = False,
) -> dict:
    if dir_path is None:
        dir_path = dir_name
    problem_url = build_problem_url(dir_name, data)
    schedule = {}
    if fetch_schedule_data:
        schedule = fetch_schedule(build_contest_url(dir_name, data))
    meta = {
        "id": dir_name,
        "title": data["title"],
        "date": data["date"],
        **schedule,
        "rank": data["rank"],
        "score": None,
        "performance": data.get("performance"),
        "language": detect_language(dir_path),
        "submissionUrl": build_submission_url(dir_name, data),
        "tags": with_term_tag([], schedule.get("durationMinutes")),
        "visualizer": detect_visualizer(dir_path),
        "thumbnail": detect_thumbnail(dir_path),
        "problemUrl": problem_url,
        "description": "",
        "editorials": [],
    }
    if fetch_descriptions:
        meta["description"] = fetch_description(problem_url)
    return order_meta(meta)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--fetch-descriptions",
        action="store_true",
        help="Fetch task descriptions from AtCoder problem pages.",
    )
    parser.add_argument(
        "--fetch-editorials",
        action="store_true",
        help="Fetch editorial links from AtCoder editorial pages.",
    )
    parser.add_argument(
        "--fetch-schedule",
        action="store_true",
        help="Fetch contest start/end times from AtCoder contest pages.",
    )
    parser.add_argument(
        "--fetch-submissions",
        action="store_true",
        help="Fetch latest submission URLs from AtCoder Problems submissions API.",
    )
    parser.add_argument(
        "--atcoder-user",
        default=ATCODER_USER,
        help="AtCoder user ID used by --fetch-submissions.",
    )
    parser.add_argument(
        "--update-existing",
        action="store_true",
        help="Update existing meta.json files instead of only creating missing files.",
    )
    parser.add_argument(
        "--force-description",
        action="store_true",
        help="Overwrite existing non-empty descriptions when fetching descriptions.",
    )
    parser.add_argument(
        "--force-editorials",
        action="store_true",
        help="Overwrite existing non-empty editorial links when fetching editorials.",
    )
    parser.add_argument(
        "--sleep",
        type=float,
        default=0.5,
        help="Seconds to wait between AtCoder requests.",
    )
    args = parser.parse_args()

    script_dir = os.path.dirname(os.path.abspath(__file__))
    existing_dirs = {
        dir_name
        for dir_name in os.listdir(script_dir)
        if os.path.isdir(os.path.join(script_dir, dir_name)) and not dir_name.startswith(".")
    }
    latest_submissions = {}
    if args.fetch_submissions:
        try:
            latest_submissions = build_latest_submission_map(
                fetch_user_submissions(args.atcoder_user)
            )
        except (urllib.error.URLError, TimeoutError, OSError) as exc:
            print(f"[ERROR] failed to fetch submissions: {exc}", file=sys.stderr)

    for dir_name, data in sorted(CONTEST_DATA.items()):
        dir_path = os.path.join(script_dir, dir_name)
        if not should_materialize_dir(dir_name, data, existing_dirs):
            continue

        os.makedirs(dir_path, exist_ok=True)
        if data.get("rank") is not None:
            ensure_placeholder_src(dir_path)

        meta = generate_meta(dir_name, data, dir_path)
        meta_path = os.path.join(dir_path, "meta.json")

        if os.path.exists(meta_path):
            if not args.update_existing:
                print(f"[SKIP] {dir_name}/meta.json already exists")
                continue

            with open(meta_path, encoding="utf-8") as f:
                meta = json.load(f)

            changed = False
            generated_meta = generate_meta(dir_name, data, dir_path)
            for key, value in generated_meta.items():
                if key not in meta:
                    meta[key] = value
                    changed = True

            for key in (
                "id",
                "title",
                "date",
                "rank",
                "performance",
                "language",
                "submissionUrl",
                "visualizer",
                "thumbnail",
                "problemUrl",
            ):
                if meta.get(key) != generated_meta[key]:
                    meta[key] = generated_meta[key]
                    changed = True

            if args.fetch_descriptions and (
                args.force_description or not meta.get("description")
            ):
                try:
                    description = fetch_description(meta["problemUrl"])
                except (urllib.error.URLError, TimeoutError, OSError) as exc:
                    print(f"[ERROR] {dir_name}: failed to fetch description: {exc}", file=sys.stderr)
                else:
                    if description and description != meta.get("description"):
                        meta["description"] = description
                        changed = True
                    time.sleep(args.sleep)

            if args.fetch_editorials and (
                args.force_editorials or not meta.get("editorials")
            ):
                editorial_url = build_editorial_url(dir_name, data)
                try:
                    editorials = fetch_editorials(editorial_url)
                except (urllib.error.URLError, TimeoutError, OSError) as exc:
                    print(f"[ERROR] {dir_name}: failed to fetch editorials: {exc}", file=sys.stderr)
                else:
                    if editorials != meta.get("editorials"):
                        meta["editorials"] = editorials
                        changed = True
                    time.sleep(args.sleep)

            if args.fetch_schedule:
                contest_url = build_contest_url(dir_name, data)
                try:
                    schedule = fetch_schedule(contest_url)
                except (urllib.error.URLError, TimeoutError, OSError) as exc:
                    print(f"[ERROR] {dir_name}: failed to fetch schedule: {exc}", file=sys.stderr)
                else:
                    for key, value in schedule.items():
                        if meta.get(key) != value:
                            meta[key] = value
                            changed = True
                    duration_minutes = schedule.get("durationMinutes")
                    tags = with_term_tag(meta.get("tags", []), duration_minutes)
                    if meta.get("tags") != tags:
                        meta["tags"] = tags
                        changed = True
                    time.sleep(args.sleep)

            if args.fetch_submissions:
                slug = contest_slug(dir_name, data)
                submission = latest_submissions.get(slug)
                submission_url = None
                language = detect_language(dir_path)
                if submission:
                    submission_url = (
                        f"https://atcoder.jp/contests/{slug}/submissions/{submission['id']}"
                    )
                    language = normalize_language(submission.get("language")) or language
                if meta.get("submissionUrl") != submission_url:
                    meta["submissionUrl"] = submission_url
                    changed = True
                if meta.get("language") != language:
                    meta["language"] = language
                    changed = True

            if not changed:
                ordered_meta = order_meta(meta)
                if list(ordered_meta.keys()) == list(meta.keys()):
                    print(f"[SKIP] {dir_name}/meta.json is up to date")
                    continue
                meta = ordered_meta
                changed = True
            else:
                meta = order_meta(meta)

            with open(meta_path, "w", encoding="utf-8") as f:
                json.dump(meta, f, indent=2, ensure_ascii=False)
                f.write("\n")

            print(f"[UPDATE] {dir_name}/meta.json")
            continue

        if args.fetch_descriptions:
            try:
                meta["description"] = fetch_description(meta["problemUrl"])
            except (urllib.error.URLError, TimeoutError, OSError) as exc:
                print(f"[ERROR] {dir_name}: failed to fetch description: {exc}", file=sys.stderr)
            else:
                time.sleep(args.sleep)

        if args.fetch_editorials:
            editorial_url = build_editorial_url(dir_name, data)
            try:
                meta["editorials"] = fetch_editorials(editorial_url)
            except (urllib.error.URLError, TimeoutError, OSError) as exc:
                print(f"[ERROR] {dir_name}: failed to fetch editorials: {exc}", file=sys.stderr)
            else:
                time.sleep(args.sleep)

        if args.fetch_schedule:
            contest_url = build_contest_url(dir_name, data)
            try:
                schedule = fetch_schedule(contest_url)
            except (urllib.error.URLError, TimeoutError, OSError) as exc:
                print(f"[ERROR] {dir_name}: failed to fetch schedule: {exc}", file=sys.stderr)
            else:
                meta.update(schedule)
                meta["tags"] = with_term_tag(meta.get("tags", []), schedule.get("durationMinutes"))
                time.sleep(args.sleep)

        if args.fetch_submissions:
            slug = contest_slug(dir_name, data)
            submission = latest_submissions.get(slug)
            if submission:
                meta["submissionUrl"] = (
                    f"https://atcoder.jp/contests/{slug}/submissions/{submission['id']}"
                )
                meta["language"] = normalize_language(submission.get("language")) or meta.get("language")

        with open(meta_path, "w", encoding="utf-8") as f:
            json.dump(order_meta(meta), f, indent=2, ensure_ascii=False)
            f.write("\n")

        print(f"[CREATE] {dir_name}/meta.json")


if __name__ == "__main__":
    main()
