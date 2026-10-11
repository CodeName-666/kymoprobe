"""Hardware integration check: real TCP and MQTT Kymo frames plus TCP reconnect.

Run with the KymoStudio venv and PYTHONPATH pointing to KymoStudio/python.
Stop Studio acquisition first: the ESP32 accepts one TCP client at a time.
"""
from __future__ import annotations

import argparse
import json
import socket
import threading
import time
import uuid

import paho.mqtt.client as mqtt
from Receiver.binary_protocol import ProtocolStreamDecoder, decode_data_frame


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("host", help="ESP32 IPv4 address from USB diagnostics")
    parser.add_argument("--seconds", type=float, default=15.0)
    parser.add_argument("--broker", default="broker.hivemq.com")
    parser.add_argument("--topic", default="kymotrace/demo/7c92e4a83f16/saw")
    args = parser.parse_args()
    subscribed = threading.Event()
    values = {"tcp": [], "mqtt": []}
    invalid = {"tcp": 0, "mqtt": 0}
    decoders = {key: ProtocolStreamDecoder() for key in values}
    collecting = threading.Event()

    def collect(transport, payload):
        if collecting.is_set():
            try:
                for frame in decoders[transport].feed(payload):
                    point = decode_data_frame(frame)
                    expected_id = 2 if transport == "tcp" else 3
                    assert point.id == expected_id and point.timestamp is not None
                    assert -1.01 <= point.value <= 1.01
                    values[transport].append(point)
            except (ValueError, AssertionError):
                invalid[transport] += 1

    def on_connect(client, userdata, flags, reason_code, properties):
        if reason_code == 0:
            client.subscribe(args.topic, qos=0)

    def on_subscribe(client, userdata, mid, reasons, properties):
        if reasons and all(not reason.is_failure for reason in reasons):
            subscribed.set()

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2,
                         client_id="kymo-check-" + uuid.uuid4().hex[:12])
    client.on_connect = on_connect
    client.on_subscribe = on_subscribe
    client.on_message = lambda client, userdata, msg: collect("mqtt", msg.payload)
    client.connect(args.broker, 1883, 30)
    client.loop_start()
    try:
        if not subscribed.wait(15):
            raise RuntimeError("MQTT subscription was not acknowledged")
        with socket.create_connection((args.host, 2323), timeout=5) as stream:
            stream.settimeout(0.5)
            collecting.set()
            deadline = time.monotonic() + args.seconds
            while time.monotonic() < deadline:
                try:
                    payload = stream.recv(4096)
                except socket.timeout:
                    continue
                if not payload:
                    raise RuntimeError("TCP stream closed during acquisition")
                collect("tcp", payload)
            collecting.clear()
        # A new stream must begin with a full frame and resume acquisition.
        reconnect_points = []
        decoder = ProtocolStreamDecoder()
        with socket.create_connection((args.host, 2323), timeout=5) as stream:
            stream.settimeout(3)
            deadline = time.monotonic() + 4
            while len(reconnect_points) < 10 and time.monotonic() < deadline:
                payload = stream.recv(4096)
                if not payload:
                    raise RuntimeError("TCP reconnect was closed")
                reconnect_points.extend(decode_data_frame(frame)
                                        for frame in decoder.feed(payload))
        report = {}
        for key, points in values.items():
            deltas = [b.timestamp - a.timestamp for a, b in zip(points, points[1:])]
            report[key] = {
                "samples": len(points), "hz": round(len(points) / args.seconds, 2),
                "invalid": invalid[key],
                "min": min((p.value for p in points), default=None),
                "max": max((p.value for p in points), default=None),
                "max_sample_gap_s": round(max(deltas, default=0), 3),
            }
            assert len(points) >= args.seconds * 7, report
            assert invalid[key] == 0 and all(delta > 0 for delta in deltas), report
            assert report[key]["min"] < -0.5 and report[key]["max"] > 0.5, report
        assert len(reconnect_points) >= 10 and all(p.id == 2 for p in reconnect_points)
        report["tcp_reconnect"] = "passed"
        print(json.dumps(report, indent=2))
    finally:
        collecting.clear()
        client.disconnect()
        client.loop_stop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
