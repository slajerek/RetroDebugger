"""Source contract checks, not native registration or bridge execution."""
from pathlib import Path
import re
root = Path(__file__).resolve().parents[2]
mcp = (root / 'src/Remote/MCP/CMCPServer.cpp').read_text()
vice = (root / 'src/Emulators/vice/ViceInterface/CDebuggerServerApiVice.cpp').read_text()
bridge = (root / 'src/Tests/CTestMCPBridge.cpp').read_text()
names = re.findall(r'tool\.name = "([^"]+)"', mcp)
assert names.count('retro_disk_directory') == 1
assert len(names) == len(set(names)), 'duplicate tools'
assert 'desc.fn = "c64/disk/directory";' in vice
assert 'server->RunEndpointFunction("c64/disk/directory"' in mcp
assert bridge.count('toolCount != 52') == 2, 'connected AND reconnected counts'
assert 'directoryCount != 1' in bridge
assert '!names.count("retro_disk_directory")' in bridge
assert 'toolCount != 49' not in bridge
assert 'toolCount != 50' not in bridge
assert 'toolCount != 51' not in bridge
assert 'tools.size() != 3' in bridge or 'toolCount != 3' in bridge
print('PASS directory registration source contracts (native execution untested)')
