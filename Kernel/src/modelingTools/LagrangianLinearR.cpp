/* Siconos-Kernel version 1.2.0, Copyright INRIA 2005-2006.
 * Siconos is a program dedicated to modeling, simulation and control
 * of non smooth dynamical systems.
 * Siconos is a free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Siconos is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Siconos; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact: Vincent ACARY vincent.acary@inrialpes.fr
 */
#include "LagrangianLinearR.h"
//
#include "LagrangianDS.h"

using namespace std;

// Default (private) constructor
LagrangianLinearR::LagrangianLinearR():
  LagrangianR(), H(NULL), b(NULL), D(NULL), isHAllocatedIn(false), isBAllocatedIn(false), isDAllocatedIn(false)
{
  relationType = LAGRANGIANLINEARRELATION;
}

// Xml constructor
LagrangianLinearR::LagrangianLinearR(RelationXML* relxml):
  LagrangianR(relxml), H(NULL), b(NULL), D(NULL)
{
  // if one between h and G is plugged, both must be.
  if (isHPlugged != isGPlugged[0])
    RuntimeException::selfThrow("LagrangianLinearR:: xml constructor, can not have plug-in for h and matrix for G=H in linear case.");

  relationType = LAGRANGIANLINEARRELATION;
  isAllocatedIn["H"] = false;
  isAllocatedIn["b"] = false;
  isAllocatedIn["D"] = false;

  if (!isHPlugged)
  {
    LagrangianLinearRXML* LLRxml = (static_cast<LagrangianLinearRXML*>(relationxml));
    unsigned int row = LLRxml->getH().size(0);

    if (LLRxml->hasD())
      setLagrangianRelationType("scleronomic+lambda");
    else
      setLagrangianRelationType("scleronomic");

    H = new SimpleMatrix(LLRxml->getH());
    isAllocatedIn["H"] = true;
    G[0] = H;
    if (LagrangianRelationType == "scleronomic+lambda")
    {
      unsigned int rowD = LLRxml ->getD().size(0);
      unsigned int colD = LLRxml ->getD().size(1);
      if (rowD != colD || (interaction != NULL && rowD !=  interaction->getInteractionSize()))
        RuntimeException::selfThrow("LagrangianLinearR:: xml constructor, inconsistent size for input matrix D");

      D = new SimpleMatrix(LLRxml->getD());
      isAllocatedIn["D"] = true;
      G[1] = D;
    }

    if (LLRxml->hasB())
    {
      unsigned int rowB = LLRxml->getB().size();
      if (row != rowB)
        RuntimeException::selfThrow("LagrangianLinearR:: xml constructor, inconsistent size between b and H");
      b = new SimpleVector(rowB);
      *b = LLRxml->getB();
      isAllocatedIn["b"] = true;
    }
  }
  else
  {
    cout << "Warning: LagrangianLinearR xml constructor, original relations uses plug-in functions for h and G=H definition." << endl;
    H = G[0];
    if (LagrangianRelationType == "scleronomic+lambda")
      D = G[1];
  }
}

// Constructor from data: H, b and interaction (optional)
LagrangianLinearR::LagrangianLinearR(const SiconosMatrix& newH, const SimpleVector& newB):
  LagrangianR(), H(NULL), b(NULL), D(NULL)
{
  relationType = LAGRANGIANLINEARRELATION;
  unsigned int row = newH.size(0);
  unsigned int row2 = newB.size(0) ;
  if (row2 != row)
    RuntimeException::selfThrow("LagrangianLinearR:: constructor from data, inconsistent size between H and b");

  setLagrangianRelationType("scleronomic");

  H = new SimpleMatrix(newH);
  isAllocatedIn["H"] = true;
  G[0] = H;

  isGPlugged.push_back(false);

  b = new SimpleVector(newB);
  isAllocatedIn["H"] = false;
  isAllocatedIn["b"] = true;
  isAllocatedIn["D"] = false;
}

// Constructor from data: H and interaction (optional)
LagrangianLinearR::LagrangianLinearR(const SiconosMatrix& newH):
  LagrangianR(), H(NULL), b(NULL), D(NULL)
{
  relationType = LAGRANGIANLINEARRELATION;
  setLagrangianRelationType("scleronomic");
  isAllocatedIn["H"] = false;
  isAllocatedIn["b"] = false;
  isAllocatedIn["D"] = false;
  H = new SimpleMatrix(newH);
  isAllocatedIn["H"] = true;
  G[0] = H;

  isGPlugged.push_back(false);
}

// Constructor from data: H, b, D and interaction (optional)
LagrangianLinearR::LagrangianLinearR(const SiconosMatrix& newH, const SimpleVector& newB, const SiconosMatrix& newD):
  LagrangianR(), H(NULL), b(NULL), D(NULL)
{
  relationType = LAGRANGIANLINEARRELATION;
  unsigned int row = newH.size(0);
  unsigned int row2 = newB.size(0);
  unsigned int rowD = newD.size(0);
  unsigned int colD = newD.size(1);

  if (row2 != row)
    RuntimeException::selfThrow("LagrangianLinearR:: constructor from data, inconsistent size between H and b");

  if (rowD != colD || rowD != row)
    RuntimeException::selfThrow("LagrangianLinearR:: constructor from data, inconsistent size for D input matrix");

  setLagrangianRelationType("scleronomic+lambda");

  H = new SimpleMatrix(newH);
  isAllocatedIn["H"] = true;
  G[0] = H;
  D = new SimpleMatrix(newD);
  isAllocatedIn["D"] = true;
  G[1] = D;

  isGPlugged.push_back(false); // G[0]
  isGPlugged.push_back(false); // G[1]
  b = new SimpleVector(newB);
  isAllocatedIn["b"] = true;
}

// copy constructor (inter is optional)
LagrangianLinearR::LagrangianLinearR(const Relation & newLLR):
  LagrangianR(newLLR), H(NULL), b(NULL), D(NULL)
{
  if (relationType != LAGRANGIANLINEARRELATION)
    RuntimeException::selfThrow("LagrangianLinearR:: copy constructor, inconsistent relation types for copy");

  // if one between h and G is plugged, both must be.
  if (isHPlugged != isGPlugged[0])
    RuntimeException::selfThrow("LagrangianLinearR:: data constructor, can not have plug-in for h and matrix for G=H in linear case.");

  const LagrangianLinearR *  llr = static_cast<const LagrangianLinearR*>(&newLLR);

  // warning: G may have already been allocated in LagrangianR copy constructor!
  isAllocatedIn["H"] = false;
  isAllocatedIn["b"] = false;
  isAllocatedIn["D"] = false;

  if (!isHPlugged)
  {
    if (G[0] == NULL)
    {
      H = new SimpleMatrix(llr->getH());
      isAllocatedIn["H"] = true;
      G[0] = H;
    }
    else
      H = G[0];

    if (llr->getBPtr() != NULL)
    {
      b = new SimpleVector(llr->getB());
      isAllocatedIn["b"] = true;
    }

    if (llr->getDPtr() != NULL)
    {
      if (G[1] == NULL)
      {
        D = new SimpleMatrix(llr->getD());
        isAllocatedIn["D"] = true;
        G[1] = D;
      }
      else
        D = G[1];
    }
  }
  else
  {
    cout << "Warning: LagrangianLinearR copy constructor, original relations uses plug-in functions for h and G=H definition." << endl;
    H = G[0];
    if (LagrangianRelationType == "scleronomic+lambda")
      G[1] = D;
  }
}

LagrangianLinearR::~LagrangianLinearR()
{
  if (isAllocatedIn["H"]) delete H;
  H = NULL;
  if (isAllocatedIn["b"]) delete b;
  b = NULL;
  if (isAllocatedIn["D"]) delete D;
  D = NULL;
}

void LagrangianLinearR::initialize()
{
  Relation::initialize();
  manageGMemory();

  // We get the size of the interaction and of the DS vector
  unsigned int sizeRef = interaction->getInteractionSize();
  unsigned int sizeDS  = interaction->getSizeOfDS();

  if (H != NULL)
    if (H->size(1) != sizeDS || H->size(0) != sizeRef)
      RuntimeException::selfThrow("LagrangianLinearR::initialize inconsistent sizes between H matrix and the interaction.");
  if (D != NULL)
    if (D->size(0) != sizeRef || D->size(1) != sizeRef)
      RuntimeException::selfThrow("LagrangianLinearR::initialize inconsistent sizes between D matrix and the interaction.");
  if (b != NULL)
    if (b->size(0) != sizeRef)
      RuntimeException::selfThrow("LagrangianLinearR::initialize inconsistent sizes between b vector and the dimension of the interaction.");
}

// Setters

void LagrangianLinearR::setH(const SiconosMatrix& newValue)
{
  isHPlugged = false;
  isGPlugged[0] = false;
  unsigned int sizeY = newValue.size(0);
  unsigned int sizeQ = newValue.size(1);

  if (interaction != NULL)
  {
    unsigned int size = interaction->getInteractionSize();
    if (size != sizeY)
      RuntimeException::selfThrow("LagrangianLinearR - setH: inconsistent dimensions with problem size for input matrix H");
  }

  if (H == NULL)
  {
    H = new SimpleMatrix(newValue);
    isAllocatedIn["H"] = true;
  }
  else
  {
    if (sizeQ == H->size(1))
      *H = newValue;
    else
      RuntimeException::selfThrow("lagrangianLinearR - setH: inconsistent dimensions with problem size for input matrix H");
  }
}

void LagrangianLinearR::setHPtr(SiconosMatrix *newPtr)
{
  isHPlugged = false;
  isGPlugged[0] = false;
  if (isAllocatedIn["H"]) delete H;
  if (interaction != NULL)
  {
    unsigned int sizeY = interaction->getInteractionSize();
    if (newPtr->size(0) != sizeY)
      RuntimeException::selfThrow("LagrangianLinearR - setHPtr: inconsistent dimensions with problem size for input matrix H");
  }
  H = newPtr;
  isAllocatedIn["H"] = false;
}

void LagrangianLinearR::setB(const SimpleVector& newValue)
{
  isHPlugged = false;
  isGPlugged[0] = false;
  unsigned int sizeY = newValue.size();
  if (interaction != NULL)
  {
    unsigned int size = interaction->getInteractionSize();
    if (size != sizeY)
      RuntimeException::selfThrow("LagrangianLinearR - setB: inconsistent dimensions with problem size for input vector b");
  }

  if (b == NULL)
  {
    b = new SimpleVector(newValue);
    isAllocatedIn["b"] = true;
  }
  else
  {
    if (sizeY == b->size())
      *b = newValue;
    else
      RuntimeException::selfThrow("LagrangianLinearR - setB: inconsistent dimensions with problem size for input vector b");
  }
}

void LagrangianLinearR::setBPtr(SimpleVector *newPtr)
{
  isHPlugged = false;
  isGPlugged[0] = false;
  if (isAllocatedIn["b"]) delete b;
  b = newPtr;
  isAllocatedIn["b"] = false;
}

void LagrangianLinearR::setD(const SiconosMatrix& newValue)
{
  unsigned int sizeY = newValue.size(0);
  unsigned int colD = newValue.size(1);
  if (sizeY != colD)
    RuntimeException::selfThrow("lagrangianLinearR - setD: inconsistent dimensions for input matrix D");

  if (interaction != NULL)
  {
    unsigned int size = interaction->getInteractionSize();
    if (size != sizeY)
      RuntimeException::selfThrow("LagrangianLinearR - setD: inconsistent dimensions with problem size for input matrix D");
  }

  if (D == NULL)
  {
    D = new SimpleMatrix(newValue);
    isAllocatedIn["D"] = true;
  }
  else
    *D = newValue;
}

void LagrangianLinearR::setDPtr(SiconosMatrix *newPtr)
{
  if (newPtr->size(0) != newPtr->size(1))
    RuntimeException::selfThrow("lagrangianLinearR - setDPtr: inconsistent dimensions for input matrix D");

  if (isAllocatedIn["D"]) delete D;
  if (interaction != NULL)
  {
    unsigned int sizeY = interaction->getInteractionSize();
    if (newPtr->size(0) != sizeY)
      RuntimeException::selfThrow("LagrangianLinearR - setDPtr: inconsistent dimensions with problem size for input matrix D");
  }
  D = newPtr;
  isAllocatedIn["D"] = false;
}

void LagrangianLinearR::getHBlockDS(DynamicalSystem * ds, SiconosMatrix& Block) const
{
  unsigned int k = 0;
  DynamicalSystemsSet vDS = interaction ->getDynamicalSystems();
  DSIterator itDS;
  itDS = vDS.begin();

  // look for ds
  while (*itDS != ds && itDS != vDS.end())
  {
    k += (*itDS)->getN() / 2;
    itDS++;
  }
  // check dimension
  if ((*itDS)->getN() / 2 != Block.size(1))
    RuntimeException::selfThrow("LagrangianLinearR - getHBlockDS: inconsistent sizes between HBlock and DS");

  // get block
  unsigned int l = k + (*itDS)->getN() / 2 - 1;
  vector<unsigned int> index_list(4);
  index_list[0] = 0;
  index_list[1] = H->size(0) - 1;
  index_list[2] = k;
  index_list[3] = l;
  H->getBlock(index_list, Block);
}

void LagrangianLinearR::getHBlockDS(const int DSNumber, SiconosMatrix& Block) const
{
  unsigned int k = 0;

  DynamicalSystemsSet vDS = interaction ->getDynamicalSystems();

  DSIterator itDS;
  itDS = vDS.begin();

  // look for DS number DSNumber ...
  while ((*itDS)->getNumber() != DSNumber && itDS != vDS.end())
  {
    k += (*itDS)->getN() / 2;
    itDS++;
  }

  // check dimension
  if ((*itDS)->getN() / 2 != Block.size(1))
    RuntimeException::selfThrow("LagrangianLinearR - getHBlockDS: inconsistent sizes between HBlock and DS");

  // get block
  unsigned int l = k + (*itDS)->getN() / 2 - 1;
  vector<unsigned int> index_list(4);
  index_list[0] = 0;
  index_list[1] = H->size(0) - 1;
  index_list[2] = k;
  index_list[3] = l;
  H->getBlock(index_list, Block);
}

void LagrangianLinearR::computeOutput(const double time, const unsigned int derivativeNumber)
{
  if (interaction == NULL)
    RuntimeException::selfThrow("LagrangianLinearR::computeOutput, no interaction linked with this relation");

  if (!isHPlugged)
  {

    // Get the DS concerned by the interaction of this relation
    DynamicalSystemsSet vDS = interaction->getDynamicalSystems();
    DSIterator it;
    BlockVector *tmp = new BlockVector();

    string type;
    LagrangianDS * lds;
    for (it = vDS.begin(); it != vDS.end() ; ++it)
    {
      // check dynamical system type
      type = (*it)->getType();
      if (type != LTIDS && type != LNLDS)
        RuntimeException::selfThrow("LagrangianLinearR::computeOutput not yet implemented for dynamical system of type: " + type);
      lds = static_cast<LagrangianDS*>(*it);
      // Put q and velocity of each DS into a block
      if (derivativeNumber == 0)
        tmp->addPtr(lds->getQPtr());
      else if (derivativeNumber == 1)
        tmp->addPtr(lds->getVelocityPtr());
      else if (derivativeNumber == 2)
        tmp->addPtr(static_cast<SimpleVector*>(lds->getAccelerationPtr()));
    }

    // get y and yDot of the interaction
    SiconosVector *y = interaction->getYPtr(derivativeNumber);
    SiconosVector *lambda = interaction->getLambdaPtr(derivativeNumber);
    //      SiconosVector *lambdaDot = interaction->getLambdaPtr(1);
    // compute y or yDot

    *y = (*H * *tmp);
    if (derivativeNumber == 0 && b != NULL)
      *y += *b;

    if (D != NULL)
      *y    += *D * *lambda ;

    // free memory
    delete tmp;
  }
  else
    LagrangianR::computeOutput(time, derivativeNumber);
}

void LagrangianLinearR::computeFreeOutput(const double time, const unsigned int derivativeNumber)
{
  if (interaction == NULL)
    RuntimeException::selfThrow("LagrangianLinearR::computeFreeOutput, no interaction linked with this relation");

  if (!isHPlugged)
  {
    // Get the DS concerned by the interaction of this relation
    DynamicalSystemsSet vDS = interaction->getDynamicalSystems();
    DSIterator it;

    BlockVector *freeTmp = new BlockVector();

    LagrangianDS* lds;
    string type;
    for (it = vDS.begin(); it != vDS.end(); ++it)
    {
      // check dynamical system type
      type = (*it)->getType();
      if (type != LTIDS && type != LNLDS)
        RuntimeException::selfThrow("LagrangianLinearR::computeFreeOutput not yet implemented for dynamical system of type " + type);

      // convert current DS into a LagrangianDS
      lds = static_cast<LagrangianDS*>(*it);

      // Put qFree and velocityFree of each DS into a block
      if (derivativeNumber == 0)
        freeTmp->addPtr(lds->getQFreePtr());
      else if (derivativeNumber == 1)
        freeTmp->addPtr(lds->getVelocityFreePtr());
      else if (derivativeNumber == 2) // Warning: no "accelerationFree", we get the last computed acceleration (ie rhs).
        freeTmp->addPtr(lds->getAccelerationPtr());
      else RuntimeException::selfThrow("LagrangianLinearR::computeFreeOutput not yet implemented for derivative number: " + derivativeNumber);
    }

    // get y and yDot of the interaction
    SiconosVector *y = interaction->getYPtr(derivativeNumber);
    //SiconosVector *yDot = interaction->getYPtr(1);
    // compute y and yDot (!! values for free state)

    *y = (*H * *freeTmp) ;

    if (derivativeNumber == 0 &&  b != NULL)
      *y += *b;

    // free memory
    delete freeTmp;
  }
  else
    LagrangianR::computeFreeOutput(time, derivativeNumber);
}

void LagrangianLinearR::computeInput(const double time, const unsigned int level)
{
  if (interaction == NULL)
    RuntimeException::selfThrow("LagrangianLinearR::computeInput, no interaction linked with this relation");

  if (!isHPlugged)
  {
    // Get the DS concerned by the interaction of this relation
    DynamicalSystemsSet vDS = interaction->getDynamicalSystems();
    DSIterator it;

    BlockVector *p = new BlockVector();
    string typeDS;

    LagrangianDS* lds;
    string type;
    for (it = vDS.begin(); it != vDS.end(); ++it)
    {
      // check dynamical system type
      typeDS = (*it)->getType();
      if (typeDS != LTIDS && typeDS != LNLDS)
        RuntimeException::selfThrow("LagrangianLinearR::computeInput not yet implemented for this type of dynamical system " + typeDS);

      // convert current DS into a LagrangianDS
      lds = static_cast<LagrangianDS*>(*it);
      // Put p of each DS into a block
      // Warning: use addPtr -> link between pointers
      p->addPtr(lds->getPPtr(2));

    }

    p->zero();

    // get lambda of the concerned interaction
    SiconosVector *lambda = interaction->getLambdaPtr(level);

    // compute p = Ht lambda
    *p += matTransVecMult(*H, *lambda);
    delete p;
  }
  else
    LagrangianR::computeInput(time, level);
}

void LagrangianLinearR::saveRelationToXML() const
{
  if (relationxml != NULL)
  {
    (static_cast<LagrangianLinearRXML*>(relationxml))->setH(*H) ;
    (static_cast<LagrangianLinearRXML*>(relationxml))->setB(*b) ;
    (static_cast<LagrangianLinearRXML*>(relationxml))->setD(*D) ;
  }
  else RuntimeException::selfThrow("LagrangianLinearR::saveRelationToXML - object RelationXML does not exist");
}

LagrangianLinearR* LagrangianLinearR::convert(Relation *r)
{
  LagrangianLinearR* llr = dynamic_cast<LagrangianLinearR*>(r);
  return llr;
}

void LagrangianLinearR::display() const
{
  LagrangianR::display();
  cout << "===== Lagrangian Linear Relation display ===== " << endl;
  cout << " H: " << endl;
  if (H != NULL)
    H->display();
  else
    cout << " -> NULL " << endl;
  cout << " b: " << endl;
  if (b != NULL)
    b->display();
  else
    cout << " -> NULL " << endl;
  cout << " D: " << endl;
  if (D != NULL)
    D->display();
  else
    cout << " -> NULL " << endl;


  cout << "===================================== " << endl;
}
