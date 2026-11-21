"""Fake SciCat backend for the scicat-qt scenario tests. Stdlib only, listens on 127.0.0.1, contacts nothing.

Just enough of backend v5.3.3 for the GUI to sign in, load a pgroup, list datasets with files and metadata,
request a retrieve and list retrieve jobs.

  python3 -I fake_scicat.py <port|0> [latency_ms]

Port 0 picks a free port; the port is printed as the first line on stdout. Latency defaults to 0.

Each SciCat server of the GUI gets its own prefix, so the request log shows which server was used:
  /production/api/...   /qa/api/...   /development/api/...   (and plain /api/... for a single test URL)

Login token: good (user jdoe). Any other token is rejected with 401.
pgroups:  p12345  proposal, 250 datasets, 120 of them in a finished retrieve job
          p22222  no proposal in SciCat, no datasets (manual PI entry)
          p33333  proposal, 1200 datasets (three pages), retrieve requests are refused with 403
          p44444  no proposal, and listing its datasets fails with 500
          p55555  proposal without the email of its principal investigator
Lists honour limits.limit/skip/sort/order and default to 100 items, like the real backend.
A retrieve job posted here is jobSubmitted for 6 s, inProgress until 15 s, then finishedSuccessful.
Every request is appended as one JSON object per line to the file named by FAKE_SCICAT_LOG:
  {"method", "server", "path", "query": {name: parsed JSON or text}, "auth", "body": parsed JSON or text}
"""
import json, os, sys, time, pathlib, urllib.parse, uuid, datetime
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

LOG = pathlib.Path(os.environ["FAKE_SCICAT_LOG"]) if os.environ.get("FAKE_SCICAT_LOG") else None
SERVERS = ("production", "qa", "development")
USER, MAIL = "jdoe", "jdoe@example.org"
LATENCY = (int(sys.argv[2]) if len(sys.argv) > 2 else 0) / 1000.0
NS = uuid.UUID("6ba7b810-9dad-11d1-80b4-00c04fd430c8")

SERIES = [("tomo_fuelcell", "/PSI/SLS/TOMCAT", "raw"), ("ptycho_catalyst", "/PSI/SLS/CSAXS", "raw"),
          ("saxs_polymer", "/PSI/SLS/CSAXS", "raw"), ("recon_fuelcell", "", "derived"), ("xrd_battery", "/PSI/SLS/MS-POWDER", "raw")]

def make_datasets(group, count):
    out = []
    for i in range(count):
        name, loc, kind = SERIES[i % len(SERIES)]
        archived = i % 4 != 3
        created = datetime.datetime(2026, 1, 5, 8, 0, 0) + datetime.timedelta(hours=7 * i)
        size = (3 + (i * 37) % 900) * 1024 ** 2 * (40 if i % 7 == 0 else 1)   # some well above 2 GiB
        d = {"pid": f"20.500.11935/{uuid.uuid5(NS, f'{group}-{i}')}",
             "datasetName": f"{name}_{i:04d}", "ownerGroup": group, "owner": "Jane Doe", "ownerEmail": "jane.doe@example.org",
             "contactEmail": "jane.doe@example.org", "type": kind, "creationTime": created.isoformat() + ".000Z",
             "sourceFolder": f"/das/work/{group[:3]}/{group}/{name}/{i:04d}", "size": size, "numberOfFiles": 3 + (i * 11) % 240,
             "description": f"Demo dataset {i} of the fake backend",
             "datasetlifecycle": {"archivable": not archived, "retrievable": archived,
                                  "archiveStatusMessage": "datasetOnArchiveDisk" if archived else "scheduledForArchiving",
                                  "retrieveStatusMessage": ""},
             "scientificMetadata": {"sample": {"name": f"S{i:04d}", "description": "demo sample", "principalInvestigator": "jane.doe@example.org"},
                                    "beam": {"energy": {"value": round(12.4 + i / 100, 2), "unit": "keV"}, "ring_current": {"value": 400, "unit": "mA"}},
                                    "detector": "eiger", "exposure_time": {"value": 0.2, "unit": "s"}}}
        if i == 1:   # three files of 4 GiB each: larger than a 32-bit int can hold
            d["size"], d["numberOfFiles"] = 12 * 1024 ** 3, 3
        if kind == "raw":
            d["creationLocation"] = loc; d["principalInvestigator"] = "jane.doe@example.org"
        else:
            d["investigator"] = MAIL; d["usedSoftware"] = ["https://gitlab.psi.ch/demo/recon@60629a1"]; d["inputDatasets"] = []
        out.append(d)
    return out

DATASETS = make_datasets("p12345", 250) + make_datasets("p33333", 1200)
BY_PID = {d["pid"]: d for d in DATASETS}
P1 = [d for d in DATASETS if d["ownerGroup"] == "p12345"]
RETRIEVABLE = [d for d in P1 if d["datasetlifecycle"]["retrievable"]]
# finished retrieve jobs for the first 120 retrievable datasets (the first one is in several jobs), two failed ones on top
JOBS = [{"id": f"job{i}", "type": "retrieve", "ownerUser": USER, "ownerGroup": "p12345", "statusCode": "finishedSuccessful",
         "statusMessage": "Job completed successfully", "contactEmail": MAIL, "createdAt": f"2026-10-01T00:{i // 60:02d}:{i % 60:02d}.000Z",
         "jobParams": {"datasetList": [{"pid": RETRIEVABLE[i % 120]["pid"], "files": []}], "username": USER, "option": "PSI"}}
        for i in range(130)]
JOBS += [{"id": f"jobfail{i}", "type": "retrieve", "ownerUser": USER, "ownerGroup": "p12345", "statusCode": "finishedUnsuccessful",
          "statusMessage": "Tape library not reachable", "contactEmail": MAIL, "createdAt": f"2026-10-02T00:00:{i:02d}.000Z",
          "jobParams": {"datasetList": [{"pid": RETRIEVABLE[125 + i]["pid"], "files": []}], "username": USER, "option": "PSI"}}
         for i in range(2)]
POSTED = []   # (monotonic time, job)
PROPOSALS = {g: {"proposalId": f"20.500.11935/{uuid.uuid5(NS, 'prop-' + g)}"[:44], "ownerGroup": g, "pi_firstname": "Jane", "pi_lastname": "Doe",
                 "pi_email": "jane.doe@example.org", "title": t, "abstract": a, "MeasurementPeriodList": [{"instrument": loc}]}
             for g, t, a, loc in [("p12345", "Operando tomography of fuel cell membranes (fake backend)",
                                   "Demo proposal served by the local fake SciCat. No real data.", "/PSI/SLS/TOMCAT"),
                                  ("p33333", "Large demo proposal with 1200 datasets (fake backend)", "Used to try long lists and a refused retrieve request.", "/PSI/SLS/CSAXS"),
                                  ("p55555", "Proposal without the email of its principal investigator (fake backend)", "", "/PSI/SLS/TOMCAT")]}
PROPOSALS["p55555"]["pi_email"] = ""

def blocks(pid):
    d = BY_PID.get(pid)
    n = min(d["numberOfFiles"], 400) if d else 0
    per = 40
    return [{"id": f"{pid}-odb{b}", "datasetId": pid, "size": 0,
             "dataFileList": [{"path": f"raw/scan_{f:05d}.h5", "size": (d["size"] // max(n, 1)), "time": d["creationTime"]}
                              for f in range(b * per, min(n, (b + 1) * per))]}
            for b in range((n + per - 1) // per)]

def live(job, t0):
    age = time.monotonic() - t0
    code = "jobSubmitted" if age < 6 else "inProgress" if age < 15 else "finishedSuccessful"
    return dict(job, statusCode=code, statusMessage={"jobSubmitted": "Job submitted", "inProgress": "Retrieving from tape",
                                                      "finishedSuccessful": "Job completed successfully"}[code])

def all_jobs():
    return JOBS + [live(j, t0) for t0, j in POSTED]

def page(items, limits):
    limits = limits or {}
    if limits.get("order"):
        key, direction = limits["order"].split(":")
        items = sorted(items, key=lambda x: x.get(key, ""), reverse=direction == "desc")
    for key, direction in (limits.get("sort") or {}).items():
        items = sorted(items, key=lambda x: x.get(key, ""), reverse=direction == "desc")
    skip, limit = int(limits.get("skip", 0)), int(limits.get("limit", 100))
    return items[skip:skip + limit]

def project(item, fields):
    return {k: v for k, v in item.items() if not fields or k in fields}

class H(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"
    def log_message(self, *a): pass
    def send(self, code, body):
        time.sleep(LATENCY)
        data = json.dumps(body).encode()
        self.send_response(code); self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data))); self.end_headers(); self.wfile.write(data)
    def record(self, body=None):
        u = urllib.parse.urlparse(self.path)
        path, server = urllib.parse.unquote(u.path), None
        first = path.split("/")[1] if path.count("/") > 1 else ""
        if first in SERVERS:
            server, path = first, path[len(first) + 1:]
        query = {}
        for k, v in urllib.parse.parse_qs(u.query, keep_blank_values=True).items():
            try:
                query[k] = json.loads(v[0])
            except ValueError:
                query[k] = v[0]
        if LOG:
            try:
                parsed_body = json.loads(body) if body else None
            except ValueError:
                parsed_body = body
            entry = {"method": self.command, "server": server, "path": path, "query": query,
                     "auth": self.headers.get("Authorization"), "body": parsed_body}
            with LOG.open("a") as f:
                f.write(json.dumps(entry, sort_keys=True) + "\n")
        flt = query.get("filter", query.get("filters", {}))
        return path, flt if isinstance(flt, dict) else {}
    def authed(self):
        return self.headers.get("Authorization", "") == "Bearer good"
    def do_GET(self):
        path, flt = self.record()
        if path.endswith("/auth/whoami"):
            if self.authed():
                return self.send(200, {"username": USER, "email": MAIL, "currentGroups": ["p12345", "p22222", "p33333", "p44444", "p55555"]})
            return self.send(401, {"message": "Unauthorized", "statusCode": 401})
        if not self.authed():
            return self.send(401, {"message": "Unauthorized", "statusCode": 401})
        where = flt.get("where", {})
        if path == "/api/v3/proposals":
            p = PROPOSALS.get(where.get("ownerGroup"))
            return self.send(200, [p] if p else [])
        if path == "/api/v4/jobs":
            return self.send(200, page([j for j in all_jobs() if all(j.get(k) == v for k, v in where.items())], flt.get("limits")))
        if path == "/api/v3/jobs":
            if where:
                return self.send(400, {"message": "where filter not supported by the v3 jobs endpoint", "statusCode": 400})
            return self.send(200, page(all_jobs(), None))
        if path == "/api/v3/datasets":
            if where.get("ownerGroup") == "p44444":
                return self.send(500, {"message": "Internal server error (simulated by the fake backend)", "statusCode": 500})
            ds = [d for d in DATASETS if all(d.get(k) == v for k, v in where.items())]
            ds = page(ds, flt.get("limits"))
            if "origdatablocks" in flt.get("include", []):
                return self.send(200, [dict(project(d, flt.get("fields")), origdatablocks=blocks(d["pid"])) for d in ds])
            return self.send(200, [project(d, flt.get("fields")) for d in ds])
        if path.startswith("/api/v3/datasets/") and path.endswith("/origdatablocks"):
            return self.send(200, blocks(path[len("/api/v3/datasets/"):-len("/origdatablocks")]))
        if path.startswith("/api/v3/datasets/"):
            d = BY_PID.get(path[len("/api/v3/datasets/"):])
            return self.send(200, project(d, flt.get("fields"))) if d else self.send(404, {"message": "not found", "statusCode": 404})
        self.send(404, {"message": f"Cannot GET {path}", "statusCode": 404})
    def do_POST(self):
        body = self.rfile.read(int(self.headers.get("Content-Length", 0))).decode()
        path, _ = self.record(body)
        if not self.authed():
            return self.send(401, {"message": "Unauthorized", "statusCode": 401})
        job = json.loads(body or "{}")
        if path == "/api/v4/jobs":
            if job.get("ownerGroup") != "p12345":
                return self.send(403, {"message": "Invalid new job. User needs to belong to job owner group.", "error": "Forbidden", "statusCode": 403})
            job = dict(job, id=f"posted{len(POSTED)}", createdAt=datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%S.000Z"))
            POSTED.append((time.monotonic(), job))
            return self.send(201, live(job, time.monotonic()))
        self.send(404, {"message": f"Cannot POST {path}", "statusCode": 404})

class Server(ThreadingHTTPServer):
    daemon_threads = True
    def server_bind(self):
        # HTTPServer.server_bind() calls socket.getfqdn(), a reverse DNS lookup that can block for half a minute offline
        import socketserver
        socketserver.TCPServer.server_bind(self)
        self.server_name, self.server_port = self.server_address[:2]

if __name__ == "__main__":
    server = Server(("127.0.0.1", int(sys.argv[1])), H)
    print(server.server_address[1], flush=True)
    server.serve_forever()
