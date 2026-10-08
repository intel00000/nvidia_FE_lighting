"""Load the setup files."""

from pathlib import Path


def set_order(path: Path, cards: tuple[int, ...]) -> None:
    """Makes the mock enumerate only these cards, in this order."""
    lines = [line for line in path.read_text().splitlines() if not is_order(line)]
    lines.insert(0, " ".join(["order", *map(str, cards)]))
    path.write_text("\n".join(lines) + "\n")


def zones(path: Path) -> list[list[dict[str, str]]]:
    """The KEY=VALUE words of each card's zone lines."""
    cards: list[list[dict[str, str]]] = []
    for line in path.read_text().splitlines():
        words = line.split()
        if words[:1] == ["card"]:
            cards.append([])
        elif words[:1] == ["zone"]:
            cards[-1].append(dict(word.split("=", 1) for word in words[1:]))
    return cards


def is_order(line: str) -> bool:
    return line.split()[:1] == ["order"]
