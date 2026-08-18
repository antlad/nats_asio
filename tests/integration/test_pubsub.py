"""End-to-end pub/sub validation driving the nats_tool binary against a real nats-server."""

import re
import time

PAYLOAD = '{"value": 123}'
PUBLISH_INTERVAL_MS = "100"

STATS_RE = re.compile(r"stats: messages (\d+) during \d+ seconds")


def _message_counts(tool) -> list[int]:
    return [int(m.group(1)) for line in tool.output if (m := STATS_RE.search(line))]


def test_subscriber_receives_published_payload(run_tool):
    """A grubber subscribed to a topic prints what a generator publishes to it."""
    subscriber = run_tool("grub", "it.pubsub", "--print")
    subscriber.wait_for_line("on connected")

    run_tool("gen", "it.pubsub", "--publish_interval", PUBLISH_INTERVAL_MS)

    assert PAYLOAD in subscriber.wait_for_line(PAYLOAD, timeout=30)


def test_subscriber_counts_delivered_messages(run_tool):
    """The grubber's stats counter advances once messages start flowing."""
    subscriber = run_tool("grub", "it.stats")
    subscriber.wait_for_line("on connected")
    assert _message_counts(subscriber) == [] or max(_message_counts(subscriber)) == 0

    run_tool("gen", "it.stats", "--publish_interval", PUBLISH_INTERVAL_MS)

    deadline = time.monotonic() + 30

    while time.monotonic() < deadline:
        if any(count > 0 for count in _message_counts(subscriber)):
            return
        time.sleep(0.2)

    raise AssertionError(
        "subscriber never reported a non-zero message count.\n--- output ---\n"
        + "\n".join(subscriber.output)
    )


def test_subscriber_does_not_receive_other_topics(run_tool):
    """Subject filtering holds: a grubber on another topic sees nothing."""
    bystander = run_tool("grub", "it.other", "--print")
    subscriber = run_tool("grub", "it.filtered", "--print")
    bystander.wait_for_line("on connected")
    subscriber.wait_for_line("on connected")

    run_tool("gen", "it.filtered", "--publish_interval", PUBLISH_INTERVAL_MS)

    subscriber.wait_for_line(PAYLOAD, timeout=30)
    time.sleep(1)

    assert not bystander.saw_line(PAYLOAD), (
        "grubber on it.other received a message published to it.filtered:\n"
        + "\n".join(bystander.output)
    )
