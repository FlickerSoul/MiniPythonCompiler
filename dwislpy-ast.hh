#ifndef __DWISLPY_AST_H_
#define __DWISLPY_AST_H_

// dwislpy-ast.hh
//
// Object classes used by the parser to represent the syntax trees of
// DwiSlpy programs.
//
// It defines a class for a DwiSlpy program and its block of statements,
// and also the two abstract classes whose subclasses form the syntax
// trees of a program's code.
//
//  * Prgm - a DwiSlpy program that consists of a block of statements.
//
//  * Blck - a series of DwiSlpy statements.
//
//  * Stmt - subclasses for the various statments that can occur
//           in a block of code, namely assignment, print, pass,
//           return, and procedure invocation. These get executed
//           when the program runs.
//
//  * Expn - subclasses for the various expressions than can occur
//           on the right-hand side of an assignment statement.
//           These get evaluated to compute a value.
//

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <utility>
#include <iostream>
#include <variant>
#include <optional>
#include "dwislpy-util.hh"
#include "dwislpy-check.hh"

// Valu
//
// The return type of `eval` and of literal values.
// Note: the type `none` is defined in *-util.hh.
//
typedef std::variant<int, bool, std::string, none> Valu;
typedef std::optional<Valu> RtnO;

//
// We "pre-declare" each AST subclass for mutually recursive definitions.
//

class Prgm;
class Defn;
class Blck;
//
class Stmt;
class Pass;
class Asgn;
class Ntro;
class Prnt;
class PCll;
//
class Expn;
class Plus;
class Mnus;
class Tmes;
class IDiv;
class IMod;
class Less;
class LsEq;
class Equl;
class Inpt;
class IntC;
class StrC;
class Lkup;
class Ltrl;
//
class Cond;
class Ifcd;
class Inif;
class Else;
class Elif;
class Whil;
class Rept;
class Coma;
class Coln;
class Pleq;
class Mneq;
class Negt;
class Conj;
class Disj;
class Cmlt;
class Cmgt;
class Cmeq;
class Cmle;
class Cmge;
class Call;
class Imus;
class FCll;
class PRtn;
class FRtn;
//


//
// We alias some types, including pointers and vectors.
//

typedef std::string Name;
typedef std::unordered_map<Name,Valu> Ctxt;
//
typedef std::shared_ptr<Lkup> Lkup_ptr; 
typedef std::shared_ptr<Ltrl> Ltrl_ptr; 
typedef std::shared_ptr<IntC> IntC_ptr; 
typedef std::shared_ptr<StrC> StrC_ptr; 
typedef std::shared_ptr<Inpt> Inpt_ptr; 
typedef std::shared_ptr<Plus> Plus_ptr; 
typedef std::shared_ptr<Mnus> Mnus_ptr; 
typedef std::shared_ptr<Tmes> Tmes_ptr;
typedef std::shared_ptr<IDiv> IDiv_ptr;
typedef std::shared_ptr<IMod> IMod_ptr;


typedef std::shared_ptr<Ifcd> Ifcd_ptr;
typedef std::shared_ptr<Inif> Inif_ptr;
typedef std::shared_ptr<Else> Else_ptr;
typedef std::shared_ptr<Elif> Elif_ptr;
typedef std::shared_ptr<Whil> Whil_ptr;
typedef std::shared_ptr<Rept> Rept_ptr;
typedef std::shared_ptr<Coma> Coma_ptr;
typedef std::shared_ptr<Coln> Coln_ptr;
typedef std::shared_ptr<Pleq> Pleq_ptr;
typedef std::shared_ptr<Mneq> Mneq_ptr;
typedef std::shared_ptr<Negt> Negt_ptr;
typedef std::shared_ptr<Imus> Imus_ptr;
typedef std::shared_ptr<Conj> Conj_ptr;
typedef std::shared_ptr<Disj> Disj_ptr;
typedef std::shared_ptr<Cmlt> Cmlt_ptr;
typedef std::shared_ptr<Cmgt> Cmgt_ptr;
typedef std::shared_ptr<Cmeq> Cmeq_ptr;
typedef std::shared_ptr<Cmle> Cmle_ptr;
typedef std::shared_ptr<Cmge> Cmge_ptr;
typedef std::shared_ptr<PCll> PCll_ptr;
typedef std::shared_ptr<FCll> FCll_ptr;

//
typedef std::shared_ptr<Pass> Pass_ptr; 
typedef std::shared_ptr<Prnt> Prnt_ptr; 
typedef std::shared_ptr<Ntro> Ntro_ptr;
typedef std::shared_ptr<Asgn> Asgn_ptr;
typedef std::shared_ptr<PCll> PCll_ptr;
typedef std::shared_ptr<PRtn> PRtn_ptr;
typedef std::shared_ptr<FRtn> FRtn_ptr;
//
typedef std::shared_ptr<Prgm> Prgm_ptr; 
typedef std::shared_ptr<Defn> Defn_ptr; 
typedef std::shared_ptr<Blck> Blck_ptr; 
typedef std::shared_ptr<Stmt> Stmt_ptr; 
typedef std::shared_ptr<Expn> Expn_ptr;
typedef std::shared_ptr<Cond> Cond_ptr;
//
typedef std::vector<Stmt_ptr> Stmt_vec;
typedef std::vector<Expn_ptr> Expn_vec;
typedef std::vector<Name> Name_vec;
typedef std::unordered_map<Name,Defn_ptr> Defs;
// typedef std::vector<Defn_ptr> Defs;

typedef std::vector<Name> Args;
typedef std::vector<Expn_ptr> Args_vec;
typedef SymT Fmag;

typedef std::vector<Ifcd_ptr> Ifcd_vec;
//
    
//
//
// ************************************************************

// Syntax tree classes used to represent a DwiSlpy program's code.
//
// The classes Blck, Stmt, Expn are all subclasses of AST.
//

//
// class AST 
//
// Cover top-level type for all "abstract" syntax tree classes.
// I.e. this is *the* abstract class (in the OO sense) that all
// the AST node subclasses are derived from.
//

class AST {
private:
    Locn locn; // Location of construct in source code (for reporting errors).
public:
    AST(Locn lo) : locn {lo} { }
    virtual ~AST(void) = default;
    virtual void output(std::ostream& os) const = 0;
    virtual void dump(int level = 0) const = 0;
    Locn where(void) const { return locn; }
};

//
// class Prgm
//
// An object in this class holds all the information gained
// from parsing the source code of a DwiSlpy program. A program
// is a series of DwiSlpy definitions followed by a block of
// statements.
//
// The method Prgm::run implements the DwiSlpy interpreter. This runs the
// DwiSlpy program, executing its statments, updating the state of
// program variables as a result, getting user input from the console,
// and outputting results to the console.  The interpreter relies on
// the Blck::exec, Stmt::exec, and Expn::eval methods of the various
// syntactic components that constitute the Prgm object.
//

class Prgm : public AST {
public:
    //
    Defs defs;
    Blck_ptr main;
    SymT main_symt;
    //
    Prgm(Defs ds, Blck_ptr mn, Locn lo) :
        AST {lo}, defs {ds}, main {mn}, main_symt {} { }
    virtual ~Prgm(void) = default;
    //
    virtual void dump(int level = 0) const;
    virtual void run(void) const; // Execute the program.
    virtual void output(std::ostream& os) const; // Output formatted code.
    void chck(void); // Check for type errors 
};

//
// class Defn
//
// This should become the AST node type that results from parsing
// a `def`. Right now it contains no information.
//
class Defn : public AST {
public:
    //
    Name name;
    Fmag args;
    Blck_ptr body;
    Type ret_type;
    Defn(Name name, Fmag args, Blck_ptr body, Type ret_type, Locn lo) :  AST {lo}, name {name}, args {args},body {body}, ret_type{ret_type} { }
    virtual ~Defn(void) = default;
    //
    virtual void dump(int level = 0) const;
    virtual void output(std::ostream& os) const; // Output formatted code.
    virtual void output(std::ostream& os, std::string indent) const; // Output formatted code.
    virtual Valu exec(const Defs& defs, const Ctxt& ctxt, const Args_vec& vec = {}) const;
    Type returns(void) const;
    unsigned int arity(void) const;
    SymInfo_ptr formal(int i) const;
    void chck(Defs& defs);
};

//
// class Stmt
//
// Abstract class for program statment syntax trees,
//
// Subclasses are
//
//   Asgn - assignment statement "v = e"
//   Prnt - output statement "print(e1,e2,...,ek)"
//   Pass - statement that does nothing
//
// These each support the methods:
//
//  * exec(ctxt): execute the statement within the stack frame
//
//  * output(os), output(os,indent): output formatted DwiSlpy code of
//        the statement to the output stream `os`. The `indent` string
//        gives us a string of spaces for indenting the lines of its
//        code.
//
//  * dump: output the syntax tree of the expression
//
class Stmt : public AST {
public:
    Stmt(Locn lo) : AST {lo} { }
    virtual ~Stmt(void) = default;
    virtual std::optional<Valu> exec(const Defs& defs, Ctxt& ctxt) const = 0;
    virtual void output(std::ostream& os, std::string indent) const = 0;
    virtual void output(std::ostream& os) const;
    virtual Rtns chck(Rtns expd, Defs& defs, SymT& symt) = 0;
};

//
// Asgn - assignment statement AST node
//
class Asgn : public Stmt {
public:
    Name     name;
    Expn_ptr expn;
    Asgn(Name x, Expn_ptr e, Locn l) : Stmt {l}, name {x}, expn {e} { }
    virtual ~Asgn(void) = default;
    virtual std::optional<Valu> exec(const Defs& defs, Ctxt& ctxt) const;
    virtual void output(std::ostream& os, std::string indent) const;
    virtual void dump(int level = 0) const;
    virtual Rtns chck(Rtns expd, Defs& defs, SymT& symt);
};

//
// Ntro - variable introduction with initializer
//
class Ntro : public Stmt {
public:
    Name     name;
    Type type;
    Expn_ptr expn;
    Ntro(Name x, Type t, Expn_ptr e, Locn l) :
        Stmt {l}, name {x}, type {t},expn {e} { }
    virtual ~Ntro(void) = default;
    virtual Rtns chck(Rtns expd, Defs& defs, SymT& symt);
    virtual std::optional<Valu> exec(const Defs& defs, Ctxt& ctxt) const;
    virtual void output(std::ostream& os, std::string indent) const;
    virtual void dump(int level = 0) const;
};

class Pleq : public Stmt {
public:
    Name     name;
    Expn_ptr expn;
    Pleq(Name x, Expn_ptr e, Locn l) : Stmt {l}, name {x}, expn {e} { }
    virtual ~Pleq(void) = default;
    virtual std::optional<Valu> exec(const Defs& defs, Ctxt& ctxt) const;
    virtual void output(std::ostream& os, std::string indent) const;
    virtual void dump(int level = 0) const;
    virtual Rtns chck([[maybe_unused]]Rtns expd, [[maybe_unused]]Defs& defs, [[maybe_unused]]SymT& symt);
};

class Mneq : public Stmt {
public:
    Name     name;
    Expn_ptr expn;
    Mneq(Name x, Expn_ptr e, Locn l) : Stmt {l}, name {x}, expn {e} { }
    virtual ~Mneq(void) = default;
    virtual std::optional<Valu> exec(const Defs& defs, Ctxt& ctxt) const;
    virtual void output(std::ostream& os, std::string indent) const;
    virtual void dump(int level = 0) const;
    virtual Rtns chck([[maybe_unused]]Rtns expd, [[maybe_unused]]Defs& defs, [[maybe_unused]]SymT& symt);
};

class Cond : public Stmt {
    public: 
        Ifcd_vec ifcds;
        Else_ptr els;
    Cond(Ifcd_vec v, Else_ptr e, Locn l) : Stmt {l}, ifcds {v}, els {e} { }
    Cond(Ifcd_vec v, Locn l) : Stmt {l}, ifcds {v}, els {nullptr} {} 
    virtual ~Cond(void) = default;
    virtual std::optional<Valu> exec(const Defs& defs, Ctxt& ctxt) const;
    virtual void output(std::ostream& os, std::string indent) const;
    virtual void dump(int level = 0) const;
    virtual Rtns chck(Rtns expd, Defs& defs, SymT& symt);
};

class Ifcd : public Stmt {
public:
    Expn_ptr cond;
    Blck_ptr body;
    Ifcd(Expn_ptr c, Blck_ptr b, Locn l) : Stmt {l}, cond {c}, body {b} { }
    virtual ~Ifcd(void) = default;
    virtual std::optional<Valu> exec(const Defs& defs, Ctxt& ctxt) const;
    virtual void output(std::ostream& os, std::string indent) const;
    virtual void dump(int level = 0) const;
    virtual Rtns chck([[maybe_unused]]Rtns expd, [[maybe_unused]]Defs& defs, [[maybe_unused]]SymT& symt);
};


class Else : public Stmt {
public:
    Blck_ptr body;
    Else(Blck_ptr b, Locn l) : Stmt {l}, body {b} { }
    virtual ~Else(void) = default;
    virtual std::optional<Valu> exec(const Defs& defs, Ctxt& ctxt) const;
    virtual void output(std::ostream& os, std::string indent) const;
    virtual void dump(int level = 0) const;
    virtual Rtns chck([[maybe_unused]]Rtns expd, [[maybe_unused]]Defs& defs, [[maybe_unused]]SymT& symt);
};


class Elif : public Ifcd {
public:
    Elif(Expn_ptr c, Blck_ptr b, Locn l) : Ifcd {c, b, l} { }
    virtual ~Elif(void) = default;
    virtual std::optional<Valu> exec(const Defs& defs, Ctxt& ctxt) const;
    virtual void output(std::ostream& os, std::string indent) const;
    virtual void dump(int level = 0) const;
    virtual Rtns chck([[maybe_unused]]Rtns expd, [[maybe_unused]]Defs& defs, [[maybe_unused]]SymT& symt);
};


class Whil : public Stmt {
public:
    Expn_ptr cond;
    Blck_ptr body;
    Whil(Expn_ptr c, Blck_ptr b, Locn l) : Stmt {l}, cond {c}, body {b} { }
    virtual ~Whil(void) = default;
    virtual std::optional<Valu> exec(const Defs& defs, Ctxt& ctxt) const;
    virtual void output(std::ostream& os, std::string indent) const;
    virtual void dump(int level = 0) const;
    virtual Rtns chck(Rtns expd, Defs& defs, SymT& symt);
};

class Rept : public Stmt {
    public: 
        Expn_ptr cond;
        Blck_ptr body;
        Rept(Expn_ptr c, Blck_ptr b, Locn l) : Stmt {l}, cond {c}, body {b} { }
        virtual ~Rept(void) = default;
        virtual std::optional<Valu> exec(const Defs& defs, Ctxt& ctxt) const;
        virtual void output(std::ostream& os, std::string indent) const;
        virtual void dump(int level = 0) const;
        virtual Rtns chck([[maybe_unused]]Rtns expd, [[maybe_unused]]Defs& defs, [[maybe_unused]]SymT& symt);
};


class FRtn : public Stmt {
public:
    Expn_ptr expn;
    FRtn(Expn_ptr e, Locn l) : Stmt {l}, expn {e} { }
    virtual ~FRtn(void) = default;
    virtual std::optional<Valu> exec(const Defs& defs, Ctxt& ctxt) const;
    void output(std::ostream& os, std::string indent) const;
    void dump(int level = 0) const;
    virtual Rtns chck(Rtns expd, Defs& defs, SymT& symt);
};

class PRtn : public Stmt {
    public: 
    PRtn(Locn lo) : Stmt(lo) { }
    virtual ~PRtn(void) = default;
    virtual std::optional<Valu> exec(const Defs& defs, Ctxt& ctxt) const;
    void output(std::ostream& os, std::string indent) const;
    void dump(int level = 0) const;
    virtual Rtns chck(Rtns expd, Defs& defs, SymT& symt);
};

//
// Prnt - print statement AST node
//
class Prnt : public Stmt {
public:
    Expn_vec expns;
    Prnt(Expn_vec e, Locn l) : Stmt {l}, expns {e} { }
    virtual ~Prnt(void) = default;
    virtual std::optional<Valu> exec(const Defs& defs, Ctxt& ctxt) const;
    virtual void output(std::ostream& os, std::string indent) const;
    virtual void dump(int level = 0) const;
    Rtns chck(Rtns expd, Defs& defs, SymT& symt);
};


//
// Pass - pass statement AST node
//
class Pass : public Stmt {
public:
    Pass(Locn l) : Stmt {l} { }
    virtual ~Pass(void) = default;
    virtual std::optional<Valu> exec(const Defs& defs, Ctxt& ctxt) const;
    virtual void output(std::ostream& os, std::string indent) const;
    virtual void dump(int level = 0) const;
    Rtns chck(Rtns expd, Defs& defs, SymT& symt);
};

//
// class Blck
//
// Represents a sequence of statements.
//
class Blck : public AST {
public:
    Stmt_vec stmts;
    virtual ~Blck(void) = default;
    Blck(Stmt_vec ss, Locn lo) : AST {lo}, stmts {ss}  { }
    virtual std::optional<Valu> exec(const Defs& defs, Ctxt& ctxt) const;
    virtual void output(std::ostream& os, std::string indent) const;
    virtual void output(std::ostream& os) const;
    virtual void dump(int level = 0) const;
    virtual Rtns chck(Rtns expd, Defs& defs, SymT& symt);
};


//
// class Expn
//
// Abstract class for integer expression syntax trees,
//
// Subclasses are
//
//   Plus - binary operation + applied to two sub-expressions
//   Mnus - binary operation - applied to two sub-expressions
//   Tmes - binary operation * applied to two sub-expressions
//   Ltrl - value constant expression
//   Lkup - variable access (i.e. "look-up") within function frame
//   Inpt - obtains a string of input (after output of a prompt)
//   IntC - converts a value to an integer value
//   StrC - converts a value to a string value
//
// These each support the methods:
//
//  * eval(ctxt): evaluate the expression; return its result
//  * output(os): output formatted DwiSlpy code of the expression.
//  * dump: output the syntax tree of the expression
//
class Expn : public Stmt {
public:
    bool is_statement = false;
    Expn(Locn lo) : Stmt {lo} { }
    virtual ~Expn(void) = default;
    std::optional<Valu> exec(const Defs& defs, Ctxt& ctxt) const final {
        auto val = this->eval(defs, ctxt);

        if (is_statement)
            return std::nullopt;
        else 
            return this->eval(defs, ctxt);
    };
    virtual Valu eval(const Defs& defs, const Ctxt& ctxt) const = 0;
    virtual void output(std::ostream& os) const = 0;
    void output(std::ostream& os, std::string indent) const final {
        if (is_statement) {
            os << indent;
        }

        this->output(os);

        if (is_statement) {
            os << std::endl;
        }
    }
    void make_stmt() {
        this->is_statement = true;
    }
    void make_expn() {
        this->is_statement = false;
    }
    Rtns chck([[maybe_unused]]Rtns expd, [[maybe_unused]]Defs& defs, [[maybe_unused]]SymT& symt) final {throw DwislpyError{where(), "EXPN should not use Stmt check"};};
    virtual Type chck(Defs& defs, SymT& symt) = 0;
};

class Inif : public Expn {
    public: 
        Expn_ptr if_br;
        Expn_ptr cond;
        Expn_ptr else_br;
        Inif(Expn_ptr if_br, Expn_ptr cond, Expn_ptr else_br, Locn l) : Expn {l}, if_br {if_br}, cond {cond}, else_br {else_br} { }
        virtual ~Inif(void) = default;
        virtual Valu eval(const Defs& defs, const Ctxt& ctxt) const;
        virtual void output(std::ostream& os) const;
        virtual void dump(int level = 0) const;
    Type chck(Defs& defs, SymT& symt);
};

class Negt : public Expn {
public:
    Expn_ptr expn;
    Negt(Expn_ptr e, Locn l) : Expn {l}, expn {e} { }
    virtual ~Negt(void) = default;
    virtual Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    virtual void output(std::ostream& os) const;
    virtual void dump(int level = 0) const;
    Type chck(Defs& defs, SymT& symt);
};

class Imus : public Expn {
public:
    Expn_ptr expn;
    Imus(Expn_ptr e, Locn l) : Expn {l}, expn {e} { }
    virtual ~Imus(void) = default;
    virtual Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    virtual void output(std::ostream& os) const;
    virtual void dump(int level = 0) const;
    Type chck(Defs& defs, SymT& symt);
};


class Conj : public Expn {
public:
    Expn_ptr lft;
    Expn_ptr rht;
    Conj(Expn_ptr lft, Expn_ptr rht, Locn l) : Expn {l}, lft {lft}, rht {rht} { }
    virtual ~Conj(void) = default;
    virtual Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    virtual void output(std::ostream& os) const;
    virtual void dump(int level = 0) const;
    Type chck(Defs& defs, SymT& symt);
};


class Disj : public Expn {
public:
    Expn_ptr lft;
    Expn_ptr rht;
    Disj(Expn_ptr lft, Expn_ptr rht, Locn l) : Expn {l}, lft {lft}, rht {rht} { }
    virtual ~Disj(void) = default;
    virtual Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    virtual void output(std::ostream& os) const;
    virtual void dump(int level = 0) const;
    Type chck(Defs& defs, SymT& symt);
};


class Cmlt : public Expn {
public:
    Expn_ptr lft;
    Expn_ptr rht;
    Cmlt(Expn_ptr lft, Expn_ptr rht, Locn l) : Expn {l}, lft {lft}, rht {rht} { }
    virtual ~Cmlt(void) = default;
    virtual Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    virtual void output(std::ostream& os) const;
    virtual void dump(int level = 0) const;
    Type chck(Defs& defs, SymT& symt);
};


class Cmgt : public Expn {
public:
    Expn_ptr lft;
    Expn_ptr rht;
    Cmgt(Expn_ptr lft, Expn_ptr rht, Locn l) : Expn {l}, lft {lft}, rht {rht} { }
    virtual ~Cmgt(void) = default;
    virtual Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    virtual void output(std::ostream& os) const;
    virtual void dump(int level = 0) const;
    Type chck(Defs& defs, SymT& symt);
};


class Cmeq : public Expn {
public:
    Expn_ptr lft;
    Expn_ptr rht;
    Cmeq(Expn_ptr lft, Expn_ptr rht, Locn l) : Expn {l}, lft {lft}, rht {rht} { }
    virtual ~Cmeq(void) = default;
    virtual Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    virtual void output(std::ostream& os) const;
    virtual void dump(int level = 0) const;
    Type chck(Defs& defs, SymT& symt);
};


class Cmle : public Expn {
public:
    Expn_ptr lft;
    Expn_ptr rht;
    Cmle(Expn_ptr lft, Expn_ptr rht, Locn l) : Expn {l}, lft {lft}, rht {rht} { }
    virtual ~Cmle(void) = default;
    virtual Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    virtual void output(std::ostream& os) const;
    virtual void dump(int level = 0) const;
    Type chck(Defs& defs, SymT& symt);
};


class Cmge : public Expn {
public:
    Expn_ptr lft;
    Expn_ptr rht;
    Cmge(Expn_ptr lft, Expn_ptr rht, Locn l) : Expn {l}, lft {lft}, rht {rht} { }
    virtual ~Cmge(void) = default;
    virtual Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    virtual void output(std::ostream& os) const;
    virtual void dump(int level = 0) const;
    Type chck(Defs& defs, SymT& symt);
};


class FCll : public Expn {
    public: 
        Name name;
        Expn_vec args;
        FCll(Name name, Expn_vec args, Locn l) : Expn {l}, name {name}, args {args} { }
        virtual ~FCll(void) = default;
        virtual Valu eval(const Defs& defs, const Ctxt& ctxt) const;
        virtual void output(std::ostream& os) const;
        virtual void dump(int level = 0) const;
        Type chck(Defs& defs, SymT& symt);
};

class PCll : public Stmt {
public:
    Name     name;
    Expn_vec args;
    PCll(Name nm, Expn_vec args, Locn l) : Stmt {l}, name {nm}, args {args} { }
    virtual ~PCll(void) = default;
    virtual Rtns chck(Rtns expd, Defs& defs, SymT& symt);
    virtual std::optional<Valu> exec(const Defs& defs, Ctxt& ctxt) const;
    virtual void output(std::ostream& os, std::string indent) const;
    virtual void dump(int level = 0) const;
};
//
// Plus - addition binary operation's AST node
//
class Plus : public Expn {
public:
    Expn_ptr left;
    Expn_ptr rght;
    Plus(Expn_ptr lf, Expn_ptr rg, Locn lo)
        : Expn {lo}, left {lf}, rght {rg} { }
    virtual ~Plus(void) = default;
    virtual Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    virtual void output(std::ostream& os) const;
    virtual void dump(int level = 0) const;
    Type chck(Defs& defs, SymT& symt);
};

//
// Mnus - subtraction binary operation's AST node
//
class Mnus : public Expn {
public:
    Expn_ptr left;
    Expn_ptr rght;
    Mnus(Expn_ptr lf, Expn_ptr rg, Locn lo)
        : Expn {lo}, left {lf}, rght {rg} { }
    virtual ~Mnus(void) = default;
    virtual Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    virtual void output(std::ostream& os) const;
    virtual void dump(int level = 0) const;
    Type chck(Defs& defs, SymT& symt);
};

//
// Tmes - multiplication binary operation's AST node
//
class Tmes : public Expn {
public:
    Expn_ptr left;
    Expn_ptr rght;
    Tmes(Expn_ptr lf, Expn_ptr rg, Locn lo)
        : Expn {lo}, left {lf}, rght {rg} { }
    virtual ~Tmes(void) = default;
    virtual Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    virtual void output(std::ostream& os) const;
    virtual void dump(int level = 0) const;
    virtual Type chck(Defs& defs, SymT& symt);
};

//
// IDiv - quotient binary operation's AST node
//
class IDiv : public Expn {
public:
    Expn_ptr left;
    Expn_ptr rght;
    IDiv(Expn_ptr lf, Expn_ptr rg, Locn lo)
        : Expn {lo}, left {lf}, rght {rg} { }
    virtual ~IDiv(void) = default;
    virtual Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    virtual void output(std::ostream& os) const;
    virtual void dump(int level = 0) const;
    virtual Type chck(Defs& defs, SymT& symt);
};

//
// IMod - remainder binary operation's AST node
//
class IMod : public Expn {
public:
    Expn_ptr left;
    Expn_ptr rght;
    IMod(Expn_ptr lf, Expn_ptr rg, Locn lo)
        : Expn {lo}, left {lf}, rght {rg} { }
    virtual ~IMod(void) = default;
    virtual Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    virtual void output(std::ostream& os) const;
    virtual void dump(int level = 0) const;
    virtual Type chck(Defs& defs, SymT& symt);
};

//
// Ltrl - literal valu AST node
//
class Ltrl : public Expn {
public:
    Valu valu;
    Ltrl(Valu vl, Locn lo) : Expn {lo}, valu {vl} { }
    virtual ~Ltrl(void) = default;
    virtual Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    virtual void output(std::ostream& os) const;
    virtual void dump(int level = 0) const;
    Type chck(Defs& defs, SymT& symt);
};


//
// Lkup - variable use/look-up AST node
//
class Lkup : public Expn {
public:
    Name name;
    Lkup(Name nm, Locn lo) : Expn {lo}, name {nm} { }
    virtual ~Lkup(void) = default;
    virtual Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    virtual void output(std::ostream& os) const;
    virtual void dump(int level = 0) const;
    virtual Type chck(Defs& defs, SymT& symt);
};

//
// Inpt - input expression AST node
//
class Inpt : public Expn {
public:
    Expn_ptr expn;
    Inpt(Expn_ptr e, Locn lo) : Expn {lo}, expn {e} { }
    virtual ~Inpt(void) = default;
    virtual Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    virtual void output(std::ostream& os) const;
    virtual void dump(int level = 0) const;
    Type chck(Defs& defs, SymT& symt);
};

//
// IntC - int conversion expression AST node
//
class IntC : public Expn {
public:
    Expn_ptr expn;
    IntC(Expn_ptr e, Locn lo) : Expn {lo}, expn {e} { }
    virtual ~IntC(void) = default;
    virtual Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    virtual void output(std::ostream& os) const;
    virtual void dump(int level = 0) const;
    Type chck(Defs& defs, SymT& symt);
};

//
// StrC - string conversion expression AST node
//
class StrC : public Expn {
public:
    Expn_ptr expn;
    StrC(Expn_ptr e, Locn lo) : Expn {lo}, expn {e} { }
    virtual ~StrC(void) = default;
    virtual Valu eval(const Defs& defs, const Ctxt& ctxt) const;
    virtual void output(std::ostream& os) const;
    virtual void dump(int level = 0) const;
    Type chck(Defs& defs, SymT& symt);
};

#endif
