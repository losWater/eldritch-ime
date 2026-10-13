"""Reproduce the v1 grouped-layout search; needs g++ or clang++ plus Python stdlib."""
import argparse,gzip,json,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',default='g++');p.add_argument('--iterations',type=int,default=900000);p.add_argument('--output',type=Path,default=ROOT/'build/searched-layout.json');a=p.parse_args()
a.output.parent.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='eldritch-v1-') as tmp:
 t=Path(tmp);inp=t/'prepared.txt';inp.write_bytes(gzip.decompress((ROOT/'data/search-input.txt.gz').read_bytes()));exe=t/'search';candidate=t/'candidate.json'
 subprocess.run([a.compiler,'-O3','-std=c++17',str(ROOT/'tools/search.cpp'),'-o',str(exe)],check=True)
 subprocess.run([str(exe),str(inp),'617','1',str(a.iterations),str(candidate),str(ROOT/'data/search-start.txt'),'.003','.001'],check=True)
 c=json.loads(candidate.read_text());groups=json.loads((ROOT/'data/root-groups.json').read_text());assignment=dict(zip(groups['units'],c['assignment']))
 data=json.loads(gzip.decompress((ROOT/'data/input.json.gz').read_bytes()))
 roots={r:chr(97+assignment[groups['root_to_unit'][r]]) for r in data['roots']}
 a.output.write_text(json.dumps({'roots':roots,'search':c},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(a.output)
