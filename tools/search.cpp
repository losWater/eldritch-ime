#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>
using namespace std;

struct Entry {int root,sy,fixed,tier,cp; double f;};
struct Metrics {double cost=0,e23=0,e34=0,fourf=0;int cap=0,over=0,g1=0,g2=0,g3=0,four1500=0,moves=0;};
Metrics operator+(Metrics a,const Metrics&b){a.cost+=b.cost;a.e23+=b.e23;a.e34+=b.e34;a.fourf+=b.fourf;a.cap+=b.cap;a.over+=b.over;a.g1+=b.g1;a.g2+=b.g2;a.g3+=b.g3;a.four1500+=b.four1500;a.moves+=b.moves;return a;}
Metrics operator-(Metrics a,const Metrics&b){a.cost-=b.cost;a.e23-=b.e23;a.e34-=b.e34;a.fourf-=b.fourf;a.cap-=b.cap;a.over-=b.over;a.g1-=b.g1;a.g2-=b.g2;a.g3-=b.g3;a.four1500-=b.four1500;a.moves-=b.moves;return a;}

int R,N,S;double eqm[27][27],sf=0,shortweight=0,moveweight=0;vector<Entry> ent;vector<vector<int>> rid,rsy,buckets;vector<pair<int,int>> mutex_groups;
vector<Metrics> cache;vector<int> oldkey,maxg,asgn,syA,syB;array<bool,26> is_left{};
array<vector<int>,26> tails;vector<unsigned char> wm(26*26*26*26,0);
bool cmpid(int a,int b){return ent[a].f!=ent[b].f ? ent[a].f>ent[b].f : ent[a].cp<ent[b].cp;}
Metrics evaluate(int bi){
 int key=bi/S,sy=bi%S;auto &ids=buckets[bi];Metrics m;int cap=tails[key].size();
 int d=max(0,int(ids.size())-cap);m.cap=d*d;m.over=d>0;
 int three=-1;for(int id:ids)if(!ent[id].fixed && ent[id].f>0){three=id;break;}
 for(int rank=0;rank<(int)ids.size();rank++){
  int id=ids[rank];auto&e=ent[id];if(e.fixed)continue;
  bool is3=id==three;int tail=rank<cap?tails[key][rank]:tails[key].back();
  m.e23+=e.f*eqm[syB[sy]][key];
  m.cost+=e.f*(eqm[syA[sy]][syB[sy]]+eqm[syB[sy]][key]+eqm[key][is3?26:tail]);
  if(is3)continue;
  m.fourf+=e.f;m.e34+=e.f*eqm[key][tail];m.four1500+=bool(e.tier&1);
  if(rank>=cap)continue;
  int code=((syA[sy]*26+syB[sy])*26+key)*26+tail;int flags=wm[code];
  m.g1+=bool(e.tier&1)&&bool(flags&1);
  m.g2+=bool(e.tier&1)&&bool(flags&2);
  m.g3+=bool(e.tier&2)&&bool(flags&1);
 }
 return m;
}
bool feasible(const Metrics&m){return m.cap==0&&m.g1==0&&m.g2==0&&m.g3==0;}
double score(const Metrics&m,double lambda){return 100.0*m.cap+10.0*(m.g1+m.g2+m.g3)+m.cost/sf+lambda*m.e23/sf+shortweight*m.four1500+moveweight*m.moves;}
bool compatible(int r,int src,int dst,int r2=-1){for(auto [a,b]:mutex_groups){int ka=a==r?dst:(a==r2?src:asgn[a]);int kb=b==r?dst:(b==r2?src:asgn[b]);if(ka>=0&&ka==kb)return false;}return true;}
void print(const Metrics&m,string tag){cout<<tag<<" cap="<<m.cap<<" groups="<<m.over<<" gates="<<m.g1<<","<<m.g2<<","<<m.g3<<" E23="<<m.e23/sf<<" E34="<<(m.fourf?m.e34/m.fourf:0)<<" shape="<<m.cost/sf<<" four1500="<<m.four1500<<" changed="<<m.moves<<endl;}
void write_solution(const string&path,const vector<int>&a,const Metrics&m,int seed,double lambda,int steps){
 ofstream o(path);o<<setprecision(15)<<"{\"seed\":"<<seed<<",\"lambda23\":"<<lambda<<",\"shortweight\":"<<shortweight<<",\"steps\":"<<steps<<",\"feasible\":"<<(feasible(m)?"true":"false")<<",\"metrics\":{\"capacity_penalty\":"<<m.cap<<",\"overflow_groups\":"<<m.over<<",\"collision_1500_10k\":"<<m.g1<<",\"collision_1500_30k\":"<<m.g2<<",\"collision_3500_10k\":"<<m.g3<<",\"actual23\":"<<m.e23/sf<<",\"actual34\":"<<(m.fourf?m.e34/m.fourf:0)<<",\"shape_cost\":"<<m.cost/sf<<",\"four_frequency\":"<<m.fourf<<",\"four_tier1500\":"<<m.four1500<<"},\"assignment\":[";
 for(int r=0;r<R;r++){if(r)o<<",";o<<a[r];}o<<"]}\n";
}

int main(int argc,char**argv){
 if(argc<6){cerr<<"search prepared.txt seed lambda23 iterations output.json [start.txt or -] [shortweight]"<<endl;return 1;}
 int seed=stoi(argv[2]),steps=stoi(argv[4]);double lambda=stod(argv[3]);string op=argv[5];mt19937 rng(seed);if(argc>7)shortweight=stod(argv[7]);if(argc>8)moveweight=stod(argv[8]);
 ifstream in(argv[1]);in>>R>>N>>S;for(auto&row:eqm)for(double&v:row)in>>v;
 string lo,ro;in>>lo>>ro;for(char c:lo)is_left[c-'a']=true;
 for(int k=0;k<26;k++)for(char c:(is_left[k]?ro:lo))tails[k].push_back(c-'a');
 oldkey.resize(R);maxg.resize(R);asgn.assign(R,-1);rid.resize(R);rsy.resize(R);
 for(int i=0;i<R;i++){int r;in>>r;in>>oldkey[r]>>maxg[r];}
 syA.resize(S);syB.resize(S);for(int i=0;i<S;i++)in>>syA[i]>>syB[i];
 ent.resize(N);for(int i=0;i<N;i++){auto&e=ent[i];in>>e.root>>e.sy>>e.f>>e.fixed>>e.tier>>e.cp;rid[e.root].push_back(i);rsy[e.root].push_back(e.sy);if(!e.fixed)sf+=e.f;}
 for(auto&sy:rsy){sort(sy.begin(),sy.end());sy.erase(unique(sy.begin(),sy.end()),sy.end());}
 int nw;in>>nw;for(int i=0;i<nw;i++){int c,v;in>>c>>v;wm[c]=v;}
 int nm=0;if(in>>nm)for(int i=0;i<nm;i++){int a,b;in>>a>>b;mutex_groups.push_back({a,b});}
 buckets.resize(26*S);cache.resize(26*S);Metrics total;
 vector<int> rorder(R);iota(rorder.begin(),rorder.end(),0);vector<double> rf(R,0);for(int r=0;r<R;r++)for(int id:rid[r])rf[r]+=ent[id].f;
 // Constraint-first greedy initialization; high-frequency roots settle before zero-frequency tails.
 shuffle(rorder.begin(),rorder.end(),rng);
 stable_sort(rorder.begin(),rorder.end(),[&](int a,int b){if((maxg[a]>11)!=(maxg[b]>11))return maxg[a]>11;if(maxg[a]!=maxg[b])return maxg[a]>maxg[b];return rf[a]>rf[b];});
 if(argc>6&&string(argv[6])!="-"){
  ifstream start(argv[6]);for(int r=0;r<R;r++)start>>asgn[r];
  for(int id=0;id<N;id++)buckets[asgn[ent[id].root]*S+ent[id].sy].push_back(id);
  for(int bi=0;bi<26*S;bi++){sort(buckets[bi].begin(),buckets[bi].end(),cmpid);cache[bi]=evaluate(bi);total=total+cache[bi];}
  for(int r=0;r<R;r++)total.moves+=asgn[r]!=oldkey[r];
 } else for(int r:rorder){
  vector<int> choices;for(int k=0;k<26;k++)if(maxg[r]<=(int)tails[k].size()&&compatible(r,-1,k))choices.push_back(k);shuffle(choices.begin(),choices.end(),rng);
  int bestk=-1;double bs=1e100;Metrics bt;
  for(int k:choices){Metrics next=total;
   for(int sy:rsy[r])next=next-cache[k*S+sy];
   for(int id:rid[r])buckets[k*S+ent[id].sy].push_back(id);
   for(int sy:rsy[r]){sort(buckets[k*S+sy].begin(),buckets[k*S+sy].end(),cmpid);next=next+evaluate(k*S+sy);}
   next.moves=total.moves+(k!=oldkey[r]);double sc=score(next,lambda)+uniform_real_distribution<double>(0,0.015)(rng);
   if(sc<bs){bs=sc;bestk=k;bt=next;}
   for(int sy:rsy[r]){auto&v=buckets[k*S+sy];v.erase(remove_if(v.begin(),v.end(),[&](int id){return ent[id].root==r;}),v.end());}
  }
  asgn[r]=bestk;for(int id:rid[r])buckets[bestk*S+ent[id].sy].push_back(id);
  for(int sy:rsy[r]){sort(buckets[bestk*S+sy].begin(),buckets[bestk*S+sy].end(),cmpid);cache[bestk*S+sy]=evaluate(bestk*S+sy);}
  total=bt;
 }
 print(total,"start");double bestscore=score(total,lambda);Metrics besttotal=total;vector<int> bestassign=asgn;double bestfeas=feasible(total)?bestscore:1e100;Metrics feasmetrics;vector<int> feasassign;
 if(feasible(total)){feasmetrics=total;feasassign=asgn;}
 vector<double> rw(R);for(int r=0;r<R;r++)rw[r]=1+sqrt((double)rid[r].size());discrete_distribution<int> rootdist(rw.begin(),rw.end());uniform_real_distribution<double> U(0,1);
 for(int step=0;step<steps;step++){
  if(step==int(0.60*steps)){
   asgn=feasassign.empty()?bestassign:feasassign;
   for(auto&v:buckets)v.clear();for(int id=0;id<N;id++)buckets[asgn[ent[id].root]*S+ent[id].sy].push_back(id);
   total=Metrics{};for(int bi=0;bi<26*S;bi++){sort(buckets[bi].begin(),buckets[bi].end(),cmpid);cache[bi]=evaluate(bi);total=total+cache[bi];}
   for(int r=0;r<R;r++)total.moves+=asgn[r]!=oldkey[r];
  }
  int r=rootdist(rng),src=asgn[r],dst=rng()%26,r2=-1;
  if(U(rng)<0.30){r2=rootdist(rng);if(r2==r)continue;dst=asgn[r2];}
  if(src==dst||maxg[r]>(int)tails[dst].size()||(r2>=0&&maxg[r2]>(int)tails[src].size())||!compatible(r,src,dst,r2))continue;
  vector<int> sy=rsy[r];if(r2>=0){sy.insert(sy.end(),rsy[r2].begin(),rsy[r2].end());sort(sy.begin(),sy.end());sy.erase(unique(sy.begin(),sy.end()),sy.end());}
  vector<int> affected;for(int s:sy){affected.push_back(src*S+s);affected.push_back(dst*S+s);}
  vector<vector<int>> saved;vector<Metrics> savem;Metrics next=total;
  for(int bi:affected){saved.push_back(buckets[bi]);savem.push_back(cache[bi]);next=next-cache[bi];auto&v=buckets[bi];v.erase(remove_if(v.begin(),v.end(),[&](int id){return ent[id].root==r||(r2>=0&&ent[id].root==r2);}),v.end());}
  for(int id:rid[r])buckets[dst*S+ent[id].sy].push_back(id);
  if(r2>=0)for(int id:rid[r2])buckets[src*S+ent[id].sy].push_back(id);
  for(int bi:affected){sort(buckets[bi].begin(),buckets[bi].end(),cmpid);cache[bi]=evaluate(bi);next=next+cache[bi];}
  next.moves=total.moves+(dst!=oldkey[r])-(src!=oldkey[r]);if(r2>=0)next.moves+=(src!=oldkey[r2])-(dst!=oldkey[r2]);
  double progress=(double)step/steps;
  // Initial search crosses constraint barriers; late search preserves a feasible layout.
  double T=progress<0.60 ? (argc>9?stod(argv[9]):15.0)*pow(0.006/(argc>9?stod(argv[9]):15.0),progress/0.60) : 0.006*pow(0.01,(progress-0.60)/0.40);
  double delta=score(next,lambda)-score(total,lambda);
  if(delta<=0||U(rng)<exp(-delta/T)){asgn[r]=dst;if(r2>=0)asgn[r2]=src;total=next;}
  else for(int j=0;j<(int)affected.size();j++){buckets[affected[j]]=move(saved[j]);cache[affected[j]]=savem[j];}
  double sc=score(total,lambda);
  if(sc<bestscore-1e-10){bestscore=sc;besttotal=total;bestassign=asgn;}
  if(feasible(total)&&sc<bestfeas-1e-10){bestfeas=sc;feasmetrics=total;feasassign=asgn;write_solution(op,feasassign,feasmetrics,seed,lambda,step+1);}
  if(step%50000==0){print(total,"step "+to_string(step));}
 }
 if(!feasassign.empty()){write_solution(op,feasassign,feasmetrics,seed,lambda,steps);print(feasmetrics,"best feasible");}
 else {write_solution(op,bestassign,besttotal,seed,lambda,steps);print(besttotal,"best incomplete");}
 // Recompute from complete assignment to check incremental metric bookkeeping.
 vector<int> answer=feasassign.empty()?bestassign:feasassign;for(auto&v:buckets)v.clear();for(int id=0;id<N;id++)buckets[answer[ent[id].root]*S+ent[id].sy].push_back(id);Metrics check;
 for(int bi=0;bi<26*S;bi++){sort(buckets[bi].begin(),buckets[bi].end(),cmpid);check=check+evaluate(bi);}
 for(int r=0;r<R;r++)check.moves+=answer[r]!=oldkey[r];for(auto [a,b]:mutex_groups)if(answer[a]==answer[b]){cerr<<"MUTEX MISMATCH"<<endl;return 3;}
 Metrics expect=feasassign.empty()?besttotal:feasmetrics;
 if(check.moves!=expect.moves||check.cap!=expect.cap||check.g1!=expect.g1||check.g2!=expect.g2||check.g3!=expect.g3||abs(check.cost-expect.cost)>1e-5||abs(check.e23-expect.e23)>1e-5){cerr<<"INCREMENTAL MISMATCH"<<endl;return 2;}
 print(check,"recomputed");return 0;
}
