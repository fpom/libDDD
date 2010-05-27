#include "IntExpression.hpp"

#include <cmath>
#include <cassert>
#include "hashfunc.hh"
#include <typeinfo>

// unique storage class
class _IntExpression {
  // count references
  mutable size_t refCount;
  // access to refcount for garbage purpose
  friend class IntExpressionFactory;
 public :
  // add a ref
  size_t ref () const { 
    return ++refCount;
  }
  // dereference
  size_t deref () const { 
    assert(refCount >0);
    return --refCount;
  }
  // default constructor
  _IntExpression (): refCount(0) {}
  // reclaim memory
  virtual ~_IntExpression () { assert (refCount==0); };
  
  ///////// Interface functions
  // for hash storage
  virtual size_t hash () const = 0 ;
  virtual bool operator==(const _IntExpression & e) const = 0;
  virtual _IntExpression * clone () const = 0;

  // to avoid excessive typeid RTTI calls.
  virtual IntExprType getType() const =0;

  // Because friend is not transitive. Offer access to IntExpression concrete to subclasses.
  static const _IntExpression * getConcrete ( const IntExpression & e) { return e.concrete ;}

  // pretty print
  virtual void print (std::ostream & os) const =0 ;

  // Evaluate an expression.
  virtual IntExpression eval() const = 0;

  virtual IntExpression setAssertion (const Assertion & a) const {
    return a.getValue(this);
  }

  virtual bool isSupport (const Variable & v) const = 0;

};


namespace d3 { namespace util {
  template<>
  struct hash<_IntExpression*> {
    size_t operator()(_IntExpression * g) const{
      return g->hash();
    }
  };

  template<>
  struct equal<_IntExpression*> {
    bool operator()(_IntExpression *g1,_IntExpression *g2) const{
      return (typeid(*g1) == typeid(*g2) && *g1 == *g2);
    }
  };
} } 

// namespace std {
//   template<>
//   struct less<const IntExpression &> {
//     bool operator()(const IntExpression &g1,const IntExpression &g2) const{
//       return g1 < g2;
//     }
//   };
// }


class VarExpr : public _IntExpression {
  const Variable & var;  

public :
  VarExpr (const Variable & vvar) : var(vvar){};
  IntExprType getType() const  { return VAR; }

  void print (std::ostream & os) const {
    os << var.getName();
  }

  IntExpression eval () const {
    return this ;
  }

  bool operator==(const _IntExpression & e) const {
    return var == ((const VarExpr &)e).var;
  }

  virtual size_t hash () const {
    return var.hash() * 70019;
  }

  _IntExpression * clone () const { return new VarExpr(*this); }

  bool isSupport (const Variable & v) const {
    return var == v;
  }

};

class ConstExpr : public _IntExpression {
  int val;

public :
  ConstExpr (int vval) : val(vval) {}
  IntExprType getType() const  { return CONST; }

  int getValue() const { return val; }

  IntExpression eval () const {
    return this;
  }

  bool operator==(const _IntExpression & e) const {
    return val == ((const ConstExpr &)e).val;
  }

  virtual size_t hash () const {
    return val * 70019;
  }

  _IntExpression * clone () const { return new ConstExpr(*this); }

  void print (std::ostream & os) const {
    os << val;
  }

  bool isSupport (const Variable&) const {
    return false;
  }

};


class NaryIntExpr : public _IntExpression {
protected :
  NaryParamType params ;
public :
  virtual const char * getOpString() const = 0;

  const NaryParamType & getParams () const { return params; }
  
  NaryIntExpr (const NaryParamType & pparams):params(pparams) {};

  // evaluate on two integers
  virtual int constEval (int i, int j) const = 0;
  // neutral element w.r.t. operation (e.g 1 for *, 0 for +)
  virtual int getNeutralElement () const = 0;


  IntExpression eval () const {
    NaryParamType p ;
    int constant = getNeutralElement();
    for (NaryParamType::const_iterator it = params.begin(); it != params.end() ; ++it ) {
      IntExpression e = it->eval();
      if (e.getType() == CONST) {
	constant = constEval(constant, ((const ConstExpr &)* getConcrete(e)).getValue());
      } else {
	p.insert(e);
      }
    }
    if (constant != getNeutralElement() || p.empty())
      p.insert ( IntExpressionFactory::createConstant(constant) );
    if (p.size() == 1) 
      return *p.begin();
    else 
      return IntExpressionFactory::createNary(getType(),p);
  }


  void print (std::ostream & os) const {
    os << "( ";
    for (NaryParamType::const_iterator it = params.begin() ;  ; ) {
      it->print(os);
      if (++it == params.end())
	break;
      else
	os << getOpString();
    }
    os << " )";
  }

  bool operator== (const _IntExpression & e) const {
    const NaryIntExpr & other = (const NaryIntExpr &)e ;
    if (params.size() != other.params.size()) 
      return false;
    NaryParamType::const_iterator it = params.begin();
    NaryParamType::const_iterator jt = other.params.begin();
    for ( ; it != params.end()  ; ++it,++jt ) 
      if (! it->equals(*jt))
	return false;
    return true;
  }

  size_t hash () const {
    size_t res = getType();
    for (NaryParamType::const_iterator it = params.begin() ; it != params.end()  ; ++it ) {
      res = res*(it->hash() +  76303);
    }
    return res;
  }

  IntExpression setAssertion (const Assertion & a) const {
    NaryParamType res ;
    for (NaryParamType::const_iterator it = params.begin() ; it != params.end()  ; ++it ) {
      res.insert( (*it) & a );
    }
    IntExpression e = IntExpressionFactory::createNary(getType(),res);    
    return a.getValue(e);
  }

  bool isSupport (const Variable & v) const {
    for (NaryParamType::const_iterator it = params.begin() ; it != params.end()  ; ++it ) {
      if (it->isSupport(v)) 
	return true;
    }
    return false;
  }

};

class PlusExpr : public NaryIntExpr {

public :
  PlusExpr (const NaryParamType & pparams):NaryIntExpr(pparams) {};
  IntExprType getType() const  { return PLUS; }
  const char * getOpString() const { return " + ";}
  
  int constEval (int i, int j) const {
    return i+j;
  }
  int getNeutralElement () const {
    return 0;
  }

  _IntExpression * clone () const { return new PlusExpr(*this); }
};

class MultExpr : public NaryIntExpr {

public :
  MultExpr (const NaryParamType  & pparams):NaryIntExpr(pparams) {};
  IntExprType getType() const  { return MULT; }
  const char * getOpString() const { return " * ";}

  int constEval (int i, int j) const {
    return i*j;
  }
  int getNeutralElement () const {
    return 1;
  }
  _IntExpression * clone () const { return new MultExpr(*this); }
};

class BinaryIntExpr : public _IntExpression {
protected :
  IntExpression left;
  IntExpression right;
public :
  virtual const char * getOpString() const = 0;
  BinaryIntExpr (const IntExpression & lleft, const IntExpression & rright) : left (lleft),right(rright){};

  virtual int constEval (int i, int j) const = 0;

  IntExpression eval () const {
    IntExpression  l = left.eval();
    IntExpression  r = right.eval();

    if (l.getType() == CONST && r.getType() == CONST ) {
      return  IntExpressionFactory::createConstant( constEval( l.getValue(), r.getValue()) );
    } else {
      return  IntExpressionFactory::createBinary( getType(), l, r );
    }
  }

  bool operator==(const _IntExpression & e) const{
    const BinaryIntExpr & other = (const BinaryIntExpr &)e ;
    return other.left.equals(left) && other.right.equals(right);
  }
 
  size_t hash () const {
    size_t res = getType();
    res *= left.hash() *  76303 + right.hash() * 76147;
    return res;
  }
  void print (std::ostream & os) const {
    os << "( ";
    left.print(os);
    os << getOpString();
    right.print(os);
    os << " )";
  }

  IntExpression setAssertion (const Assertion & a) const {
    IntExpression l = left & a;
    IntExpression r = right & a;
    IntExpression e = IntExpressionFactory::createBinary(getType(),l,r);    
    return a.getValue(e);
  }

  bool isSupport (const Variable & v) const {
    return left.isSupport(v) || right.isSupport(v);
  }

};

class MinusExpr : public BinaryIntExpr {

public :
  MinusExpr (const IntExpression & left, const IntExpression & right) : BinaryIntExpr(left,right) {};
  IntExprType getType() const  { return MINUS; }
  const char * getOpString() const { return " - ";}
  int constEval (int i, int j) const {
    return i-j;
  }
  _IntExpression * clone () const { return new MinusExpr(*this); }

};

class DivExpr : public BinaryIntExpr {

public :
  DivExpr (const IntExpression & left, const IntExpression & right) : BinaryIntExpr(left,right) {};
  IntExprType getType() const  { return DIV; }
  const char * getOpString() const { return " / ";}

  int constEval (int i, int j) const {
    return i/j;
  }

  _IntExpression * clone () const { return new DivExpr(*this); }

};

class ModExpr : public BinaryIntExpr {

public :
  ModExpr (const IntExpression & left, const IntExpression & right) : BinaryIntExpr(left,right) {};
  IntExprType getType() const  { return MOD; }
  const char * getOpString() const { return " % ";}

  int constEval (int i, int j) const {
    return i % j;
  }

  _IntExpression * clone () const { return new ModExpr(*this); }

};

class PowExpr : public BinaryIntExpr {

public :
  PowExpr (const IntExpression & left, const IntExpression & right) : BinaryIntExpr(left,right) {};
  IntExprType getType() const  { return POW; }
  const char * getOpString() const { return " ** ";}

  int constEval (int i, int j) const {
    return int(pow(i,j));
  }
  _IntExpression * clone () const { return new PowExpr(*this); }

};


/********************************************************/
/***********  Assertion *********************************/

Assertion::Assertion (const IntExpression & var, const IntExpression & val) : mapping(var,val) {};

IntExpression Assertion::getValue (const IntExpression & v) const {
  if (v.equals(mapping.first)) 
    return mapping.second;
  else 
    return v;
}

size_t Assertion::hash() const {
  return mapping.first.hash() ^ mapping.second.hash();
}

bool Assertion::operator== (const Assertion & other) const {
  return mapping.first.equals(other.mapping.first) && mapping.second.equals(other.mapping.second);
}

Assertion Assertion::operator & (const Assertion & other) const {
  return IntExpressionFactory::createAssertion(mapping.first, (mapping.second & other).eval());
}

/// To determine whether a given variable is mentioned in an expression.
bool Assertion::isSupport (const Variable & v) const {
  return mapping.first.isSupport(v) || mapping.second.isSupport(v);
}


/*******************************************************/
/******* Factory ***************************************/
// namespace IntExpressionFactory {

UniqueTable<_IntExpression>  IntExpressionFactory::unique = UniqueTable<_IntExpression>();


IntExpression IntExpressionFactory::createNary (IntExprType type, NaryParamType params) {
  switch (type) {
  case PLUS :
    return unique(PlusExpr (params));      
  case MULT :
    return unique(MultExpr (params));      
  default :
    throw "Operator is not nary";
  }
}

IntExpression IntExpressionFactory::createBinary (IntExprType type, const IntExpression & l, const IntExpression & r) {
  switch (type) {
  case MINUS :
    return unique(MinusExpr (l,r));      
  case DIV :
    return unique(DivExpr (l,r));      
  case MOD :
    return unique(ModExpr (l,r));      
  case POW :
    return unique(PowExpr (l,r));      
  default :
    throw "Operator is not binary";
  }
}

IntExpression IntExpressionFactory::createConstant (int v) {
  return unique (ConstExpr(v));
}

IntExpression IntExpressionFactory::createVariable (const Variable & v) {
  return unique (VarExpr(v));
}

Assertion IntExpressionFactory::createAssertion (const Variable & v,const IntExpression & e) {
  return createAssertion (createVariable(v),e);
}

Assertion IntExpressionFactory::createAssertion (const IntExpression & v,const IntExpression & e) {
  return Assertion(v,e);
}

const _IntExpression * IntExpressionFactory::createUnique(const _IntExpression &e) {
  return unique(e);
}

void IntExpressionFactory::destroy (_IntExpression * e) {
  if (  e->deref() == 0 ){
    UniqueTable<_IntExpression>::Table::iterator ci = unique.table.find(e);
    assert (ci != unique.table.end());
    unique.table.erase(ci);
    delete e;
  }
}

void IntExpressionFactory::printStats (std::ostream &os) {
  os << "Integer expression entries :" << unique.size() << std::endl;
}

// } end namespace IntExpressionFactory


// namespace IntExpression {
// friend operator
IntExpression operator+(const IntExpression & l,const IntExpression & r) {  
  NaryParamType p;
  if (l.getType() == PLUS) {
    const NaryParamType & p2 = ((const NaryIntExpr *) l.concrete)->getParams();
    p.insert(p2.begin(), p2.end());
  } else {
    p.insert(l);
  }
  if (r.getType() == PLUS) {
    const NaryParamType & p2 = ((const NaryIntExpr *) r.concrete)->getParams();
    p.insert(p2.begin(), p2.end());
  } else {
    p.insert (r);
  }
  return IntExpressionFactory::createNary(PLUS, p);
} 

IntExpression operator*(const IntExpression & l,const IntExpression & r) {  
  NaryParamType p;
  if (l.getType() == MULT) {
    const NaryParamType & p2 =  ((const NaryIntExpr *)l.concrete)->getParams();
    p.insert(p2.begin(), p2.end());
  } else {
    p.insert(l);
  }
  if (r.getType() == MULT) {
    const NaryParamType & p2 = ((const NaryIntExpr *)r.concrete)->getParams();
    p.insert(p2.begin(), p2.end());
  } else {
    p.insert (r);
  }
  return IntExpressionFactory::createNary(MULT, p);
} 



// necessary administrative trivia
// refcounting
IntExpression::IntExpression (const _IntExpression * concret): concrete(concret) {
  concrete->ref();
}

IntExpression::IntExpression (const IntExpression & other) {
  if (this != &other) {
    concrete = other.concrete;
    concrete->ref();
  }
}


IntExpression::IntExpression (int cst) {
  concrete = IntExpressionFactory::createUnique(ConstExpr(cst));
  concrete->ref();
}

IntExpression::IntExpression (const Variable & var) {
  concrete = IntExpressionFactory::createUnique(VarExpr(var));
  concrete->ref();
}

IntExpression::~IntExpression () {
  // remove const qualifier for delete call
  IntExpressionFactory::destroy((_IntExpression *) concrete);  
}

IntExpression & IntExpression::operator= (const IntExpression & other) {
  if (this != &other) {
    // remove const qualifier for delete call
    IntExpressionFactory::destroy((_IntExpression *) concrete);
    concrete = other.concrete;
    concrete->ref();
  }
  return *this;
}

bool IntExpression::equals (const IntExpression & other) const {
  return concrete == other.concrete ;
}

bool IntExpression::less (const IntExpression & other) const {
  return concrete < other.concrete;
}


void IntExpression::print (std::ostream & os) const {
  concrete->print(os);
}

IntExpression IntExpression::eval () const {
  return concrete->eval();
}

/// only valid for CONST expressions
/// use this call only in form : if (e.getType() == CONST) { int j = e.getValue() ; ...etc }
/// Exceptions will be thrown otherwise.
int IntExpression::getValue () const {
  if (getType() != CONST) {
    throw "Do not call getValue on non constant int expressions.";
  } else {
    return ((const ConstExpr *) concrete)->getValue();    
  }
}

bool IntExpression::isSupport(const Variable & var) const {
  return concrete->isSupport(var);
}

IntExpression IntExpression::operator& (const Assertion &a) const {
  return concrete->setAssertion(a);
}

IntExprType IntExpression::getType() const {
  return concrete->getType();
}
// binary
IntExpression IntExpression::operator-(const IntExpression & e) const {
  return IntExpressionFactory::createBinary(MINUS,*this,e);
}
IntExpression IntExpression::operator/(const IntExpression & e) const {
  return IntExpressionFactory::createBinary(DIV,*this,e);
}
IntExpression IntExpression::operator%(const IntExpression & e) const {
  return IntExpressionFactory::createBinary(MOD,*this,e);
}
IntExpression IntExpression::operator^(const IntExpression & e) const {
  return IntExpressionFactory::createBinary(POW,*this,e);
}

size_t IntExpression::hash () const { 
  return ddd::knuth32_hash(reinterpret_cast<const size_t>(concrete)); 
}

std::ostream & operator << (std::ostream & os, const IntExpression & e) {
  e.print(os);
  return os;
}

// end class IntExpression}
