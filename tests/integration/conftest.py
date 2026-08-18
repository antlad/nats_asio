"""Fixtures for nats_tool integration tests.

A real nats-server runs in a container (testcontainers); the tool under test is
the compiled `nats_tool` binary, driven as a subprocess exactly as a user would.
"""

import os
import signal
import subprocess
import threading
import time
from pathlib import Path

import pytest
from testcontainers.core.container import DockerContainer
from testcontainers.core.wait_strategies import LogMessageWaitStrategy

REPO_ROOT = Path(__file__).resolve().parents[2]
NATS_IMAGE = os.environ.get("NATS_IMAGE", "nats:2.10-alpine")
NATS_PORT = 4222


def _discover_tool() -> Path | None:
    for candidate in ("build/Release/nats_tool", "build/Debug/nats_tool"):
        path = REPO_ROOT / candidate
        if path.is_file():
            return path
    return None


@pytest.fixture(scope="session")
def nats_tool_bin() -> str:
    """Path to the nats_tool binary. Override with NATS_TOOL_BIN."""
    override = os.environ.get("NATS_TOOL_BIN")
    path = Path(override) if override else _discover_tool()

    if path is None or not path.is_file():
        pytest.fail(
            "nats_tool binary not found. Build it with "
            "`cmake --preset conan-release -DBUILD_NATS_TOOL=ON && cmake --build build/Release`, "
            "or point NATS_TOOL_BIN at it."
        )

    return str(path)


@pytest.fixture(scope="session")
def nats_server() -> tuple[str, int]:
    """A running nats-server; yields the (host, port) reachable from the test process."""
    container = (
        DockerContainer(NATS_IMAGE)
        .with_exposed_ports(NATS_PORT)
        .waiting_for(LogMessageWaitStrategy("Server is ready").with_startup_timeout(60))
    )

    with container:
        yield container.get_container_host_ip(), int(container.get_exposed_port(NATS_PORT))


@pytest.fixture(scope="session")
def nats_url(nats_server) -> str:
    """Connection URL for a native NATS client, independent of nats_tool."""
    host, port = nats_server
    return f"nats://{host}:{port}"


class ToolProcess:
    """A running nats_tool, with its merged stdout/stderr collected in the background."""

    def __init__(self, args: list[str]):
        self.args = args
        self._lines: list[str] = []
        self._lock = threading.Lock()
        self._proc = subprocess.Popen(
            args,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
        )
        self._pump = threading.Thread(target=self._collect, daemon=True)
        self._pump.start()

    def _collect(self) -> None:
        for line in self._proc.stdout:
            with self._lock:
                self._lines.append(line.rstrip("\r\n"))

    @property
    def output(self) -> list[str]:
        with self._lock:
            return list(self._lines)

    def wait_for_lines(self, needle: str, count: int = 1, timeout: float = 30.0) -> list[str]:
        """Block until `count` lines containing `needle` are emitted; return them."""
        deadline = time.monotonic() + timeout

        while time.monotonic() < deadline:
            matches = [line for line in self.output if needle in line]

            if len(matches) >= count:
                return matches[:count]

            if self._proc.poll() is not None:
                raise AssertionError(
                    f"{self.args[0]} exited with {self._proc.returncode} after {len(matches)}/{count} "
                    f"lines containing {needle!r}.\n--- output ---\n" + "\n".join(self.output)
                )

            time.sleep(0.1)

        raise AssertionError(
            f"timed out after {timeout}s waiting for {count} line(s) containing {needle!r}.\n"
            "--- output ---\n" + "\n".join(self.output)
        )

    def wait_for_line(self, needle: str, timeout: float = 30.0) -> str:
        """Block until a line containing `needle` is emitted; return it."""
        return self.wait_for_lines(needle, count=1, timeout=timeout)[0]

    def saw_line(self, needle: str) -> bool:
        return any(needle in line for line in self.output)

    def stop(self) -> None:
        if self._proc.poll() is None:
            self._proc.send_signal(signal.SIGINT)

            try:
                self._proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self._proc.kill()
                self._proc.wait(timeout=5)

        self._pump.join(timeout=5)


@pytest.fixture
def run_tool(nats_tool_bin, nats_server):
    """Factory starting nats_tool against the containerised server; all are stopped on teardown."""
    host, port = nats_server
    started: list[ToolProcess] = []

    def _start(mode: str, topic: str, *extra: str) -> ToolProcess:
        args = [
            nats_tool_bin,
            mode,
            "--address", host,
            "--port", str(port),
            "--topic", topic,
            *extra,
        ]
        tool = ToolProcess(args)
        started.append(tool)
        return tool

    yield _start

    for tool in started:
        tool.stop()
