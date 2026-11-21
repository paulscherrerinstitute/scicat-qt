#!/usr/bin/env python3
"""Run the SciCat GUI scenarios in tests/parity/scenarios against a driver and compare what came out.

  run_scenarios.py --driver <executable> [-k <substring> | --name <scenario>] [--record] [--outputs-only] [--tolerate-known]
                   [--differences <folder>]

Each scenario runs in its own folder, with its own fake SciCat (fake_scicat.py), its own settings file and the
stand-ins for scicat-cli and klist (bin/) first on PATH. The driver is started once per scenario with the
environment described in README.md ("Driver protocol"); it plays the steps on the GUI and writes observations.json.

Compared afterwards, in this order:
  outputs   requests  every request the fake SciCat received
            cli       every call of scicat-cli (its arguments)
            files     files the GUI wrote (metadata.json, filelisting.txt, ingestion.log, proposal files, copies)
            settings  the settings file after the run
  GUI       steps     the result the driver reports for each step
            messages  dialogs (exact) and banners shown
            snapshots what the GUI showed at the snapshot steps
--outputs-only compares only the first group. --record writes what was observed into the scenario's "expect"
(keeping "$known" marks), for a new scenario or after a deliberate change; review the diff before committing it.

--unordered-reads is for a GUI that sends requests without waiting for the answer to the one before: GET requests
that follow each other directly are compared in the order of their paths, not in the order they arrived in.

--differences <folder> is for a second GUI that does some things differently on purpose. What it does differently is
kept there and not in the scenarios: <folder>/<scenario>.json, and <folder>/_every.json for what differs in every
scenario. Each is {"why": ..., "expect": {...}}, where "expect" holds only the parts that differ and is laid over the
scenario's own: objects key by key, {"$at": {"2": ...}} for single items of a list, {"$remove": true} takes a key
away, anything else replaces what is there.
Everything else has to equal what the scenario expects. With --record the differences are written to that folder
(keeping each "why") and the scenarios stay as they are.
"""
import argparse, configparser, json, os, pathlib, re, shutil, signal, socket, subprocess, sys, tempfile, time

HERE = pathlib.Path(__file__).resolve().parent
OUTPUT_SECTIONS = ("requests", "cli", "files", "settings")
GUI_SECTIONS = ("steps", "messages", "snapshots")
DEFAULT_SETTINGS = {"version": "0.4", "serverID": 1, "logPath": "${LOG}"}
DEFAULT_DATA = ["scan_0001/frame_0001.h5", "scan_0001/frame_0002.h5", "scan_0002/frame_0001.h5", "notes.txt"]


# ---------------------------------------------------------------------------------------------------------------
# placeholders: ${HOME} ${LOG} ${DATA} ${OUT} ${CLI} ${BIN} are the paths of this run and ${SCICAT} is the address of
# its fake SciCat (host:port); in expectations ${STAMP} is the time in seconds (10 digits) in the name of an ingestion
# folder and ${ANY} is anything

def substitute(value, variables):
    if isinstance(value, str):
        return re.sub(r"\$\{(\w+)\}", lambda m: variables.get(m.group(1), m.group(0)), value)
    if isinstance(value, list):
        return [substitute(v, variables) for v in value]
    if isinstance(value, dict):
        return {k: substitute(v, variables) for k, v in value.items()}
    return value


def pattern(text, variables):
    parts, pos = [], 0
    for m in re.finditer(r"\$\{(\w+)\}", text):
        parts.append(re.escape(text[pos:m.start()]))
        name = m.group(1)
        parts.append(r"\d{10}" if name == "STAMP" else ".*" if name == "ANY" else
                     re.escape(variables[name]) if name in variables else re.escape(m.group(0)))
        pos = m.end()
    parts.append(re.escape(text[pos:]))
    return re.compile("".join(parts), re.S)


def generalise(value, variables):
    """the reverse of substitute(), for --record"""
    if isinstance(value, str):
        for name, path in sorted(variables.items(), key=lambda kv: -len(kv[1])):
            if name in ("HOME", "LOG", "DATA", "OUT", "CLI", "BIN", "SCICAT") and path:
                value = value.replace(path, "${%s}" % name)
        return re.sub(r"(/ingestion/[^/\s]*)\d{10}(?=/|\s|$)", r"\1${STAMP}", value)
    if isinstance(value, list):
        return [generalise(v, variables) for v in value]
    if isinstance(value, dict):
        return {k: generalise(v, variables) for k, v in value.items()}
    return value


# ---------------------------------------------------------------------------------------------------------------
# matching. Plain values must be equal (strings with placeholders as patterns), dicts must have the same keys, lists
# the same length. Operators:
#   {"$count": n, "$includes": [...]}  a list of n items that contains a partial match for each listed item
#   {"$countWhere": [[{partial}, n], ...]}  so many items match each partial dict (combines with $count/$includes)
#   {"$partial": {...}}                  only the listed keys of a dict are compared
#   {"$known": "<id>", "value": v}      v is what main does today, but it is a known bug or a deliberate difference of
#                                        the QML GUI; with --tolerate-known a mismatch here is reported, not counted

def match(expected, actual, variables, where, problems, known, partial=False):
    if isinstance(expected, dict) and "$known" in expected:
        sub = []
        match(expected.get("value"), actual, variables, where, sub, known, partial)
        if sub:
            known.append((expected["$known"], sub))
        return
    if isinstance(expected, dict) and "$partial" in expected:
        return match(expected["$partial"], actual, variables, where, problems, known, partial=True)
    if isinstance(expected, dict) and any(k in expected for k in ("$count", "$includes", "$countWhere")):
        if not isinstance(actual, list):
            problems.append(f"{where}: expected a list, got {short(actual)}")
            return
        if "$count" in expected and len(actual) != expected["$count"]:
            problems.append(f"{where}: {len(actual)} items, expected {expected['$count']}")
        for item in expected.get("$includes", []):
            if not any(not trial(item, a, variables, known, True) for a in actual):
                problems.append(f"{where}: no item matches {short(item)}")
        for item, n in expected.get("$countWhere", []):
            got = sum(1 for a in actual if not trial(item, a, variables, known, True))
            if got != n:
                problems.append(f"{where}: {got} items match {short(item)}, expected {n}")
        return
    if isinstance(expected, dict):
        if not isinstance(actual, dict):
            problems.append(f"{where}: expected an object, got {short(actual)}")
            return
        for key in expected:
            if key not in actual:
                problems.append(f"{where}.{key}: missing")
            else:
                match(expected[key], actual[key], variables, f"{where}.{key}", problems, known, partial)
        if not partial:
            for key in actual:
                if key not in expected:
                    problems.append(f"{where}.{key}: not expected, got {short(actual[key])}")
        return
    if isinstance(expected, list):
        if not isinstance(actual, list):
            problems.append(f"{where}: expected a list, got {short(actual)}")
            return
        if len(expected) != len(actual):
            problems.append(f"{where}: {len(actual)} items, expected {len(expected)}")
        for i, (e, a) in enumerate(zip(expected, actual)):
            match(e, a, variables, f"{where}[{i}]", problems, known, partial)
        return
    if isinstance(expected, str) and isinstance(actual, str):
        if not pattern(expected, variables).fullmatch(actual):
            problems.append(f"{where}: {short(actual)}, expected {short(expected)}")
        return
    if expected != actual or type(expected) is not type(actual):
        problems.append(f"{where}: {short(actual)}, expected {short(expected)}")


def trial(expected, actual, variables, known, partial):
    problems = []
    match(expected, actual, variables, "", problems, known, partial)
    return problems


def short(value):
    text = json.dumps(value)
    return text if len(text) < 160 else text[:157] + "..."


def condense(new, old, limit=12):
    """for --record: long lists (a retrieve list of 250 datasets) become their length, first and last item, plus the
    counts of any $countWhere the old expectation had"""
    if isinstance(new, list):
        if len(new) <= limit:
            return [condense(n, o, limit) for n, o in zip(new, old if isinstance(old, list) and len(old) == len(new)
                                                         else [None] * len(new))]
        out = {"$count": len(new), "$includes": [new[0], new[-1]]}
        if isinstance(old, dict) and "$countWhere" in old:
            out["$countWhere"] = [[item, sum(1 for a in new if not trial(item, a, {}, [], True))]
                                  for item, _ in old["$countWhere"]]
        return out
    if isinstance(new, dict):
        if isinstance(old, dict) and "$known" in old:
            old = old.get("value")
        return {k: condense(v, old.get(k) if isinstance(old, dict) else None, limit) for k, v in new.items()}
    return new


def rewrap(old, new):
    """keep the $known marks of an existing expectation when recording a new one"""
    if isinstance(old, dict) and "$known" in old:
        return {"$known": old["$known"], "value": rewrap(old.get("value"), new)}
    if isinstance(old, dict) and isinstance(new, dict):
        return {k: rewrap(old.get(k), v) for k, v in new.items()}
    if isinstance(old, list) and isinstance(new, list) and len(old) == len(new):
        return [rewrap(o, n) for o, n in zip(old, new)]
    return new


# ---------------------------------------------------------------------------------------------------------------
# --unordered-reads: a GUI that does not wait for one answer before it asks the next question sends its reads in an
# order that is not its own doing. GET requests that follow each other directly are then compared in the order of
# their paths (requests to the same path keep the order they came in); a request that changes something stays
# where it is.

def by_path(requests):
    if not isinstance(requests, list):
        return requests

    def part(request, key):
        if isinstance(request, dict) and "$known" in request:
            request = request.get("value")
        return request.get(key) if isinstance(request, dict) else None

    out, run = [], []
    for request in requests:
        if part(request, "method") == "GET":
            run.append(request)
            continue
        out += sorted(run, key=lambda r: str(part(r, "path")))
        run = []
        out.append(request)
    return out + sorted(run, key=lambda r: str(part(r, "path")))


# ---------------------------------------------------------------------------------------------------------------
# the differences of a second GUI (--differences)

REMOVE = {"$remove": True}


def has_operator(value):
    """$count, $known, ... but not a path that starts with a placeholder such as ${LOG}"""
    return isinstance(value, dict) and any(k.startswith("$") and not k.startswith("${") for k in value)


def overlay(expected, differences):
    """the scenario's expectation with a GUI's differences laid over it"""
    if isinstance(expected, dict) and "$known" in expected:
        expected = expected.get("value")      # what the second GUI does at a marked place is said by its differences
    if isinstance(differences, dict) and set(differences) == {"$at"} and isinstance(expected, list):
        out = list(expected)
        for index, value in differences["$at"].items():
            out[int(index)] = overlay(expected[int(index)], value)
        return out
    if not isinstance(expected, dict) or not isinstance(differences, dict) or has_operator(expected) or has_operator(differences):
        return differences
    out = dict(expected)
    for key, value in differences.items():
        if value == REMOVE:
            out.pop(key, None)
        else:
            out[key] = overlay(expected[key], value) if key in expected else value
    return out


def load_differences(folder, name):
    """(the expectation differences of every scenario, those of this one, this one's "why")"""
    loaded = []
    for stem in ("_every", name):
        path = folder / f"{stem}.json"
        loaded.append(json.loads(path.read_text()) if path.exists() else {})
    return loaded[0].get("expect", {}), loaded[1].get("expect", {}), loaded[1].get("why")


SAME = object()


def difference(expected, actual, variables, record):
    """what a differences file has to say so that "expected" accepts "actual": SAME when nothing, else the smallest
    part that differs, written the way --record writes expectations"""
    known = []
    if not trial(expected, actual, variables, known, False) and not known:
        return SAME
    if isinstance(expected, dict) and "$known" in expected:
        return difference(expected.get("value"), actual, variables, record)
    if isinstance(expected, list) and isinstance(actual, list) and len(expected) == len(actual):
        at = {}
        for index, (e, a) in enumerate(zip(expected, actual)):
            part = difference(e, a, variables, record)
            if part is not SAME:
                at[str(index)] = part
        return {"$at": at}
    if isinstance(expected, dict) and isinstance(actual, dict) and not has_operator(expected):
        out = {}
        for key in list(expected) + [k for k in actual if k not in expected]:
            if key not in actual:
                out[key] = REMOVE
            elif key not in expected:
                out[key] = record(actual[key])
            else:
                part = difference(expected[key], actual[key], variables, record)
                if part is not SAME:
                    out[key] = part
        return out
    return record(actual)


def file_differences(expected, work, variables, written, before):
    """the same for the files a GUI wrote. "written" is every file of the run as --record writes it, by its path;
    "before" is what the differences said so far: what still holds is kept as it is written there, so that a check
    made looser by hand (a "contains" with the part of a line that is the same on every system) survives"""
    def holds(file_pattern, spec):
        problems, known = [], []
        check_file(file_pattern, spec, work, variables, problems, known)
        return not problems and not known

    out = {}
    passing = [pattern(p, variables) for p, spec in expected.items() if holds(p, spec)]
    kept = [pattern(p, variables) for p, spec in before.items() if spec != REMOVE and holds(p, spec)]
    out.update({p: spec for p, spec in before.items() if spec != REMOVE and holds(p, spec)})
    record = lambda value: condense(generalise(value, variables), None)
    for path, spec in written.items():
        if any(rx.fullmatch(path) for rx in passing + kept):
            continue
        same_file = [p for p in expected if pattern(p, variables).fullmatch(path) and expected[p] != {"absent": True}]
        if same_file:       # the scenario's file with other content: only what differs in it
            out[same_file[0]] = difference(expected[same_file[0]], spec, variables, record)
        else:               # a file the scenario does not know
            out[generalise(path, variables)] = generalise(spec, variables)
    for file_pattern, spec in expected.items():
        if not holds(file_pattern, spec) and file_pattern not in out:
            out[file_pattern] = REMOVE               # the scenario's file is not written, or under another name
    return out


# ---------------------------------------------------------------------------------------------------------------
# one run

def dump(value, indent=0, width=118):
    """JSON with one line for every value that fits, so a scenario file reads like a script"""
    flat = json.dumps(value, ensure_ascii=False)
    if len(flat) + indent <= width or not isinstance(value, (dict, list)) or not value:
        return flat
    pad = " " * (indent + 2)
    if isinstance(value, list):
        return "[\n" + ",\n".join(pad + dump(v, indent + 2, width) for v in value) + "\n" + " " * indent + "]"
    return "{\n" + ",\n".join(f"{pad}{json.dumps(k, ensure_ascii=False)}: {dump(v, indent + 2, width)}"
                               for k, v in value.items()) + "\n" + " " * indent + "}"


def free_port():
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


def read_settings(path):
    if not path.exists():
        return None
    parser = configparser.RawConfigParser(strict=False)
    parser.optionxform = str
    parser.read(path)
    out = {}
    for section in parser.sections():
        for key, value in parser.items(section):
            out[key if section == "General" else f"{section}/{key}"] = value
    return out


def write_settings(path, values):
    lines = ["[General]"] + [f"{k}={'true' if v is True else 'false' if v is False else v}" for k, v in values.items()]
    path.write_text("\n".join(lines) + "\n")


def collect_files(roots):
    files = {}
    for root in roots:
        if not root.exists():
            continue
        for p in sorted(root.rglob("*")):
            if p.is_file():
                text = p.read_text(errors="replace")
                if p.suffix == ".json":
                    try:
                        files[str(p)] = {"json": json.loads(text)}
                        continue
                    except ValueError:
                        pass
                if p.name.endswith(".log"):   # output of the stand-in scicat-cli: its first and last line
                    lines = [l for l in text.splitlines() if l.strip()]
                    files[str(p)] = {"contains": lines[:1] + lines[-1:]}
                else:
                    files[str(p)] = {"lines": text.splitlines()}
    return files


def check_file(file_pattern, spec, work, variables, problems, known):
    rx = pattern(file_pattern, variables)
    hits = [p for p in (work.rglob("*")) if p.is_file() and rx.fullmatch(str(p))]
    if spec == {"absent": True}:
        if hits:
            problems.append(f"files {file_pattern}: exists, expected absent")
        return
    if len(hits) != 1:
        problems.append(f"files {file_pattern}: {len(hits)} files match, expected 1")
        return
    text = hits[0].read_text(errors="replace")
    where = f"files {file_pattern}"
    if "json" in spec:
        try:
            match(spec["json"], json.loads(text), variables, where, problems, known)
        except ValueError:
            problems.append(f"{where}: not JSON")
    if "lines" in spec:
        match(spec["lines"], text.splitlines(), variables, where, problems, known)
    for needle in spec.get("contains", []):
        if not pattern("${ANY}" + needle + "${ANY}", variables).fullmatch(text):
            problems.append(f"{where}: does not contain {short(needle)}")


def run_scenario(path, args, work_root):
    scenario = json.loads(path.read_text())
    name = path.stem
    work = work_root / name
    shutil.rmtree(work, ignore_errors=True)
    for d in ("home", "data", "out"):
        (work / d).mkdir(parents=True)
    variables = {"HOME": str(work / "home"), "LOG": str(work / "log"), "DATA": str(work / "data"),
                 "OUT": str(work / "out"), "BIN": str(HERE / "bin"), "CLI": str(HERE / "bin" / "scicat-cli")}
    for f in scenario.get("data", DEFAULT_DATA):
        p = work / "data" / f
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text("demo\n")
    settings = scenario.get("settings", DEFAULT_SETTINGS)
    if settings is not None:
        write_settings(work / "settings.ini", substitute(settings, variables))
    for f, text in scenario.get("files", {}).items():   # files that exist before the run
        target = pathlib.Path(substitute(f, variables))
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text)
    resolved = dict(scenario, steps=substitute(scenario["steps"], variables))
    (work / "scenario.json").write_text(json.dumps(resolved, indent=1))

    env = dict(os.environ)
    env.update({"HOME": variables["HOME"], "SCICAT_TEST_SETTINGS": str(work / "settings.ini"),
                "SCICAT_TEST_SCENARIO": str(work / "scenario.json"), "SCICAT_TEST_OUT": str(work / "observations.json"),
                "FAKE_CLI_CALLS": str(work / "cli-calls.jsonl"), "FAKE_CLI_PAUSE_SCALE": "0", "FAKE_CLI_STEP": "0.01",
                "QT_QPA_PLATFORM": "offscreen", "PATH": variables["BIN"] + os.pathsep + env.get("PATH", "")})
    if not scenario.get("cliOnPath", True):   # only klist: no scicat-cli next to the GUI or on PATH
        (work / "bin").mkdir()
        (work / "bin" / "klist").symlink_to(HERE / "bin" / "klist")
        env["PATH"] = str(work / "bin") + os.pathsep + "/usr/bin:/bin"
    env.update(substitute(scenario.get("env", {}), variables))
    backend = None
    if scenario.get("backend", "up") == "up":
        backend = subprocess.Popen([sys.executable, "-I", str(HERE / "fake_scicat.py"), "0"], stdout=subprocess.PIPE,
                                   env=dict(env, FAKE_SCICAT_LOG=str(work / "requests.jsonl")), text=True)
        env["SCICAT_TEST_API"] = f"http://127.0.0.1:{backend.stdout.readline().strip()}"
    else:
        env["SCICAT_TEST_API"] = f"http://127.0.0.1:{free_port()}"   # nothing listens there
    variables["SCICAT"] = env["SCICAT_TEST_API"][len("http://"):]
    started = time.monotonic()
    try:
        with open(work / "driver.log", "w") as log:
            driver = subprocess.run(args.driver, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=args.timeout,
                                    cwd=work)
        exit_code = driver.returncode
    except subprocess.TimeoutExpired:
        exit_code = "timeout"
    finally:
        if backend:
            backend.send_signal(signal.SIGTERM)
            backend.wait()
    seconds = time.monotonic() - started

    observations = json.loads((work / "observations.json").read_text()) if (work / "observations.json").exists() else {}
    def jsonl(p):
        return [json.loads(l) for l in p.read_text().splitlines()] if p.exists() else []
    actual = {"requests": jsonl(work / "requests.jsonl"), "cli": jsonl(work / "cli-calls.jsonl"),
              "settings": read_settings(work / "settings.ini"),
              "steps": [s.get("result") for s in observations.get("steps", [])],
              "messages": observations.get("messages", {}), "snapshots": observations.get("snapshots", {})}
    expect = scenario.get("expect", {})
    if args.unordered_reads:
        actual["requests"] = by_path(actual["requests"])
        if "requests" in expect:
            expect = dict(expect, requests=by_path(expect["requests"]))
    why = None
    if args.differences:
        every, own, why = load_differences(args.differences, name)
        expect = overlay(expect, every)

    problems, known = [], []
    if exit_code != 0:
        problems.append(f"driver exited with {exit_code}, see {work / 'driver.log'}")
    sections = OUTPUT_SECTIONS + (() if args.outputs_only else GUI_SECTIONS)

    if args.record and args.differences:
        # what this GUI does differently from the scenario (with what differs in every scenario already laid over it)
        record = lambda value: condense(generalise(value, variables), None)
        found = {}
        for section in OUTPUT_SECTIONS + GUI_SECTIONS:
            if section not in expect or (isinstance(expect[section], dict) and "$skip" in expect[section]):
                continue
            if section == "files":
                written = collect_files([work / "log", work / "out", work / "home" / ".scicat"])
                part = file_differences(expect["files"], work, variables, written, own.get("files", {})) or SAME
            else:
                part = difference(expect[section], actual[section], variables, record)
            if part is not SAME:
                found[section] = part
        target = args.differences / f"{name}.json"
        if found:
            target.write_text(dump({"why": why or "TODO: say why", "expect": found}) + "\n")
        elif target.exists():
            target.unlink()
        return name, problems, known, seconds
    if args.differences:
        if own and not why or (isinstance(why, str) and why.startswith("TODO")):
            problems.append(f"{args.differences / (name + '.json')} does not say why it differs")
        expect = overlay(expect, own)

    if args.record:
        files = {generalise(p, variables): generalise(v, variables)
                 for p, v in collect_files([work / "log", work / "out", work / "home"]).items()}
        recorded = {"requests": generalise(actual["requests"], variables), "cli": generalise(actual["cli"], variables),
                    "files": files, "settings": generalise(actual["settings"], variables), "steps": actual["steps"],
                    "messages": generalise(actual["messages"], variables),
                    "snapshots": generalise(actual["snapshots"], variables)}
        for key in list(recorded):
            if key in expect and isinstance(expect[key], dict) and "$skip" in expect[key]:
                recorded[key] = expect[key]
        scenario["expect"] = rewrap(expect, condense(recorded, expect))
        path.write_text(dump(scenario) + "\n")
        return name, problems, known, seconds

    for section in sections:
        if section not in expect or (isinstance(expect[section], dict) and "$skip" in expect[section]):
            continue
        if section == "files":
            for file_pattern, spec in expect["files"].items():
                check_file(file_pattern, spec, work, variables, problems, known)
            continue
        match(expect[section], actual[section], variables, section, problems, known)
    if not args.tolerate_known:
        problems += [f"[{k}] {p}" for k, sub in known for p in sub]
        known = []
    return name, problems, known, seconds


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--driver", required=True, nargs="+", help="driver executable (and its arguments)")
    parser.add_argument("--scenarios", type=pathlib.Path, default=HERE / "scenarios")
    parser.add_argument("-k", dest="select", default="", help="only scenarios whose file name contains this")
    parser.add_argument("--name", help="only the scenario of this name (file name without .json)")
    parser.add_argument("--work", type=pathlib.Path, help="folder for the runs (default: a new temporary folder)")
    parser.add_argument("--record", action="store_true", help="write the observed results into the scenarios")
    parser.add_argument("--outputs-only", action="store_true", help="compare only requests, cli, files and settings")
    parser.add_argument("--tolerate-known", action="store_true", help="report mismatches marked $known, do not fail")
    parser.add_argument("--unordered-reads", action="store_true",
                        help="compare GET requests that follow each other directly in the order of their paths")
    parser.add_argument("--differences", type=pathlib.Path,
                        help="folder with what the GUI under test does differently on purpose (see above)")
    parser.add_argument("--timeout", type=int, default=120)
    args = parser.parse_args()
    if pathlib.Path(args.driver[0]).exists():   # the driver runs in the scenario's folder
        args.driver[0] = str(pathlib.Path(args.driver[0]).resolve())

    if args.differences:
        args.differences = args.differences.resolve()
        args.differences.mkdir(parents=True, exist_ok=True)
    work_root = (args.work or pathlib.Path(tempfile.mkdtemp(prefix="scicat-scenarios-"))).resolve()
    work_root.mkdir(parents=True, exist_ok=True)
    store = pathlib.Path.home() / "Library/Preferences/com.psi.SciCat.plist"
    before = store.read_bytes() if store.exists() else None

    paths = sorted(p for p in args.scenarios.glob("*.json") if args.select in p.stem and args.name in (None, p.stem))
    if not paths:
        sys.exit("no scenario selected")
    failed = 0
    for path in paths:
        name, problems, known, seconds = run_scenario(path, args, work_root)
        status = "RECORDED" if args.record else "FAIL" if problems else "ok"
        print(f"{status:8} {name}  ({seconds:.1f} s)")
        for p in problems:
            print(f"         {p}")
        for k, sub in known:
            print(f"         known difference {k}:")
            for p in sub:
                print(f"           {p}")
        failed += bool(problems)

    if store.exists() and store.read_bytes() != before:
        print(f"FAIL     the real settings store {store} changed during the run")
        failed += 1
    print(f"{len(paths) - failed} of {len(paths)} scenarios passed; runs in {work_root}")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
