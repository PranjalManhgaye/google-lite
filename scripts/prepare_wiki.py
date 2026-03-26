#!/usr/bin/env python3
"""
Convert raw lines into TSV format: id<TAB>title<TAB>body.
This is a lightweight helper for quickly preparing custom corpora.
"""

import argparse


def main() -> None:
  parser = argparse.ArgumentParser()
  parser.add_argument("--input", required=True, help="Input text file (one document per line)")
  parser.add_argument("--output", required=True, help="Output TSV file")
  args = parser.parse_args()

  with open(args.input, "r", encoding="utf-8") as fin, open(args.output, "w", encoding="utf-8") as fout:
    for i, line in enumerate(fin, start=1):
      text = line.strip().replace("\t", " ")
      if not text:
        continue
      title = text[:40] if len(text) > 40 else text
      fout.write(f"{i}\t{title}\t{text}\n")


if __name__ == "__main__":
  main()
