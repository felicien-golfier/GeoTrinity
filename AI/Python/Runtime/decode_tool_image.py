"""Host-side: decode a base64 image saved in an oversized MCP tool-result file into a PNG to Read. Usage: decode_tool_image.py <result file> <out.png>"""
import base64
import re
import sys


def decode(source, target):
    text = open(source, encoding="utf-8").read()
    data = re.search(r'"data"\s*:\s*"([A-Za-z0-9+/=\\]+)"', text).group(1).replace("\\/", "/")
    open(target, "wb").write(base64.b64decode(data))


if __name__ == "__main__":
    decode(sys.argv[1], sys.argv[2])
