%skeleton "lalr1.cc"
%require  "3.0"
%debug 
%defines 
%define api.namespace {DWISLPY}
%define api.parser.class {Parser}
%define parse.error verbose
    
%code requires{
    
    #include "dwislpy-ast.hh"
    #include "dwislpy-check.hh"

    namespace DWISLPY {
        class Driver;
        class Lexer;
    }

    #define YY_NULLPTR nullptr

}

%parse-param { Lexer  &lexer }
%parse-param { Driver &main  }
    
%code{

    #include <sstream>
    #include "dwislpy-util.hh"    
    #include "dwislpy-main.hh"

    #undef yylex
    #define yylex lexer.yylex
    
}


%define api.value.type variant
%define parse.assert
%define api.token.prefix {Token_}

%locations

%token               EOFL  0  
%token               EOLN
%token               INDT
%token               DEDT

%token               DEFF "def"
%token               IFCD "if"
%token               ELSE "else"
%token               ELIF "elif"
%token               WHIL "while"
%token               COMA ","
%token               COLN ":"
%token               ARRW "->"
%token               RTRN "return"
%token               PLEQ "+="
%token               MNEQ "-="
%token               NEGT "not"
%token               CONJ "and"
%token               DISJ "or"
%token               CMLT "<"
%token               CMGT ">"
%token               CMEQ "=="
%token               CMLE "<="
%token               CMGE ">="

%token               PASS "pass"
%token               PRNT "print"
%token               INPT "input"
%token               INTC "int"
%token               STRC "str"
%token               ASGN "="
%token               PLUS "+"
%token               MNUS "-"
%token               TMES "*"
%token               IDIV "//"
%token               IMOD "%"
%token               LPAR "(" 
%token               RPAR ")"
%token               NONE "None"
%token               TRUE "True"
%token               FALS "False"
%token               REPT "repeat"
%token               UNTL "until"
%token <int>         NMBR
%token <std::string> NAME
%token <std::string> STRG

%type <Prgm_ptr> prgm
%type <Defs>     defs
%type <Defn_ptr> defn
%type <Blck_ptr> nest
%type <Blck_ptr> blck
%type <Args>     args
%type <Args_vec> expns
%type <Stmt_vec> stms
%type <Stmt_ptr> stmt
%type <Expn_ptr> expn
%type <Ifcd_vec> elifb
%type <Else_ptr> elseb

%%

%start main;


%left DISJ;
%left CONJ;
%precedence NEGT;
%left CMEQ CMLE CMGE CMLT CMGT;
%left PLUS MNUS;
%left TMES IMOD IDIV;

main:
  prgm {
      main.set($1);
  }
;

prgm:
  defs blck {
      Defs ds = $1; 
      Blck_ptr b = $2;

      $$ = Prgm_ptr { new Prgm {ds, b, lexer.locate(@1)} };
  }
| defs {
      $$ = Prgm_ptr { new Prgm {$1, Blck_ptr{nullptr}, lexer.locate(@1)} };
  }
;

defs:
    defs defn {
        Defs ds = $1;
        ds.push_back($2);
        $$ = ds;
    }
|   defn {
        $$ = Defs {$1};
    }
|   {
        $$ = Defs {};
    }
;

args:
    args COMA NAME {
        Args as = $1;
        as.push_back($3);
        $$ = as;
    }
|   NAME {
        $$ = Args {$1};
    }
|   {
        Args as {};
        $$ = as;
    }
;

defn: 
    DEFF NAME LPAR args RPAR COLN EOLN nest {
        $$ = Defn_ptr { new Defn { $2, $4, $8, $8->where() } };
    }
;

nest: 
    INDT blck DEDT {
        $$ = $2;
    }
|   INDT blck EOFL {
        $$ = $2; 
    }
;

blck:
  stms {
      Stmt_vec ss = $1;
      $$ = Blck_ptr { new Blck {ss, ss[0]->where()} };
  }
;

stms:
  stms stmt {
      Stmt_vec ss = $1;
      ss.push_back($2);
      $$ = ss;
  }
| stmt {
      Stmt_vec ss { };
      ss.push_back($1);
      $$ = ss;
  }
;
  
stmt: 
  expn EOLN{
    $1->make_stmt();
    $$ = $1;
  }

| REPT COLN EOLN nest UNTL expn EOLN {
    $$ = Rept_ptr { new Rept { $6, $4, lexer.locate(@1) } };
  }

| NAME ASGN expn EOLN {
      $$ = Asgn_ptr { new Asgn {$1,$3,lexer.locate(@2)} };
  }

| NAME PLEQ expn EOLN {
      $$ = Pleq_ptr { new Pleq {$1, $3, lexer.locate(@2)} };
  }

| NAME MNEQ expn EOLN {
      $$ = Mneq_ptr { new Mneq {$1, $3, lexer.locate(@2)} };
  }

| IFCD expn COLN EOLN nest elifb elseb {
      $6.push_back(Ifcd_ptr {new Ifcd {$2, $5, $2->where()}});
      $$ = Cond_ptr { new Cond {$6, $7, lexer.locate(@1)} };
  }

| WHIL expn COLN EOLN nest {
      $$ = Whil_ptr { new Whil {$2, $5, lexer.locate(@1)} };
  }

| RTRN expn EOLN {
      $$ = Rtrn_ptr { new Rtrn {$2, lexer.locate(@1)} };
  }

| RTRN EOLN {
      $$ = Rtrn_ptr { new Rtrn {lexer.locate(@1)} };
  }

| PASS EOLN {
      $$ = Pass_ptr { new Pass {lexer.locate(@1)} };
  }
| PRNT LPAR expns RPAR EOLN {
      $$ = Prnt_ptr { new Prnt {$3,lexer.locate(@1)} };
  }
;

elifb: 
    elifb ELIF expn COLN EOLN nest {
        $1.push_back(Ifcd_ptr {new Ifcd {$3, $6, $3->where()}});
        $$ = $1;   
    }
|   {
        $$ = Ifcd_vec {};
    }
;

elseb:
    ELSE COLN EOLN nest {
        $$ = Else_ptr { new Else {$4, $4->where()} };
    }
|   {
        $$ = Else_ptr { nullptr };
    }
;

expns:
    expns COMA expn {
        $1.push_back($3);
        $$ = $1;
    }
|   expn {
        $$ = Expn_vec {$1};
    }
|   {
        $$ = Expn_vec {};
    }
;

expn:
  NAME LPAR expns RPAR {
    $$ = Call_ptr { new Call {$1, $3, lexer.locate(@1)} };
  }

| expn IFCD expn ELSE expn {
    $$ = Inif_ptr { new Inif {$1, $3, $5, $1->where()} };
  }

| expn PLUS expn {
      $$ = Plus_ptr { new Plus {$1,$3,lexer.locate(@2)} };
  }
| expn MNUS expn {
      $$ = Mnus_ptr { new Mnus {$1,$3,lexer.locate(@2)} };
  }
| MNUS expn {
      $$ = Imus_ptr { new Imus {$2, lexer.locate(@1)} };
  }
| expn TMES expn {
      $$ = Tmes_ptr { new Tmes {$1,$3,lexer.locate(@2)} };
  }
| expn IDIV expn {
      $$ = IDiv_ptr { new IDiv {$1,$3,lexer.locate(@2)} };
  }
| expn IMOD expn {
      $$ = IMod_ptr { new IMod {$1,$3,lexer.locate(@2)} };
  }
| expn CMLT expn {
      $$ = Cmlt_ptr { new Cmlt {$1,$3,lexer.locate(@2)} };
  }
| expn CMGT expn {
      $$ = Cmgt_ptr { new Cmgt {$1,$3,lexer.locate(@2)} };
  }
| expn CMLE expn {
      $$ = Cmle_ptr { new Cmle {$1,$3,lexer.locate(@2)} };
  }
| expn CMGE expn {
    $$ = Cmge_ptr { new Cmge {$1,$3,lexer.locate(@2)} };
  }
| expn CMEQ expn {
    $$ = Cmeq_ptr { new Cmeq {$1,$3,lexer.locate(@2)} };
  }
| expn CONJ expn {
    $$ = Conj_ptr { new Conj {$1,$3,lexer.locate(@2)} };
  }
| expn DISJ expn {
    $$ = Disj_ptr { new Disj {$1,$3,lexer.locate(@2)} };
  }
| NEGT expn {
    $$ = Negt_ptr { new Negt {$2,lexer.locate(@1)} };
  }

| NMBR {
      $$ = Ltrl_ptr { new Ltrl {Valu {$1},lexer.locate(@1)} };
  }
| STRG {
      $$ = Ltrl_ptr { new Ltrl {Valu {de_escape($1)},lexer.locate(@1)} };
  }
| TRUE {
      $$ = Ltrl_ptr { new Ltrl {Valu {true},lexer.locate(@1)} };
  }
| FALS {
      $$ = Ltrl_ptr { new Ltrl {Valu {false},lexer.locate(@1)} };
  }
| NONE {
      $$ = Ltrl_ptr { new Ltrl {Valu {None},lexer.locate(@1)} };
  }    
| INPT LPAR expn RPAR {
      $$ = Inpt_ptr { new Inpt {$3,lexer.locate(@1)} };
  }
| INTC LPAR expn RPAR {
      $$ = IntC_ptr { new IntC {$3,lexer.locate(@1)} };
  }
| STRC LPAR expn RPAR {
      $$ = StrC_ptr { new StrC {$3,lexer.locate(@1)} };
  }
| NAME {
      $$ = Lkup_ptr { new Lkup {$1,lexer.locate(@1)} };
  }
| LPAR expn RPAR {
      $$ = $2;
  }
;

%%
       
void DWISLPY::Parser::error(const location_type &loc, const std::string &msg) {
    throw DwislpyError { lexer.locate(loc), msg };
}
