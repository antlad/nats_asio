"""Payload validation against an independent NATS client.

test_pubsub.py drives nats_tool on both ends, so a symmetric protocol defect
could pass unnoticed. Here a native `nats-py` client sits on the other side and
inspects the actual bytes on the wire, in both directions.
"""

import asyncio

import nats

GENERATOR_PAYLOAD = b'{"value": 123}'
PUBLISH_INTERVAL_MS = "100"
CLIENT_TIMEOUT = 30.0


def _payload_lines(tool) -> list[str]:
    """Everything the tool wrote that is not an spdlog record, i.e. `--print` output."""
    return [line for line in tool.output if not line.startswith("[")]


def _run(coro):
    return asyncio.run(asyncio.wait_for(coro, CLIENT_TIMEOUT + 5))


def test_generator_publishes_expected_bytes_on_expected_subject(nats_url, run_tool):
    """nats_tool gen must put exactly {"value": 123} on the subject it was given."""
    subject = "it.payload.out"

    async def scenario():
        client = await nats.connect(nats_url)

        try:
            # Wildcard, so a message published to the wrong subject is still
            # captured and fails the subject assertion rather than timing out.
            subscription = await client.subscribe("it.payload.>")
            await client.flush()

            run_tool("gen", subject, "--publish_interval", PUBLISH_INTERVAL_MS)

            return [await subscription.next_msg(timeout=CLIENT_TIMEOUT) for _ in range(3)]
        finally:
            await client.close()

    messages = _run(scenario())

    assert [m.subject for m in messages] == [subject] * 3
    assert [m.data for m in messages] == [GENERATOR_PAYLOAD] * 3
    assert [m.reply for m in messages] == ["", "", ""]


def test_grubber_receives_exact_bytes_published_by_client(nats_url, run_tool):
    """nats_tool grub must deliver a payload byte-for-byte to its message callback."""
    subject = "it.payload.in"
    payload = b'{"probe":"nats_asio","n":42,"blob":"aGVsbG8="}'

    subscriber = run_tool("grub", subject, "--print")
    subscriber.wait_for_line("on connected")

    async def scenario():
        client = await nats.connect(nats_url)

        try:
            for _ in range(3):
                await client.publish(subject, payload)

            await client.flush()
        finally:
            await client.close()

    _run(scenario())

    subscriber.wait_for_lines(payload.decode(), count=3, timeout=CLIENT_TIMEOUT)

    # Exact equality over *all* non-log output: catches both a truncated payload
    # and trailing read-buffer bytes leaking past the end of the message.
    assert _payload_lines(subscriber) == [payload.decode()] * 3


def test_grubber_handles_payload_with_embedded_crlf(nats_url, run_tool):
    """\\r\\n inside a payload is length-delimited data, not a frame boundary."""
    subject = "it.payload.crlf"
    payload = b"line-one\r\nline-two\r\nEND"

    subscriber = run_tool("grub", subject, "--print")
    subscriber.wait_for_line("on connected")

    async def scenario():
        client = await nats.connect(nats_url)

        try:
            await client.publish(subject, payload)
            await client.flush()
            # Round-trip the same payload through a native subscriber too, so a
            # failure here is unambiguously attributable to nats_tool.
            echo = await client.subscribe(subject)
            await client.flush()
            await client.publish(subject, payload)
            message = await echo.next_msg(timeout=CLIENT_TIMEOUT)
            return message.data
        finally:
            await client.close()

    assert _run(scenario()) == payload

    # The payload spans three output lines because it contains CRLFs itself;
    # nats_tool must emit all of them, not stop at the first delimiter.
    subscriber.wait_for_lines("END", count=2, timeout=CLIENT_TIMEOUT)
    assert _payload_lines(subscriber) == ["line-one", "line-two", "END"] * 2
