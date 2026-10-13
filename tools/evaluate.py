"""Generate and independently verify Eldritch test-v1 codes (Python standard library)."""
import argparse,gzip,json,math
from pathlib import Path
from collections import Counter,defaultdict
ROOT=Path(__file__).resolve().parents[1]
def read_json(path):
 if str(path).endswith('.gz'):
  with gzip.open(path,'rt',encoding='utf-8') as f:return json.load(f)
 return json.loads(Path(path).read_text(encoding='utf-8'))
def build(data,layout):
 rows=data['entries'];keys=layout['roots'];buckets=defaultdict(list);fulls=['']*len(rows);typed=['']*len(rows)
 for i,r in enumerate(rows):buckets[r[1]+keys[data['roots'][r[2]]]].append(i)
 for prefix,ids in buckets.items():
  order=data['rightOrder'] if prefix[2] in data['leftOrder'] else data['leftOrder']
  ids.sort(key=lambda i:(-rows[i][3],ord(rows[i][0])))
  if len(ids)>len(order):raise ValueError(f'{prefix}: {len(ids)} candidates exceed {len(order)} keys')
  eligible=[i for i in ids if not rows[i][4] and rows[i][3]>0];three=eligible[0] if eligible else None
  for rank,i in enumerate(ids):fulls[i]=prefix+order[rank];typed[i]=rows[i][4] or (prefix if i==three else fulls[i])
 return fulls,typed,buckets

def metrics(data,fulls,typed):
 rows=data['entries'];eq=data['eq'];m=Counter();words=defaultdict(list);allwords=set(data['allWordCodes'])
 for w,c,rank in data['topWords']:words[c].append((rank,w))
 for r,full,code in zip(rows,fulls,typed):
  f=r[3];m['frequency']+=f
  if len(code)>=3:
   m['shape_frequency']+=f;m['actual23_sum']+=f*eq[code[1:3]]
   cc=code+(' ' if len(code)<4 else '');m['shape_cost_sum']+=f*sum(eq[cc[j:j+2]] for j in range(len(cc)-1))
  if len(code)==4:
   m['four_frequency']+=f;m['actual34_sum']+=f*eq[full[2:4]];m['four_tier1500']+=bool(r[7]&1)
   m['all_fixed_word_required_collision']+=full in allwords
   first=min((rank for rank,w in words[full]),default=10**9)
   m['collision_1500_10k']+=bool(r[7]&1) and first<=10000
   m['collision_1500_30k']+=bool(r[7]&1) and first<=30000
   m['collision_3500_10k']+=bool(r[7]&2) and first<=10000
 for name,num,den in [('actual23','actual23_sum','shape_frequency'),('actual34','actual34_sum','four_frequency'),('shape_cost','shape_cost_sum','shape_frequency')]:m[name]=m[num]/m[den] if m[den] else None
 return dict(m)

def word_matrix(data,fulls,typed):
 freq=read_json(ROOT/'data/reading-frequency.json.gz');targets=read_json(ROOT/'data/word-targets.json.gz')
 total=Counter();main={};idx={(r[0],r[1]):i for i,r in enumerate(data['entries'])}
 for ch,ab,f in freq:
  total[ch]+=f
  if ch not in main or f>main[ch][1]:main[ch]=(ab,f)
 ordered=[c for c,f in total.most_common()];first={}
 for rank,(word,code,source_rank,source) in enumerate(targets,1):first.setdefault(code,rank)
 ns=[300,500,1000,1500,2000,2500,3000,3500,4000,5000,6000,7000,8105,15496];wn=[1000,3000,5000,10000,20000,30000,40000,50000,60000];matrix=[];tier_counts=[]
 for n in ns:
  if n==15496:ids=set(range(len(data['entries'])))
  else:
   th=total[ordered[n-1]];identities={(ch,main[ch][0]) for ch in ordered[:n]};identities|={(ch,ab) for ch,ab,f in freq if f>=th};ids={idx[k] for k in identities}
  if n in [1500,3500,6000]:
   bit={1500:1,3500:2,6000:4}[n];assert ids=={i for i,r in enumerate(data['entries']) if r[7]&bit}
  tier_counts.append(len(ids));matrix.append([sum(len(typed[i])==4 and first.get(fulls[i],10**9)<=limit for i in ids) for limit in wn])
 return {'character_limits':ns,'word_limits':wn,'tier_reading_counts':tier_counts,'collision_reading_matrix':matrix,
 'note':'Cumulative tiers. Count required-full reading identities, not character-word pairs. Rows beyond 40,000 words contain supplemental targets.'}

def table_text(rows,fulls,typed):
 table={(r[0],c) for r,full,short in zip(rows,fulls,typed) for c in [full,short]}
 return ''.join(ch+'\t'+c+'\n' for ch,c in sorted(table,key=lambda x:(x[1],x[0])))

def evaluate(layout_path=None,check=False,export=False):
 data=read_json(ROOT/'data/input.json.gz');layout=read_json(layout_path or ROOT/'data/layout.json');rows=data['entries'];fulls,typed,buckets=build(data,layout)
 assert len(fulls)==len(set(fulls))==15844
 assert len({r[0] for r in rows})==15496
 assert all((c[2] in data['leftOrder'])!=(c[3] in data['leftOrder']) for c in fulls)
 assert all(not r[4] or r[4]==c for r,c in zip(rows,typed))
 groups=read_json(ROOT/'data/root-groups.json')
 for unit,p in groups['parts'].items():
  assert len({layout['roots'][r] for r in p['members'] if r in layout['roots']})<=1
 assert layout['roots']['虫']!=layout['roots']['鸟']
 m=metrics(data,fulls,typed);matrix=word_matrix(data,fulls,typed);text=table_text(rows,fulls,typed)
 if check:
  if layout_path:raise ValueError('--check checks the saved first version; omit --layout')
  expected=read_json(ROOT/'reports/summary.json')['metrics']
  for k,v in expected.items():
   if isinstance(v,int):assert m[k]==v,(k,m[k],v)
   else:assert math.isclose(m[k],v,rel_tol=1e-12,abs_tol=1e-7),(k,m[k],v)
  assert text==(ROOT/'tables/奇诡音形测试第一版.txt').read_text(encoding='utf-8')
  assert matrix==read_json(ROOT/'reports/word-clash-matrix.json')
 if export:
  if layout_path:raise ValueError('Use a separate output path for alternative layouts; --export regenerates test-v1 only')
  (ROOT/'tables/奇诡音形测试第一版.txt').write_text(text,encoding='utf-8')
  (ROOT/'reports/word-clash-matrix.json').write_text(json.dumps(matrix,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
 return {'verified':check,'characters':15496,'reading_entries':len(rows),'code_table_rows':len(text.splitlines()),'abx_families':len(buckets),'largest_family':max(map(len,buckets.values())),'metrics':m}
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--check',action='store_true');p.add_argument('--export',action='store_true');p.add_argument('--layout',type=Path);a=p.parse_args()
 print(json.dumps(evaluate(a.layout,a.check,a.export),ensure_ascii=False,indent=2))
