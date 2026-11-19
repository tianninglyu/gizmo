"""Compile the actual snapshot state handlers and thermal event branch in isolation."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[2]
def handler(path, start, end):
    text = (root / path).read_text().split(start, 1)[1].split(end, 1)[0]
    return text.replace("#ifdef GALSF_FB_THERMAL", "").replace("#endif", "")
write = handler("file_io/io.cc", "case IO_THERMAL_FB_STATE:", "case IO_PROMPT_SN_STATE:")
read = handler("file_io/read_ic.cc", "case IO_THERMAL_FB_STATE:", "case IO_PROMPT_SN_STATE:")
event = (root / "galaxy_sf/stellar_evolution.cc").read_text().split("#ifdef GALSF_FB_THERMAL /* STELLAR-POPULATION", 1)[1].split("\n", 1)[1].split("#endif", 1)[0]
source = r"""
#include <cassert>
#include <stdexcept>
#define UNIT_MASS_IN_SOLAR 1.
#define terminate(x) throw std::runtime_error(x)
struct Particle {int Type; double SNe_ThisTimeStep, Mass;} P[3];
struct {double ThermalFBDelayMyr;} All;
double age;
double evaluate_stellar_age_Gyr(int) {return age;}
unsigned int buffer[3]; void *CommBuffer=buffer;
void save() {int n, pc=3, pindex=0, type=4; auto ip_int=buffer; switch(0) {case 0:
""" + write + r"""}}
void load() {int n, pc=3, offset=0; switch(0) {case 0:
""" + read + r"""}}
double event(int i) {
""" + event + r"""}
int main() {
P[0]={4,0,91}; P[1]={4,-1,91}; P[2]={4,2,91};
save(); assert(buffer[0]==0 && buffer[1]==1 && buffer[2]==1);
for(auto &p:P) p.SNe_ThisTimeStep=0;
load();
All.ThermalFBDelayMyr=0; age=0;
assert(event(0)==1); assert(event(1)==0); assert(event(2)==0);
assert(event(0)==0);
buffer[0]=2; bool rejected=false;
try {load();} catch(const std::runtime_error&) {rejected=true;}
assert(rejected);
}
"""
with tempfile.TemporaryDirectory() as tmp:
    path=Path(tmp)
    (path/"test.cc").write_text(source)
    subprocess.run(["c++", "-std=c++11", str(path/"test.cc"), "-o", str(path/"test")], check=True)
    subprocess.run([str(path/"test")], check=True)
print("PASS: snapshot state round trip, pending/spent events, no repeated feedback, invalid state")
