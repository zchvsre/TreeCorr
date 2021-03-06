
#include "dbg.h"
#include "OldCell.h"
#include "OldCorr.h"
#include "OldCorrIO.h"

#include <string>
#include <fstream>
#include <vector>

#ifdef MEMDEBUG
AllocList* allocList;
#endif

// Constants to set:
// Three parameters are r,u,v
// if three sides are d1 >= d2 >= d3, then 
// r = d3
// u = d3/d2 (this is reciprocal of usual definition)
// v = (d1-d2)/d3
// r ranges from rmin to rmax in logarithmic bins
// u ranges from 0..1
// v ranges from 0..1
// This prescription ignores sense of the three points, 
// so I take v range from -1..1 with 
// negative v corresponding to 1,2,3 in clockwise order
// positive v corresponding to counterclockwise order
const double minsep = 1.;       // (arcsec) Minimum separation to consider
const double maxsep = 300.*60.;  // (arcsec) Maximum separation to consider
const double binsize = 0.10;     // Size of bins in logr, u, v
const double binslop = 1.0;      // Allowed slop on getting right bin (in number of bins)
// Note binslop on u,v are ~2*binslop

// Derived Constants:
const int nrbins = int(ceil(log(maxsep/minsep)/binsize));
const int nubins = int(ceil(1./binsize));
const double du = 1./nubins;
const int nvbins = 2*nubins;
const double dv = du;
const double b = binslop*binsize;

template<class T> inline T SQR(T x) { return x*x; }

const double outputsize = 1.e3; 
// Only ouput progress lines for cells larger than outputsize

std::ostream* dbgout;
bool XDEBUG = false;

//#define XAssert(s) assert(s)
#define XAssert(s) 
// Switch commented line of these two to get more 
// error checking during the execution 

int recursen=-1;
#include "Process3.h"

void DirectProcessV(
    BinData3<3,3,3>& bin, const Cell& c1, const Cell& c2, const Cell& c3,
    const double d1, const double d2, const double d3,
    const bool swap12, const double u, const double v)
{
    double w = c3.getWeight();
    if (swap12) {
        bin.gam1 += c3.getWE();
    } else {
        bin.gam0 += c3.getWE();
    }
    bin.weight += w;
    bin.meanu += w*u;
    bin.meanv += w*v;
    bin.ntri += c3.getN();
}

void Add(
    std::vector<std::vector<BinData3<3,3,3> > >& data,
    const std::vector<std::vector<BinData3<3,3,3> > >& temp, 
    const Cell& c1, const Cell& c2, const Position2D& r3, double d3)
{
    const double w1w2 = c1.getWeight()*c2.getWeight();
    const double n1n2 = c1.getN()*c2.getN();
    std::complex<double> cr3(r3.getX(),r3.getY());
    const std::complex<double> expm2iarg3 = SQR(conj(cr3)/d3);
    const std::complex<double> e1 = c1.getWE() * expm2iarg3;
    const std::complex<double> e2 = c2.getWE() * expm2iarg3;
    const std::complex<double> e1e2 = e1*e2;
    const std::complex<double> e1e2c = e1*conj(e2);

    for(int ku = 0; ku < int(data.size()); ++ku) {
        std::vector<BinData3<3,3,3> >& dataku = data[ku];
        const std::vector<BinData3<3,3,3> >& tempku = temp[ku];
        for(int kv = 0; kv < int(dataku.size()); ++kv) {
            const BinData3<3,3,3>& tempbin = tempku[kv];
            if (tempbin.weight == 0.0) continue;

            const double www = w1w2*tempbin.weight;
            const std::complex<double>& e3A = tempbin.gam0*expm2iarg3;
            const std::complex<double>& e3B = tempbin.gam1*expm2iarg3;
            const std::complex<double>& e3AB = e3A + e3B;

            BinData3<3,3,3>& databin = dataku[kv];

            databin.gam0 += e1e2*(e3AB);
            databin.gam1 += conj(e1e2c)*e3A + e1e2c*e3B;
            databin.gam2 += e1e2c*e3A + conj(e1e2c)*e3B;
            databin.gam3 += e1e2*conj(e3AB);
            databin.weight += www;
            databin.meanr += www*d3;
            databin.meanu += w1w2*tempbin.meanu;
            databin.meanv += w1w2*tempbin.meanv;
            databin.ntri += n1n2*tempbin.ntri;
        }
    }
}

void FinalizeProcess(
    std::vector<std::vector<std::vector<BinData3<3,3,3> > > >& data, double vare)
// Converts the "means" from weighted sums into weighted means
{
    for(int i=0;i<int(data.size());++i) {
        for(int j=0;j<int(data[i].size());++j) {
            for(int k=0;k<int(data[i][j].size());++k) {
                BinData3<3,3,3>& bindata=data[i][j][k];
                const double w = bindata.weight;
                if (w == 0.) continue;
                bindata.gam0 /= w;
                bindata.gam1 /= w;
                bindata.gam2 /= w;
                bindata.gam3 /= w;
                bindata.meanr /= w; 
                bindata.meanu /= w;
                bindata.meanv /= w;
                bindata.vargam = vare*vare*vare/bindata.ntri;
            }
        }
    }
}

int main(int argc, char* argv[])
{
#ifdef MEMDEBUG
    atexit(&DumpUnfreed);
#endif

    if (argc < 2) myerror("Usage: correeea infilename");
    std::ifstream infile(argv[1]);

    dbgout = &std::cout;
    //dbgout = new std::ofstream("e3.debug");

    double vare;
    std::vector<CellData> celldata;
    Read(infile,minsep,binsize,celldata,vare);

    Cell* wholefield = new Cell(celldata);

    dbg<<"ngal = "<<celldata.size()<<std::endl;
    Assert(vare >= 0.);
    dbg<<"vare = "<<vare<<": sig_sn (per component) = "<<sqrt(vare)<<std::endl;
    dbg<<"nrbins = "<<nrbins<<": min,maxsep = "<<minsep<<','<<maxsep<<std::endl;
    dbg<<"nubins = "<<nubins<<",  nvbins = "<<nvbins<<std::endl;

    dbg<<"wholefield size = "<<wholefield->getSize()<<", meanpos = "<<wholefield->getMeanPos()<<std::endl;

    std::vector<std::vector<std::vector<BinData3<3,3,3> > > > data(
        nrbins,
        std::vector<std::vector<BinData3<3,3,3> > >(
            nubins,std::vector<BinData3<3,3,3> >(nvbins)));
    Process3(data,minsep,maxsep,*wholefield);
    //Process111(data,minsep,maxsep,*wholefield,*wholefield,*wholefield);

    FinalizeProcess(data,vare);
    dbg<<"done processing\n";

    delete wholefield;

    std::ofstream fout("e3.out");
    WriteEEE(fout,minsep,binsize,data);

    std::ofstream fout2("m3.out");
    WriteM3(fout2,minsep,binsize,data);

    if (dbgout && dbgout != &std::cout) 
    { delete dbgout; dbgout=0; }
    return 0;
}
