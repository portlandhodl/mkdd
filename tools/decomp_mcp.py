#!/usr/bin/env python3
"""Minimal MCP (Model Context Protocol) stdio server for this decomp project.

Wraps tools/decomp.py as MCP tools so an agent can drive the match loop with
structured calls instead of ad-hoc shell commands. No third-party packages.

Register it in opencode.json:

    {
      "mcp": {
        "mkdd-decomp": {
          "type": "local",
          "command": ["python3", "tools/decomp_mcp.py"],
          "enabled": true
        }
      }
    }

The server speaks newline-delimited JSON-RPC 2.0 on stdin/stdout. Diagnostics
go to stderr so they never corrupt the protocol stream.
"""

import json
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DECOMP = os.path.join(ROOT, "tools", "decomp.py")
PROTOCOL = "2024-11-05"
MAX_OUTPUT = 150_000

TOOLS = [
    {
        "name": "decomp_progress",
        "description": "Overall code/function/data progress for the project.",
        "inputSchema": {"type": "object", "properties": {}},
    },
    {
        "name": "decomp_units",
        "description": "List decompilation units with match statistics. Use to pick a unit.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "unmatched": {"type": "boolean", "description": "Only units with unmatched code"},
                "category": {"type": "string", "enum": ["game", "jsys", "sdk", "msl"]},
                "sort": {"type": "string", "enum": ["bytes", "pct", "funcs"]},
                "ascending": {"type": "boolean"},
                "limit": {"type": "integer", "minimum": 1, "maximum": 300},
            },
        },
    },
    {
        "name": "decomp_functions",
        "description": "List functions in a unit with per-function match score and size.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "unit": {"type": "string"},
                "unmatched": {"type": "boolean"},
                "limit": {"type": "integer", "minimum": 1, "maximum": 1000},
            },
            "required": ["unit"],
        },
    },
    {
        "name": "decomp_next",
        "description": "Suggest the highest-value unmatched functions to work on next.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "n": {"type": "integer", "minimum": 1, "maximum": 100},
                "mode": {"type": "string", "enum": ["mixed", "finish", "sweep", "portable"]},
            },
        },
    },
    {
        "name": "decomp_unit_info",
        "description": "Unit metadata: source path, target object, sections, match stats.",
        "inputSchema": {
            "type": "object",
            "properties": {"unit": {"type": "string"}},
            "required": ["unit"],
        },
    },
    {
        "name": "decomp_asm",
        "description": "Disassemble one symbol from the original target object (or our build with ours=true). Includes relocations.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "unit": {"type": "string"},
                "symbol": {"type": "string"},
                "ours": {"type": "boolean"},
            },
            "required": ["unit", "symbol"],
        },
    },
    {
        "name": "decomp_diff",
        "description": "Compile one translation unit in isolation and diff the named symbols against the target. Returns MATCH or a score plus hints. Strict by default: branch targets, immediates, call/relocation targets, literal values and jump tables must match too (loose_match/strict_reason explain strict-only failures); set loose=true for the old text-only check.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "unit": {"type": "string", "description": "Unit name or src path"},
                "symbols": {"type": "array", "items": {"type": "string"}},
                "diff_text": {"type": "boolean"},
                "loose": {"type": "boolean", "description": "Skip strict checks"},
            },
            "required": ["unit", "symbols"],
        },
    },
    {
        "name": "decomp_diff_unit",
        "description": "Compile a translation unit and summarize all function diffs (worst first). Strict by default (see decomp_diff); strict_only_failures lists functions that only match loosely.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "unit": {"type": "string"},
                "worst": {"type": "integer", "minimum": 1, "maximum": 100},
                "loose": {"type": "boolean", "description": "Skip strict checks"},
            },
            "required": ["unit"],
        },
    },
    {
        "name": "decomp_check",
        "description": "Compile one translation unit to verify it builds.",
        "inputSchema": {
            "type": "object",
            "properties": {"unit": {"type": "string"}},
            "required": ["unit"],
        },
    },
    {
        "name": "decomp_symbols",
        "description": "Search symbols and function names (mangled or demangled).",
        "inputSchema": {
            "type": "object",
            "properties": {
                "query": {"type": "string"},
                "limit": {"type": "integer", "minimum": 1, "maximum": 500},
            },
            "required": ["query"],
        },
    },
    {
        "name": "decomp_callers",
        "description": "Find callers of a symbol using the original linker map.",
        "inputSchema": {
            "type": "object",
            "properties": {"symbol": {"type": "string"}},
            "required": ["symbol"],
        },
    },
    {
        "name": "decomp_callees",
        "description": "Find callees of a symbol using the original linker map.",
        "inputSchema": {
            "type": "object",
            "properties": {"symbol": {"type": "string"}},
            "required": ["symbol"],
        },
    },
    {
        "name": "decomp_context",
        "description": "Generate a decomp.me-style context file for a unit.",
        "inputSchema": {
            "type": "object",
            "properties": {"unit": {"type": "string"}},
            "required": ["unit"],
        },
    },
    {
        "name": "decomp_port",
        "description": "Search sibling decomp projects under references/ for an existing implementation of a symbol.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "query": {"type": "string"},
                "limit": {"type": "integer", "minimum": 1, "maximum": 200},
            },
            "required": ["query"],
        },
    },
    {
        "name": "decomp_regress",
        "description": "Check for match regressions against the saved baseline.",
        "inputSchema": {"type": "object", "properties": {}},
    },
]

DISPATCH = {
    "decomp_progress": ["progress"],
    "decomp_units": ["units"],
    "decomp_functions": ["funcs"],
    "decomp_next": ["next"],
    "decomp_unit_info": ["info"],
    "decomp_asm": ["asm"],
    "decomp_diff": ["diff"],
    "decomp_diff_unit": ["diff-unit"],
    "decomp_check": ["check"],
    "decomp_symbols": ["symbols"],
    "decomp_callers": ["callers"],
    "decomp_callees": ["callees"],
    "decomp_context": ["context"],
    "decomp_port": ["port"],
    "decomp_regress": ["regress"],
}

FLAG_ARGS = {
    "unmatched": "--unmatched",
    "ours": "--ours",
    "diff_text": "--diff-text",
    "ascending": "--ascending",
    "loose": "--loose",
}


def log(*a):
    print(*a, file=sys.stderr, flush=True)


def send(obj):
    sys.stdout.write(json.dumps(obj, separators=(",", ":")) + "\n")
    sys.stdout.flush()


def result(msg_id, content, is_error=False):
    send({
        "jsonrpc": "2.0",
        "id": msg_id,
        "result": {"content": [{"type": "text", "text": content}],
                   "isError": is_error},
    })


def error(msg_id, code, message):
    send({"jsonrpc": "2.0", "id": msg_id,
          "error": {"code": code, "message": message}})


def run_tool(name, arguments):
    base = DISPATCH.get(name)
    if base is None:
        raise KeyError(f"unknown tool: {name}")
    cmd = [sys.executable, DECOMP, "--json"] + base
    for key, flag in FLAG_ARGS.items():
        if arguments.get(key):
            cmd.append(flag)
    for key in ("sort", "category", "mode"):
        if arguments.get(key):
            cmd += [f"--{key}", str(arguments[key])]
    for key in ("unit", "query", "symbol"):
        if arguments.get(key) is not None:
            cmd.append(str(arguments[key]))
    if arguments.get("limit") is not None:
        cmd += ["--limit", str(arguments["limit"])]
    if arguments.get("n") is not None:
        cmd += ["--n", str(arguments["n"])]
    if arguments.get("worst") is not None:
        cmd += ["--worst", str(arguments["worst"])]
    if name == "decomp_diff":
        symbols = arguments.get("symbols") or []
        if not symbols:
            raise ValueError("symbols is required")
        cmd += [str(s) for s in symbols]

    r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True,
                       timeout=300)
    text = r.stdout
    if r.returncode != 0 and r.stderr:
        text += ("\n" if text else "") + r.stderr
    if len(text) > MAX_OUTPUT:
        text = text[:MAX_OUTPUT] + f"\n... truncated ({len(text)} bytes)"
    # diff/regress intentionally exit 1 on mismatch; stderr-less output is OK.
    is_error = not text.strip()
    return text or "(no output)", is_error


def handle(msg):
    method = msg.get("method")
    msg_id = msg.get("id")
    params = msg.get("params") or {}

    if method == "initialize":
        send({
            "jsonrpc": "2.0", "id": msg_id,
            "result": {
                "protocolVersion": params.get("protocolVersion", PROTOCOL),
                "capabilities": {"tools": {"listChanged": False}},
                "serverInfo": {"name": "mkdd-decomp", "version": "1.0.0"},
            },
        })
    elif method == "notifications/initialized":
        pass
    elif method == "ping":
        send({"jsonrpc": "2.0", "id": msg_id, "result": {}})
    elif method == "tools/list":
        send({"jsonrpc": "2.0", "id": msg_id, "result": {"tools": TOOLS}})
    elif method == "tools/call":
        name = params.get("name")
        arguments = params.get("arguments") or {}
        try:
            text, is_error = run_tool(name, arguments)
            result(msg_id, text, is_error)
        except Exception as e:  # noqa: BLE001 - report to client
            result(msg_id, f"{type(e).__name__}: {e}", True)
    elif msg_id is not None:
        error(msg_id, -32601, f"method not found: {method}")


def main():
    log(f"mkdd-decomp MCP ready ({len(TOOLS)} tools)")
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        try:
            msg = json.loads(line)
        except json.JSONDecodeError as e:
            log(f"bad json: {e}")
            continue
        try:
            handle(msg)
        except Exception as e:  # noqa: BLE001
            log(f"handler error: {e}")
            if msg.get("id") is not None:
                error(msg["id"], -32603, str(e))


if __name__ == "__main__":
    main()
