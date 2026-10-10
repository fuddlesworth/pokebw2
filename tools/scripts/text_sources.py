"""The messages that come from the game data in JSON rather than from the text files, as pokeplatinum's text banks come
from res/: a line `\\from{species.name}` in a file of data/text/ stands for the names of all the species, one per line,
from data/pokemon/. text_data.py expands these lines when it packs the text.

In the JSON, text is written as in the text files (see text_data.py), except that a line break is a real one ("\\n" in
the JSON) rather than `\\n`: control codes stay `{TTTT:a,b}`, and other characters `\\x{HHHH}`.
"""
import json
import re
import sys
from functools import cache
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from datajson import ROOT, DataError  # noqa: E402
from gen_constants import load as load_list  # noqa: E402


def to_json(line: str) -> str:
    """Returns a line of a text file as JSON text: the `\\n` escapes become line breaks."""
    out = []
    i = 0
    while i < len(line):
        if line[i] == "\\" and i + 1 < len(line):
            out.append("\n" if line[i + 1] == "n" else line[i:i + 2])
            i += 2
        else:
            out.append(line[i])
            i += 1
    return "".join(out)


def to_line(text: str) -> str:
    """Returns JSON text as a line of a text file."""
    return text.replace("\n", "\\n")


ESCAPE = re.compile(r"(\\x\{[0-9a-fA-F]+\}|\{[^}]*\}|\\.)")


def upper(line: str) -> str:
    """Returns a line in capitals, leaving its escapes and control codes as they are."""
    return "".join(part if ESCAPE.fullmatch(part) else part.upper() for part in ESCAPE.split(line))


def ordered(list_name: str) -> list[str]:
    items = sorted(load_list(list_name).items(), key=lambda item: item[1])
    return [name for name, _ in items]


@cache
def species() -> list[dict]:
    """The species' data.json files, in the order of species.txt (not the forms or extra records)."""
    return [read(ROOT / "data/pokemon" / name.removeprefix("SPECIES_").lower() / "data.json")
            for name in ordered("species")]


@cache
def moves() -> list[dict]:
    return [read(ROOT / "data/moves" / name.removeprefix("MOVE_").lower() / "data.json") for name in ordered("moves")]


@cache
def trainers() -> dict[str, dict]:
    """The trainers' files by constant, in the order of trainers.txt, without TRAINER_NONE, which has none."""
    return {name: read(ROOT / "data/trainers" / (name.removeprefix("TRAINER_").lower() + ".json"))
            for name in ordered("trainers")[1:]}


def read(path: Path) -> dict:
    try:
        return json.loads(path.read_text())
    except (OSError, json.JSONDecodeError) as error:
        raise DataError(f"{path}: {error}") from None


def field(data: dict, key: str, where: str) -> str:
    if key not in data:
        raise DataError(f"{where}: {key} is missing")
    return data[key]


def article(data: dict) -> str:
    """The article of a species' name: name_article if the file has one, else "an" before a vowel and "a" before
    anything else."""
    return data.get("name_article") or ("an" if data["name"][:1] in "AEIOU" else "a")


def message_order() -> list[str]:
    """The trainers whose messages the trainer message table has, in its order: the ones of
    data/trainers/message_order.json, then any other trainer with messages, in ID order."""
    listed = json.loads((ROOT / "data/trainers/message_order.json").read_text())
    for name in listed:
        if not trainers().get(name, {}).get("messages"):
            raise DataError(f"data/trainers/message_order.json: {name} is not a trainer with messages")
    rest = [name for name, data in trainers().items() if data.get("messages") and name not in listed]
    return listed + rest


def trainer_messages() -> list[str]:
    return [to_line(message["text"]) for name in message_order() for message in trainers()[name]["messages"]]


def species_lines(key: str, format_line=to_line) -> list[str]:
    return [format_line(field(data, key, f"species {i}")) for i, data in enumerate(species())]


SOURCES = {
    "species.name": lambda: species_lines("name"),
    "species.name_upper": lambda: species_lines("name", lambda name: upper(to_line(name))),
    # From species 1: SPECIES_NONE's line is empty
    "species.name_with_article": lambda: [f"{{bd01}}{article(data)} {{ff00:255}}{to_line(data['name'])}"
                                          for data in species()[1:]],
    "species.category": lambda: species_lines("category"),
    "species.pokedex_entry": lambda: species_lines("pokedex_entry"),
    "moves.name": lambda: [to_line(data["name"]) for data in moves()],
    "moves.name_upper": lambda: [upper(to_line(data["name"])) for data in moves()],
    "moves.description": lambda: [to_line(data["description"]) for data in moves()],
    "trainers.name": lambda: [("\\c" if data.get("compress_name", True) else "") + to_line(data["name"])
                              for data in trainers().values()],
    "trainers.messages": trainer_messages,
}


def expand(source: str) -> list[str]:
    if source not in SOURCES:
        raise DataError(f"\\from{{{source}}}: no such source; there are {', '.join(SOURCES)}")
    return SOURCES[source]()
