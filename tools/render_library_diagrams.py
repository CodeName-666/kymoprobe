"""Render package documentation diagrams; requires Pillow (documentation only)."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

OUT = Path(__file__).resolve().parents[1] / "lib/KymoCore/docs/images"
INK = "#183044"
MUTED = "#526778"
BLUE = "#dcecff"
TEAL = "#d9f2eb"
AMBER = "#fff0ca"


def font(size, bold=False):
    candidates = [
        Path("C:/Windows/Fonts") / ("segoeuib.ttf" if bold else "segoeui.ttf"),
        Path("/usr/share/fonts/truetype/dejavu") /
        ("DejaVuSans-Bold.ttf" if bold else "DejaVuSans.ttf"),
    ]
    for candidate in candidates:
        if candidate.exists():
            return ImageFont.truetype(str(candidate), size)
    raise RuntimeError("Install Segoe UI or DejaVu Sans to render diagrams")


def canvas(title, subtitle, height):
    picture = Image.new("RGB", (1600, height), "#f5f8fb")
    draw = ImageDraw.Draw(picture)
    draw.text((64, 38), "KYMOCORE / TECHNISCHE DOKUMENTATION", font=font(20, True), fill=MUTED)
    draw.text((64, 84), title, font=font(43, True), fill=INK)
    draw.text((64, 149), subtitle, font=font(24), fill=MUTED)
    return picture, draw


def box(draw, bounds, title, lines, fill=BLUE):
    x, y, right, bottom = bounds
    draw.rounded_rectangle(bounds, 18, fill=fill, outline="#b7c9d5", width=2)
    draw.text((x + 22, y + 18), title, font=font(28, True), fill=INK)
    for index, line in enumerate(lines):
        draw.text((x + 22, y + 64 + index * 35), line, font=font(23), fill=INK)


def arrow(draw, start, end):
    x1, y1 = start
    x2, y2 = end
    draw.line((start, end), fill=MUTED, width=4)
    if y1 == y2:
        draw.polygon([(x2, y2), (x2 - 13, y2 - 8), (x2 - 13, y2 + 8)], fill=MUTED)
    else:
        draw.polygon([(x2, y2), (x2 - 8, y2 - 13), (x2 + 8, y2 - 13)], fill=MUTED)


def save(picture, name):
    OUT.mkdir(parents=True, exist_ok=True)
    picture.save(OUT / name, optimize=True)


def architecture():
    picture, draw = canvas("Vom Messwert zum Diagramm", "Hardware bleibt in der Anwendung; KymoCore kennt nur Daten und Callbacks.", 880)
    box(draw, (64, 225, 510, 465), "1  Anwendung", ["Sensor / Parameter", "clock_ms() und sample()", "Statische Kanalkonfiguration"], TEAL)
    box(draw, (575, 225, 1020, 465), "2  KymoCore", ["Init/Main + Round-Robin", "Codec: Little Endian", "Ein TX-Puffer: maximal 21 Bytes"])
    box(draw, (1085, 225, 1536, 465), "3  Anwendungstreiber", ["write() / busy() / service()", "UART, USB, TCP, MQTT ...", "Kopieren oder Puffer ausleihen"], TEAL)
    arrow(draw, (510, 340), (575, 340))
    arrow(draw, (1020, 340), (1085, 340))
    arrow(draw, (1310, 465), (1310, 555))
    box(draw, (1085, 555, 1536, 785), "4  KymoStudio", ["Stream zusammensetzen", "Frame prüfen / Kanal zuordnen", "Messwerte visualisieren"], AMBER)
    box(draw, (64, 555, 1020, 785), "Vertrag zwischen den Ebenen", ["Keine SDK-Abhängigkeit und kein Heap im Library-Kern.", "Anwendung besitzt Uhr, Konfiguration, Zustände und Treiber.", "Wire-Format: A5 5A + Descriptor + ID + Payload + Trailer."], "#ffffff")
    save(picture, "architecture.png")


def transmission():
    picture, draw = canvas("Ein Frame über mehrere Main-Aufrufe", "Beispiel: 9 Bytes; zuerst 4 Bytes angenommen, danach die restlichen 5.", 1040)
    headings = [(64, "Zyklus"), (285, "KymoCore / Kontext"), (860, "Transport / Pufferbesitz")]
    for x, title in headings:
        draw.text((x, 215), title, font=font(26, True), fill=INK)
    rows = [
        (280, "Main 1", "Kanal fällig → messen → 9 Bytes kodieren", "write(frame, 9) nimmt 4 Bytes an", "Offset = 4; Ergebnis BUSY", "Treiber leiht den angenommenen Bereich", BLUE),
        (440, "Main 2", "busy() ist wahr: Frame unverändert", "DMA / USB verarbeitet die 4 Bytes", "Kein Sample, kein weiterer Write", "Puffer bleibt Eigentum des Kontexts", AMBER),
        (600, "Main 3", "busy() ist falsch: bei Offset 4 fortsetzen", "write(frame + 4, 5) nimmt 5 Bytes an", "Offset = 9; Ergebnis OK", "OK = angenommen, nicht fertig übertragen", BLUE),
        (760, "Main 4", "Bei erneutem Busy weiter warten", "Treiber gibt Puffer nach Abschluss frei", "Erst ohne Busy nächsten Kanal bearbeiten", "Danach darf der Puffer ersetzt werden", TEAL),
    ]
    for y, label, left, right, left2, right2, color in rows:
        draw.text((64, y + 24), label, font=font(27, True), fill=INK)
        box(draw, (265, y, 810, y + 132), "", [], color)
        box(draw, (840, y, 1536, y + 132), "", [], color)
        for x, first, second in [(285, left, left2), (860, right, right2)]:
            draw.text((x, y + 25), first, font=font(21), fill=INK)
            draw.text((x, y + 70), second, font=font(21), fill=MUTED)
    draw.text((64, 950), "service() läuft zu Beginn jedes gültigen Main-Aufrufs. Ohne Busy muss write() die Bytes kopieren.", font=font(24), fill=INK)
    save(picture, "transmission.png")


def protocol():
    picture, draw = canvas("Wire-Protokoll v6.1 / Version 1", "Alle Mehrbytewerte Little Endian. Y ist Pflicht; X, Z und Zeit sind optional.", 1100)
    fields = [("SYNC", "A5 5A", "2 Bytes", BLUE, 170), ("DESC", "Flags", "1 Byte", AMBER, 155),
              ("ID", "0–255", "1 Byte", BLUE, 145), ("X?", "float32", "4 Bytes", TEAL, 180),
              ("Y", "float32", "4 Bytes", BLUE, 180), ("Z?", "float32", "4 Bytes", TEAL, 180),
              ("ZEIT?", "uint32 ms", "4 Bytes", TEAL, 220), ("ENDE", "CRC / 00", "1 Byte", AMBER, 220)]
    x = 64
    for title, value, length, color, width in fields:
        box(draw, (x, 235, x + width - 10, 405), title, [value, length], color)
        x += width
    draw.text((64, 440), "Länge = 9 + 4 × (X vorhanden + Z vorhanden + Zeit vorhanden)  →  9 bis 21 Bytes", font=font(27, True), fill=INK)
    draw.text((64, 505), "Descriptor: Bit 7 links, Bit 0 rechts", font=font(27, True), fill=INK)
    bits = [("7–6", "01", "Version 1", 370, BLUE), ("5–4", "00", "Messpunkt", 370, BLUE),
            ("3", "X", "vorhanden", 182, TEAL), ("2", "Z", "vorhanden", 182, TEAL),
            ("1", "Zeit", "vorhanden", 182, TEAL), ("0", "NO_CRC", "1 = ohne", 186, AMBER)]
    x = 64
    for bit, value, meaning, width, color in bits:
        box(draw, (x, 555, x + width - 10, 720), "Bit " + bit, [value, meaning], color)
        x += width
    box(draw, (64, 765, 1536, 1025), "Beispiel: Kanal 7, Y = 1.0, keine optionalen Felder", [
        "Ohne CRC:   A5 5A | 41 | 07 | 00 00 80 3F | 00     (Default)",
        "Mit CRC:     A5 5A | 40 | 07 | 00 00 80 3F | 54",
        "CRC-8/ATM: Polynom 0x07, Init 0, keine Reflexion, XOR-out 0.",
        "CRC umfasst Descriptor + ID + Payload; Sync und Trailer sind ausgenommen."], "#ffffff")
    save(picture, "protocol.png")


if __name__ == "__main__":
    architecture()
    transmission()
    protocol()
