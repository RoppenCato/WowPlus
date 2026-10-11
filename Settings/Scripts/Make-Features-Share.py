"""Makes two files to send to someone, from the features page in docs/:

  Share/WowPlus-Features.html   the whole page in one file; opens in any browser, needs nothing else
  Share/WowPlus-Features.txt    the same as a plain list, to paste into a chat or a mail

Neither contains the link to the repository. Run it again whenever docs/changes.js has changed:

    python Settings/Scripts/Make-Features-Share.py
"""
import io
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DOCS = os.path.join(ROOT, "docs")
OUT = os.path.join(ROOT, "Share")


def main():
    html = io.open(os.path.join(DOCS, "index.html"), encoding="utf-8").read()
    data = io.open(os.path.join(DOCS, "changes.js"), encoding="utf-8").read()
    data = re.sub(r'\n\s*repo: "[^"]*",', '\n  repo: "",', data)
    os.makedirs(OUT, exist_ok=True)

    # One file: the content goes into the page itself
    page = html.replace('<script src="changes.js"></script>', "<script>\n" + data + "\n</script>")
    io.open(os.path.join(OUT, "WowPlus-Features.html"), "w", encoding="utf-8", newline="\n").write(page)

    # A plain list
    title = re.search(r'title: "(.*?)"', data).group(1)
    tagline = re.search(r'tagline: "(.*?)",\n', data).group(1)
    lines = [title.upper(), tagline, ""]
    for area in re.finditer(r'name: "(.*?)",\s*summary: "(.*?)",\s*items: \[(.*?)\n      \]', data, re.S):
        lines += ["", area.group(1).upper(), area.group(2), ""]
        for item in re.finditer(r'title: "((?:[^"\\]|\\.)*)".*?text: "((?:[^"\\]|\\.)*)"', area.group(3), re.S):
            lines.append("- " + item.group(1).replace('\\"', '"') + ": " + item.group(2).replace('\\"', '"'))
    io.open(os.path.join(OUT, "WowPlus-Features.txt"), "w", encoding="utf-8", newline="\r\n").write("\n".join(lines) + "\n")
    print("Written to", OUT, "-", sum(1 for l in lines if l.startswith("- ")), "features")


if __name__ == "__main__":
    main()