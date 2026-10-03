"""Run the 68000 conversion/reference tests on an ST in Hatari."""
import subprocess
import sys
from pathlib import Path
sys.path.insert(0, str(Path.home()/'.codex/skills/hatari-debugging/scripts'))
from hatari_session import HatariSession
root = Path(__file__).resolve().parent
binary = root/'verify.tos'
lines = subprocess.check_output(['m68k-atari-mintelf-nm', '-n', str(binary)], text=True)
symbols = {p[2]: int(p[0],16) for line in lines.splitlines() if len(p:=line.split()) == 3}
with HatariSession(str(binary), screenshot_dir='/tmp/chunky-shots', sound=False,
 debug_except='bus,address,illegal', extra_args=['--machine','st','--memsize','4']) as h:
 h.wait(12)
 base = h.basepage()['text']
 result = base+symbols['gChunkyTestResult']
 cases = base+symbols['gChunkyTestCases']
 value = h.mem(result,2).u16(result)
 count = h.mem(cases,2).u16(cases)
 print('result:',value,'cases:',count, flush=True)
 assert value == 0 and count == 240
 assert not any('Address Error' in l or 'Bus Error' in l or 'Exception' in l for l in h.lines)
print('PASS: both conversions match independent references on 68000 ST')
